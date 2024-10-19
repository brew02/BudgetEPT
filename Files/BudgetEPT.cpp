#include <ntddk.h>
#include <intrin.h>

#include "CIA32.h"

#include "BudgetEPT.h"
#include "Handlers.h"
#include "Memory.h"
#include "x64.h"

/*
	Checks if SMEP and SMAP are supported.

	@return True if SMAP and SMEP are supported, false otherwise
*/
bool SupportsSMEPSMAP()
{
	CPUID cpuid{ 0 };
	__cpuid(cpuid.all, 7);

	if (!cpuid.EAX07hECX0h.EBX.smep)
	{
		DbgPrint("SMEP is not supported on this processor!\n");
	}

	if (!cpuid.EAX07hECX0h.EBX.smap)
	{
		DbgPrint("SMAP is not supported on this processor!\n");
		return false;
	}

	return true;
}

/*
	Updates the supervisor privileges by toggling bits in the CR4 and RFLAG registers.
*/
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
	WriteRFlags(rflags);
}

/*
	Creates a GDT copy with a new privilege level entry.

	@param gdtr - The original GDTR as input, new GDTR as output
	@param cs - The output CS value
	@param ss - The output SS value
	@param tr - The output TR value
	@param pl - The privilege value for the GDT entry

	@return The new GDT's virtual address
*/
void* CreateGDT(GDTR* gdtr, SegmentSelector* cs, SegmentSelector* ss, SegmentSelector* tr, uint64 pl)
{
	ReadGDTR(gdtr);
	OldGDTR = *gdtr;

	if (!gdtr->base)
	{
		DbgPrint("Failed to find GDT base\n");
		return nullptr;
	}

	SegmentDescriptor32* gdt = reinterpret_cast<SegmentDescriptor32*>
		(AllocateContiguousMemory(PAGE_SIZE, PAGE_READWRITE));

	if (!gdt)
	{
		DbgPrint("Failed to allocate new GDT\n");
		return nullptr;
	}

	memcpy(gdt, reinterpret_cast<void*>(gdtr->base), static_cast<size_t>(gdtr->limit) + 1);

	constexpr uint16 newEntriesSize = (sizeof(SegmentDescriptor32) * 2 + sizeof(SegmentDescriptor64));

	if ((gdtr->limit + newEntriesSize) > MAX_GDT_LIMIT)
		gdtr->limit = MAX_GDT_LIMIT;
	else
		gdtr->limit += newEntriesSize;

	uint64 idx = (static_cast<uint64>(gdtr->limit) + 1) / sizeof(SegmentDescriptor32);

	*cs = OldCS = ReadCS();
	SegmentDescriptor32* csEntry = &gdt[--idx];
	if (csEntry->all)
	{
		/*
			I'm too lazy, but the fix is simple:
			Just search the whole GDT for empty entries.
		*/
		FreeContiguousMemory(gdt);
		DbgPrint("CS entry is already initialized\n");
		return nullptr;
	}

	*csEntry = gdt[cs->index];

	csEntry->type = CODE_DATA_TYPE_EXECUTE_CONFORMING_READ; /* The conforming flag may be unnecessary */
	csEntry->dpl = pl;
	cs->index = idx;
	cs->rpl = pl;

	*ss = OldSS = ReadSS();
	SegmentDescriptor32* ssEntry = &gdt[--idx];
	if (ssEntry->all)
	{
		FreeContiguousMemory(gdt);
		DbgPrint("SS entry is already initialized\n");
		return nullptr;
	}

	*ssEntry = gdt[ss->index];

	ssEntry->type = CODE_DATA_TYPE_WRITE;
	ssEntry->dpl = pl;
	ss->index = idx;
	ss->rpl = pl;

	*tr = OldTR = ReadTR();
	idx -= 2;
	SegmentDescriptor64* trEntry = reinterpret_cast<SegmentDescriptor64*>(&gdt[idx]);
	if (trEntry->all[0] || trEntry->all[1])
	{
		FreeContiguousMemory(gdt);
		DbgPrint("TR entry is already initialized\n");
		return nullptr;
	}

	*trEntry = *reinterpret_cast<SegmentDescriptor64*>(&gdt[tr->index]);

	trEntry->desc.type = SYSTEM_TYPE_NOT_BUSY;
	trEntry->desc.dpl = pl;
	tr->index = idx;
	tr->rpl = pl;

	gdtr->base = reinterpret_cast<uint64>(gdt);

	return gdt;
}

/*
	Creates an IDT copy with certain modified interrupt handlers.

	@param idtr - The original IDTR as input, new IDTR as output

	@return The new IDT's virtual address
*/
void* CreateIDT(IDTR* idtr)
{
	__sidt(idtr);

	if (!idtr->base)
	{
		DbgPrint("Failed to find IDT base\n");
		return nullptr;
	}

	IDTEntry* idt = reinterpret_cast<IDTEntry*>(
		AllocateContiguousMemory(PAGE_SIZE, PAGE_READWRITE));
	
	if (!idt)
	{
		DbgPrint("Failed to allocate new IDT\n");
		return nullptr;
	}

	memcpy(idt, reinterpret_cast<void*>(idtr->base), static_cast<size_t>(idtr->limit) + 1);

	idt[INTERRUPT_VECTOR_DB].baseLow = (reinterpret_cast<uint64>(DBHandler) & MAXUINT16);
	idt[INTERRUPT_VECTOR_DB].baseMiddle = ((reinterpret_cast<uint64>(DBHandler) >> 16) & MAXUINT16);
	idt[INTERRUPT_VECTOR_DB].baseHigh = ((reinterpret_cast<uint64>(DBHandler) >> 32) & MAXUINT32);

	idt[INTERRUPT_VECTOR_GP].baseLow = (reinterpret_cast<uint64>(GPHandler) & MAXUINT16);
	idt[INTERRUPT_VECTOR_GP].baseMiddle = ((reinterpret_cast<uint64>(GPHandler) >> 16) & MAXUINT16);
	idt[INTERRUPT_VECTOR_GP].baseHigh = ((reinterpret_cast<uint64>(GPHandler) >> 32) & MAXUINT32);

	idt[INTERRUPT_VECTOR_PF].baseLow = (reinterpret_cast<uint64>(PFHandler) & MAXUINT16);
	idt[INTERRUPT_VECTOR_PF].baseMiddle = ((reinterpret_cast<uint64>(PFHandler) >> 16) & MAXUINT16);
	idt[INTERRUPT_VECTOR_PF].baseHigh = ((reinterpret_cast<uint64>(PFHandler) >> 32) & MAXUINT32);

	idtr->base = reinterpret_cast<uint64>(idt);

	return idt;
}

/*
	Runs a budget EPT test with a CR3 value, shellcode address, and privilege level.

	@param cr3 - The CR3 value to use for the budget EPT test.
	@param shellAddress - The shellcode address to execute.
	@param pl - The privilege value for the GDT entry

	@return The result of running the budget EPT test
*/
uint64 RunBudgetEPTTest(CR3* cr3, void* shellAddress, uint64 pl)
{
	CR3 oldCR3{ __readcr3() };

	SegmentSelector cs{ 0 };
	SegmentSelector ss{ 0 };
	SegmentSelector tr{ 0 };

	GDTR gdtr{ 0 };
	void* gdt = CreateGDT(&gdtr, &cs, &ss, &tr, pl);
	if (!gdt)
	{
		DbgPrint("Failed to create GDT\n");
		return 0;
	}

	IDTR oldIDTR{ 0 };
	__sidt(&oldIDTR);

	IDTR idtr{ 0 };
	void* idt = CreateIDT(&idtr);
	if (!idt)
	{
		FreeContiguousMemory(gdt);
		DbgPrint("Failed to create IDT\n");
		return 0;
	}

	_disable();

	__writecr3(cr3->all);

	FlushCaches(shellAddress);
	UpdateSupervisorPrivileges();
	WriteGDTR(&gdtr);

	__lidt(&idtr);

	uint64 result = BudgetEPTTest(cs, ss, tr, shellAddress);

	__lidt(&oldIDTR);

	UpdateSupervisorPrivileges();

	__writecr3(oldCR3.all);
	FlushCaches(shellAddress);

	_enable();

	FreeContiguousMemory(idt);
	FreeContiguousMemory(gdt);

	return result;
}

extern "C" PTE* ptEntry = nullptr;
extern "C" uint64 originalPFN = 0;
extern "C" uint64 hookPFN = 0;

/*
	The main code of the program.

	@param context - The starting context

	@return An NTSTATUS value
*/
NTSTATUS Startup(void* context)
{
	UNREFERENCED_PARAMETER(context);

	if (!SupportsSMEPSMAP())
	{
		DbgPrint("Unsupported processor features, unable to continue\n");
		return PsTerminateSystemThread(STATUS_UNSUCCESSFUL);
	}

	DbgPrint("All processor features supported\n");

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

	int idx = 0;

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

	void* hookedEntry = NewTableEntry(pt, &idx, flags);
	if (!hookedEntry)
	{
		FreeContiguousMemory(pt);
		FreeContiguousMemory(pd);
		FreeContiguousMemory(pdpt);
		FreeContiguousMemory(pml4);
		DbgPrint("Failed to allocate hook entry\n");
		return PsTerminateSystemThread(STATUS_UNSUCCESSFUL);
	}

	virt.pt = idx;

	ptEntry = &pt[idx];
	hookPFN = VirtToPFN(hookedEntry);

	void* originalEntry = AllocateContiguousMemory(PAGE_SIZE, PAGE_READWRITE);
	if (!originalEntry)
	{
		FreeContiguousMemory(hookedEntry);
		FreeContiguousMemory(pt);
		FreeContiguousMemory(pd);
		FreeContiguousMemory(pdpt);
		FreeContiguousMemory(pml4);
		DbgPrint("Failed to allocate original entry\n");
		return PsTerminateSystemThread(STATUS_UNSUCCESSFUL);
	}

	originalPFN = VirtToPFN(originalEntry);

	/*
		mov eax, 0xdeadbeef
		mov rcx, cr3							; #GP only on test 2
		mov ecx, dword ptr [rip - 13]			; #PF on both tests
		cmp ecx, 0xcafefeed
		jz ExitShellcode
		xor rax, rax
	ExitShellcode:
		ret
	*/
	uint8 hookedShellcode[] =
	{
		0xB8, 0xEF, 0xBE, 0xAD, 0xDE, 
		0x0F, 0x20, 0xD9, 
		0x8B, 0x0D, 0xF3, 0xFF, 0xFF, 0xFF, 
		0x81, 0xF9, 0xED, 0xFE, 0xFE, 0xCA, 
		0x74, 0x03, 
		0x48, 0x31, 0xC0, 
		0xC3
	};

	/*
		mov eax, 0xcafefeed
		mov rcx, cr3
		mov ecx, dword ptr [rip - 13]
		cmp ecx, 0xcafefeed
		jz ExitShellcode
		xor rax, rax
	ExitShellcode:
		ret
	*/
	uint8 originalShellcode[] =
	{
		0xB8, 0xED, 0xFE, 0xFE, 0xCA,
		0x0F, 0x20, 0xD9,
		0x8B, 0x0D, 0xF3, 0xFF, 0xFF, 0xFF,
		0x81, 0xF9, 0xED, 0xFE, 0xFE, 0xCA,
		0x74, 0x03,
		0x48, 0x31, 0xC0,
		0xC3
	};

	memcpy(hookedEntry, hookedShellcode, sizeof(hookedShellcode));
	memcpy(originalEntry, originalShellcode, sizeof(originalShellcode));

	uint64 result = RunBudgetEPTTest(&cr3, reinterpret_cast<void*>(virt.all), 0);
	if (!result)
		DbgPrint("Budget EPT Test 2 Failed\n");
	else
		DbgPrint("Budget EPT Test 2 Passed: 0x%llx\n", result);

	result = RunBudgetEPTTest(&cr3, reinterpret_cast<void*>(virt.all), 1);
	if(!result)
		DbgPrint("Budget EPT Test 2 Failed\n");
	else
		DbgPrint("Budget EPT Test 2 Passed: 0x%llx\n", result);
		
	
	FreeContiguousMemory(originalEntry);
	FreeContiguousMemory(hookedEntry);
	FreeContiguousMemory(pt);
	FreeContiguousMemory(pd);
	FreeContiguousMemory(pdpt);
	FreeContiguousMemory(pml4);

	return PsTerminateSystemThread(STATUS_SUCCESS);
}

/*
	The initial entry point.

	@param DriverObject - The driver object
	@param RegistryPath - The registry path

	@return An NTSTATUS value
*/
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