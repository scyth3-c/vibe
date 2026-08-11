//
// Application environment: values loaded from the .env file sitting next to
// the executable, plus runtime "session" values kept in memory. Node.js-style
// access through the global vibe::environment object:
//
//   vibe::environment.get("TOKEN")             // .env / set(), else OS env, else ""
//   vibe::environment.get("TOKEN", "fallback")
//   vibe::environment.get_as<int>("PORT", 8080) // typed conversion
//   vibe::environment["TOKEN"]
//   vibe::environment.set("request_count", "42")  // runtime session value
//
// Lookup order: values from the .env file and set() take precedence over the
// OS environment. All methods are thread-safe (the server handles requests
// from a thread pool).
//

#ifndef VIBE_ENVIRONMENT_H
#define VIBE_ENVIRONMENT_H

#include <cctype>
#include <cstddef>
#include <shared_mutex>
#include <sstream>
#include <string>
#include <type_traits>
#include <unordered_map>

namespace vibe {

    namespace detail {
        inline std::string to_lower(std::string value) {
            for (auto& c : value)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return value;
        }
    } // namespace detail

    class Environment {
    public:
        Environment(const Environment&) = delete;
        Environment& operator=(const Environment&) = delete;

        // Value of `key`: .env file / set() values win over the OS
        // environment; `fallback` when the key is found nowhere.
        [[nodiscard]] std::string get(const std::string& key, const std::string& fallback = {}) const;
        [[nodiscard]] std::string operator[](const std::string& key) const { return get(key); }
        [[nodiscard]] bool has(const std::string& key) const;

        // Typed conversion of get(): arithmetic types via stream parsing,
        // bool accepts 1/0, true/false, yes/no, on/off (case-insensitive).
        template <class T>
        [[nodiscard]] T get_as(const std::string& key, T fallback) const {
            const std::string raw = get(key);
            if (raw.empty())
                return fallback;
            if constexpr (std::is_same_v<T, std::string>) {
                return raw;
            } else if constexpr (std::is_same_v<T, bool>) {
                const std::string value = detail::to_lower(raw);
                if (value == "1" || value == "true" || value == "yes" || value == "on")
                    return true;
                if (value == "0" || value == "false" || value == "no" || value == "off")
                    return false;
                return fallback;
            } else {
                static_assert(std::is_arithmetic_v<T>,
                              "get_as<T>() supports arithmetic types, bool and std::string");
                std::istringstream in(raw);
                T value{};
                in >> value;
                if (in.fail())
                    return fallback;
                in >> std::ws; // trailing whitespace is fine, anything else is not
                return in.eof() ? value : fallback;
            }
        }

        // Runtime "session" value: kept in memory, shadows file/OS values.
        void set(const std::string& key, const std::string& value);
        void unset(const std::string& key);

        // Merges a .env file into the current values (keys absent from the
        // file are kept, so session values survive a reload). Returns false
        // when the file cannot be opened.
        bool load(const std::string& path);
        bool reload();
        // Path of the .env file currently in use ("" when none loaded yet).
        [[nodiscard]] std::string path() const;
        [[nodiscard]] size_t size() const;

    private:
        Environment(); // loads <executable dir>/.env when present
        friend Environment& environment_instance();

        mutable std::shared_mutex mutex_;
        std::unordered_map<std::string, std::string> values_;
        std::string path_;
    };

    // The single instance behind vibe::environment (initialized on first use).
    Environment& environment_instance();

    // Node.js-style global: vibe::environment.get(...)
    inline Environment& environment = environment_instance();

} // namespace vibe

#endif // VIBE_ENVIRONMENT_H
