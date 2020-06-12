/**
 * @file scan_helper.h
 */

#pragma once

#include "base_node.h"
#include "border_node.h"
#include "interior_node.h"
#include "scheme.h"

namespace yakushima {

// forward decralation
template<class ValueType>
static status scan_border(border_node **target, std::string_view l_key, bool l_exclusive, std::string_view r_key,
                          bool r_exclusive, std::vector<std::tuple<ValueType *, std::size_t>> &tuple_list,
                          node_version64_body &v_at_fetch_lv);

template<class ValueType>
status scan_check_retry(border_node *const bn, const node_version64_body &v_at_fetch_lv,
                        std::vector<std::tuple<ValueType *, std::size_t>> &tuple_list,
                        const std::size_t &tuple_pushed_num) {
  node_version64_body check = bn->get_stable_version();
  if (check != v_at_fetch_lv) {
    if (tuple_pushed_num != 0) {
      tuple_list.erase(tuple_list.end() - tuple_pushed_num, tuple_list.end());
    }
    if (check.get_vsplit() != v_at_fetch_lv.get_vsplit() ||
        check.get_deleted()) {
      return status::OK_RETRY_FROM_ROOT;
    } else {
      return status::OK_RETRY_FETCH_LV;
    }
  }
  return status::OK;
}

template<class ValueType>
status scan_check_retry(border_node *const bn, const node_version64_body &v_at_fetch_lv) {
  std::vector<std::tuple<ValueType *, std::size_t>> dammy_list;
  std::size_t dammy_ctr(0);
  return scan_check_retry<ValueType>(bn, v_at_fetch_lv, dammy_list, dammy_ctr);
}

template<class ValueType>
static status scan_all(base_node *root, std::vector<std::tuple<ValueType *, std::size_t>> &tuple_list) {
retry:
  if (root->get_version_deleted() || !root->get_version_root()) {
    return status::OK_RETRY_FROM_ROOT;
  }

  std::tuple<border_node *, node_version64_body> node_and_v;
  constexpr std::size_t tuple_node_index = 0;
  constexpr std::size_t tuple_v_index = 1;
  status check_status;
  node_and_v = find_border(root, 0, 0, check_status);
  if (check_status == status::WARN_RETRY_FROM_ROOT_OF_ALL) {
    return status::OK_RETRY_FETCH_LV;
  }
  border_node *bn(std::get<tuple_node_index>(node_and_v));
  node_version64_body check_v = std::get<tuple_v_index>(node_and_v);

  for (;;) {
    check_status = scan_border<ValueType>(&bn, std::string_view(0, 0), false, std::string_view(0, 0), false,
                                          tuple_list, check_v);
    if (check_status == status::OK_SCAN_END) {
      return status::OK;
    } else if (check_status == status::OK_SCAN_CONTINUE) {
      continue;
    } else if (check_status == status::OK_RETRY_FETCH_LV) {
      node_version64_body re_check_v = bn->get_stable_version();
      if (check_v.get_vsplit() != re_check_v.get_vsplit() ||
          re_check_v.get_deleted()) {
        return status::OK_RETRY_FETCH_LV;
      } else if (check_v.get_vinsert() != re_check_v.get_vinsert() ||
                 check_v.get_vdelete() != re_check_v.get_vdelete()) {
        check_v = re_check_v;
        continue;
      }
    } else if (check_status == status::OK_RETRY_FROM_ROOT) {
      goto retry;
    }
  }

  return status::OK;
}

template<class ValueType>
static status scan_border(border_node **target, std::string_view l_key, bool l_exclusive, std::string_view r_key,
                          bool r_exclusive, std::vector<std::tuple<ValueType *, std::size_t>> &tuple_list,
                          node_version64_body &v_at_fetch_lv) {
retry:
  std::size_t tuple_pushed_num{0};
  border_node *bn = *target;
  border_node *next = bn->get_next();
  permutation perm(bn->get_permutation().get_body());
  for (std::size_t i = 0; i < perm.get_cnk(); ++i) {
    std::size_t index = perm.get_index_of_rank(i);
    key_slice_type ks = bn->get_key_slice_at(index);
    key_length_type kl = bn->get_key_length_at(index);
    link_or_value *lv = bn->get_lv_at(index);
    void *vp = lv->get_v_or_vp_();
    std::size_t vsize = lv->get_value_length();
    base_node *next_layer = lv->get_next_layer();
    status check_status = scan_check_retry(bn, v_at_fetch_lv, tuple_list, tuple_pushed_num);
    if (check_status == status::OK_RETRY_FROM_ROOT) {
      return status::OK_RETRY_FROM_ROOT;
    } else if (check_status == status::OK_RETRY_FETCH_LV) {
      goto retry;
    }
    if (kl > sizeof(key_slice_type)) {
      check_status = scan_all(next_layer, tuple_list);
      if (check_status != status::OK) {
        goto retry;
      }
    } else {
      std::string_view resident{reinterpret_cast<char *>(&ks), kl};
      std::string_view inf{0, 0};
      if (resident < l_key || (resident == l_key && l_exclusive)) {
        continue;
      } else if ((l_key == inf && r_key == inf) || // all range
                 (l_key == inf && resident < r_key) || // left is inf, in range
                 (l_key < resident && r_key == inf) || // right is inf, in range
                 (l_key < resident && resident < r_key) || // no inf, in range
                 (resident == l_key && !l_exclusive) ||
                 (resident == r_key && !r_exclusive)) {
        tuple_list.emplace_back(std::make_tuple(reinterpret_cast<ValueType *>(vp), vsize));
        ++tuple_pushed_num;
      } else {
        return status::OK_SCAN_END;
      }
    }
  }

  if (next == nullptr) {
    return status::OK_SCAN_END;
  } else {
    *target = next;
    v_at_fetch_lv = next->get_stable_version();
    return status::OK_SCAN_CONTINUE;
  }
}

} // namespace yakushima