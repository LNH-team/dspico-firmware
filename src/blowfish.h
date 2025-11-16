#pragma once
#include "common.h"

/// @brief Blowfish s boxes struct.
typedef struct
{
    u32 sbox0[0x100];
    u32 sbox1[0x100];
    u32 sbox2[0x100];
    u32 sbox3[0x100];
} bf_sboxes_t;

/// @brief Blowfish context struct.
typedef struct
{
    const u32* pTable;
    const bf_sboxes_t* sBoxes;
} bf_context_t;

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Initializes the given blowfish \p context with the given \p pTable and \p sBoxes.
/// @param context The blowfish context to initialize.
/// @param pTable The p table to use.
/// @param sBoxes The s boxes to use.
void bf_init(bf_context_t* context, const u32* pTable, const bf_sboxes_t* sBoxes);

/// @brief Decrypts the given blowfish \p block using the given \p context.
/// @param context The blowfish context to use.
/// @param block The blowfish block to decrypt.
/// @return The decrypted blowfish block.
u64 bf_decrypt(const bf_context_t* context, u64 block);

#ifdef __cplusplus
}
#endif
