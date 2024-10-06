#pragma once
#include "CIA32.h"

extern "C" inline SegmentSelector OldCS{ 0 };
extern "C" inline SegmentSelector OldSS{ 0 };
extern "C" inline SegmentSelector OldTR{ 0 };

extern "C" inline GDTR OldGDTR{ 0 };

/*
	The debug fault handler for the modified IDT.
*/
extern "C" void DBHandler(void);

/*
	The general protection fault handler for the modified IDT.
*/
extern "C" void GPHandler(void);

/*
	The page fault handler for the modified IDT.
*/
extern "C" void PFHandler(void);