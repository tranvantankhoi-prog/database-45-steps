#include <gtest/gtest.h>
#include "database.h"

// Helper to convert string literals to bytes for cleaner tests
bytes to_bytes(std::string_view s) {
    bytes b;
    for (char c : s) b.push_back(static_cast<std::byte>(c));
    return b;
}

// Overload for Entry comparison (required for EXPECT_EQ)
bool operator==(const Entry& a, const Entry& b) {
    return a.key == b.key && a.val == b.val;
}

TEST(KVTest, SetUpdates) {
    KV kv;
    ASSERT_FALSE(kv.Open());

    bytes key = to_bytes("conf");
    bytes val1 = to_bytes("v1");
    bytes val2 = to_bytes("v2");

    // 1. Initial Set (Success, state changed)
    auto [upd1, err1] = kv.Set(key, val1);
    EXPECT_TRUE(upd1); 
    EXPECT_FALSE(err1);

    // 2. Update with different value (Success, state changed)
    auto [upd2, err2] = kv.Set(key, val2);
    EXPECT_TRUE(upd2); // Returns true because val2 != val1
    EXPECT_FALSE(err2);

    // 3. Update with identical value (Unsuccessful update, state unchanged)
    auto [upd3, err3] = kv.Set(key, val2);
    EXPECT_FALSE(upd3); // Returns false because value is the same
    EXPECT_FALSE(err3);

    // 4. Test Get success
    auto [g1, g_err1] = kv.Get(key);
    ASSERT_TRUE(g1.has_value());
    EXPECT_EQ(g1.value(), val2);
    EXPECT_FALSE(g_err1);

    // 5. Test Get missing
    auto [g2, g_err2] = kv.Get(to_bytes("xxx"));
    EXPECT_FALSE(g2.has_value());
    EXPECT_FALSE(g_err2);

    // 6. Test Del missing
    auto [d1, d_err1] = kv.Del(to_bytes("xxx"));
    EXPECT_FALSE(d1);
    EXPECT_FALSE(d_err1);

    // Test Del success
    auto [d2, d_err2] = kv.Del(key);
    EXPECT_TRUE(d2);
    EXPECT_FALSE(d_err2);

    // Verify final state remains correct
    auto [res, err4] = kv.Get(key);
    EXPECT_FALSE(res.has_value());
    
    kv.Close();
}

TEST(EntryTest, EncodeDecode) {
    Entry ent{to_bytes("k1"), to_bytes("xxx")};

    bytes expected_data = {
        std::byte{2}, std::byte{0}, std::byte{0}, std::byte{0}, // klen
        std::byte{3}, std::byte{0}, std::byte{0}, std::byte{0}, // vlen
        std::byte{'k'}, std::byte{'1'},                         // key
        std::byte{'x'}, std::byte{'x'}, std::byte{'x'}          // val
    };

    // Test encode
    EXPECT_EQ(ent.Encode(), expected_data);

    // Test decode using a stringstream as buffer
    std::string s_data(reinterpret_cast<const char *>(expected_data.data()), expected_data.size());
    std::stringstream ss(s_data);

    Entry decoded;
    error err = decoded.Decode(ss);

    EXPECT_EQ(ent, decoded);
    EXPECT_FALSE(err);
}