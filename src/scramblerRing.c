#include "common.h"
#include "scrambler.h"
#include "scramblerRing.h"

u32* volatile gScramblerRingWPtr = gScramblerRing;

scr_state_t gScramblerState;
volatile bool gComputeScrambler = false;

u8 gScrambleBuffers[2][512];
volatile int gFreeScrambleBuf = 0;
