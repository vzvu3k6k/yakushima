/**
 * @file mt_interior_node.h
 */

#pragma once

#include <cstdint>

#include "atomic_wrapper.h"
#include "base_node.h"
#include "border_node.h"

namespace yakushima {
class interior_node final : public base_node {
public:
  static constexpr std::size_t child_length = 16;

  interior_node() = default;

  ~interior_node() = default;

  /**
   * @brief release all heap objects and clean up.
   * @pre This function is called by single thread.
   */
  void destroy() final {
    for (auto i = 0; i < nkeys_; ++i) {
      child[i]->destroy();
    }
  }

  base_node *get_child(std::uint64_t key_slice) {
    node_version64_body v = get_stable_version();
    for (;;) {
      std::uint8_t nkey = nkeys_.load(std::memory_order_acquire);
      base_node *ret_child{nullptr};
      for (auto i = 0; i < nkey; ++i) {
        ret_child = loadAcquire(&child[i]);
        if (ret_child != nullptr && key_slice == get_key_slice_at(i))
          break;
        else
          ret_child = nullptr;
      }
      node_version64_body check = get_stable_version();
      if (v == check) return ret_child;
      else v = check;
    }
  }


private:
  /**
   * first member of base_node is aligned along with cache line size.
   */

  /**
   * @attention This variable is read/written concurrently.
   */
  base_node *child[child_length]{};
  /**
   * @attention This variable is read/written concurrently.
   */
  std::atomic<uint8_t> nkeys_{};
};

} // namespace yakushima
