// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include <windows.h>
#include <crtdbg.h>
#include <ostream>
#include <commctrl.h> // potrebuju LPCOLORMAP

#if defined(_DEBUG) && defined(_MSC_VER) // without passing file+line to 'new' operator, list of memory leaks shows only 'crtdbg.h(552)'
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

#ifndef STR_DISABLE

#ifdef INSIDE_SPL // pro pouziti v pluginech
#include "spl_base.h"
#include "dbg.h"
#else                     //INSIDE_SPL
#pragma warning(3 : 4706) // warning C4706: assignment within conditional expression
#include "trace.h"
#include "messages.h"
#include "handles.h"
#endif //INSIDE_SPL

#include "str.h"

#ifndef INSIDE_SPL // krome pouziti v pluginech

// The order here is important.
// Section names must be 8 characters or less.
// The sections with the same name before the $
// are merged into one section. The order that
// they are merged is determined by sorting
// the characters after the $.
// i_str and i_str_end are used to set
// boundaries so we can find the real functions
// that we need to call for initialization.

#pragma warning(disable : 4075) // chceme definovat poradi inicializace modulu

typedef void(__cdecl* _PVFV)(void);

#pragma section(".i_str$a", read)
__declspec(allocate(".i_str$a")) const _PVFV i_str = (_PVFV)1; // na zacatek sekce .i_str si dame promennou i_str

#pragma section(".i_str$z", read)
__declspec(allocate(".i_str$z")) const _PVFV i_str_end = (_PVFV)1; // a na konec sekce .i_str si dame promennou i_str_end

void Initialize__Str()
{
    const _PVFV* x = &i_str;
    for (++x; x < &i_str_end; ++x)
        if (*x != NULL)
            (*x)();
}

#pragma init_seg(".i_str$m")

#endif //INSIDE_SPL

wchar_t LowerCase[256];
wchar_t UpperCase[256];

void InitializeCase();

class C__STR_module // automaticka inicializace modulu
{
public:
    C__STR_module() { InitializeCase(); }
} __STR_module;

// ****************************************************************************

wchar_t* DupStr(const wchar_t* txt)
{
    if (txt == NULL)
        return NULL;
    int l = (int)wcslen(txt);
    wchar_t* s = (wchar_t*)malloc((l + 1) * sizeof(wchar_t));
    if (s == NULL)
    {
        TRACE_E("Low memory.");
        return NULL;
    }
    memcpy(s, txt, (l + 1) * sizeof(wchar_t));
    return s;
}

wchar_t* DupStrEx(const wchar_t* str, BOOL& err)
{
    wchar_t* s = DupStr(str);
    if (str != NULL && s == NULL)
        err = TRUE;
    return s;
}

// ****************************************************************************

wchar_t* StrNCat(wchar_t* dst, const wchar_t* src, int dstSize)
{
    int i = lstrlenW(dst);
    lstrcpynW(dst + i, src, dstSize - i);
    return dst;
}

// ****************************************************************************

void InitializeCase()
{
    int i;
    for (i = 0; i < 256; i++)
        LowerCase[i] = (wchar_t)(UINT_PTR)CharLowerW((LPWSTR)(UINT_PTR)i);
    for (i = 0; i < 256; i++)
        UpperCase[i] = (wchar_t)(UINT_PTR)CharUpperW((LPWSTR)(UINT_PTR)i);
}

//
//*****************************************************************************

int StrICpy(wchar_t* dest, const wchar_t* src)
{
    const wchar_t* s = src;
    while (*src != 0)
    {
        // Assuming src contains primarily ASCII-range characters
        // or characters that have a direct lowercase mapping in the 0-255 range.
        // For full Unicode, this approach to LowerCase is insufficient.
        unsigned int char_val = (unsigned int)(*src);
        if (char_val < 256)
            *dest++ = LowerCase[char_val];
        else
            *dest++ = *src; // Keep non-ASCII range chars as is
        src++;
    }
    *dest = 0;
    return (int)(src - s); // return number of characters copied
}

//
//*****************************************************************************

// No ASM version for wchar_t
int StrICmp(const wchar_t* s1, const wchar_t* s2)
{
    wchar_t c1, c2;
    while (1)
    {
        unsigned int val1 = (unsigned int)(*s1);
        unsigned int val2 = (unsigned int)(*s2);

        c1 = (val1 < 256) ? LowerCase[val1] : *s1;
        c2 = (val2 < 256) ? LowerCase[val2] : *s2;

        if (c1 < c2) return -1;
        if (c1 > c2) return 1;
        if (*s1 == 0) return 0; // Both are equal and s1 is null terminator

        s1++;
        s2++;
    }
}

//
//*****************************************************************************

/*
// puvodni funkce
// pozor, zde je chyba StrNICmp("a", "aa", 2) vraci 0
int StrNICmp(const char *s1, const char *s2, int n)
{
  int res;
  while (n--)
  {
    res = (unsigned)LowerCase[*s1++] - (unsigned)LowerCase[*s2++];
    if (res != 0) return (res < 0) ? -1 : 1;               // < a >
    if (*s1 == 0) return 0;                                // ==
  }
  return 0;
}
*/

#ifdef _WIN64
// C++ wchar_t version of StrNICmp
int StrNICmp(const wchar_t* s1, const wchar_t* s2, int n)
{
    wchar_t c1, c2;
    while (n-- > 0) // Iterate n times or until difference/null terminator
    {
        unsigned int val1 = (unsigned int)(*s1);
        unsigned int val2 = (unsigned int)(*s2);

        c1 = (val1 < 256) ? LowerCase[val1] : *s1;
        c2 = (val2 < 256) ? LowerCase[val2] : *s2;

        if (c1 < c2) return -1;
        if (c1 > c2) return 1;
        if (*s1 == 0) return 0; // Both are equal and s1 is null terminator (implies s2 is also null or they would differ)

        s1++;
        s2++;
    }
    return 0; // Compared n characters and all were equal or reached null terminator on both
}

//
//*****************************************************************************

// C++ wchar_t version of MemICmp. Compares 'n' characters.
int MemICmp(const void* buf1, const void* buf2, int n)
{
    const wchar_t* b1 = (const wchar_t*)buf1;
    const wchar_t* b2 = (const wchar_t*)buf2;
    wchar_t c1, c2;

    while (n-- > 0)
    {
        unsigned int val1 = (unsigned int)(*b1);
        unsigned int val2 = (unsigned int)(*b2);

        c1 = (val1 < 256) ? LowerCase[val1] : *b1;
        c2 = (val2 < 256) ? LowerCase[val2] : *b2;

        if (c1 < c2) return -1;
        if (c1 > c2) return 1;

        // If characters are equal, continue.
        // MemICmp compares exactly n characters.
        // If one buffer ends with NUL, it will be compared against
        // subsequent characters of the other buffer (or NULs).
        // If both are NUL, they are equal for this character.
        if (*b1 == 0 && *b2 == 0 && n > 0) // if both are null and we are not done, they are equal for this char
        {
            // but if we are expected to compare more chars, and they are both NULL this means they are equal so far
            // effectively, this check isn't strictly needed as comparison of c1 and c2 handles it.
            // if *b1 is NUL, c1 will be NUL (or LowerCase[0]).
            // if *b1 and *b2 are both NUL, c1 and c2 will be equal.
        }


        b1++;
        b2++;
    }
    return 0; // Compared n characters and all were equal.
}

//
//*****************************************************************************

// C++ wchar_t version of StrICmpEx
int StrICmpEx(const wchar_t* s1, int l1, const wchar_t* s2, int l2)
{
    int l = (l1 < l2) ? l1 : l2; // Determine shorter length for comparison
    wchar_t c1, c2;

    const wchar_t* s1_end = s1 + l; // End pointer for the common length part

    while (s1 < s1_end) // Compare up to the shorter length
    {
        unsigned int val1 = (unsigned int)(*s1);
        unsigned int val2 = (unsigned int)(*s2);

        c1 = (val1 < 256) ? LowerCase[val1] : *s1;
        c2 = (val2 < 256) ? LowerCase[val2] : *s2;

        if (c1 < c2) return -1;
        if (c1 > c2) return 1;

        // Optimization: if *s1 is NUL, and they were equal, s2 must also be NUL.
        // In this case, if we haven't returned, they are equal up to this NUL.
        // The loop will terminate, and l1 vs l2 will decide.
        if (*s1 == 0) break;

        s1++;
        s2++;
    }

    // If we've compared 'l' characters and found no difference
    if (l1 == l2) return 0; // If lengths are equal, strings are equal
    return (l1 < l2) ? -1 : 1; // Otherwise, the shorter string is "less"
}

//
//*****************************************************************************

/*
// puvodni funkce
int StrCmpEx(const char *s1, int l1, const char *s2, int l2)
{
  int res, l = (l1 < l2) ? l1 : l2;
  while (l--)
  {
    res = (unsigned)(*s1++) - (unsigned)(*s2++);
    if (res != 0) return (res < 0) ? -1 : 1;    // < a >
  }
  if (l1 != l2) return (l1 < l2) ? -1 : 1;    // < a >
  else return 0;
}
*/

// wchar_t varianta
int StrCmpEx(const wchar_t* s1, int l1, const wchar_t* s2, int l2)
{
    int l = (l1 < l2) ? l1 : l2; // Determine shorter length for comparison

    const wchar_t* s1_end = s1 + l;

    while (s1 < s1_end) // Compare up to the shorter length
    {
        if (*s1 < *s2) return -1;
        if (*s1 > *s2) return 1;
        if (*s1 == 0) break; // Optimization similar to StrICmpEx
        s1++;
        s2++;
    }

    if (l1 == l2) return 0;
    return (l1 < l2) ? -1 : 1;
}

//
//*****************************************************************************

const wchar_t* StrIStr(const wchar_t* txt, const wchar_t* pattern)
{
    if (txt == NULL || pattern == NULL)
        return NULL;

    const wchar_t* s = txt;
    int len = (int)wcslen(pattern);
    int txtLen = (int)wcslen(txt);
    while (txtLen >= len)
    {
        if (StrNICmp(s, pattern, len) == 0) // Uses updated StrNICmp (wchar_t)
            return s;
        s++;
        txtLen--;
    }
    return NULL;
}

const wchar_t* StrIStr(const wchar_t* txtStart, const wchar_t* txtEnd,
                       const wchar_t* patternStart, const wchar_t* patternEnd)
{
    if (txtStart == NULL || patternStart == NULL)
        return NULL;

    const wchar_t* s = txtStart;
    int len = (int)(patternEnd - patternStart);
    int txtLen = (int)(txtEnd - txtStart);

    if (len == 0) return txtStart; // Empty pattern matches at the beginning
    if (len < 0 || txtLen < 0) return NULL; // Invalid lengths

    while (txtLen >= len)
    {
        if (StrNICmp(s, patternStart, len) == 0) // Uses updated StrNICmp (wchar_t)
            return s;
        s++;
    }
    return NULL;
}

//
//*****************************************************************************

/*
int StrLen(const char *str)
{
  const char *s = str;
  while (1)
  {
    if ((((*(DWORD *)s) & 0xF0F0F0F0) - 0x10101010) & 0x8F0F0F0F)
    {                            // je tam znak < 16
      if (*s != 0) s++;
      else break;
      if (*s != 0) s++;
      else break;
      if (*s != 0) s++;
      else break;
      if (*s != 0) s++;
      else break;
    }
    else s += 4;
  }
  return s - str;
}
*/

/*
//*****************************************************************************
//
// Tabulky pro prevod kodu Kamenickych do MS Windows
//

BYTE KodKamenickych[CONVERT_TAB_CHARS] =
{
0x84,0x8e,
0xa0,0x8f,
0x87,0x80,
0x83,0x85,
0x82,0x90,
0x88,0x89,
0xa1,0x8b,
0x8d,0x8a,
0x8c,0x9c,
0xa4,0xa5,
0xa2,0x95,
0x94,0x99,
0x93,0xa7,
0xaa,0xab,
0xa9,0x9e,
0xa8,0x9b,
0x9f,0x86,
0xa3,0x97,
0x96,0xa6,
0x81,0x9a,
0x98,0x9d,
0x91,0x92,
}; //done

BYTE KodWindows[CONVERT_TAB_CHARS] =
{
0xe4,0xc4,
0xe1,0xc1,
0xe8,0xc8,
0xef,0xcf,
0xe9,0xc9,
0xec,0xcc,
0xed,0xcd,
0xe5,0xc5,
0xbe,0xbc,
0xf2,0xd2,
0xf3,0xd3,
0xf6,0xd6,
0xf4,0xd4,
0xe0,0xc0,
0xf8,0xd8,
0x9a,0x8a,
0x9d,0x8d,
0xfa,0xda,
0xf9,0xd9,
0xfc,0xdc,
0xfd,0xdd,
0x9e,0x8e
}; //done

CConvertTab::CConvertTab()
{
  WORD i;
  for(i = 0; i < CONVERT_TAB_MAX_CHARS; i++)
    Data[i] = (BYTE)i;
  for(i = 0; i < CONVERT_TAB_CHARS; i++)
    Data[KodKamenickych[i]] = KodWindows[i];
}

void
CConvertTab::Convert(char *str)
{
  char *ptr = str;
  while(*ptr != 0) *ptr++ = Data[*ptr];
}

CConvertTab ConvertTab;
*/

#endif // STR_DISABLE
