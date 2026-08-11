#include "../include/vibe/util/environment.h"

#include "../include/vibe/util/process.h"

#include <cstdlib>
#include <fstream>
#include <mutex>

namespace {

    std::string trim(const std::string& str) {
        const auto first = str.find_first_not_of(" \t");
        if (first == std::string::npos)
            return {};
        return str.substr(first, str.find_last_not_of(" \t") - first + 1);
    }

    // Minimal dotenv syntax: KEY=VALUE lines, # comments, optional "export "
    // prefix, optional single/double quotes around values, " #" trailing
    // comments on unquoted values.
    std::unordered_map<std::string, std::string> parse_env(std::istream& in) {
        std::unordered_map<std::string, std::string> parsed;
        std::string line;
        while (std::getline(in, line)) {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            line = trim(line);
            if (line.empty() || line.front() == '#')
                continue;
            if (line.rfind("export ", 0) == 0)
                line = trim(line.substr(7));

            const auto eq = line.find('=');
            if (eq == std::string::npos)
                continue;

            std::string key = trim(line.substr(0, eq));
            std::string value = trim(line.substr(eq + 1));
            if (key.empty())
                continue;

            if (value.size() >= 2 &&
                ((value.front() == '"' && value.back() == '"') ||
                 (value.front() == '\'' && value.back() == '\''))) {
                value = value.substr(1, value.size() - 2); // quoted: keep inner content as-is
            } else if (const auto hash = value.find(" #"); hash != std::string::npos) {
                value = trim(value.substr(0, hash)); // unquoted: drop trailing comment
            }

            parsed[std::move(key)] = std::move(value);
        }
        return parsed;
    }

} // namespace

vibe::Environment::Environment() {
    // The .env sits next to the executable, wherever it is run from.
    // process_instance() (not the vibe::process inline reference) keeps this
    // safe no matter which static initialization order a TU ends up with.
    load(process_instance().pwd + "/.env");
}

std::string vibe::Environment::get(const std::string& key, const std::string& fallback) const {
    {
        std::shared_lock lock(mutex_);
        if (const auto it = values_.find(key); it != values_.end())
            return it->second;
    }
    if (const char* value = std::getenv(key.c_str()); value != nullptr)
        return value;
    return fallback;
}

bool vibe::Environment::has(const std::string& key) const {
    {
        std::shared_lock lock(mutex_);
        if (values_.find(key) != values_.end())
            return true;
    }
    return std::getenv(key.c_str()) != nullptr;
}

void vibe::Environment::set(const std::string& key, const std::string& value) {
    std::unique_lock lock(mutex_);
    values_[key] = value;
}

void vibe::Environment::unset(const std::string& key) {
    std::unique_lock lock(mutex_);
    values_.erase(key);
}

bool vibe::Environment::load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open())
        return false;

    auto parsed = parse_env(file);
    std::unique_lock lock(mutex_);
    for (auto& [key, value] : parsed)
        values_[std::move(key)] = std::move(value);
    path_ = path;
    return true;
}

bool vibe::Environment::reload() {
    std::string current;
    {
        std::shared_lock lock(mutex_);
        current = path_;
    }
    return !current.empty() && load(current);
}

std::string vibe::Environment::path() const {
    std::shared_lock lock(mutex_);
    return path_;
}

size_t vibe::Environment::size() const {
    std::shared_lock lock(mutex_);
    return values_.size();
}

vibe::Environment& vibe::environment_instance() {
    static Environment instance;
    return instance;
}
