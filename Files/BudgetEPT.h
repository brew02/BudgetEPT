#pragma once
#include "CIA32.h"

/*
	Executes a budget EPT test with specified cs, ss, tr, and rip values.

	@param cs - The CS value to use for the budget EPT test.
	@param ss - The SS value to use for the budget EPT test.
	@param tr - The TR value to use for the budget EPT test.
	@param rip - The rip value to execute for the budget EPT test.

	@return The result of running the budget EPT test
*/
extern "C" uint64 BudgetEPTTest(SegmentSelector cs, SegmentSelector ss, SegmentSelector tr, void* rip);