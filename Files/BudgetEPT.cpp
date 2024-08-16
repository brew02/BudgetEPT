#include <ntddk.h>
#include <intrin.h>

#include "CIA32.h"

#include "BudgetEPT.h"
#include "Handlers.h"
#include "Memory.h"
#include "x64.h"

void UpdateSupervisorPrivileges()
{
	static bool smepRemoved = false;
	static bool smapSet = false;
	static bool acCleared = false;
	static bool ntCleared = false;

	CR4 cr4{ __readcr4() };
	if (cr4.smep)
	{
		smepRemoved = true;
		cr4.smep = 0;
	}
	else if (smepRemoved)
	{
		smepRemoved = false;
		cr4.smep = 1;
	}

	if (!cr4.smap)
	{
		smapSet = true;
		cr4.smap = 1;
	}
	else if (smapSet)
	{
		smapSet = false;
		cr4.smap = 1;
	}

	RFlags rflags{ __readeflags() };
	if (rflags.ac)
	{
		acCleared = true;
		rflags.ac = 0;
	}
	else if (acCleared)
	{
		acCleared = false;
		rflags.ac = 1;
	}

	if (rflags.nt)
	{
		ntCleared = true;
		rflags.nt = 0;
	}
	else if (ntCleared)
	{
		ntCleared = false;
		rflags.nt;
	}

	__writecr4(cr4.all);
	writeRFlags(rflags);
}

void* CreateGDT()
{
	return nullptr;
}

void* CreateIDT()
{
	return nullptr;
}

NTSTATUS Startup(void* context)
{
	UNREFERENCED_PARAMETER(context);
	
	uint64 flags = (TABLE_FLAG_READ | TABLE_FLAG_WRITE | TABLE_FLAG_USERMODE);

	/*
		Put this into a separate function probably
		Maybe just call PFNToVirt on each entry within
		the pml4 (and the pml4 itself) and free the memory,
		allowing for recursion perhaps?
	*/
	PML4E* pml4 = reinterpret_cast<PML4E*>(
		NewTableEntry(nullptr, nullptr, flags));

	if (!pml4)
	{
		DbgPrint("Failed to allocate new PML4\n");
		return PsTerminateSystemThread(STATUS_UNSUCCESSFUL);
	}

	DbgPrint("New PML4: 0x%p\n", pml4);

	CR3 oldCR3{ __readcr3() };
	CR3 cr3{ oldCR3 };

	cr3.pfn = VirtToPFN(pml4);

	void* oldPML4 = PFNToVirt(oldCR3.pfn);
	if (!oldPML4)
	{
		FreeContiguousMemory(pml4);
		DbgPrint("Failed to get PML4\n");
		return PsTerminateSystemThread(STATUS_UNSUCCESSFUL);
	}

	DbgPrint("Old PML4: 0x%p\n", oldPML4);
	memcpy(pml4, oldPML4, PAGE_SIZE);

	VAddress virt{ 0 };

	uint64 idx = 0;

	PDPTE* pdpt = reinterpret_cast<PDPTE*>(
		NewTableEntry(pml4, &idx, flags));

	if (!pdpt)
	{
		FreeContiguousMemory(pml4);
		DbgPrint("Failed to allocate new PDPT\n");
		return PsTerminateSystemThread(STATUS_UNSUCCESSFUL);
	}

	/*
		Make it canonical
	*/
	virt.pml4 = idx;
	if (virt.pml4 & BIT_FLAG(8))
	{
		virt.reserved = MAXUINT16;
	}

	PDE* pd = reinterpret_cast<PDE*>(
		NewTableEntry(pdpt, &idx, flags));

	if (!pd)
	{
		FreeContiguousMemory(pdpt);
		FreeContiguousMemory(pml4);
		DbgPrint("Failed to allocate new PD\n");
		return PsTerminateSystemThread(STATUS_UNSUCCESSFUL);
	}

	virt.pdpt = idx;

	PTE* pt = reinterpret_cast<PTE*>(
		NewTableEntry(pd, &idx, flags));

	if (!pt)
	{
		FreeContiguousMemory(pd);
		FreeContiguousMemory(pdpt);
		FreeContiguousMemory(pml4);
		DbgPrint("Failed to allocate new PT\n");
		return PsTerminateSystemThread(STATUS_UNSUCCESSFUL);
	}

	virt.pd = idx;

	void* entry = NewTableEntry(pt, &idx, flags);
	if (!entry)
	{
		FreeContiguousMemory(pt);
		FreeContiguousMemory(pd);
		FreeContiguousMemory(pdpt);
		FreeContiguousMemory(pml4);
		DbgPrint("Failed to allocate new entry\n");
		return PsTerminateSystemThread(STATUS_UNSUCCESSFUL);
	}

	virt.pt = idx;

	/*
		mov eax, 0xdeadbeef
		mov rax, cr3			; Should cause #GP
		mov rax, [rip]			; Should cause #PF
		ret
	*/
	uint8 shellcode[] = { 0xB8, 0xEF, 0xBE, 0xAD, 0xDE, 0x0F, 0x20, 0xD8, 0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, 0xC3 };

	memcpy(entry, shellcode, sizeof(shellcode));

	GDTR gdtr{ 0 };
	readGDTR(&gdtr);
	OldGDTR = gdtr;

	SegmentDescriptor32* gdt = reinterpret_cast<SegmentDescriptor32*>
		(AllocateContiguousMemory(PAGE_SIZE, PAGE_READWRITE));

	if (!gdt || !gdtr.base)
	{
		FreeContiguousMemory(entry);
		FreeContiguousMemory(pt);
		FreeContiguousMemory(pd);
		FreeContiguousMemory(pdpt);
		FreeContiguousMemory(pml4);
		DbgPrint("Failed to find GDT base\n");
		return PsTerminateSystemThread(STATUS_UNSUCCESSFUL);
	}

	IDTEntry* idt = reinterpret_cast<IDTEntry*>(
		AllocateContiguousMemory(PAGE_SIZE, PAGE_READWRITE));

	IDTR idtr{ 0 };
	__sidt(&idtr);

	IDTR oldIDTR{ idtr };

	if (!idt || !idtr.base)
	{
		FreeContiguousMemory(gdt);
		FreeContiguousMemory(entry);
		FreeContiguousMemory(pt);
		FreeContiguousMemory(pd);
		FreeContiguousMemory(pdpt);
		FreeContiguousMemory(pml4);
		DbgPrint("Failed to find GDT base\n");
		return PsTerminateSystemThread(STATUS_UNSUCCESSFUL);
	}

	memcpy(gdt, reinterpret_cast<void*>(gdtr.base), static_cast<size_t>(gdtr.limit) + 1);
	memcpy(idt, reinterpret_cast<void*>(idtr.base), static_cast<size_t>(idtr.limit) + 1);

	idt[INTERRUPT_VECTOR_GP].baseLow = (reinterpret_cast<uint64>(GPHandler) & MAXUINT16);
	idt[INTERRUPT_VECTOR_GP].baseMiddle = ((reinterpret_cast<uint64>(GPHandler) >> 16) & MAXUINT16);
	idt[INTERRUPT_VECTOR_GP].baseHigh = ((reinterpret_cast<uint64>(GPHandler) >> 32) & MAXUINT32);

	idt[INTERRUPT_VECTOR_PF].baseLow = (reinterpret_cast<uint64>(PFHandler) & MAXUINT16);
	idt[INTERRUPT_VECTOR_PF].baseMiddle = ((reinterpret_cast<uint64>(PFHandler) >> 16) & MAXUINT16);
	idt[INTERRUPT_VECTOR_PF].baseHigh = ((reinterpret_cast<uint64>(PFHandler) >> 32) & MAXUINT32);

	idtr.base = reinterpret_cast<uint64>(idt);

	/*
		Don't use ending idx like this because it could be something else
		Just make sure that our entries don't collide with existing cs, ss, or tr descriptors
	*/
	idx = (static_cast<uint64>(gdtr.limit) + 1) / sizeof(SegmentDescriptor32);

	SegmentSelector cs = OldCS = readCS();
	SegmentDescriptor32* csEntry = &gdt[idx];
	*csEntry = gdt[cs.index];

	csEntry->type = CODE_DATA_TYPE_EXECUTE_CONFORMING_READ; /* The conforming flag may be unnecessary */
	csEntry->dpl = 1;
	cs.index = idx++;
	cs.rpl = 1;

	SegmentSelector ss = OldSS = readSS();
	SegmentDescriptor32* ssEntry = &gdt[idx];
	*ssEntry = gdt[ss.index];

	ssEntry->type = CODE_DATA_TYPE_WRITE;
	ssEntry->dpl = 1;
	ss.index = idx++;
	ss.rpl = 1;

	_disable();

	SegmentSelector tr = OldTR = readTR();
	SegmentDescriptor64* trEntry = reinterpret_cast<SegmentDescriptor64*>(&gdt[idx]);
	*trEntry = *reinterpret_cast<SegmentDescriptor64*>(&gdt[tr.index]);

	trEntry->desc.type = SYSTEM_TYPE_NOT_BUSY;
	trEntry->desc.dpl = 1;
	tr.index = idx++;
	tr.rpl = 1;

	gdtr.base = reinterpret_cast<uint64>(gdt);
	gdtr.limit += (sizeof(SegmentDescriptor32) * 2 + sizeof(SegmentDescriptor64));

	__writecr3(cr3.all);

	FlushCaches(reinterpret_cast<void*>(virt.all));
	UpdateSupervisorPrivileges();
	writeGDTR(&gdtr);

	__lidt(&idtr);

	uint64 result = BudgetEPTTest(cs, ss, tr, virt.all);

	__lidt(&oldIDTR);

	UpdateSupervisorPrivileges();

	__writecr3(oldCR3.all);
	FlushCaches(reinterpret_cast<void*>(virt.all));

	_enable();

	DbgPrint("Result of call: 0x%llx\n", result);

	FreeContiguousMemory(idt);
	FreeContiguousMemory(gdt);
	FreeContiguousMemory(entry);
	FreeContiguousMemory(pt);
	FreeContiguousMemory(pd);
	FreeContiguousMemory(pdpt);
	FreeContiguousMemory(pml4);

	return PsTerminateSystemThread(STATUS_SUCCESS);
}

NTSTATUS DriverEntry(DRIVER_OBJECT* DriverObject, UNICODE_STRING* RegistryPath)
{
	UNREFERENCED_PARAMETER(DriverObject);
	UNREFERENCED_PARAMETER(RegistryPath);

	OBJECT_ATTRIBUTES obj{ 0 };
	InitializeObjectAttributes(&obj, nullptr, OBJ_KERNEL_HANDLE, nullptr, nullptr);

	HANDLE threadHandle = nullptr;
	if (!NT_SUCCESS(PsCreateSystemThread(&threadHandle, GENERIC_ALL, &obj,
		nullptr, nullptr, reinterpret_cast<PKSTART_ROUTINE>(Startup), nullptr)))
	{
		DbgPrint("Failed to create system thread\n");
		return STATUS_UNSUCCESSFUL;
	}

	if (threadHandle)
		ZwClose(threadHandle);

	return STATUS_SUCCESS;
}