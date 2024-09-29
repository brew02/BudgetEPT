#pragma once
#include "CIA32.h"

extern "C" inline SegmentSelector OldCS{ 0 };
extern "C" inline SegmentSelector OldSS{ 0 };
extern "C" inline SegmentSelector OldTR{ 0 };

extern "C" inline GDTR OldGDTR{ 0 };

extern "C" void GPHandler(void);
extern "C" void PFHandler(void);