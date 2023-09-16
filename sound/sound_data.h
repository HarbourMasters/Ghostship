#pragma once

#include "types.h"
#include "align_asset_macro.h"

extern u8 gSoundDataADSR[]; // sound_data.ctl
extern u8 gSoundDataRaw[];  // sound_data.tbl
extern u8 gMusicData[];     // sequences.s

#define dgBankSetsData "__OTR__sound/bank_sets"
static const ALIGN_ASSET(2) char gBankSetsData[] = dgBankSetsData;