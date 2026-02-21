/**
 * @file database.h
 * @author Tran Van Tan Khoi (tranvantankhoi@gmail.com)
 * @brief A basic in-memory Key-Value database implementation
 * @version 0.2.0
 * @date 2026-02-20
 * 
 * @copyright Copyright (c) under MIT license, 2026
 * 
 */

#pragma once

#include <unordered_map>    // std::unordered_map
#include <vector>           // std::vector
#include <string>           // std::string
#include <system_error>     // std::error_code
#include <cstddef>          // std::byte
#include <optional>         // std::optional
#include <algorithm>        // std::copy
#include <cstdint>          // uint32_t
#include <iostream>         // std::istream

/** Aliases for common types */

using bytes = std::vector<std::byte>;
using error = std::error_code;
using std::string;

/**
 * @brief A functor allowing `bytes` to be used as keys in hash maps.
 */
struct ByteVectorHash {
    size_t operator()(const bytes &v) const noexcept {
        return std::hash<std::string_view>{}(
            std::string_view(reinterpret_cast<const char*>(v.data()), v.size())
        );
    }
};

/**
 * @brief KV provides a simple in-memory key-value store with binary support.
 * It tracks state changes for Set and Del operations.
 */
class KV {
    private:

    std::unordered_map<bytes, bytes, ByteVectorHash> mem;

    public:

    /**
     * @brief Initializes the database. Clears any existing in-memory data.
     * 
     * @return An error code. This method returns no error code for now.
     */
    error Open() {
        mem.clear();
        return {};
    }

    /**
     * @brief Closes the database. Currently a no-op for the in-memory version.
     * 
     * @return An error code. This method returns no error code for now.
     */
    error Close() { return {}; }

    /**
     * @brief Retrieves a value by key.
     * Returns a pair containing the value (if found) and an error code.
     * @param key 
     * @return `pair<optional<bytes>, error>`. This method returns no error code for now. 
     */
    std::pair<std::optional<bytes>, error> Get(const bytes &key) const {
        auto item = mem.find(key);
        
        if (item == mem.end())
            return { std::nullopt, {} };
        return { item->second, {} };
    }

    /**
     * @brief Inserts or updates a value.
     * 
     * @param key 
     * @param val 
     * @return `true` if the key was newly added or the value was different.
     */
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

    /**
     * @brief Removes a key from the store.
     * 
     * @param key 
     * @return `true` if the key existed and was successfully deleted.
     */
    std::pair<bool, error> Del(const bytes &key) {
        return { mem.erase(key) > 0, {} };
    }
};


/**
 * @brief Represents a single database entry for serialization
 * 
 */
struct Entry {
    bytes key;
    bytes val;

    /**
     * @brief Serializes the entry into a byte buffer.
     * The format is [4-byte klen | 4-byte vlen | key payload | value payload].
     * @return bytes containing the encoded entry.
     */
    bytes Encode() const {
        uint32_t klen = static_cast<uint32_t>(key.size());
        uint32_t vlen = static_cast<uint32_t>(val.size());

        bytes buf(8 + klen + vlen);

        // Packing `klen` into buffer in Little Endian
        buf[0] = static_cast<std::byte>(klen & 0xFF);
        buf[1] = static_cast<std::byte>((klen >> 8) & 0xFF);
        buf[2] = static_cast<std::byte>((klen >> 16) & 0xFF);
        buf[3] = static_cast<std::byte>((klen >> 24) & 0xFF);

        // Packing 'vlen' into buffer in Little Endian
        buf[4] = static_cast<std::byte>(vlen & 0xFF);
        buf[5] = static_cast<std::byte>((vlen >> 8) & 0xFF);
        buf[6] = static_cast<std::byte>((vlen >> 16) & 0xFF);
        buf[7] = static_cast<std::byte>((vlen >> 24) & 0xFF);

        // Copying key and value data
        std::copy(key.begin(), key.end(), buf.begin() + 8);
        std::copy(val.begin(), val.end(), buf.begin() + 8 + klen);

        return buf;
    }

    /**
     * @brief Deserializes an entry from a stream.
     * 
     * @param `is` The input stream
     * @return An error if reading fails.
     */
    error Decode(std::istream &is) {
        uint8_t header[8];
        if (!is.read(reinterpret_cast<char *>(header), 8)) {
            return std::make_error_code(std::errc::io_error);
        }

        // Unpacking lengths
        uint32_t klen = header[0] | (header[1] << 8) | (header[2] << 16) | (header[3] << 24);
        uint32_t vlen = header[4] | (header[5] << 8) | (header[6] << 16) | (header[7] << 24);

        key.resize(klen);
        val.resize(vlen);

        if (!is.read(reinterpret_cast<char *>(key.data()), klen))
            return std::make_error_code(std::errc::io_error);
        
        if (!is.read(reinterpret_cast<char *>(val.data()), vlen))
            return std::make_error_code(std::errc::io_error);
        
        return {};
    }
};