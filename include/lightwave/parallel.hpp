/**
 * @file parallel.hpp
 * @brief Utilities to ease multi-threaded execution of tasks.
 */

#pragma once

#include <mutex>
#include <thread>

#include <lightwave/color.hpp>
#include <lightwave/logger.hpp>

#ifdef LW_DEBUG
// if you feel uncomfortable debugging multi-threaded code, feel free to enable
// this define and compile in Debug mode
// #define SINGLE_THREADED
#endif

namespace lightwave {

/// @brief Invokes @c f for each element of the iterator, parallelized across
/// all available cores.
template <class ForwardIt, class UnaryFunction>
void for_each_parallel(ForwardIt first, ForwardIt last, UnaryFunction f) {
#ifdef SINGLE_THREADED
    std::for_each(first, last, f);
    return;
#endif

    std::mutex m_lock;

    const int numThreads = std::thread::hardware_concurrency();
    std::vector<std::thread> m_threads;
    m_threads.reserve(numThreads);

    // build a thread pool
    for (int i = 0; i < numThreads; i++) {
        m_threads.emplace_back([&]() {
            while (true) {
                m_lock.lock();
                if (!(first != last)) {
                    // no more work to do
                    m_lock.unlock();
                    break;
                }

                // grab a work item
                auto obj = *first;
                ++first;
                m_lock.unlock();

                // execute the work item
                f(obj);
            }
        });
    }

    // wait until all threads have finished
    for (auto &thread : m_threads)
        thread.join();
}

/// @brief Invokes @c f for each element of the iterator, parallelized across
/// all available cores.
template <class Iterator, class UnaryFunction>
void for_each_parallel(Iterator it, UnaryFunction f) {
    for_each_parallel(it.begin(), it.end(), f);
}

/// @brief Atomically increment a floating point number.
inline float atomicAdd(float &dst, float delta) {
#if defined(__clang__)
    // Atomic FP addition from PBRT
    union bits { float f; int32_t i; };
    bits oldVal, newVal;
    do {
        // On IA32/x64, adding a PAUSE instruction in compare/exchange loops
        // is recommended to improve performance.  (And it does!)
#if (defined(__i386__) || defined(__amd64__))
        __asm__ __volatile__ ("pause\n");
#endif
        oldVal.f = dst;
        newVal.f = oldVal.f + delta;
    } while (!__sync_bool_compare_and_swap((volatile int32_t *)&dst, newVal.i, oldVal.i));
    return newVal.f;
#else
    return std::atomic_ref<float>(dst) += delta;
#endif
}

/// @brief Atomically increment a 64-bit integer.
inline int64_t atomicAdd(int64_t &dst, int64_t delta) {
#if defined(__clang__)
    return __sync_add_and_fetch(&dst, delta);
#else
    return std::atomic_ref<int64_t>(dst) += delta;
#endif
}

/// @brief Atomically increment a color.
inline void atomicAdd(Color &dst, const Color &delta) {
    for (int channel = 0; channel < dst.NumComponents; channel++)
        atomicAdd(dst[channel], delta[channel]);
}

} // namespace lightwave
