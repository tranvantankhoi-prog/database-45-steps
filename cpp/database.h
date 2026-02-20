#pragma once

#include <unordered_map>    // std::unordered_map
#include <vector>           // std::vector
#include <string>           // std::string
#include <system_error>     // std::error_code
#include <cstddef>          // std::byte
#include <optional>         // std::optional

using bytes = std::vector<std::byte>;
using error = std::error_code;
using std::string;

struct ByteVectorHash {
    size_t operator()(const bytes &v) const noexcept {
        return std::hash<std::string_view>{}(
            std::string_view(reinterpret_cast<const char*>(v.data()), v.size())
        );
    }
};

class KV {
    private:

    std::unordered_map<bytes, bytes, ByteVectorHash> mem;

    public:

    error Open() {
        mem.clear();
        return {};
    }

    error Close() { return {}; }

    std::pair<std::optional<bytes>, error> Get(const bytes &key) const {
        auto item = mem.find(key);
        
        if (item == mem.end())
            return { std::nullopt, {} };
        return { item->second, {} };
    }

    std::pair<bool, error> Set(const bytes &key, const bytes &val) {
        auto item = mem.find(key);
        if (item == mem.end()) {
            mem.emplace(key, val);
            return { true, {} };
        }

        bool updated = (item->second != val);
        if (updated)
            item->second = val;
        return { updated, {} };
    }

    std::pair<bool, error> Del(const bytes &key) {
        return { mem.erase(key) > 0, {} };
    }
};
