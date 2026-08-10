//
// Shared hardening primitives for the file-rendering family.
//
//   - read_bounded(): regular-file check + size cap (no FIFOs, no /dev,
//     no memory exhaustion).
//   - is_within():    canonical containment check for the optional jail.
//   - valid_include_name(): strict whitelist for compose() module names.
//   - sha256_hex():   cache keys for the readFileX compiler cache.
//   - escape_html():  safe reflection of names/paths in error pages.
//

#ifndef VIBE_SECURE_RENDER_H
#define VIBE_SECURE_RENDER_H

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>

namespace vibe::srender {

    // ---------------- bounded file reading ----------------

    enum class ReadErr { Ok, NotFound, Forbidden, TooLarge, IoError };

    struct ReadResult {
        ReadErr err = ReadErr::Ok;
        std::string data{};
    };

    [[nodiscard]] inline const char* status_of(const ReadErr err) noexcept {
        switch (err) {
            case ReadErr::NotFound:  return "404";
            case ReadErr::Forbidden: return "403";
            case ReadErr::TooLarge:  return "413";
            case ReadErr::IoError:   return "500";
            default:                 return "200";
        }
    }

    // Reads a regular file fully, refusing anything that is not a plain
    // regular file (directories, FIFOs, devices, /proc entries with a
    // lied-about size are capped by the streaming read below) and anything
    // larger than max_bytes.
    [[nodiscard]] inline ReadResult read_bounded(const std::string& path, const size_t max_bytes) {
        namespace fs = std::filesystem;

        std::error_code ec;
        if (!fs::exists(fs::path(path), ec) || ec)
            return {ReadErr::NotFound, {}};

        if (!fs::is_regular_file(fs::path(path), ec) || ec)
            return {ReadErr::Forbidden, {}};

        const auto size = fs::file_size(fs::path(path), ec);
        if (!ec && size > max_bytes)
            return {ReadErr::TooLarge, {}};

        std::ifstream in(path, std::ios::binary);
        if (!in.is_open())
            return {ReadErr::NotFound, {}};

        std::string out;
        out.reserve(std::min<size_t>(max_bytes, ec ? 0 : static_cast<size_t>(size)));

        std::array<char, 16384> chunk{};
        while (in) {
            const size_t room = max_bytes - out.size();
            if (room == 0)
                return {ReadErr::TooLarge, {}}; // grew past the cap mid-read
            in.read(chunk.data(), static_cast<std::streamsize>(std::min(room, chunk.size())));
            out.append(chunk.data(), static_cast<size_t>(in.gcount()));
        }
        return {ReadErr::Ok, std::move(out)};
    }

    // ---------------- jail containment ----------------

    // True when `target` resolves (symlinks resolved for the existing
    // prefix, '.'/'..' folded lexically) inside `base`. Both may be
    // relative; they are anchored to the CWD first.
    [[nodiscard]] inline bool is_within(const std::string& base, const std::string& target) {
        namespace fs = std::filesystem;
        if (base.empty() || target.empty())
            return false;

        std::error_code ec;
        const fs::path abs_base   = fs::absolute(fs::path(base), ec);
        if (ec) return false;
        const fs::path abs_target = fs::absolute(fs::path(target), ec);
        if (ec) return false;

        const fs::path cb = fs::weakly_canonical(abs_base, ec);
        if (ec || cb.empty()) return false;
        const fs::path ct = fs::weakly_canonical(abs_target, ec);
        if (ec || ct.empty()) return false;

        // Component-wise prefix check: base must be an ancestor of target.
        auto b = cb.begin();
        auto t = ct.begin();
        for (; b != cb.end(); ++b, ++t) {
            if (t == ct.end() || *b != *t)
                return false;
        }
        return true;
    }

    // ---------------- compose() module names ----------------

    // Module names in "#[name];" must be bare file names: this kills
    // "../../etc/passwd" style traversal regardless of the template folder.
    [[nodiscard]] inline bool valid_include_name(const std::string_view name) noexcept {
        if (name.empty() || name.size() > 255)
            return false;
        if (name == "." || name == "..")
            return false;
        for (const char c : name) {
            const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
                         || (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
            if (!ok)
                return false;
        }
        return name.find("..") == std::string_view::npos;
    }

    // ---------------- HTML escaping for reflected values ----------------

    [[nodiscard]] inline std::string escape_html(const std::string_view in) {
        std::string out;
        out.reserve(in.size());
        for (const char c : in) {
            switch (c) {
                case '&':  out += "&amp;";  break;
                case '<':  out += "&lt;";   break;
                case '>':  out += "&gt;";   break;
                case '"':  out += "&quot;"; break;
                case '\'': out += "&#39;";  break;
                default:
                    // Drop C0 control chars: they have no business in a page.
                    if (static_cast<unsigned char>(c) >= 0x20 || c == '\n' || c == '\t')
                        out.push_back(c);
            }
        }
        return out;
    }

    // ---------------- SHA-256 (cache keys) ----------------

    namespace detail {

        class Sha256 {
        public:
            Sha256() { reset(); }

            void update(const void* data, const size_t len) {
                const auto* p = static_cast<const uint8_t*>(data);
                for (size_t i = 0; i < len; ++i) {
                    block_[block_len_++] = p[i];
                    if (block_len_ == 64) {
                        transform();
                        bit_len_ += 512;
                        block_len_ = 0;
                    }
                }
            }

            [[nodiscard]] std::string hex() {
                uint8_t hash[32];
                finish(hash);
                static constexpr char digits[] = "0123456789abcdef";
                std::string out;
                out.reserve(64);
                for (const uint8_t b : hash) {
                    out.push_back(digits[b >> 4]);
                    out.push_back(digits[b & 0x0f]);
                }
                return out;
            }

        private:
            uint32_t state_[8];
            uint8_t  block_[64]{};
            size_t   block_len_ = 0;
            uint64_t bit_len_   = 0;

            void reset() {
                state_[0] = 0x6a09e667; state_[1] = 0xbb67ae85;
                state_[2] = 0x3c6ef372; state_[3] = 0xa54ff53a;
                state_[4] = 0x510e527f; state_[5] = 0x9b05688c;
                state_[6] = 0x1f83d9ab; state_[7] = 0x5be0cd19;
            }

            static uint32_t rotr(const uint32_t x, const uint32_t n) noexcept {
                return (x >> n) | (x << (32 - n));
            }

            void transform() {
                static constexpr uint32_t K[64] = {
                    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
                    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
                    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
                    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
                    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
                    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
                    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
                    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
                };

                uint32_t w[64];
                for (int i = 0; i < 16; ++i) {
                    w[i] = (static_cast<uint32_t>(block_[i * 4])     << 24)
                         | (static_cast<uint32_t>(block_[i * 4 + 1]) << 16)
                         | (static_cast<uint32_t>(block_[i * 4 + 2]) << 8)
                         | (static_cast<uint32_t>(block_[i * 4 + 3]));
                }
                for (int i = 16; i < 64; ++i) {
                    const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
                    const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
                    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
                }

                uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
                uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];

                for (int i = 0; i < 64; ++i) {
                    const uint32_t s1   = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
                    const uint32_t ch   = (e & f) ^ (~e & g);
                    const uint32_t t1   = h + s1 + ch + K[i] + w[i];
                    const uint32_t s0   = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
                    const uint32_t maj  = (a & b) ^ (a & c) ^ (b & c);
                    const uint32_t t2   = s0 + maj;
                    h = g; g = f; f = e; e = d + t1;
                    d = c; c = b; b = a; a = t1 + t2;
                }

                state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d;
                state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
            }

            void finish(uint8_t out[32]) {
                uint64_t bits = bit_len_ + block_len_ * 8;

                block_[block_len_++] = 0x80;
                if (block_len_ > 56) {
                    while (block_len_ < 64) block_[block_len_++] = 0;
                    transform();
                    block_len_ = 0;
                }
                while (block_len_ < 56) block_[block_len_++] = 0;
                // Message length as a 64-bit BIG-endian integer (per FIPS 180-4).
                for (int i = 7; i >= 0; --i)
                    block_[block_len_++] = static_cast<uint8_t>((bits >> (i * 8)) & 0xff);
                transform();

                for (int i = 0; i < 8; ++i) {
                    out[i * 4]     = static_cast<uint8_t>(state_[i] >> 24);
                    out[i * 4 + 1] = static_cast<uint8_t>(state_[i] >> 16);
                    out[i * 4 + 2] = static_cast<uint8_t>(state_[i] >> 8);
                    out[i * 4 + 3] = static_cast<uint8_t>(state_[i]);
                }
            }
        };

    } // namespace detail

    [[nodiscard]] inline std::string sha256_hex(const std::string_view input) {
        detail::Sha256 ctx;
        ctx.update(input.data(), input.size());
        return ctx.hex();
    }

} // namespace vibe::srender

#endif // VIBE_SECURE_RENDER_H
