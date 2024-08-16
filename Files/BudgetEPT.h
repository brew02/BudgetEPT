#pragma once
#include "CIA32.h"

extern "C" uint64 BudgetEPTTest(SegmentSelector cs, SegmentSelector ss, SegmentSelector tr, void* rip);