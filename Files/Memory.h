#pragma once
#include "CIA32.h"

void* AllocateContiguousMemory(size_t size, ULONG protect, bool zero = true);
void FreeContiguousMemory(void* virt);

uint64 VirtToPhys(void* virt);
uint64 VirtToPFN(void* virt);
void* PhysToVirt(uint64 phys);
void* PFNToVirt(uint64 pfn);

void* NewTableEntry(void* currentTable, int* idx, uint64 flags);

void FlushCaches(void* address);