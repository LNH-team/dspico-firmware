#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Xors \p scrA with \p srcB and writes the result to \p dst. \p length bytes will be xorred.
/// @param srcA The first source buffer.
/// @param srcB The second source buffer.
/// @param dst The destination buffer.
/// @param length The number of bytes to xor. Must be a multiple of 8.
extern void applyXor(const void* srcA, const void* srcB, void* dst, u32 length);

#ifdef __cplusplus
}
#endif
