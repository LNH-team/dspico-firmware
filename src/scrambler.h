#pragma once
#include "common.h"

/// @brief Struct representing the scrambler state.
typedef struct
{
    u64 x;
    u64 y;
} scr_state_t;

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Initializes the scrambler with the given \p seed0.
/// @param state The scrambler state.
/// @param seed0 The scrambler seed 0.
void scr_init(scr_state_t* state, u64 seed0);

/// @brief Calculates the next 32 bits of the scrambler.
/// @param state The scrambler state.
/// @return The next 32 bits of the scrambler.
u32 scr_getNext32(scr_state_t* state);

#ifdef __cplusplus
}
#endif
