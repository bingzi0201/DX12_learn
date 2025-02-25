#pragma once
#include <stdint.h>
#include <type_traits>

// 实现了一个通用的哈希函数 GetHash<T>，用于计算 任意 Trivial 类型（POD 类型）的哈希值，
// 并在 x64 平台上对小数据（小于 32 字节）进行优化处理。
namespace xxh {
size_t xxhash_gethash(void const* ptr, size_t sz);
//Size must less than 32 in x64
size_t xxhash_gethash_small(void const* ptr, size_t sz);
}// namespace xxh
template<typename T>
requires(std::is_trivial_v<T> && (!std::is_reference_v<T>))
	size_t GetHash(T const& v) {
	if constexpr (sizeof(T) < 32) {
		return xxh::xxhash_gethash_small(&v, sizeof(T));
	} else {
		return xxh::xxhash_gethash(&v, sizeof(T));
	}
}
