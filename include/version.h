/**
 * @file version.h
 * @brief version number layout
 */

#pragma once

#include <atomic>
#include <bitset>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>
#include <xmmintrin.h>

#include "atomic_wrapper.h"

namespace yakushima {

/**
 * @brief Teh body of node_version64.
 * @details This class is designed to be able to be wrapped by std::atomic,
 * so it can't declare default constructor. Therefore, it should use init function to initialize
 * before using this class object.
 */
class node_version64_body {
public:
  using vinsert_delete_type = std::uint32_t;
  using vsplit_type = std::uint32_t;

  node_version64_body() = default;

  node_version64_body(const node_version64_body &) = default;

  node_version64_body(node_version64_body &&) = default;

  node_version64_body &operator=(const node_version64_body &) = default;

  node_version64_body &operator=(node_version64_body &&) = default;

  ~node_version64_body() = default;

  bool operator==(const node_version64_body &rhs) const {
    return get_locked() == rhs.get_locked()
           && get_inserting_deleting() == rhs.get_inserting_deleting()
           && get_splitting() == rhs.get_splitting()
           && get_deleted() == rhs.get_deleted()
           && get_root() == rhs.get_root()
           && get_border() == rhs.get_border()
           && get_vinsert_delete() == rhs.get_vinsert_delete()
           && get_vsplit() == rhs.get_vsplit();
  }

  /**
   * @details display function for analysis and debug.
   */
  void display() const {
    std::cout << "node_version64_body::display" << std::endl;
    std::cout << "locked : " << get_locked() << std::endl;
    std::cout << "inserting_deleting : " << get_inserting_deleting() << std::endl;
    std::cout << "splitting : " << get_splitting() << std::endl;
    std::cout << "deleted : " << get_deleted() << std::endl;
    std::cout << "root : " << get_root() << std::endl;
    std::cout << "border : " << get_border() << std::endl;
    std::cout << "vinsert_delete : " << get_vinsert_delete() << std::endl;
    std::cout << "vsplit: " << get_vsplit() << std::endl;
  }

  bool operator!=(const node_version64_body &rhs) const {
    return !(*this == rhs);
  }

  [[nodiscard]] bool get_border() const {
    return border;
  }

  [[nodiscard]] bool get_deleted() const {
    return deleted;
  }

  [[nodiscard]] bool get_inserting_deleting() const {
    return inserting_deleting;
  }

  [[nodiscard]] bool get_locked() const {
    return locked;
  }

  [[nodiscard]] bool get_root() const {
    return root;
  }

  [[nodiscard]] bool get_splitting() const {
    return splitting;
  }

  [[nodiscard]] vinsert_delete_type get_vinsert_delete() const {
    return vinsert_delete;
  }

  [[nodiscard]] vsplit_type get_vsplit() const {
    return vsplit;
  }

  void inc_vinsert_delete() {
    ++vinsert_delete;
  }

  void inc_vsplit() {
    ++vsplit;
  }

  void init() {
    locked = false;
    inserting_deleting = false;
    splitting = false;
    deleted = false;
    root = false;
    border = false;
    vinsert_delete = 0;
    vsplit = 0;
  }

  void set_border(bool new_border) &{
    border = new_border;
  }

  void set_deleted(bool new_deleted) &{
    deleted = new_deleted;
  }

  void set_inserting_deleting(bool new_inserting_deleting) &{
    inserting_deleting = new_inserting_deleting;
  }

  void set_locked(bool new_locked) &{
    locked = new_locked;
  }

  void set_root(bool new_root) &{
    root = new_root;
  }

  void set_splitting(bool new_splitting) &{
    splitting = new_splitting;
  }

private:
  /**
   * These details is based on original paper Fig. 3.
   * Declaration order is because class size does not exceed 8 bytes.
   */
  /**
   * @attention tanabe : In the original paper, the interior node does not have a delete count field.
   * On the other hand, the border node has this (nremoved) but no details in original paper.
   * Since there is a @a deleted field in the version, you can check whether the node you are checking has been deleted.
   * However, you do not know that the position has been moved.
   * Maybe you took the node from the wrong position.
   * The original paper cannot detect it.
   * Therefore, add notion of delete field.
   * @details It is a counter incremented after inserting_deleting/deleting.
   */
  vinsert_delete_type vinsert_delete: 29;
  /**
   * @details It is claimed by update or insert.
   */
  bool locked: 1;
  /**
   * @details It is a dirty bit set during inserting_deleting.
   */
  bool inserting_deleting: 1;
  /**
   * @details It is a dirty bit set during splitting.
   * If this flag is set, vsplit is incremented when the lock is unlocked.
   * The function find_lowest_key takes the value from the node when this flag is up. Read. When we raise this flag,
   * we guarantee that no problems will occur with it.
   */
  bool splitting: 1;
  /**
   * @details It is a counter incremented after splitting.
   */
  vsplit_type vsplit: 29;
  /**
   * @details It is a delete bit set after delete.
   */
  bool deleted: 1;
  /**
   * @details It tells whether the node is the root of some B+-tree.
   */
  bool root: 1;
  /**
   * @details It tells whether the node is interior or border.
   */
  bool border: 1;
};

/**
 * @brief The class which has atomic<node_version>
 */
class node_version64 {
public:
  /**
   * @details Basically, it should use this default constructor to use init func.
   * Of course, it can use this class without default constructor if it use init function().
   */
  node_version64() : body_{} {}

  /**
   * @details This is atomic increment.
   * If you use "setter(getter + 1)", that is not atomic increment.
   */
  void atomic_inc_vinsert() {
    node_version64_body expected(get_body());
    node_version64_body desired{};
    for (;;) {
      desired = expected;
      desired.inc_vinsert_delete();
      if (body_.compare_exchange_weak(expected, desired, std::memory_order_acq_rel, std::memory_order_acquire)) {
        break;
      }
    }
  }

  void atomic_set_border(bool tf) {
    node_version64_body expected(get_body());
    node_version64_body desired{};
    for (;;) {
      desired = expected;
      desired.set_border(tf);
      if (body_.compare_exchange_weak(expected, desired, std::memory_order_acq_rel, std::memory_order_acquire)) {
        break;
      }
    }
  }

  void atomic_set_deleted(bool tf) {
    node_version64_body expected(get_body());
    node_version64_body desired{};
    for (;;) {
      desired = expected;
      desired.set_deleted(tf);
      if (body_.compare_exchange_weak(expected, desired, std::memory_order_acq_rel, std::memory_order_acquire)) {
        break;
      }
    }
  }

  void atomic_set_inserting_deleting(bool tf) &{
    node_version64_body expected(get_body());
    node_version64_body desired{};
    for (;;) {
      desired = expected;
      desired.set_inserting_deleting(tf);
      if (body_.compare_exchange_weak(expected, desired, std::memory_order_acq_rel, std::memory_order_acquire)) {
        break;
      }
    }
  }

  void atomic_set_root(bool tf) {
    node_version64_body expected(get_body());
    node_version64_body desired{};
    for (;;) {
      desired = expected;
      desired.set_root(tf);
      if (body_.compare_exchange_weak(expected, desired, std::memory_order_acq_rel, std::memory_order_acquire)) {
        break;
      }
    }
  }

  void atomic_set_splitting(bool tf) {
    node_version64_body expected(get_body());
    node_version64_body desired{};
    for (;;) {
      desired = expected;
      desired.set_splitting(tf);
      if (body_.compare_exchange_weak(expected, desired, std::memory_order_acq_rel, std::memory_order_acquire)) {
        break;
      }
    }
  }

  /**
   * @details display function for analysis and debug.
   */
  void display() {
    get_body().display();
  }

  /**
   * @details This function locks atomically.
   * @return void
   */
  void lock() {
    node_version64_body expected(get_body());
    node_version64_body desired{};
    for (;;) {
      if (expected.get_locked()) {
        _mm_pause();
        expected = get_body();
        continue;
      }
      desired = expected;
      desired.set_locked(true);
      if (body_.compare_exchange_weak(expected, desired, std::memory_order_acq_rel, std::memory_order_acquire)) {
        break;
      }
    }
  }

  [[nodiscard]] node_version64_body get_body() {
    return body_.load(std::memory_order_acquire);
  }

  [[nodiscard]] bool get_border() {
    return get_body().get_border();
  }

  [[nodiscard]] bool get_deleted() {
    return get_body().get_deleted();
  }

  [[nodiscard]] bool get_locked() {
    return get_body().get_locked();
  }

  [[nodiscard]] bool get_root() {
    return get_body().get_root();
  }

  [[nodiscard]] node_version64_body get_stable_version() {
    for (;;) {
      node_version64_body sv = get_body();
      /**
       * In the original paper, lock is not checked.
       * However, if the lock is acquired, the member of that node can be changed.
       * Even if the locked version is immutable, the members read at that time may be broken.
       * Therefore, you have to check the lock.
       */
      if (!sv.get_inserting_deleting() && !sv.get_locked() && !sv.get_splitting()) {
        return sv;
      }
      _mm_pause();
    }
  }

  [[nodiscard]] node_version64_body::vinsert_delete_type get_vinsert_delete() {
    return get_body().get_vinsert_delete();
  }

  [[nodiscard]] node_version64_body::vsplit_type get_vsplit() {
    return get_body().get_vsplit();
  }

  /**
   * @pre This function is called by only single thread.
   */
  void init() {
    set_body(node_version64_body());
  }

  void set_body(node_version64_body newv) &{
    body_.store(newv, std::memory_order_release);
  }

  /**
   * @details This function unlocks @a atomically.
   * @pre The caller already succeeded acquiring lock.
   */
  void unlock() &{
    node_version64_body expected(get_body());
    node_version64_body desired{};
    for (;;) {
      desired = expected;
      if (desired.get_inserting_deleting()) {
        desired.inc_vinsert_delete();
        desired.set_inserting_deleting(false);
      }
      if (desired.get_splitting()) {
        desired.inc_vsplit();
        desired.set_splitting(false);
      }
      desired.set_locked(false);
      if (body_.compare_exchange_weak(expected, desired, std::memory_order_acq_rel, std::memory_order_acquire)) break;
    }
  }

  static void unlock(std::vector<node_version64 *> &lock_list) {
    for (auto &&l : lock_list) {
      l->unlock();
    }
  }

private:
  std::atomic<node_version64_body> body_;
};

} // namespace yakushima