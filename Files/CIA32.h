#pragma once
#include <xmmintrin.h>

#include "CTypes.h"

#define BIT_FLAG(index) (1llu << index)

#define PAGE_SIZE 0x1000
#define PAGE_MASK (~(PAGE_SIZE - 1))
// #define PAGE_ALIGN(address) (void*)((uint64)address & PAGE_MASK)
#define PAGE_ROUND(address) (void*)((uint64)(PAGE_ALIGN((uint64)address - 1)) + PAGE_SIZE)

#if defined(_MSC_EXTENSIONS)
#pragma warning(push)
#pragma warning(disable: 4201)
#endif

/*
	General Purpose Registers
*/

union RFlags
{
	uint64 all;

	struct
	{
		uint64 cf : 1;
		uint64 reserved1 : 1; /* Always 1 */
		uint64 pf : 1;
		uint64 reserved2 : 1; /* Always 0 */
		uint64 af : 1;
		uint64 reserved3 : 1; /* Always 0 */
		uint64 zf : 1;
		uint64 sf : 1;
		uint64 tf : 1;
		uint64 intf : 1;
		uint64 df : 1;
		uint64 of : 1;
		uint64 iopl : 2;
		uint64 nt : 1;
		uint64 reserved4 : 1; /* Always 0 */
		uint64 rf : 1;
		uint64 vm : 1;
		uint64 ac : 1;
		uint64 vif : 1;
		uint64 vip : 1;
		uint64 id : 1;
		uint64 reserved5 : 42; /* Always 0 */
	};
};

/*
	Interrupts
*/

#define IDT_TYPE_INTERRUPT_GATE 0xE

#define INTERRUPT_VECTOR_DE 0
#define INTERRUPT_VECTOR_DB 1
#define INTERRUPT_VECTOR_GP 13
#define INTERRUPT_VECTOR_PF 14

#include <pshpack1.h>
struct IDTR
{
	uint16 limit;
	uint64 base;
};
#include <poppack.h>

union IDTEntry
{
	uint32 all[4];

	struct
	{
		uint16 baseLow;
		uint16 segmentSelector;
		uint8 ist : 3;
		uint8 reserved1 : 5;
		uint8 type : 4;
		uint8 reserved2 : 1;
		uint8 dpl : 2;
		uint8 p : 1;
		uint16 baseMiddle;
		uint32 baseHigh;
		uint32 reserved3;
	};
};

struct InterruptFrame
{
	uint64 rip;
	uint64 cs;
	uint64 rflags;
	uint64 rsp;
	uint64 ss;
};

struct InterruptFrameErrorCode
{
	uint64 errorCode;
	InterruptFrame frame;
};

/*
	Segmentation
*/

#define CODE_DATA_TYPE_WRITE BIT_FLAG(1)

#define CODE_DATA_TYPE_READ BIT_FLAG(1)
#define CODE_DATA_TYPE_CONFORMING BIT_FLAG(2)
#define CODE_DATA_TYPE_EXECUTE BIT_FLAG(3)
#define CODE_DATA_TYPE_EXECUTE_CONFORMING_READ (CODE_DATA_TYPE_EXECUTE | CODE_DATA_TYPE_CONFORMING | CODE_DATA_TYPE_READ)

#define SYSTEM_TYPE_NOT_BUSY (BIT_FLAG(3) | BIT_FLAG(0))

#define MAX_GDT_SIZE 8196
#define MAX_GDT_LIMIT (MAX_GDT_SIZE - 1)

typedef IDTR GDTR;

union SegmentSelector
{
	uint16 all;
	struct
	{
		uint16 rpl : 2;					// Requested Privilege Level
		uint16 ti : 1;					// Table Indicator
		uint16 index: 13;
	};
};

union SegmentDescriptor32
{
	uint64 all;
	struct
	{
		uint64 segmentLimitLow : 16;
		uint64 baseLow : 16;
		uint64 baseMid : 8;
		uint64 type : 4;
		uint64 descriptorType : 1;
		uint64 dpl : 2;					// Descriptor Privilege Level
		uint64 p : 1;					// Present
		uint64 segmentLimitHigh : 4;
		uint64 system : 1;
		uint64 longMode : 1;
		uint64 defaultBig : 1;
		uint64 granularity : 1;
		uint64 baseHigh : 8;
	};
};

union SegmentDescriptor64
{
	uint64 all[2];
	struct
	{
		SegmentDescriptor32 desc;
		uint32 baseUpper;
		uint32 setZero;
	};
};

/*
	Debug Registers
*/

typedef uint64 DR0;
typedef uint64 DR1;
typedef uint64 DR2;
typedef uint64 DR3;

union DR7
{
	uint64 all;

	struct
	{
		uint64 l0 : 1;
		uint64 g0 : 1;
		uint64 l1 : 1;
		uint64 g1 : 1;
		uint64 l2 : 1;
		uint64 g2 : 1;
		uint64 l3 : 1;
		uint64 g3 : 1;
		uint64 le : 1;
		uint64 ge : 1;
		uint64 set_10_1 : 1;
		uint64 rtm : 1;
		uint64 set_12_0 : 1;
		uint64 gd : 1;
		uint64 set_14_15_0 : 2;
		uint64 rw0 : 2;
		uint64 len0 : 2;
		uint64 rw1 : 2;
		uint64 len1 : 2;
		uint64 rw2 : 2;
		uint64 len2 : 2;
		uint64 rw3 : 2;
		uint64 len3 : 2;

	};
};

/*
	Control Registers
*/

union CR0
{
	uint64 all;

	struct
	{
		uint64 pe : 1;			// [0] Protected mode enabled
		uint64 mp : 1;			// [1] Monitor co-processor
		uint64 em : 1;			// [2] Emulation
		uint64 ts : 1;			// [3] Task switched
		uint64 et : 1;			// [4] Extension type
		uint64 ne : 1;			// [5] Numeric error
		uint64 reserved_1 : 10;	// [6:15]
		uint64 wp : 1;			// [16] Write protect
		uint64 reserved_2 : 1;		// [17]
		uint64 am : 1;			// [18] Alignment mask
		uint64 reserved_3 : 10;	// [19:28]
		uint64 nw : 1;			// [29] Not write-through
		uint64 cd : 1;			// [30] Cache disable
		uint64 pg : 1;			// [31] Paging
		uint64 reserved_4 : 32;	// [32:63]
	};
};

union CR3
{
	uint64 all;

	struct
	{
		uint64 ignored1 : 3;		// [0:2]
		uint64 pwt : 1;			// [3] Page-level write-through
		uint64 pcd : 1;			// [4] Page-level cache disable
		uint64 ignored2 : 7;		// [5:11]
		uint64 pfn : 36;			// [12:47] Page frame number
		uint64 reserved_1 : 16;	// [48:63]
	} pcide0;

	struct
	{
		uint64 pcid : 12; 			// [0:11] Process context identifier
		uint64 pfn : 36;			// [12:47] Page frame number
		uint64 reserved_1 : 16;	// [48:63]
	} pcide1;

	struct
	{
		uint64 varied : 12;
		uint64 pfn : 36;
		uint64 reserved : 16;
	};
};

union CR4
{
	uint64 all;
	struct
	{
		uint64 vme : 1;
		uint64 pvi : 1;
		uint64 tsd : 1;
		uint64 de : 1;
		uint64 pse : 1;
		uint64 pae : 1;
		uint64 mce : 1;
		uint64 pge : 1;
		uint64 pce : 1;
		uint64 osfxsr : 1;
		uint64 osxmmexcpt : 1;
		uint64 umip : 1;
		uint64 la57 : 1;
		uint64 vmxe : 1;
		uint64 smxe : 1;
		uint64 reserved1 : 1;
		uint64 fsgsbase : 1;
		uint64 pcide : 1;
		uint64 osxsave : 1;
		uint64 kl : 1;
		uint64 smep : 1;
		uint64 smap : 1;
		uint64 pke : 1;
		uint64 cet : 1;
		uint64 pks : 1;
		uint64 reserved2 : 39;
	};
};

/*
	Page Tables
*/

#define TABLE_FLAG_READ		BIT_FLAG(0)
#define TABLE_FLAG_WRITE	BIT_FLAG(1)
#define TABLE_FLAG_USERMODE BIT_FLAG(2)
#define TABLE_FLAG_NX		BIT_FLAG(63)

union PML4E
{
	uint64 all;

	struct
	{
		uint64 p : 1;				// [0] Present
		uint64 rw : 1;			// [1] Read/Write; if 0, writes may not be allowed
		uint64 us : 1;			// [2] User/Supervisor; if 0, user-mode accesses are not allowed
		uint64 pwt : 1;			// [3] Page-level write-through
		uint64 pcd : 1;			// [4] Page-level cache disable
		uint64 a : 1;				// [5] Accessed
		uint64 ignored_1 : 1;		// [6]
		uint64 ps : 1;			// [7] Page size (must be zero)
		uint64 ignored_2 : 4;		// [8:11]
		uint64 pfn : 36;			// [12:47] Page frame number
		uint64 reserved_1 : 4;		// [48:51]
		uint64 ignored_3 : 11;		// [52:62]
		uint64 xd : 1;			// [63] Execute disable
	};
};

union PDPTE
{
	uint64 all;

	struct
	{
		uint64 p : 1;				// [0] Present
		uint64 rw : 1;			// [1] Read/Write; if 0, writes may not be allowed
		uint64 us : 1;			// [2] User/Supervisor; if 0, user-mode accesses are not allowed
		uint64 pwt : 1;			// [3] Page-level write-through
		uint64 pcd : 1;			// [4] Page-level cache disable
		uint64 a : 1;				// [5] Accessed
		uint64 d : 1;				// [6] Dirty
		uint64 ps : 1;				// [7] Page size
		uint64 g : 1;				// [8] Global
		uint64 ignored_1 : 3;		// [9:11]
		uint64 pat : 1;			// [12] Page access type
		uint64 reserved_1 : 17;	// [13:29]
		uint64 pfn : 18;			// [30:47] Page frame number
		uint64 reserved_2 : 4;		// [48:51]
		uint64 ignored_2 : 7;		// [52:58]
		uint64 pk : 4;			// [59:62] Protection key
		uint64 xd : 1;			// [63] Execute disable
	} map;

	struct
	{
		uint64 p : 1;				// [0] Present
		uint64 rw : 1;			// [1] Read/Write; if 0, writes may not be allowed
		uint64 us : 1;			// [2] User/Supervisor; if 0, user-mode accesses are not allowed
		uint64 pwt : 1;			// [3] Page-level write-through
		uint64 pcd : 1;			// [4] Page-level cache disable
		uint64 a : 1;				// [5] Accessed
		uint64 ignored_1 : 1;		// [6]
		uint64 ps : 1;			// [7] Page size
		uint64 ignored_2 : 4;		// [8:11]
		uint64 pfn : 36;			// [12:47] Page frame number
		uint64 reserved_1 : 4;		// [48:51]
		uint64 ignored_3 : 11;		// [52:62]
		uint64 xd : 1;			// [63] Execute disable
	} ref;
};

union PDE
{
	uint64 all;

	struct
	{
		uint64 p : 1;				// [0] Present
		uint64 rw : 1;			// [1] Read/Write; if 0, writes may not be allowed
		uint64 us : 1;			// [2] User/Supervisor; if 0, user-mode accesses are not allowed
		uint64 pwt : 1;			// [3] Page-level write-through
		uint64 pcd : 1;			// [4] Page-level cache disable
		uint64 a : 1;				// [5] Accessed
		uint64 d : 1;				// [6] Dirty
		uint64 ps : 1;			// [7] Page size
		uint64 g : 1;				// [8] Global
		uint64 ignored_1 : 3;		// [9:11]
		uint64 pat : 1;			// [12] Page access type
		uint64 reserved_1 : 8;		// [13:20]
		uint64 pfn : 27;			// [21:47] Page frame number
		uint64 reserved_2 : 4;		// [48:51]
		uint64 ignored_2 : 7;		// [52:58]
		uint64 pk : 4;			// [59:62] Protection key
		uint64 xd : 1;			// [63] Execute disable
	} map;

	struct
	{
		uint64 p : 1;				// [0] Present
		uint64 rw : 1;			// [1] Read/Write; if 0, writes may not be allowed
		uint64 us : 1;			// [2] User/Supervisor; if 0, user-mode accesses are not allowed
		uint64 pwt : 1;			// [3] Page-level write-through
		uint64 pcd : 1;			// [4] Page-level cache disable
		uint64 a : 1;				// [5] Accessed
		uint64 ignored_1 : 1;		// [6]
		uint64 ps : 1;			// [7] Page size
		uint64 ignored_2 : 4;		// [8:11]
		uint64 pfn : 36;			// [12:47] Page frame number
		uint64 reserved_1 : 4;		// [48:51]
		uint64 ignored_3 : 11;		// [52:62]
		uint64 xd : 1;			// [63] Execute disable
	} ref;
};

union PTE
{
	uint64 all;
	struct
	{
		uint64 p : 1;				// [0] Present
		uint64 rw : 1;			// [1] Read/Write; if 0, writes may not be allowed
		uint64 us : 1;			// [2] User/Supervisor; if 0, user-mode accesses are not allowed
		uint64 pwt : 1;			// [3] Page-level write-through
		uint64 pcd : 1;			// [4] Page-level cache disable
		uint64 a : 1;				// [5] Accessed
		uint64 d : 1;				// [6] Dirty
		uint64 pat : 1;			// [7] Page access type
		uint64 g : 1;             // [8] Global
		uint64 ignored_1 : 3;		// [9:11]
		uint64 pfn : 36;          // [12:47] Page frame number
		uint64 reserved_1 : 4;		// [48:51]
		uint64 ignored_2 : 7;		// [52:58]
		uint64 pk : 4;			// [59:62] Protection key
		uint64 xd : 1;            // [63] Execute disable
	};
};

inline constexpr int PageTableEntries = (PAGE_SIZE / sizeof(PTE));

/*
	Memory
*/

union VAddress
{
	uint64 all;

	struct
	{
		uint64 offset : 12;
		uint64 pt : 9;
		uint64 pd : 9;
		uint64 pdpt : 9;
		uint64 pml4 : 9;
		uint64 reserved : 16;
	};
};

#if defined(_MSC_EXTENSIONS)
#pragma warning(pop)
#endif