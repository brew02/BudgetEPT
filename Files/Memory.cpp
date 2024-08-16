#include <ntddk.h>
#include <intrin.h>

#include "Memory.h"

void* AllocateContiguousMemory(size_t size, ULONG protect, bool zero)
{
	PHYSICAL_ADDRESS high{ 0 };
	high.QuadPart = ~0llu;

	void* mem = MmAllocateContiguousNodeMemory(size, PHYSICAL_ADDRESS{ 0 },
		high, PHYSICAL_ADDRESS{ 0 }, protect, MM_ANY_NODE_OK);

	if (mem && zero)
		RtlSecureZeroMemory(mem, size);

	return mem;
}

void FreeContiguousMemory(void* virt)
{
	return MmFreeContiguousMemory(virt);
}

uint64 VirtToPhys(void* virt)
{
	return MmGetPhysicalAddress(virt).QuadPart;
}

uint64 VirtToPFN(void* virt)
{
	return VirtToPhys(virt) >> 12;
}

void* PhysToVirt(uint64 phys)
{
	PHYSICAL_ADDRESS address{ 0 };
	address.QuadPart = phys;

	return MmGetVirtualForPhysical(address);
}

void* PFNToVirt(uint64 pfn)
{
	return PhysToVirt(pfn << 12);
}

void* NewTableEntry(void* currentTable, uint64* idx, uint64 flags)
{
	void* newTable = AllocateContiguousMemory(PAGE_SIZE, PAGE_READWRITE);
	if (!newTable)
		return nullptr;

	PTE* table = reinterpret_cast<PTE*>(currentTable);
	if (!table)
		return newTable;

	for (int i = PageTableEntries - 1; i >= 0; --i)
	{
		PTE* entry = &table[i];
		if (!entry->p && !entry->pfn)
		{
			entry->all = 0;

			entry->pfn = VirtToPFN(newTable);
			if (idx)
				*idx = i;

			entry->all |= flags;
			return newTable;
		}
	}

	return nullptr;
}

void FlushCaches(void* address)
{
	CR4 cr4{ __readcr4() };
	if (cr4.pcide || cr4.pge)
	{
		cr4.pge = ~(cr4.pge);
		__writecr4(cr4.all);
		cr4.pge = ~(cr4.pge);
		__writecr4(cr4.all);
	}
	else
	{
		__writecr3(__readcr3());
	}

	__wbinvd();
	__invlpg(address);
}