#pragma once
#include "CIA32.h"

extern "C" uint64 BudgetEPTTest(SegmentSelector cs, SegmentSelector ss, SegmentSelector tr, uint64 rip);