#pragma once

#include <cpuid.h>
#include <sched.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <cstdint>
#include <iostream>

#define CPUID(INFO, LEAF, SUBLEAF) \
  __cpuid_count(LEAF, SUBLEAF, INFO[0], INFO[1], INFO[2], INFO[3])

#define GETCPU(CPU)                                                 \
  {                                                                 \
    uint32_t CPUInfo[4];                                            \
    CPUID(CPUInfo, 1, 0);                                           \
    /* CPUInfo[1] is EBX, bits 24-31 are APIC ID */                 \
    if ((CPUInfo[3] & (1 << 9)) == 0) {                             \
      CPU = -1; /* no APIC on chip */                               \
    } else {                                                        \
      CPU = (unsigned)CPUInfo[1] >> 24;                             \
      /*unsigned int cores = ((unsigned)CPUInfo[1] >> 16) & 0xff;*/ \
      /*if ((CPUInfo[3] & (1 << 28)) == 1) printf("HTT\n");*/       \
      /*printf("total core number : %d\n", cores);*/                \
    }                                                               \
    if (CPU < 0) CPU = 0;                                           \
  }

#ifdef YAKUSHIMA_LINUX
[[maybe_unused]] static void set_thread_affinity(const int myid) {
  using namespace std;
  static std::atomic<int> nprocessors(-1);
  int local_nprocessors, desired;
  local_nprocessors = nprocessors.load(memory_order_acquire);
  for (;;) {
    if (local_nprocessors != -1) {
      break;
    } else {
      desired = sysconf(_SC_NPROCESSORS_CONF);
      if (nprocessors.compare_exchange_strong(
            local_nprocessors, desired, memory_order_acq_rel, memory_order_acquire)) {
        break;
      }
    }
  }

  pid_t pid = syscall(SYS_gettid);
  cpu_set_t cpu_set;

  CPU_ZERO(&cpu_set);
  CPU_SET(myid % local_nprocessors, &cpu_set);

  if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_set) != 0) {
    std::cout << __FILE__ << " : " << __LINE__ << " : error" << std::endl;
    std::abort();
  }
  return;
}

[[maybe_unused]] static void set_thread_affinity(const cpu_set_t id) {
  pid_t pid = syscall(SYS_gettid);

  if (sched_setaffinity(pid, sizeof(cpu_set_t), &id) != 0) {
    std::cout << __FILE__ << " : " << __LINE__ << " : error" << std::endl;
    std::abort();
  }
  return;
}

[[maybe_unused]] static cpu_set_t get_thread_affinity() {
  pid_t pid = syscall(SYS_gettid);
  cpu_set_t result;

  if (sched_getaffinity(pid, sizeof(cpu_set_t), &result) != 0) {
    std::cout << __FILE__ << " : " << __LINE__ << " : error" << std::endl;
    std::abort();
  }

  return result;
}

#endif  // YAKUSHIMA_LINUX