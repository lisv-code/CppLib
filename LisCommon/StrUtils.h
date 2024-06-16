// ****** Utilities for work with String (char*, wchar_t*) data types. (c) 2006-2024 LISV ******
#pragma once
#ifndef _LIS_STRING_UTILS_H_
#define _LIS_STRING_UTILS_H_

#include <stddef.h>
#include <stdint.h>

// Code Page values.
#ifndef CP_ACP
#define CP_ACP 0 // Environment's default locale (Windows: ANSI code page)
#endif
#ifndef CP_UTF8
#define CP_UTF8 65001 // UTF-8
#endif

namespace LisStr {

const char* StrChr(const char* str, int chr);
const wchar_t* StrChr(const wchar_t* str, int chr);
char* StrCpy(char* dst, const char* src);
wchar_t* StrCpy(wchar_t* dst, const wchar_t* src);
int StrICmp(const char* str1, const char* str2);
const char* StrIStr(const char* str, const char* substr);
const wchar_t* StrIStr(const wchar_t* str, const wchar_t* substr);
size_t StrLen(const char* str);
size_t StrLen(const wchar_t* str);
char* StrNCpy(char* dst, const char* src, size_t num);
wchar_t* StrNCpy(wchar_t* dst, const wchar_t* src, size_t num);
size_t StrSpn(const char* str, const char* ctrl);
size_t StrSpn(const wchar_t* str, const wchar_t* ctrl);
const char* StrStr(const char* str, const char* substr);
const wchar_t* StrStr(const wchar_t* str, const wchar_t* substr);
double StrToD(const char* str, char** endptr);
double StrToD(const wchar_t* str, wchar_t** endptr);

#define StrIntMaxLen 40 // Maximal length of 64-bit integer (in decimal digits) multiplied by 2

// StrConcatArr: uArrLen = 0 - null-terminated array
char* StrConcatArr(const char* pBuffer, char** aStrArr, size_t uArrLen, const char* sStrDelim);
wchar_t* StrConcatArr(const wchar_t* pBuffer, wchar_t** aStrArr, size_t uArrLen, const wchar_t* sStrDelim);

// sBuffer == nullptr - new memory will be allocated,
// sBuffer == Str - strings will be appended to existing Str
char* StrConcat(char* sBuffer, const char* Str, ...);
wchar_t* StrConcat(wchar_t* sBuffer, const wchar_t* Str, ...);

// sDstBuf must be not equal to sSrc
// sDstBuf == nullptr - new memory will be allocated,
// iLen = how many characters to copy (if iLen <= 0 - copy all source)
char* StrCopy(const char* sSrc, char* sDstBuf = nullptr, size_t iLen = 0);
wchar_t* StrCopy(const wchar_t* sSrc, wchar_t* sDstBuf = nullptr, size_t iLen = 0);

wchar_t* StrMB2WC(const char* sSrc, unsigned uCodePage);
size_t StrMB2WC(const char* sSrc, unsigned uCodePage, wchar_t* sDstBuf, int iDstBufLen);
char* StrWC2MB(const wchar_t* sSrc, unsigned uCodePage);
size_t StrWC2MB(const wchar_t* sSrc, unsigned uCodePage, char* sDstBuf, int iDstBufLen);

/// <summary>
/// Returns an array containing the substrings in source string that are delimited by a specified character
/// </summary>
/// <param name="sSrc">Source string</param>
/// <param name="pStrCnt">[In] pointer to maximal number of strings to return (NULL = UINT_MAX);
///   [Out] - number of strings in result array
/// </param>
/// <param name="cSeparator">Character that delimit the substrings</param>
/// <param name="aResultBuffer">Pointer to buffer to get result array</param>
/// <param name="bSkipEmpty">Do not store empty strings to result array</param>
/// <param name="uSrcMaxLen">Maximal length of source string (default: 0 - entire string)</param>
/// <returns>Array of substrings (char* pointers)</returns>
char** StrSplit(char* sSrc, unsigned* pStrCnt, char cSeparator, char** aResultBuffer, bool bSkipEmpty, size_t uSrcMaxLen = 0);
wchar_t** StrSplit(wchar_t* sSrc, unsigned* pStrCnt, wchar_t cSeparator, wchar_t** aResultBuffer, bool bSkipEmpty, size_t uSrcMaxLen = 0);

enum StrTrimOptions { stoNewStr = 1, stoLeft = 2, stoRight = 4 };
extern const char* TrimSymbolsA;
char* StrTrim(char* str, StrTrimOptions options, const char* trim_symbols = TrimSymbolsA);
extern const wchar_t* TrimSymbolsW;
wchar_t* StrTrim(wchar_t* str, unsigned options, const wchar_t* trim_symbols = TrimSymbolsW);

wchar_t* StrReplace(wchar_t* Str, const wchar_t OldSym, const wchar_t NewSym);

// Valid Radix value is from 2 up to 36
char* IntToStr(int iValue, char* sBuffer = nullptr, unsigned uRadix = 10);
char* IntToStr(unsigned int uValue, char* sBuffer = nullptr, unsigned uRadix = 10);
char* IntToStr(int64_t iValue, char* sBuffer = nullptr, unsigned uRadix = 10);
char* IntToStr(uint64_t iValue, char* sBuffer = nullptr, unsigned uRadix = 10);
wchar_t* IntToStr(int iValue, wchar_t* sBuffer = nullptr, unsigned uRadix = 10);
wchar_t* IntToStr(int64_t iValue, wchar_t* sBuffer = nullptr, unsigned uRadix = 10);

char* DblToStr(double dValue, char* sBuffer = nullptr, unsigned uDecNum = 6);
wchar_t* DblToStr(double dValue, wchar_t* sBuffer = nullptr, unsigned uDecNum = 6);

int StrToInt(const char* S);
int StrToInt(const wchar_t* S);
int64_t StrToInt64(const char* S);
int64_t StrToInt64(const wchar_t* S);

double StrToDbl(const char* S, int *pErrChrIdx = nullptr);
double StrToDbl(const wchar_t* S, int *pErrChrIdx = nullptr);

char* StrFill(char* sFillValue, size_t uLength, char* sDestination = nullptr);
wchar_t* StrFill(wchar_t* sFillValue, size_t uLength, wchar_t* sDestination = nullptr);

class CStrConvert
{
	unsigned codePage;
	char *mbData;
	wchar_t *wcData;
	bool isConstMb, isConstWc;
public:
	CStrConvert(const char* str, unsigned code_page = CP_ACP);
	CStrConvert(const wchar_t* str, unsigned code_page = CP_ACP);
	operator char*();
	operator wchar_t*();
	~CStrConvert();
};

class CStrConcat
{
	void* sResult;
public:
	CStrConcat(const char* str, ...);
	CStrConcat(const wchar_t* str, ...);
	operator char*() const;
	operator wchar_t*() const;
	~CStrConcat();
};

class CIntToStr
{
	int64_t iValue;
	unsigned uRadix;
	void* sResult;
public:
	CIntToStr(): sResult(nullptr) {};
	CIntToStr(const int64_t iValue, unsigned uRadix = 10);
	operator char* const();
	operator wchar_t* const();
	~CIntToStr();
};

class CDblToStr
{
	double dValue;
	unsigned uDecNum;
	void* sResult;
public:
	CDblToStr(): sResult(nullptr) {};
	CDblToStr(const double dValue, unsigned uDecNum = 6);
	operator char* const();
	operator wchar_t* const();
	~CDblToStr();
};

} // namespace LisStr

#endif // #ifndef _LIS_STRING_UTILS_H_
