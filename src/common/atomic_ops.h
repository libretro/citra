// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"

#if _MSC_VER
#include <intrin.h>
#else
#include <cstring>
#endif

#if !defined(__SIZEOF_INT128__)
#include <mutex>
inline std::mutex g_atomic128_mutex;
#endif

namespace Common {

#if _MSC_VER

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u8* pointer, u8 value, u8 expected) {
    const u8 result =
        _InterlockedCompareExchange8(reinterpret_cast<volatile char*>(pointer), value, expected);
    return result == expected;
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u16* pointer, u16 value, u16 expected) {
    const u16 result =
        _InterlockedCompareExchange16(reinterpret_cast<volatile short*>(pointer), value, expected);
    return result == expected;
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u32* pointer, u32 value, u32 expected) {
    const u32 result =
        _InterlockedCompareExchange(reinterpret_cast<volatile long*>(pointer), value, expected);
    return result == expected;
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u64* pointer, u64 value, u64 expected) {
    const u64 result = _InterlockedCompareExchange64(reinterpret_cast<volatile __int64*>(pointer),
                                                     value, expected);
    return result == expected;
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u64* pointer, u128 value, u128 expected) {
    return _InterlockedCompareExchange128(reinterpret_cast<volatile __int64*>(pointer), value[1],
                                          value[0],
                                          reinterpret_cast<__int64*>(expected.data())) != 0;
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u8* pointer, u8 value, u8 expected,
                                               u8& actual) {
    actual =
        _InterlockedCompareExchange8(reinterpret_cast<volatile char*>(pointer), value, expected);
    return actual == expected;
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u16* pointer, u16 value, u16 expected,
                                               u16& actual) {
    actual =
        _InterlockedCompareExchange16(reinterpret_cast<volatile short*>(pointer), value, expected);
    return actual == expected;
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u32* pointer, u32 value, u32 expected,
                                               u32& actual) {
    actual =
        _InterlockedCompareExchange(reinterpret_cast<volatile long*>(pointer), value, expected);
    return actual == expected;
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u64* pointer, u64 value, u64 expected,
                                               u64& actual) {
    actual = _InterlockedCompareExchange64(reinterpret_cast<volatile __int64*>(pointer), value,
                                           expected);
    return actual == expected;
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u64* pointer, u128 value, u128 expected,
                                               u128& actual) {
    const bool result =
        _InterlockedCompareExchange128(reinterpret_cast<volatile __int64*>(pointer), value[1],
                                       value[0], reinterpret_cast<__int64*>(expected.data())) != 0;
    actual = expected;
    return result;
}

[[nodiscard]] inline u128 AtomicLoad128(volatile u64* pointer) {
    u128 result{};
    _InterlockedCompareExchange128(reinterpret_cast<volatile __int64*>(pointer), result[1],
                                   result[0], reinterpret_cast<__int64*>(result.data()));
    return result;
}

#else

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u8* pointer, u8 value, u8 expected) {
    return __sync_bool_compare_and_swap(pointer, expected, value);
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u16* pointer, u16 value, u16 expected) {
    return __sync_bool_compare_and_swap(pointer, expected, value);
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u32* pointer, u32 value, u32 expected) {
    return __sync_bool_compare_and_swap(pointer, expected, value);
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u64* pointer, u64 value, u64 expected) {
    return __sync_bool_compare_and_swap(pointer, expected, value);
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u8* pointer, u8 value, u8 expected,
                                               u8& actual) {
    actual = __sync_val_compare_and_swap(pointer, expected, value);
    return actual == expected;
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u16* pointer, u16 value, u16 expected,
                                               u16& actual) {
    actual = __sync_val_compare_and_swap(pointer, expected, value);
    return actual == expected;
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u32* pointer, u32 value, u32 expected,
                                               u32& actual) {
    actual = __sync_val_compare_and_swap(pointer, expected, value);
    return actual == expected;
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u64* pointer, u64 value, u64 expected,
                                               u64& actual) {
    actual = __sync_val_compare_and_swap(pointer, expected, value);
    return actual == expected;
}

#if !defined(__SIZEOF_INT128__)

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u64* pointer, u128 value, u128 expected)
{
    std::lock_guard<std::mutex> lock(g_atomic128_mutex);

    auto ptr64 = reinterpret_cast<volatile u64*>(pointer);

    u64 cur_lo = ptr64[0];
    u64 cur_hi = ptr64[1];

    u64 exp_lo, exp_hi;
    std::memcpy(&exp_lo, expected.data() + 0, sizeof(u64));
    std::memcpy(&exp_hi, expected.data() + 1, sizeof(u64));

    if (cur_lo == exp_lo && cur_hi == exp_hi) {
        u64 val_lo, val_hi;
        std::memcpy(&val_lo, value.data() + 0, sizeof(u64));
        std::memcpy(&val_hi, value.data() + 1, sizeof(u64));

        ptr64[0] = val_lo;
        ptr64[1] = val_hi;
        return true;
    }

    return false;
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u64* pointer, u128 value, u128 expected, u128& actual)
{
    std::lock_guard<std::mutex> lock(g_atomic128_mutex);

    auto ptr64 = reinterpret_cast<volatile u64*>(pointer);

    u64 cur_lo = ptr64[0];
    u64 cur_hi = ptr64[1];

    // write back actual
    std::memcpy(actual.data() + 0, &cur_lo, sizeof(u64));
    std::memcpy(actual.data() + 1, &cur_hi, sizeof(u64));

    u64 exp_lo, exp_hi;
    std::memcpy(&exp_lo, expected.data() + 0, sizeof(u64));
    std::memcpy(&exp_hi, expected.data() + 1, sizeof(u64));

    if (cur_lo == exp_lo && cur_hi == exp_hi) {
        u64 val_lo, val_hi;
        std::memcpy(&val_lo, value.data() + 0, sizeof(u64));
        std::memcpy(&val_hi, value.data() + 1, sizeof(u64));

        ptr64[0] = val_lo;
        ptr64[1] = val_hi;
        return true;
    }

    return false;
}

[[nodiscard]] inline u128 AtomicLoad128(volatile u64* pointer)
{
    std::lock_guard<std::mutex> lock(g_atomic128_mutex);

    auto ptr64 = reinterpret_cast<volatile u64*>(pointer);

    u128 result;
    u64 lo = ptr64[0];
    u64 hi = ptr64[1];

    std::memcpy(result.data() + 0, &lo, sizeof(u64));
    std::memcpy(result.data() + 1, &hi, sizeof(u64));

    return result;
}

#else

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u64* pointer, u128 value, u128 expected)
{
    unsigned __int128 value_a;
    unsigned __int128 expected_a;
    std::memcpy(&value_a, value.data(), sizeof(u128));
    std::memcpy(&expected_a, expected.data(), sizeof(u128));
    return __sync_bool_compare_and_swap((unsigned __int128*)pointer, expected_a, value_a);
}

[[nodiscard]] inline bool AtomicCompareAndSwap(volatile u64* pointer, u128 value, u128 expected,
                                               u128& actual)
{
    unsigned __int128 value_a;
    unsigned __int128 expected_a;
    unsigned __int128 actual_a;
    std::memcpy(&value_a, value.data(), sizeof(u128));
    std::memcpy(&expected_a, expected.data(), sizeof(u128));
    actual_a = __sync_val_compare_and_swap((unsigned __int128*)pointer, expected_a, value_a);
    std::memcpy(actual.data(), &actual_a, sizeof(u128));
    return actual_a == expected_a;
}

[[nodiscard]] inline u128 AtomicLoad128(volatile u64* pointer)
{
    unsigned __int128 zeros_a = 0;
    unsigned __int128 result_a =
        __sync_val_compare_and_swap((unsigned __int128*)pointer, zeros_a, zeros_a);

    u128 result;
    std::memcpy(result.data(), &result_a, sizeof(u128));
    return result;
}

#endif // __SIZEOF_INT128__
#endif // _MSC_VER

} // namespace Common
