/******************************************
Copyright (C) 2022 Axel Kemper (27-Sep-2022)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
***********************************************/

#pragma once

#if _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#define _SCL_SECURE_NO_WARNINGS

#include <intrin.h>
#include "stdint.h"

#define UINT32_MAX _UI32_MAX
#define UINT64_MAX _UI64_MAX

#if !defined(INTMAX_MAX)
#define INTMAX_MAX _I32_MAX
#endif

#ifdef _WIN64 // [
#  define PTRDIFF_MIN  _I64_MIN
#  define PTRDIFF_MAX  _I64_MAX
#else  // _WIN64 ][
#  define PTRDIFF_MIN  _I32_MIN
#  define PTRDIFF_MAX  _I32_MAX
#endif  // _WIN64 ]

#pragma warning(disable : 4244)  //  C4244 : 'Argument': Konvertierung von 'const uint64_t' in 'double', möglicher Datenverlust
#pragma warning(disable : 4267)  //  C4267 : 'return': Konvertierung von 'size_t' nach 'uint32_t', Datenverlust möglich
#pragma warning(disable : 4302)  //  C4302 : truncation
#pragma warning(disable : 4305)  //  C4302 : truncation double to float
#pragma warning(disable : 4311)  //  C4311 : pointer truncation
#pragma warning(disable : 4312)  //  C4312 : conversion from .. of greater size
#pragma warning(disable : 4789)  //  C4789 : buffer '' of size xx bytes will be overrun;

#pragma warning(disable : 4800)  //  C4800 : 'const uint32_t' : Variable wird auf booleschen Wert('True' oder 'False') gesetzt(Auswirkungen auf Leistungsverhalten möglich)
#pragma warning(disable : 4805)  //  C4805 : '==' : unsichere Kombination von Typ 'unsigned short' mit Typ 'bool' in einer Operation
#pragma warning(disable : 4996)  //  C4996 : deprecated
#pragma warning(disable : 26495)  // Always initialize a member variable
#pragma warning(disable : 26819)  //	Unannotated fallthrough between switch labels



#if !defined(__PRETTY_FUNCTION__) && !defined(__GNUC__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif

//  https://gist.github.com/pps83/3210a2f980fd02bb2ba2e5a1fc4a2ef0
static inline int __builtin_ctzll(unsigned long long x) {
	unsigned long ret;
	_BitScanForward64(&ret, x);
	return (int)ret;
}

#include <nmmintrin.h>
#define __builtin_popcountll _mm_popcnt_u64
#define NO_DLL_EXPORT 1

//  picosat
#define isatty(x) _isatty(x)
#define NO_USE_GZ  1
#define NO_USE_PIPES 1
#define NGETRUSAGE 1
#define NALLSIGNALS 1

#endif
