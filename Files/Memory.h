#pragma once
#include "CIA32.h"

/*
	Allocates physically contiguous memory using MmAllocateContiguousNodeMemory.

	@param size - The size of memory to allocate
	@param protect - The memory protection for the allocation
	@param zero - Zero the allocation or not

	@return The virtual address of the allocated memory
*/
void* AllocateContiguousMemory(size_t size, ULONG protect, bool zero = true);

/*
	Frees physically contiguous memory using MmFreeContiguousMemory.

	@param virt - The virtual address of the allocated memory.
*/
void FreeContiguousMemory(void* virt);

/*
	Converts a virtual address to a physical address.

	@param virt - The virtual address to convert

	@return The physical address
*/
uint64 VirtToPhys(void* virt);

/*
	Converts a virtual address to a page frame number (PFN).

	@param virt - The virtual address to convert

	@return The PFN
*/
uint64 VirtToPFN(void* virt);

/*
	Converts a physical address to a virtual address.

	@param virt - The physical address to convert

	@return The virtual address
*/
void* PhysToVirt(uint64 phys);

/*
	Converts a PFN to a virtual address.

	@param virt - The PFN to convert

	@return The virtual address
*/
void* PFNToVirt(uint64 pfn);

/*
	Creates a new table entry for a supplied page table structure.

	@param currentTable - The current page table structure to create a new entry for.
	@param idx - The new table entry's index
	@param flags - The flags for the new table entry

	@return The virtual address of the new table entry
*/
void* NewTableEntry(void* currentTable, int* idx, uint64 flags);

/*
	Flushes the translation lookaside buffer (TLB) for the address and instruction cache.

	@param address - The address to flush
*/
void FlushCaches(void* address);