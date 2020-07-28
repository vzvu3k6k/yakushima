/**
 * @file border_node.h
 */

#pragma once

#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>

#include "atomic_wrapper.h"
#include "interior_node.h"
#include "link_or_value.h"
#include "permutation.h"
#include "thread_info.h"

namespace yakushima {

using std::cout;
using std::endl;

class alignas(CACHE_LINE_SIZE) border_node final : public base_node { // NOLINT
public:

  ~border_node() override {} // NOLINT

  /**
   * @pre This function is called by delete_of function.
   * @details delete the key-value corresponding to @a pos as position.
   * @param[in] pos The position of being deleted.
   * @param[in] target_is_value
   */
  void delete_at(Token token, std::size_t pos, bool target_is_value) {
    thread_info *ti = reinterpret_cast<thread_info *>(token); // NOLINT
    if (target_is_value) {
      ti->move_value_to_gc_container(lv_.at(pos).get_v_or_vp_());
    }
    lv_.at(pos).init_lv();

    /**
     * rearrangement.
     */
    if (pos == static_cast<std::size_t>(get_permutation_cnk() - 1)) { // tail
      init_border(pos);
    } else { // not-tail
      shift_left_base_member(pos + 1, 1);
      shift_left_border_member(pos + 1, 1);
      init_border(static_cast<size_t>(permutation_.get_cnk() - 1));
    }
    permutation_.dec_key_num();
    permutation_rearrange();
  }

  /**
   * @pre There is a lv which points to @a child.
   * @details Delete operation on the element matching @a child.
   * @param[in] token
   * @param[in] child
   */
  void delete_of(Token token, base_node *child, std::vector<node_version64 *> &lock_list) {
    std::size_t cnk = get_permutation_cnk();
    for (std::size_t i = 0; i < cnk; ++i) {
      if (child == lv_.at(i).get_next_layer()) {
        delete_of < false > (token, get_key_slice_at(i), get_key_length_at(i), lock_list); // NOLINT
        return;
      }
    }
    // unreachable points.
    std::cerr << __FILE__ << " : " << __LINE__ << " : " << std::endl;
    std::abort();
  }

  /**
   * @brief release all heap objects and clean up.
   */
  status destroy() override {
    for (auto i = 0; i < permutation_.get_cnk(); ++i) {
      lv_.at(static_cast<std::uint32_t>(i)).destroy();
    }
    return status::OK_DESTROY_BORDER;
  }

  /**
   * @pre
   * This border node was already locked by caller.
   * This function is called by remove func.
   * The key-value corresponding to @a key_slice and @a key_length exists in this node.
   * @details delete value corresponding to @a key_slice and @a key_length
   * @param[in] token
   * @param[in] key_slice The key slice of key-value.
   * @param[in] key_slice_length The @a key_slice length.
   * @param[in] target_is_value
   */
  template<bool target_is_value>
  void delete_of(Token token, key_slice_type key_slice, key_length_type key_slice_length,
                 std::vector<node_version64 *> &lock_list) {
    set_version_inserting_deleting(true);
    /**
     * find position.
     */
    std::size_t cnk = get_permutation_cnk();
    for (std::size_t i = 0; i < cnk; ++i) {
      if (key_slice == get_key_slice_at(i) && key_slice_length == get_key_length_at(i)) {
        delete_at(token, i, target_is_value);
        if (cnk == 1) { // attention : this cnk is before delete_at;
          /**
           * After this delete operation, this border node is empty.
           */
retry_prev_lock:
          border_node *prev = get_prev();
          if (prev != nullptr) {
            prev->lock();
            if (prev->get_version_deleted() ||
                prev != get_prev()) {
              prev->version_unlock();
              goto retry_prev_lock; // NOLINT
            } else {
              prev->set_next(get_next());
              if (get_next() != nullptr) {
                get_next()->set_prev(prev);
              }
              prev->version_unlock();
            }
          } else if (get_next() != nullptr) {
            get_next()->set_prev(nullptr);
          }
          /**
           * lock order is next to prev and lower to higher.
           */
          base_node *pn = lock_parent();
          if (pn == nullptr) {
            base_node::set_root(nullptr);
          } else {
            if (pn->get_version_border()) {
              dynamic_cast<border_node *>(pn)->delete_of(token, this, lock_list);
            } else {
              dynamic_cast<interior_node *>(pn)->delete_of<border_node>(token, this);
            }
            pn->version_unlock();
          }
          set_version_deleted(true);
          reinterpret_cast<thread_info *>(token)->move_node_to_gc_container(this); // NOLINT
        }
        return;
      }
    }
    /**
     * unreachable.
     */
    std::cerr << __FILE__ << ": " << __LINE__ << " : it gets to unreachable points." << endl;
    std::abort();
  }

  /**
   * @details display function for analysis and debug.
   */
  void display() override {
    display_base();
    cout << "border_node::display" << endl;
    permutation_.display();
    for (std::size_t i = 0; i < get_permutation_cnk(); ++i) {
      lv_.at(i).display();
    }
    cout << "next : " << get_next() << endl;
  }

  /**
  * @post It is necessary for the caller to verify whether the extraction is appropriate.
  * @param[out] next_layers
  * @attention layers are stored in ascending order.
  * @return
  */
  [[maybe_unused]]void get_all_next_layer(std::vector<base_node *> &next_layers) {
    next_layers.clear();
    std::size_t cnk = permutation_.get_cnk();
    for (std::size_t i = 0; i < cnk; ++i) {
      link_or_value *lv = get_lv_at(permutation_.get_index_of_rank(i));
      base_node *nl = lv->get_next_layer();
      if (nl != nullptr) {
        next_layers.emplace_back(nl);
      }
    }
  }

  [[maybe_unused]] [[nodiscard]] std::array<link_or_value, key_slice_length> &get_lv() {
    return lv_;
  }

  /**
  * @details Find link_or_value element whose next_layer is the same as @a next_layer of the argument.
  * @pre Executor has lock of this node. There is always a lv that points to a pointer given as an argument.
  * @param[in] next_layer
  * @return link_or_value*
  */
  [[maybe_unused]] [[nodiscard]] link_or_value *get_lv(base_node *next_layer) {
    for (std::size_t i = 0; i < base_node::key_slice_length; ++i) {
      if (lv_.at(i).get_next_layer() == next_layer) {
        return &lv_.at(i);
      }
    }
    /**
     * unreachable point.
     */
    std::cerr << __FILE__ << " : " << __LINE__ << " : " << "fatal error." << std::endl;
    std::abort();
  }

  [[nodiscard]] link_or_value *get_lv_at(std::size_t index) {
    return &lv_.at(index);
  }

  /**
   * @attention This function must not be called with locking of this node. Because this function executes
   * get_stable_version and it waits own (lock-holder) infinitely.
   * @param[in] key_slice
   * @param[in] key_length
   * @param[out] stable_v  the stable version which is at atomically fetching lv.
   * @param[out] lv_pos
   * @return
   */
  [[nodiscard]] link_or_value *get_lv_of(key_slice_type key_slice, key_length_type key_length,
                                         node_version64_body &stable_v, std::size_t &lv_pos) {
    node_version64_body v = get_stable_version();
    for (;;) {
      /**
       * It loads cnk atomically by get_cnk func.
       */
      std::size_t cnk = permutation_.get_cnk();
      link_or_value *ret_lv{nullptr};
      for (std::size_t i = 0; i < cnk; ++i) {
        if (key_slice == get_key_slice_at(i)) {
          if ((key_length == get_key_length_at(i)) ||
              (key_length > sizeof(key_slice_type) && get_key_length_at(i) > sizeof(key_slice_type))) {
            ret_lv = get_lv_at(i);
            lv_pos = i;
            break;
          }
        }
      }
      node_version64_body v_check = get_stable_version();
      if (v == v_check) {
        stable_v = v;
        return ret_lv;
      }
      v = v_check;
    }
  }

  border_node *get_next() {
    return loadAcquireN(next_);
  }

  permutation &get_permutation() {
    return permutation_;
  }

  [[nodiscard]] std::uint8_t get_permutation_cnk() {
    return permutation_.get_cnk();
  }

  [[maybe_unused]] [[nodiscard]] std::size_t get_permutation_lowest_key_pos() {
    return permutation_.get_lowest_key_pos();
  }

  border_node *get_prev() {
    return loadAcquireN(prev_);
  }

  void init_border() {
    init_base();
    init_border_member_range(0);
    set_version_border(true);
    permutation_.init();
    set_next(nullptr);
    set_prev(nullptr);
  }

  /**
   * @details init at @a pos as position.
   * @param[in] pos This is a position (index) to be initialized.
   */
  void init_border(std::size_t pos) {
    init_base(pos);
    lv_.at(pos).init_lv();
  }

  /**
   * @pre This function is called by put function.
   * @pre @a arg_value_length is divisible by sizeof( @a ValueType ).
   * @pre This function can not be called for updating existing nodes.
   * @pre If this function is used for node creation, link after set because
   * set function does not execute lock function.
   * @details This function inits border node by using arguments.
   * @param[in] key_view
   * @param[in] value_ptr
   * @param[in] root is the root node of the layer.
   * @param[in] arg_value_length
   * @param[in] value_align
   */
  template<class ValueType>
  void init_border(std::string_view key_view,
                   ValueType *value_ptr,
                   bool root,
                   value_length_type arg_value_length = sizeof(ValueType),
                   std::size_t value_align = alignof(ValueType)) {
    init_border();
    set_version_root(root);
    set_version_border(true);
    insert_lv_at(0, key_view, value_ptr, arg_value_length, value_align);
  }

  void init_border_member_range(std::size_t start) {
    for (auto i = start; i < lv_.size(); ++i) {
      lv_.at(i).init_lv();
    }
  }

  /**
   * @pre It already locked this node.
   * @param[in] index
   * @param[in] key_view
   * @param[in] value_ptr
   * @param[in] arg_value_length
   * @param[in] value_align
   */
  void insert_lv_at(std::size_t index,
                    std::string_view key_view,
                    void *value_ptr,
                    value_length_type arg_value_length,
                    value_align_type value_align) {
    /**
     * @attention key_slice must be initialized to 0.
     * If key_view.size() is smaller than sizeof(key_slice_type),
     * it is not possible to update the whole key_slice object with memcpy.
     * It is possible that undefined values may remain from initialization.
     */
    key_slice_type key_slice(0);
    if (key_view.size() > sizeof(key_slice_type)) {
      /**
       * Create multiple border nodes.
       */
      memcpy(&key_slice, key_view.data(), sizeof(key_slice_type));
      set_key_slice_at(index, key_slice);
      /**
       * You only need to know that it is 8 bytes or more. If it is stored obediently,
       * key_length_type must be a large size type.
       */
      set_key_length_at(index, sizeof(key_slice_type) + 1);
      border_node *next_layer_border = new border_node(); // NOLINT
      key_view.remove_prefix(sizeof(key_slice_type));
      /**
       * @attention next_layer_border is the root of next layer.
       */
      next_layer_border->init_border(key_view, value_ptr, true, arg_value_length, value_align);
      next_layer_border->set_parent(this);
      set_lv_next_layer(index, next_layer_border);
    } else {
      memcpy(&key_slice, key_view.data(), key_view.size());
      set_key_slice_at(index, key_slice);
      set_key_length_at(index, static_cast<key_length_type>(key_view.size()));
      set_lv_value(index, value_ptr, arg_value_length, value_align);
    }
    permutation_.inc_key_num();
    permutation_rearrange();
  }

  void permutation_rearrange() {
    permutation_.rearrange(get_key_slice(), get_key_length());
  }

  [[maybe_unused]] void set_permutation_cnk(std::uint8_t n) {
    permutation_.set_cnk(n);
  }

  /**
   * @pre This is called by split process.
   * @param index
   * @param nlv
   */
  void set_lv(std::size_t index, link_or_value *nlv) {
    lv_.at(index).set(nlv);
  }

  void set_lv_value(std::size_t index,
                    void *value,
                    value_length_type arg_value_length,
                    value_align_type value_align) {
    lv_.at(index).set_value(value, arg_value_length, value_align);
  }

  void set_lv_next_layer(std::size_t index, base_node *next_layer) {
    lv_.at(index).set_next_layer(next_layer);
  }

  void set_next(border_node *nnext) {
    storeReleaseN(next_, nnext);
  }

  void set_prev(border_node *prev) {
    storeReleaseN(prev_, prev);
  }

  void shift_left_border_member(std::size_t start_pos, std::size_t shift_size) {
    memmove(get_lv_at(start_pos - shift_size), get_lv_at(start_pos),
            sizeof(link_or_value) * (key_slice_length - start_pos));
  }

private:
  // first member of base_node is aligned along with cache line size.
  /**
   * @attention This variable is read/written concurrently.
   */
  permutation permutation_{};
  /**
   * @attention This variable is read/written concurrently.
   */
  std::array<link_or_value, key_slice_length> lv_{};
  /**
   * @attention This is protected by its previous sibling's lock.
   */
  border_node *prev_{nullptr};
  /**
   * @attention This variable is read/written concurrently.
   */
  border_node *next_{nullptr};
};
} // namespace yakushima
