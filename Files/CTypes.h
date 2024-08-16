#pragma once
#include <xmmintrin.h>

typedef __m128				uint128;
typedef unsigned __int64	uint64;
typedef unsigned __int32	uint32;
typedef unsigned __int16	uint16;
typedef unsigned __int8		uint8;

typedef __int64				int64;
typedef __int32				int32;
typedef __int16				int16;
typedef __int8				int8;

typedef unsigned long long	ulonglong;
typedef unsigned long		ulong;
typedef unsigned short		ushort;
typedef unsigned char		uchar;

typedef wchar_t wchar;

#ifdef _WIN64
	typedef unsigned __int64 uintn;
	typedef __int64 intn;
#else
	typedef unsigned __int32 uintn;
	typedef __int32 intn;
#endif