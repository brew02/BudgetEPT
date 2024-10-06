#pragma once
#include "CIA32.h"

/*
	When operating in protected, compatibility, or 64-bit mode at privilege
	level 0 (or in real-address mode, the equivalent to privilege level 0),
	all non-reserved flags in the EFLAGS register except RF, VIP, VIF, and
	VM may be modified. VIP, VIF, and VM remain unaffected.
*/

/*
	Writes to the RFLAG register with the specified value.

	@param rflags - The updated RFLAG value
*/
extern "C" void writeRFlags(RFlags rflags);

/*
	Reads the GDTR value.

	@param gdtr - A variable to recieve the GDTR value
*/
extern "C" void readGDTR(GDTR* gdtr);

/*
	Writes the GDTR value.

	@param gdtr - A variable to write the GDTR value
*/
extern "C" void writeGDTR(GDTR* gdtr);

/*
	Reads the CS value.

	@return The CS value
*/
extern "C" SegmentSelector readCS(void);

/*
	Reads the SS value.

	@return The SS value
*/
extern "C" SegmentSelector readSS(void);

/*
	Reads the TR value.

	@return The TR value
*/
extern "C" SegmentSelector readTR(void);