#pragma once

/**
 * @file
 * @brief atomic wrapper of builtin function.
 */

namespace yakushima {

/**
 * @brief atomic relaxed load.
 */
template<typename T>
static T loadRelaxed(T &ptr) {
  return __atomic_load_n(&ptr, __ATOMIC_RELAXED); // NOLINT
}

/**
 * @brief atomic acquire load.
 */
template<typename T>
static T loadAcquireN(T &ref) { // NOLINT
  return __atomic_load_n(&ref, __ATOMIC_ACQUIRE); // NOLINT
}

#if 0
template<class type>
void loadAcquire(type *ptr, type *ret) {
  __atomic_load(ptr, ret, __ATOMIC_ACQUIRE);
}
#endif

/**
 * @brief atomic relaxed store.
 */
template<typename T, typename T2>
static void storeRelaxed(T &ptr, T2 val) {
  __atomic_store_n(&ptr, static_cast<T>(val), __ATOMIC_RELAXED); // NOLINT
}

/**
 * @brief atomic release store.
 */
template<typename T, typename T2>
static void storeReleaseN(T &ptr, T2 val) {
  __atomic_store_n(&ptr, static_cast<T>(val), __ATOMIC_RELEASE); // NOLINT
}

#if 0
template<class type>
void storeRelease(type *ptr, type *val) {
  __atomic_store(ptr, val, __ATOMIC_RELEASE);
}

/**
 * @brief atomic acq-rel cas.
 */
template<typename type>
bool weakCompareExchange(type *ptr, type *expected, type *desired) {
  /**
   * Built-in Function: bool __atomic_compare_exchange_n
   * (type *ptr, type *expected, type desired, bool weak, int success_memorder, int failure_memorder)
   */
  return __atomic_compare_exchange(ptr, expected, desired, true,
                                   __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
}
#endif

} // namespace yakushima
