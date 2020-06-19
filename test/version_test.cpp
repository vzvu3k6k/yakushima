/**
 * @file version_test.cpp
 */

#include <future>
#include <thread>

#include "gtest/gtest.h"

#include "version.h"

using namespace yakushima;
using std::cout;
using std::endl;

namespace yakushima::testing {

class version_test : public ::testing::Test {
protected:
  version_test() = default;

  ~version_test() = default;
};

TEST_F(version_test, operator_node_version64_body_test) {
  node_version64_body b1, b2;
  b1.init();
  b2.init();
  ASSERT_EQ(b1, b2);
  b1.set_locked(!b1.get_locked());
  ASSERT_NE(b1, b2);
}

TEST_F(version_test, basic_node_version_test) {
// basic member test.
  {
    node_version64 ver;
    node_version64_body verbody;
    ASSERT_EQ(ver.get_body().get_locked(), false);
    verbody = ver.get_body();
    verbody.set_locked(true);
    ver.set_body(verbody);
    ASSERT_EQ(ver.get_body().get_locked(), true);
  }

  // single update test.
  {
    node_version64 ver;
    auto vinsert_inc_100 = [&ver]() {
      for (auto i = 0; i < 100; ++i) {
        ver.atomic_inc_vinsert();
      }
    };
    vinsert_inc_100();
    ASSERT_EQ(ver.get_body().get_vinsert(), 100);
  }

// concurrent update test.
  {
    node_version64 ver;
    auto vinsert_inc_100 = [&ver]() {
      for (auto i = 0; i < 100; ++i) {
        ver.atomic_inc_vinsert();
      }
    };
    std::future<void> f = std::async(std::launch::async, vinsert_inc_100);
    vinsert_inc_100();
    f.wait();
    ASSERT_EQ(ver.get_body().get_vinsert(), 200);
  }

}

TEST_F(version_test, display) {
  node_version64_body vb;
  vb.init();
  ASSERT_EQ(true, true);
  //vb.display();

  node_version64 v;
  v.init();
  ASSERT_EQ(true, true);
  //v.display();
}

}  // namespace yakushima::testing
