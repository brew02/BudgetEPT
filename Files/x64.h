#pragma once
#include "CIA32.h"

/*
	When operating in protected, compatibility, or 64-bit mode at privilege
	level 0 (or in real-address mode, the equivalent to privilege level 0),
	all non-reserved flags in the EFLAGS register except RF, VIP, VIF, and
	VM may be modified. VIP, VIF, and VM remain unaffected.
*/
extern "C" void writeRFlags(RFlags rflags);
extern "C" void readGDTR(GDTR* gdtr);
extern "C" void writeGDTR(GDTR* gdtr);
extern "C" SegmentSelector readCS(void);
extern "C" SegmentSelector readSS(void);
extern "C" SegmentSelector readTR(void);