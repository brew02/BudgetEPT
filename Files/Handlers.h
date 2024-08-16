#pragma once
#include "CIA32.h"

extern "C" SegmentSelector OldCS;
extern "C" SegmentSelector OldSS;
extern "C" SegmentSelector OldTR;

extern "C" GDTR OldGDTR{ 0 };

extern "C" void GPHandler(void);
extern "C" void PFHandler(void);