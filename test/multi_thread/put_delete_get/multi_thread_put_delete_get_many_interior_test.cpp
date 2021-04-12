/**
 * @file multi_thread_put_delete_get_many_interior_test.cpp
 */

#include <algorithm>
#include <array>
#include <random>
#include <thread>

#include "gtest/gtest.h"

#include "kvs.h"

using namespace yakushima;

namespace yakushima::testing {

std::string test_storage_name{"1"};// NOLINT

class mtpdgt : public ::testing::Test {
protected:
    void SetUp() override {
        init();
        create_storage(test_storage_name);
    }

    void TearDown() override {
        fin();
    }
};

TEST_F(mtpdgt, many_interior) {// NOLINT
    /**
     * concurrent put/delete/get in the state between none to many split of interior.
     */

    constexpr std::size_t ary_size = interior_node::child_length * base_node::key_slice_length * 1.4;
    constexpr std::size_t th_nm{ary_size / 20};

#ifndef NDEBUG
    for (std::size_t h = 0; h < 1; ++h) {
#else
    for (std::size_t h = 0; h < 10; ++h) {
#endif
        create_storage(test_storage_name);

        struct S {
            static void work(std::size_t th_id) {
                std::vector<std::pair<std::string, std::string>> kv;
                kv.reserve(ary_size / th_nm);
                // data generation
                for (std::size_t i = (ary_size / th_nm) * th_id; i < (th_id != th_nm - 1 ? (ary_size / th_nm) * (th_id + 1) : ary_size); ++i) {
                    if (i <= INT8_MAX) {
                        kv.emplace_back(std::make_pair(std::string(1, i), std::to_string(i)));
                    } else {
                        kv.emplace_back(std::make_pair(std::string(i / INT8_MAX, INT8_MAX) + std::string(1, i % INT8_MAX), std::to_string(i)));
                    }
                }

                Token token{};
                enter(token);
#ifndef NDEBUG
                for (std::size_t j = 0; j < 1; ++j) {
#else
                for (std::size_t j = 0; j < 10; ++j) {
#endif
                    for (auto& i : kv) {
                        std::string k(std::get<0>(i));
                        std::string v(std::get<1>(i));
                        ASSERT_EQ(put(test_storage_name, k, v.data(), v.size()), status::OK);
                    }
                    for (auto& i : kv) {
                        std::string k(std::get<0>(i));
                        std::string v(std::get<1>(i));
                        std::pair<char*, std::size_t> ret = get<char>(test_storage_name, k);
                        if (std::get<0>(ret) == nullptr) {
                            ret = get<char>(test_storage_name, k);
                            ASSERT_EQ(true, false);
                        }
                        ASSERT_EQ(memcmp(std::get<0>(ret), v.data(), v.size()), 0);
                    }
                    for (auto& i : kv) {
                        std::string k(std::get<0>(i));
                        std::string v(std::get<1>(i));
                        ASSERT_EQ(remove(token, test_storage_name, k), status::OK);
                    }
                }
                for (auto& i : kv) {
                    std::string k(std::get<0>(i));
                    std::string v(std::get<1>(i));
                    ASSERT_EQ(put(test_storage_name, k, v.data(), v.size()), status::OK);
                }

                leave(token);
            }
        };

        std::vector<std::thread> thv;
        thv.reserve(th_nm);
        for (std::size_t i = 0; i < th_nm; ++i) {
            thv.emplace_back(S::work, i);
        }
        for (auto&& th : thv) { th.join(); }
        thv.clear();

        struct parallel_verify {
            static void work(std::size_t i) {
                std::string k;
                if (i <= INT8_MAX) {
                    k = std::string(1, i);
                } else {
                    k = std::string(i / INT8_MAX, INT8_MAX) + std::string(1, i % INT8_MAX);
                }
                std::vector<std::pair<char*, std::size_t>> tuple_list;
                scan<char>(test_storage_name, "", scan_endpoint::INF, k, scan_endpoint::INCLUSIVE, tuple_list);
                if (tuple_list.size() != i + 1) {
                    scan<char>(test_storage_name, "", scan_endpoint::INF, k, scan_endpoint::INCLUSIVE, tuple_list);
                    ASSERT_EQ(tuple_list.size(), i + 1);
                }
                for (std::size_t j = 0; j < i + 1; ++j) {
                    std::string v(std::to_string(j));
                    constexpr std::size_t v_index = 0;
                    ASSERT_EQ(memcmp(std::get<v_index>(tuple_list.at(j)), v.data(), v.size()), 0);
                }
            }
        };

        thv.reserve(ary_size);
        for (std::size_t i = 0; i < ary_size; ++i) {
            thv.emplace_back(parallel_verify::work, i);
        }
        for (auto&& th : thv) { th.join(); }

        destroy();
    }
}

TEST_F(mtpdgt, many_interior_shuffle) {// NOLINT
    /**
     * concurrent put/delete/get in the state between none to many split of interior with shuffle.
     */

    constexpr std::size_t ary_size = interior_node::child_length * base_node::key_slice_length * 1.4;
    constexpr std::size_t th_nm{ary_size / 20};

#ifndef NDEBUG
    for (size_t h = 0; h < 1; ++h) {
#else
    for (size_t h = 0; h < 100; ++h) {
#endif
        create_storage(test_storage_name);

        struct S {
            static void work(std::size_t th_id) {
                std::vector<std::pair<std::string, std::string>> kv;
                kv.reserve(ary_size / th_nm);
                // data generation
                for (std::size_t i = (ary_size / th_nm) * th_id; i < (th_id != th_nm - 1 ? (ary_size / th_nm) * (th_id + 1) : ary_size); ++i) {
                    if (i <= INT8_MAX) {
                        kv.emplace_back(std::make_pair(std::string(1, i), std::to_string(i)));
                    } else {
                        kv.emplace_back(std::make_pair(std::string(i / INT8_MAX, INT8_MAX) + std::string(1, i % INT8_MAX), std::to_string(i)));
                    }
                }

                std::random_device seed_gen;
                std::mt19937 engine(seed_gen());
                Token token{};
                enter(token);

#ifndef NDEBUG
                for (std::size_t j = 0; j < 1; ++j) {
#else
                for (std::size_t j = 0; j < 10; ++j) {
#endif
                    std::shuffle(kv.begin(), kv.end(), engine);

                    for (auto& i : kv) {
                        std::string k(std::get<0>(i));
                        std::string v(std::get<1>(i));
                        ASSERT_EQ(put(test_storage_name, k, v.data(), v.size()), status::OK);
                    }
                    for (auto& i : kv) {
                        std::string k(std::get<0>(i));
                        std::string v(std::get<1>(i));
                        std::pair<char*, std::size_t> ret = get<char>(test_storage_name, k);
                        ASSERT_EQ(memcmp(std::get<0>(ret), v.data(), v.size()), 0);
                    }
                    for (auto& i : kv) {
                        std::string k(std::get<0>(i));
                        std::string v(std::get<1>(i));
                        ASSERT_EQ(remove(token, test_storage_name, k), status::OK);
                    }
                }
                for (auto& i : kv) {
                    std::string k(std::get<0>(i));
                    std::string v(std::get<1>(i));
                    ASSERT_EQ(put(test_storage_name, std::string_view(k), v.data(), v.size()), status::OK);
                }

                leave(token);
            }
        };

        std::vector<std::thread> thv;
        thv.reserve(th_nm);
        for (std::size_t i = 0; i < th_nm; ++i) {
            thv.emplace_back(S::work, i);
        }
        for (auto&& th : thv) { th.join(); }
        thv.clear();

        struct parallel_verify {
            static void work(std::size_t i) {
                std::string k;
                if (i <= INT8_MAX) {
                    k = std::string(1, i);
                } else {
                    k = std::string(i / INT8_MAX, INT8_MAX) + std::string(1, i % INT8_MAX);
                }
                std::vector<std::pair<char*, std::size_t>> tuple_list;
                scan<char>(test_storage_name, "", scan_endpoint::INF, k, scan_endpoint::INCLUSIVE, tuple_list);
                if (tuple_list.size() != i + 1) {
                    scan<char>(test_storage_name, "", scan_endpoint::INF, k, scan_endpoint::INCLUSIVE, tuple_list);
                    ASSERT_EQ(tuple_list.size(), i + 1);
                }
                for (std::size_t j = 0; j < i + 1; ++j) {
                    std::string v(std::to_string(j));
                    constexpr std::size_t v_index = 0;
                    ASSERT_EQ(memcmp(std::get<v_index>(tuple_list.at(j)), v.data(), v.size()), 0);
                }
            }
        };

        thv.reserve(ary_size);
        for (std::size_t i = 0; i < ary_size; ++i) {
            thv.emplace_back(parallel_verify::work, i);
        }
        for (auto&& th : thv) { th.join(); }

        destroy();
    }
}

}// namespace yakushima::testing
