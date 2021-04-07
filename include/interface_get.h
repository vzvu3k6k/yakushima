#pragma once

#include "base_node.h"
#include "border_node.h"
#include "common_helper.h"
#include "link_or_value.h"
#include "kvs.h"

namespace yakushima {

template<class ValueType>
[[maybe_unused]] static std::pair<ValueType*, std::size_t> get(std::string_view key_view) {
retry_from_root:
    base_node* root = base_node::get_root_ptr();
    if (root == nullptr) {
        return std::make_pair(nullptr, 0);
    }
    std::string_view traverse_key_view{key_view};
retry_find_border:
    /**
     * prepare key_slice
     */
    key_slice_type key_slice(0);
    auto key_slice_length = static_cast<key_length_type>(traverse_key_view.size());
    if (traverse_key_view.size() > sizeof(key_slice_type)) {
        memcpy(&key_slice, traverse_key_view.data(), sizeof(key_slice_type));
    } else {
        if (!traverse_key_view.empty()) {
            memcpy(&key_slice, traverse_key_view.data(), traverse_key_view.size());
        }
    }
    /**
     * traverse tree to border node.
     */
    status special_status{status::OK};
    std::tuple<border_node*, node_version64_body> node_and_v = find_border(root, key_slice, key_slice_length,
                                                                           special_status);
    if (special_status == status::WARN_RETRY_FROM_ROOT_OF_ALL) {
        /**
         * @a root is the root node of the some layer, but it was deleted.
         * So it must retry from root of the all tree.
         */
        goto retry_from_root; // NOLINT
    }
    constexpr std::size_t tuple_node_index = 0;
    constexpr std::size_t tuple_v_index = 1;
    border_node* target_border = std::get<tuple_node_index>(node_and_v);
    node_version64_body v_at_fb = std::get<tuple_v_index>(node_and_v);
retry_fetch_lv:
    node_version64_body v_at_fetch_lv{};
    std::size_t lv_pos{0};
    link_or_value* lv_ptr = target_border->get_lv_of(key_slice, key_slice_length, v_at_fetch_lv, lv_pos);
    /**
     * check whether it should get from this node.
     */
    if (v_at_fetch_lv.get_vsplit() != v_at_fb.get_vsplit() || v_at_fetch_lv.get_deleted()) {
        /**
         * The correct border was changed between atomically fetching border node and atomically fetching lv.
         */
        goto retry_from_root; // NOLINT
    }

    if (lv_ptr == nullptr) {
        return std::make_pair(nullptr, 0);
    }

    if (target_border->get_key_length_at(lv_pos) <= sizeof(key_slice_type)) {
        void* vp = lv_ptr->get_v_or_vp_();
        std::size_t v_size = lv_ptr->get_value_length();
        node_version64_body final_check = target_border->get_stable_version();
        if (final_check.get_vsplit() != v_at_fb.get_vsplit()
            || final_check.get_deleted()) {
            goto retry_from_root; // NOLINT
        }
        if (final_check.get_vinsert_delete() != v_at_fetch_lv.get_vinsert_delete()) {
            goto retry_fetch_lv; // NOLINT
        }
        return std::make_pair(reinterpret_cast<ValueType*>(vp), v_size); // NOLINT
    }

    root = lv_ptr->get_next_layer();
    node_version64_body final_check = target_border->get_stable_version();
    if (final_check.get_vsplit() != v_at_fb.get_vsplit()
        || final_check.get_deleted()) {
        goto retry_from_root; // NOLINT
    }
    if (final_check.get_vinsert_delete() != v_at_fetch_lv.get_vinsert_delete()) {
        goto retry_fetch_lv; // NOLINT
    }
    traverse_key_view.remove_prefix(sizeof(key_slice_type));
    goto retry_find_border; // NOLINT
}

}