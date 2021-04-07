/**
 * @file scan_test.cpp
 */

#include <array>

#include "gtest/gtest.h"

#include "kvs.h"

using namespace yakushima;

namespace yakushima::testing {

class st : public ::testing::Test {
    void SetUp() override {
        init();
    }

    void TearDown() override {
        fin();
    }
};

TEST_F(st, scan_against_single_put_null_key_to_one_border) { // NOLINT
    /**
     * put one key-value non-null key
     */
    std::string k;
    std::string v("v");
    Token token{};
    ASSERT_EQ(enter(token), status::OK);
    ASSERT_EQ(status::OK, put(std::string_view(k), v.data(), v.size()));
    std::vector<std::pair<char*, std::size_t>> tup_lis{}; // NOLINT
    std::vector<std::pair<node_version64_body, node_version64*>> nv;
    auto verify_exist = [&tup_lis, &nv, &v]() {
        if (tup_lis.size() != 1) return false;
        if (tup_lis.size() != nv.size()) return false;
        if (std::get<1>(tup_lis.at(0)) != v.size())return false;
        if (memcmp(std::get<0>(tup_lis.at(0)), v.data(), v.size()) != 0)return false;
        return true;
    };
    ASSERT_EQ(status::OK, scan<char>("", scan_endpoint::INF, "", scan_endpoint::INF, tup_lis, &nv));
    ASSERT_EQ(verify_exist(), true);
    ASSERT_EQ(status::OK, scan<char>("", scan_endpoint::INCLUSIVE, "", scan_endpoint::INCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_exist(), true);
    ASSERT_EQ(status::OK, scan<char>("", scan_endpoint::INF, "", scan_endpoint::INCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_exist(), true);
    ASSERT_EQ(status::OK, scan<char>("", scan_endpoint::INCLUSIVE, "", scan_endpoint::INF, tup_lis, &nv));
    ASSERT_EQ(verify_exist(), true);
    ASSERT_EQ(leave(token), status::OK);
}

TEST_F(st, scan_against_single_put_non_null_key_to_one_border) { // NOLINT
    /**
     * put one key-value non-null key
     */
    std::string k("k");
    std::string v("v");
    Token token{};
    ASSERT_EQ(enter(token), status::OK);
    ASSERT_EQ(status::OK, put(std::string_view(k), v.data(), v.size()));
    std::vector<std::pair<char*, std::size_t>> tup_lis{}; // NOLINT
    std::vector<std::pair<node_version64_body, node_version64*>> nv;
    auto verify_exist = [&tup_lis, &nv, &v]() {
        if (tup_lis.size() != 1) return false;
        if (tup_lis.size() != nv.size()) return false;
        if (std::get<1>(tup_lis.at(0)) != v.size()) return false;
        if (memcmp(std::get<0>(tup_lis.at(0)), v.data(), v.size()) != 0) return false;
        return true;
    };
    auto verify_no_exist = [&tup_lis, &nv]() {
        if (!tup_lis.empty()) return false;
        if (tup_lis.size() != nv.size()) return false;
        return true;
    };
    ASSERT_EQ(status::OK, scan<char>("", scan_endpoint::INF, "", scan_endpoint::INF, tup_lis, &nv));
    ASSERT_EQ(verify_exist(), true);
    ASSERT_EQ(status::OK, scan<char>("", scan_endpoint::INCLUSIVE, "", scan_endpoint::INCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_no_exist(), true);
    ASSERT_EQ(status::OK, scan<char>("", scan_endpoint::INF, "", scan_endpoint::INCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_no_exist(), true);
    ASSERT_EQ(status::OK, scan<char>("", scan_endpoint::INCLUSIVE, "", scan_endpoint::INF, tup_lis, &nv));
    ASSERT_EQ(verify_exist(), true);
    ASSERT_EQ(status::OK, scan<char>("", scan_endpoint::INF, k, scan_endpoint::INF, tup_lis, &nv));
    ASSERT_EQ(verify_exist(), true);
    ASSERT_EQ(status::OK, scan<char>("", scan_endpoint::INF, k, scan_endpoint::INCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_exist(), true);
    ASSERT_EQ(status::OK, scan<char>("", scan_endpoint::INF, k, scan_endpoint::EXCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_no_exist(), true);
    ASSERT_EQ(status::OK, scan<char>("", scan_endpoint::INCLUSIVE, k, scan_endpoint::INF, tup_lis, &nv));
    ASSERT_EQ(verify_exist(), true);
    ASSERT_EQ(status::OK, scan<char>("", scan_endpoint::INCLUSIVE, k, scan_endpoint::EXCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_no_exist(), true);
    ASSERT_EQ(status::OK, scan<char>("", scan_endpoint::INCLUSIVE, k, scan_endpoint::INCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_exist(), true);
    ASSERT_EQ(status::OK, scan<char>(k, scan_endpoint::INF, "", scan_endpoint::INF, tup_lis, &nv));
    ASSERT_EQ(verify_exist(), true);
    ASSERT_EQ(status::OK, scan<char>(k, scan_endpoint::INF, "", scan_endpoint::INCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_no_exist(), true);
    ASSERT_EQ(status::OK, scan<char>(k, scan_endpoint::INF, "", scan_endpoint::EXCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_no_exist(), true);
    ASSERT_EQ(status::OK, scan<char>(k, scan_endpoint::INCLUSIVE, "", scan_endpoint::INF, tup_lis, &nv));
    ASSERT_EQ(verify_exist(), true);
    ASSERT_EQ(status::OK, scan<char>(k, scan_endpoint::INCLUSIVE, "", scan_endpoint::INCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_no_exist(), true);
    ASSERT_EQ(status::OK, scan<char>(k, scan_endpoint::INCLUSIVE, "", scan_endpoint::EXCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_no_exist(), true);
    ASSERT_EQ(status::OK, scan<char>(k, scan_endpoint::EXCLUSIVE, "", scan_endpoint::EXCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_no_exist(), true);
    ASSERT_EQ(status::OK, scan<char>(k, scan_endpoint::EXCLUSIVE, "", scan_endpoint::EXCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_no_exist(), true);
    ASSERT_EQ(status::OK, scan<char>(k, scan_endpoint::EXCLUSIVE, "", scan_endpoint::EXCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_no_exist(), true);
    ASSERT_EQ(status::OK, scan<char>(k, scan_endpoint::INF, k, scan_endpoint::INF, tup_lis, &nv));
    ASSERT_EQ(verify_exist(), true);
    ASSERT_EQ(status::OK, scan<char>(k, scan_endpoint::INF, k, scan_endpoint::INCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_exist(), true);
    ASSERT_EQ(status::ERR_BAD_USAGE, scan<char>(k, scan_endpoint::INF, k, scan_endpoint::EXCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(status::OK, scan<char>(k, scan_endpoint::INCLUSIVE, k, scan_endpoint::INF, tup_lis, &nv));
    ASSERT_EQ(verify_exist(), true);
    ASSERT_EQ(status::OK, scan<char>(k, scan_endpoint::INCLUSIVE, k, scan_endpoint::INCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(verify_exist(), true);
    ASSERT_EQ(status::ERR_BAD_USAGE,
              scan<char>(k, scan_endpoint::INCLUSIVE, k, scan_endpoint::EXCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(status::ERR_BAD_USAGE,
              scan<char>(k, scan_endpoint::EXCLUSIVE, k, scan_endpoint::EXCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(status::ERR_BAD_USAGE,
              scan<char>(k, scan_endpoint::EXCLUSIVE, k, scan_endpoint::EXCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(status::ERR_BAD_USAGE,
              scan<char>(k, scan_endpoint::EXCLUSIVE, k, scan_endpoint::EXCLUSIVE, tup_lis, &nv));
    ASSERT_EQ(leave(token), status::OK);
}


TEST_F(st, scan_multiple_same_null_char_key_1) { // NOLINT
    /**
     * scan against multiple put same null char key whose length is different each other against single border node.
     */
    Token token{};
    ASSERT_EQ(enter(token), status::OK);
    constexpr std::size_t ary_size = 8;
    std::array<std::string, ary_size> k; // NOLINT
    std::array<std::string, ary_size> v; // NOLINT
    for (std::size_t i = 0; i < ary_size; ++i) {
        k.at(i).assign(i, '\0');
        v.at(i) = std::to_string(i);
        ASSERT_EQ(status::OK, put(std::string_view(k.at(i)), v.at(i).data(), v.at(i).size()));
    }

    std::vector<std::pair<char*, std::size_t>> tuple_list{}; // NOLINT
    constexpr std::size_t v_index = 0;
    for (std::size_t i = 0; i < ary_size; ++i) {
        scan<char>("", scan_endpoint::INF, std::string_view(k.at(i)), scan_endpoint::INCLUSIVE, tuple_list);
        for (std::size_t j = 0; j < i + 1; ++j) {
            ASSERT_EQ(memcmp(std::get<v_index>(tuple_list.at(j)), v.at(j).data(), v.at(j).size()), 0);
        }
    }

    for (std::size_t i = ary_size - 1; i > 1; --i) {
        std::vector<std::pair<node_version64_body, node_version64*>> nv;
        scan<char>(std::string_view(k.at(i)), scan_endpoint::INCLUSIVE, "", scan_endpoint::INF, tuple_list, &nv);
        ASSERT_EQ(tuple_list.size(), ary_size - i);
        ASSERT_EQ(tuple_list.size(), nv.size());
        for (std::size_t j = i; j < ary_size; ++j) {
            ASSERT_EQ(memcmp(std::get<v_index>(tuple_list.at(j - i)), v.at(j).data(), v.at(j).size()), 0);
        }
    }
    ASSERT_EQ(leave(token), status::OK);
}

} // namespace yakushima
