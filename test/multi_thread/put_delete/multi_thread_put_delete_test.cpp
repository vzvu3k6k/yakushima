/**
 * @file multi_thread_put_delete_test.cpp
 */

#include <algorithm>
#include <random>
#include <thread>
#include <tuple>

#include "gtest/gtest.h"

#include "kvs.h"

using namespace yakushima;

namespace yakushima::testing {

class mtpdt : public ::testing::Test {
    void SetUp() override {
        init();
    }

    void TearDown() override {
        fin();
    }
};

std::string test_storage_name{"1"}; // NOLINT

TEST_F(mtpdt, concurrent_put_delete_between_none_and_many_split_of_interior_with_shuffle_in_multi_layer) { // NOLINT
    /**
     * multi-layer put-delete test.
     */

    constexpr std::size_t ary_size = interior_node::child_length * base_node::key_slice_length * 10;
    std::vector<std::pair<std::string, std::string>> kv1; // NOLINT
    std::vector<std::pair<std::string, std::string>> kv2; // NOLINT
    for (std::size_t i = 0; i < ary_size / 2; ++i) {
        if (i <= INT8_MAX) {
            kv1.emplace_back(std::make_pair(std::string(1, i), std::to_string(i)));
        } else {
            kv1.emplace_back(
                    std::make_pair(std::string(i / INT8_MAX, INT8_MAX) + std::string(1, i - INT8_MAX),
                                   std::to_string(i)));
        }
    }
    for (std::size_t i = ary_size / 2; i < ary_size; ++i) {
        if (i <= INT8_MAX) {
            kv2.emplace_back(std::make_pair(std::string(1, i), std::to_string(i)));
        } else {
            kv2.emplace_back(
                    std::make_pair(std::string(i / INT8_MAX, INT8_MAX) + std::string(1, i - INT8_MAX),
                                   std::to_string(i)));
        }
    }

    std::random_device seed_gen{};
    std::mt19937 engine(seed_gen());

#ifndef NDEBUG
    for (size_t h = 0; h < 1; ++h) {
#else
        for (size_t h = 0; h < 20; ++h) {
#endif
        create_storage(test_storage_name);
        std::atomic<base_node*>* target_storage{};
        find_storage(test_storage_name, &target_storage);
        ASSERT_EQ(target_storage->load(std::memory_order_acquire), nullptr);
        std::array<Token, 2> token{};
        ASSERT_EQ(enter(token[0]), status::OK);
        ASSERT_EQ(enter(token[1]), status::OK);

        std::shuffle(kv1.begin(), kv1.end(), engine);
        std::shuffle(kv2.begin(), kv2.end(), engine);

        struct S {
            static void work(std::vector<std::pair<std::string, std::string>> &kv, Token &token) {
                for (std::size_t j = 0; j < 10; ++j) {
                    for (auto &i : kv) {
                        std::string v(std::get<1>(i));
                        std::string k(std::get<0>(i));
                        status ret = put(test_storage_name, k, v.data(), v.size());
                        if (status::OK != ret) {
                            ret = put(test_storage_name, k, v.data(), v.size());
                            ASSERT_EQ(status::OK, ret);
                            std::abort();
                        }
                    }
                    for (auto &i : kv) {
                        std::string v(std::get<1>(i));
                        std::string k(std::get<0>(i));
                        status ret = remove(token, test_storage_name, k);
                        if (status::OK != ret) {
                            ret = remove(token, test_storage_name, k);
                            ASSERT_EQ(status::OK, ret);
                            std::abort();
                        }
                    }
                }
                for (auto &i : kv) {
                    std::string k(std::get<0>(i));
                    std::string v(std::get<1>(i));
                    status ret = put(test_storage_name, k, v.data(), v.size());
                    if (status::OK != ret) {
                        ASSERT_EQ(status::OK, ret);
                        std::abort();
                    }
                }
            }
        };

        std::thread t(S::work, std::ref(kv2), std::ref(token[0]));
        S::work(std::ref(kv1), std::ref(token[1]));
        t.join();

        std::vector<std::pair<char*, std::size_t>> tuple_list; // NOLINT
        constexpr std::size_t v_index = 0;
        for (std::size_t i = 0; i < ary_size / 100; ++i) {
            std::string k;
            if (i <= INT8_MAX) {
                k = std::string(1, i);
            } else {
                k = std::string(i / INT8_MAX, INT8_MAX) + std::string(1, i - INT8_MAX);
            }
            scan<char>(test_storage_name, "", scan_endpoint::INF, k, scan_endpoint::INCLUSIVE, tuple_list);
            if (tuple_list.size() != i + 1) {
                scan<char>(test_storage_name, "", scan_endpoint::INF, k, scan_endpoint::INCLUSIVE, tuple_list);
                ASSERT_EQ(tuple_list.size(), i + 1);
            }
            for (std::size_t j = 0; j < i + 1; ++j) {
                std::string v(std::to_string(j));
                ASSERT_EQ(memcmp(std::get<v_index>(tuple_list.at(j)), v.data(), v.size()), 0);
            }
        }
        ASSERT_EQ(leave(token[0]), status::OK);
        ASSERT_EQ(leave(token[1]), status::OK);
        destroy();
    }
}

}  // namespace yakushima::testing
