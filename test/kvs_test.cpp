/**
 * @file kvs_test.cpp
 */

#include <tuple>

#include "gtest/gtest.h"

#include "kvs.h"

using namespace yakushima;

namespace yakushima::testing {

class kvs_test : public ::testing::Test { // NOLINT
public:
  kvs_test() {
    masstree_kvs::init();
  }

  ~kvs_test() {
    masstree_kvs::fin();
  }
};

TEST_F(kvs_test, init) { // NOLINT
  ASSERT_EQ(base_node::get_root(), nullptr);
  Token token{};
  ASSERT_EQ(masstree_kvs::enter(token), status::OK);
  std::string k("a"), v("v-a");
  ASSERT_EQ(status::OK, masstree_kvs::put(std::string_view(k), v.data(), v.size()));
  ASSERT_NE(base_node::get_root(), nullptr);
  ASSERT_EQ(masstree_kvs::leave(token), status::OK);
}

}  // namespace yakushima::testing
