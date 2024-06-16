// ****** Utilities for work with String (char*, wchar_t*) data types. (c) 2006-2024 LISV ******
#include "StrUtils.h"
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <stdarg.h>
#include <limits.h>

#include <sstream>
#include <iomanip>
#include <string>
#include <cctype>
#include <cwctype>

#ifdef _WINDOWS

#include <windows.h>

#include <shlwapi.h>
#undef StrChr
#undef StrCpy
#undef StrNCpy
#undef StrSpn
#undef StrStr
#pragma comment(lib, "shlwapi.lib")

#else

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif // _GNU_SOURCE
#include <string.h>
#include <ctype.h>
#include <cwchar>
#include <cwctype>

#endif

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

namespace LisStr {

const char* StrChr(const char* str, int chr) { return strchr(str, chr); }
const wchar_t* StrChr(const wchar_t* str, int chr) { return wcschr(str, chr); }
char* StrCpy(char* dst, const char* src) { return strcpy(dst, src); }
wchar_t* StrCpy(wchar_t* dst, const wchar_t* src) { return wcscpy(dst, src); }
#ifdef _WINDOWS
int StrICmp(const char* str1, const char* str2) { return stricmp(str1, str2); }
const char* StrIStr(const char* str, const char* substr) { return StrStrIA(str, substr); }
const wchar_t* StrIStr(const wchar_t* str, const wchar_t* substr) { return StrStrIW(str, substr); }
#else
int StrICmp(const char* str1, const char* str2) { return strcasecmp(str1, str2); }
const char* StrIStr(const char* str, const char* substr) { return strcasestr(str, substr); }
const wchar_t* StrIStrW(const wchar_t* str, const wchar_t* sub); // forward declaration
const wchar_t* StrIStr(const wchar_t* str, const wchar_t* substr) { return StrIStrW(str, substr); }
#endif
size_t StrLen(const char* str) { return strlen(str); }
size_t StrLen(const wchar_t* str) { return wcslen(str); }
char* StrNCpy(char* dst, const char* src, size_t num) { return strncpy(dst, src, num); }
wchar_t* StrNCpy(wchar_t* dst, const wchar_t* src, size_t num) { return wcsncpy(dst, src, num); }
size_t StrSpn(const char* str, const char* ctrl) { return strspn(str, ctrl); }
size_t StrSpn(const wchar_t* str, const wchar_t* ctrl) { return wcsspn(str, ctrl); }
const char* StrStr(const char* str, const char* substr) { return strstr(str, substr); }
const wchar_t* StrStr(const wchar_t* str, const wchar_t* substr) { return wcsstr(str, substr); }
double StrToD(const char* str, char** endptr) { return strtod(str, endptr); }
double StrToD(const wchar_t* str, wchar_t** endptr) { return wcstod(str, endptr); }

// Set locale to system default (needed for functions like strupr()...)
const char* LC_MB = setlocale(LC_ALL, "");
#ifdef _WINDOWS
const wchar_t* LC_WC = _wsetlocale(LC_ALL, L"");
#endif

const struct lconv* LOCALE_INFO = localeconv();

template<typename T>
T* StrConcatArr(const T* pBuffer, T** aStrArr, size_t uArrLen, const T* sStrDelim)
{
	T *sResult = (T*)pBuffer, *pPos;
	if (!aStrArr) return NULL;
	size_t uLen = 0, uStrCnt = 0;
	while ((0 == uArrLen || uStrCnt < uArrLen) && aStrArr[uStrCnt] != NULL) { // Get length of full result string
		uLen += StrLen(aStrArr[uStrCnt]);  ++uStrCnt;
	}
	if (uLen) {  // Get memory for string and copy all substrings into it
		size_t uStrDelimLen = 0;
		if (1 < uStrCnt) {
			uStrDelimLen = StrLen(sStrDelim);
			uLen += uStrDelimLen * (uStrCnt - 1);
		}
		if (!sResult) sResult = (T*)malloc((uLen + 1) * sizeof(T));
		sResult[uLen] = 0;
		pPos = sResult;
		uStrCnt = 0;
		while ((0 == uArrLen || uStrCnt < uArrLen) && aStrArr[uStrCnt] != NULL) { // Copy all data into result
			if (0 < uStrDelimLen && 0 < uStrCnt) { StrCpy(pPos, sStrDelim);  pPos += uStrDelimLen; }
			StrCpy(pPos, aStrArr[uStrCnt]);
			pPos += StrLen(aStrArr[uStrCnt]);
			++uStrCnt;
		}
	} else sResult = NULL;
	return sResult;
}

char* StrConcatArr(const char* pBuffer, char** aStrArr, size_t uArrLen, const char* sStrDelim)
{
	return StrConcatArr<char>(pBuffer, aStrArr, uArrLen, sStrDelim);
}

wchar_t* StrConcatArr(const wchar_t* pBuffer, wchar_t** aStrArr, size_t uArrLen, const wchar_t* sStrDelim)
{
	return StrConcatArr<wchar_t>(pBuffer, aStrArr, uArrLen, sStrDelim);
}

template<typename T>
T* StrCopy(const T* sSrc, T* sDstBuf, size_t iLen)
{
	if (!sDstBuf && sSrc) {
		if (0 >= iLen) iLen = StrLen(sSrc);
		sDstBuf = (T*)malloc((iLen + 1) * sizeof(T));
	}
	if (sDstBuf) {
		if (sSrc)
			if (0 < iLen) {
				StrNCpy(sDstBuf, sSrc, iLen);
				sDstBuf[iLen] = 0;
			} else StrCpy(sDstBuf, sSrc);
		else sDstBuf[0] = 0;
	}
	return sDstBuf;
}

char* StrCopy(const char* sSrc, char* sDstBuf, size_t iLen)
{
	return StrCopy<char>(sSrc, sDstBuf, iLen);
}

wchar_t* StrCopy(const wchar_t* sSrc, wchar_t* sDstBuf, size_t iLen)
{
	return StrCopy<wchar_t>(sSrc, sDstBuf, iLen);
}

#ifdef _WINDOWS

wchar_t* StrMB2WC(const char* sSrc, unsigned uCodePage)
{
	size_t iDstBufLen = StrMB2WC(sSrc, uCodePage, nullptr, 0);
	wchar_t* sResult = (wchar_t*)calloc(iDstBufLen + 1, sizeof(wchar_t));
	if (0 == StrMB2WC(sSrc, uCodePage, sResult, iDstBufLen)) {
		free(sResult);
		return nullptr;
	}
	return sResult;
}

size_t StrMB2WC(const char* sSrc, unsigned uCodePage, wchar_t* sDstBuf, int iDstBufLen)
{
	return MultiByteToWideChar(uCodePage, 0, sSrc, -1, sDstBuf, iDstBufLen);
}

char* StrWC2MB(const wchar_t* sSrc, unsigned uCodePage)
{
	size_t iDstBufLen = StrWC2MB(sSrc, uCodePage, nullptr, 0);
	char* sResult = (char*)calloc(iDstBufLen + 1, sizeof(char));
	if (0 == StrWC2MB(sSrc, uCodePage, sResult, iDstBufLen)) {
		free(sResult);
		return nullptr;
	}
	return sResult;
}

size_t StrWC2MB(const wchar_t* sSrc, unsigned uCodePage, char* sDstBuf, int iDstBufLen)
{
	return WideCharToMultiByte(uCodePage, 0, sSrc, -1, sDstBuf, iDstBufLen, NULL, NULL);
}

#else

bool AdjustLocale(unsigned codePage, int category = LC_ALL)
{
	bool result = false;
	if (CP_ACP != codePage) {
		switch (codePage) {
			case CP_UTF8: if (setlocale(category, "C.UTF-8")) result = true; break;
		}
	}
	return result;
}

wchar_t* StrMB2WC(const char* sSrc, unsigned uCodePage)
{
	bool isLocChanged = AdjustLocale(uCodePage, LC_CTYPE);
	size_t iDstBufLen = StrMB2WC(sSrc, uCodePage, nullptr, 0);
	wchar_t* sResult = (wchar_t*)calloc(iDstBufLen + 1, sizeof(wchar_t));
	StrMB2WC(sSrc, uCodePage, sResult, iDstBufLen);
	if (isLocChanged) setlocale(LC_CTYPE, "");
	return sResult;
}

size_t StrMB2WC(const char* sSrc, unsigned uCodePage, wchar_t* sDstBuf, int iDstBufLen)
{
	bool isLocChanged = AdjustLocale(uCodePage, LC_CTYPE);
	size_t result = mbstowcs(sDstBuf, sSrc, iDstBufLen);
	if ((size_t)-1 == result) result = 0;
	if (isLocChanged) setlocale(LC_CTYPE, "");
	return result;
}

char* StrWC2MB(const wchar_t* sSrc, unsigned uCodePage)
{
	bool isLocChanged = AdjustLocale(uCodePage, LC_CTYPE);
	size_t iDstBufLen = StrWC2MB(sSrc, uCodePage, nullptr, 0);
	char* sResult = (char*)calloc(iDstBufLen + 1, sizeof(char));
	StrWC2MB(sSrc, uCodePage, sResult, iDstBufLen);
	if (isLocChanged) setlocale(LC_CTYPE, "");
	return sResult;
}

size_t StrWC2MB(const wchar_t* sSrc, unsigned uCodePage, char* sDstBuf, int iDstBufLen)
{
	bool isLocChanged = AdjustLocale(uCodePage, LC_CTYPE);
	size_t result = wcstombs(sDstBuf, sSrc, iDstBufLen);
	if ((size_t)-1 == result) result = 0;
	if (isLocChanged) setlocale(LC_CTYPE, "");
	return result;
}

#endif

template<typename T>
T** StrSplit(T* sSrc, unsigned* pStrCnt, T cSeparator, T** aResultBuffer, bool bSkipEmpty, size_t uSrcMaxLen)
{
	T *sSrcPos0 = sSrc, *sSrcPosX;
	T *sSrcPosMax = (uSrcMaxLen ? &sSrc[uSrcMaxLen] : nullptr);
	unsigned uStrCntMax = (pStrCnt ? *pStrCnt : UINT_MAX);
	unsigned uStrCnt = 0;
	while (uStrCnt < uStrCntMax && (sSrcPosX = const_cast<T*>(StrChr(sSrcPos0, cSeparator)))) {
		aResultBuffer[uStrCnt] = sSrcPos0;
		*sSrcPosX = 0;
		sSrcPos0 = sSrcPosX + 1;
		if (!bSkipEmpty || (aResultBuffer[uStrCnt] && StrLen(aResultBuffer[uStrCnt]))) ++uStrCnt;
		if (sSrcPosMax && sSrcPos0 >= sSrcPosMax) { sSrcPos0 = nullptr;  break; }
	}
	if (!bSkipEmpty || (sSrcPos0 && StrLen(sSrcPos0))) {
		aResultBuffer[uStrCnt] = sSrcPos0;
		++uStrCnt;
	}
	if (pStrCnt) *pStrCnt = uStrCnt;
	return aResultBuffer;
}

char** StrSplit(char* sSrc, unsigned* pStrCnt, char cSeparator, char** aResultBuffer, bool bSkipEmpty, size_t uSrcMaxLen)
{
	return StrSplit<char>(sSrc, pStrCnt, cSeparator, aResultBuffer, bSkipEmpty, uSrcMaxLen);
}

wchar_t** StrSplit(wchar_t* sSrc, unsigned* pStrCnt, wchar_t cSeparator, wchar_t** aResultBuffer, bool bSkipEmpty, size_t uSrcMaxLen)
{
	return StrSplit<wchar_t>(sSrc, pStrCnt, cSeparator, aResultBuffer, bSkipEmpty, uSrcMaxLen);
}

const char* TrimSymbolsA = " \n\r\t";
const wchar_t* TrimSymbolsW = L" \n\r\t";

template<typename T>
T* StrTrim(T* str, StrTrimOptions options, const T* trim_symbols)
{
	size_t uLen, uLeft, uRight, i;
	T* sResult = NULL;
	if (str) uLen = StrLen(str); else uLen = 0;
	if (0 < uLen) {
		i = 0;  uLeft = 0;
		if (options & stoLeft) uLeft = StrSpn(str, trim_symbols); // Trim left
		uRight = 0;
		if ((options & stoRight) && (uLeft < uLen)) { // Trim right
			i = uLen - 1;
			while ((i > 0) && StrChr(trim_symbols, str[i])) { --i;  ++uRight; }
		}
		uLen = uLen - uRight - uLeft;
		if (options & stoNewStr) {
			sResult = (T*)calloc(uLen + 1, sizeof(T));
			memcpy(sResult, str + uLeft, uLen * sizeof(T));
		} else {
			if (uLeft) memmove(str, str + uLeft, uLen * sizeof(T));
			str[uLen] = 0;
			sResult = str;
		}
	}
	return sResult;
}

char* StrTrim(char* str, StrTrimOptions options, const char* trim_symbols)
{
	return StrTrim<char>(str, options, trim_symbols);
}

wchar_t* StrTrim(wchar_t* str, StrTrimOptions options, const wchar_t* trim_symbols)
{
	return StrTrim<wchar_t>(str, options, trim_symbols);
}

wchar_t* StrReplace(wchar_t* Str, const wchar_t OldSym, const wchar_t NewSym)
{
	if (Str) {
		unsigned i = 0;
		while (0 != Str[i]) {
			if (OldSym == Str[i]) Str[i] = NewSym;
			++i;
		}
	}
	return Str;
}

#define Num_SymbolsA "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"
#define Num_SymbolsW L"zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz"

template<typename TNum, typename TChr>
TChr* itoa(TNum Value, TChr* Buffer, unsigned Radix)
{
	// Check that the base is valid
	if (Radix < 2 || Radix > 36) { *Buffer = '\0'; return Buffer; }

	TChr* ptr = Buffer, *ptr1 = Buffer, tmp_char;
	TNum tmp_value;

	const TChr* symbols = sizeof(TChr) == 1 ? (TChr*)Num_SymbolsA : (TChr*)Num_SymbolsW;
	do {
		tmp_value = Value;
		Value /= Radix;
		*ptr++ = symbols[35 + (tmp_value - Value * Radix)];
	} while (Value);

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = (TChr)'\0';
	while (ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr-- = *ptr1;
		*ptr1++ = tmp_char;
	}
	return Buffer;
}

template<typename TNum, typename TChr>
TChr* IntToStr(TNum iValue, TChr* sBuffer, unsigned uRadix)
{
	TChr* sResult = sBuffer;
	if (NULL == sResult) sResult = (TChr*)calloc(StrIntMaxLen, sizeof(TChr));
	itoa<TNum, TChr>(iValue, sResult, uRadix);
	return sResult;
}

char* IntToStr(int iValue, char* sBuffer, unsigned uRadix)
{
	return IntToStr<int, char>(iValue, sBuffer, uRadix);
}

char* IntToStr(unsigned int uValue, char* sBuffer, unsigned uRadix)
{
	return IntToStr<unsigned int, char>(uValue, sBuffer, uRadix);
}

char* IntToStr(int64_t iValue, char* sBuffer, unsigned uRadix)
{
	return IntToStr<int64_t, char>(iValue, sBuffer, uRadix);
}

char* IntToStr(uint64_t iValue, char* sBuffer, unsigned uRadix)
{
	return IntToStr<uint64_t, char>(iValue, sBuffer, uRadix);
}

wchar_t* IntToStr(int iValue, wchar_t* sBuffer, unsigned uRadix)
{
	return IntToStr<int, wchar_t>(iValue, sBuffer, uRadix);
}

wchar_t* IntToStr(int64_t iValue, wchar_t* sBuffer, unsigned uRadix)
{
	return IntToStr<int64_t, wchar_t>(iValue, sBuffer, uRadix);
}

template<typename T>
static T* DblToStr(double dValue, T* sBuffer, unsigned uDecNum)
{
	std::basic_stringstream<T> stm;
	stm << std::fixed << std::setprecision(uDecNum) << dValue;
	std::basic_string<T> str = stm.str();

	// Remove padding
	size_t pos1 = str.find_last_not_of((T)'0');
	if (pos1 != std::string::npos) str.erase(pos1 + 1);
	pos1 = str.find_last_not_of('.');
	if (pos1 != std::string::npos) str.erase(pos1 + 1);

	return StrCopy(str.c_str(), sBuffer, str.size());
}

char* DblToStr(double dValue, char* sBuffer, unsigned uDecNum)
{
	return DblToStr<char>(dValue, sBuffer, uDecNum);
}

wchar_t* DblToStr(double dValue, wchar_t* sBuffer, unsigned uDecNum)
{
	return DblToStr<wchar_t>(dValue, sBuffer, uDecNum);
}

int StrToInt(const char* S) { return NULL != S ? atoi(S) : 0; }

int StrToInt(const wchar_t* S) { return NULL != S ? wcstol(S, nullptr, 10) : 0; }

int64_t StrToInt64(const char* S) { return NULL != S ? atoll(S) : 0; }

int64_t StrToInt64(const wchar_t* S) { return NULL != S ? wcstoll(S, nullptr, 10) : 0; }

template<typename T>
double StrToDbl(const T* S, int* pErrChrIdx)
{
	if (!S) return 0;
	T* endPtr;
	double result = StrToD(S, pErrChrIdx ? &endPtr : nullptr);
	if (pErrChrIdx) *pErrChrIdx = 0 != endPtr[0] ? endPtr - S : -1;
	return result;
}

double StrToDbl(const char* S, int* pErrChrIdx) { return StrToDbl<char>(S, pErrChrIdx); }

double StrToDbl(const wchar_t* S, int* pErrChrIdx) { return StrToDbl<wchar_t>(S, pErrChrIdx); }

template<typename T>
T* StrFill(T* sFillValue, size_t uLength, T* sDestination)
{
	T* sResult = sDestination;
	if (uLength && sFillValue) {
		size_t uPos = 0, uLenF = StrLen(sFillValue);
		if (!sResult) sResult = (T*)calloc(uLength + 1, sizeof(T));
		while (uPos < (uLength - uLenF)) { StrCpy(sResult + uPos, sFillValue);  uPos += uLenF; }
		memcpy(sResult + uPos, sFillValue, (uLength - uPos) * sizeof(T));
	}
	return sResult;
}

char* StrFill(char* sFillValue, size_t uLength, char* sDestination)
{
	return StrFill<char>(sFillValue, uLength, sDestination);
}

wchar_t* StrFill(wchar_t* sFillValue, size_t uLength, wchar_t* sDestination)
{
	return StrFill<wchar_t>(sFillValue, uLength, sDestination);
}

char std_tolower(char chr) { return std::tolower(chr); }
wchar_t std_tolower(wchar_t chr) { return std::towlower(chr); }

template<typename T>
const T* StrIStrT1(const T* str, const T* sub)
{
	if (!str || !sub) return nullptr;
	if (str == sub) return str;

	const T *pos_str = str, *pos_sub = sub, *result = nullptr;
	T chrX = std_tolower(*pos_sub);
	while (0 != *pos_str) {
		if (std_tolower(*pos_str) == chrX) {
			if (!result) result = pos_str;
			++pos_sub;
			if (0 == *pos_sub) return result;
			chrX = std_tolower(*pos_sub);
		} else if (result) {
			result = nullptr;
			pos_sub = sub;
			chrX = std_tolower(*pos_sub);
		}
		++pos_str;
	}
	return nullptr;
}

const wchar_t* StrIStrW(const wchar_t* str, const wchar_t* sub)
{
	return StrIStrT1(str, sub);
}

// *************************************** CStrConvert class ***************************************

CStrConvert::CStrConvert(const char* str, unsigned code_page)
{
	codePage = code_page;
	wcData = nullptr; isConstWc = false;
	mbData = const_cast<char*>(str); isConstMb = true;
}

CStrConvert::CStrConvert(const wchar_t* str, unsigned code_page)
{
	codePage = code_page;
	mbData = nullptr; isConstMb = false;
	wcData = const_cast<wchar_t*>(str); isConstWc = true;
}

CStrConvert::operator char*()
{
	if (!mbData && wcData) mbData = StrWC2MB(wcData, codePage);
	return mbData;
}

CStrConvert::operator wchar_t*()
{
	if (!wcData && mbData) wcData = StrMB2WC(mbData, codePage);
	return wcData;
}

CStrConvert::~CStrConvert()
{
	if (!isConstMb && mbData) free(mbData);
	if (!isConstWc && wcData) free(wcData);
}

// ************************************** StrConcat functions **************************************

template<typename T>
size_t StrLen_VA(const T* Str, va_list Args)
{
	size_t uLen = StrLen(Str);
	T *sArg;
	while (nullptr != (sArg = va_arg(Args, T*)))
		uLen += StrLen(sArg);
	return uLen;
}

template<typename T>
T* StrConcat_VA(T* sBuffer, const T* Str, va_list Args)
{
	if (sBuffer != Str) StrCpy(sBuffer, Str);
	T *sArg, *pBufPos = sBuffer + StrLen(Str);
	while (nullptr != (sArg = va_arg(Args, T*))) {
		StrCpy(pBufPos, sArg);
		pBufPos += StrLen(sArg);
	}
	pBufPos = 0;
	return sBuffer;
}

char* StrConcat(char* sBuffer, const char* Str, ...)
{
	if (!Str) return nullptr;
	char *sResult;
	va_list Args;
	va_start(Args, Str);
	if (!sBuffer) {
		size_t uLen = StrLen_VA(Str, Args);
		sResult = (char*)malloc((uLen + 1) * sizeof(char));
		va_end(Args);
		va_start(Args, Str);
	} else sResult = sBuffer;
	StrConcat_VA(sResult, Str, Args);
	va_end(Args);
	return sResult;
}

wchar_t* StrConcat(wchar_t* sBuffer, const wchar_t* Str, ...)
{
	if (!Str) return nullptr;
	wchar_t *sResult;
	va_list Args;
	va_start(Args, Str);
	if (!sBuffer) {
		size_t uLen = StrLen_VA(Str, Args);
		sResult = (wchar_t*)malloc((uLen + 1) * sizeof(wchar_t));
		va_end(Args);
		va_start(Args, Str);
	} else sResult = sBuffer;
	StrConcat_VA(sResult, Str, Args);
	va_end(Args);
	return sResult;
}

// *************************************** CStrConcat class ****************************************

CStrConcat::CStrConcat(const char* str, ...)
{
	if (!str) { sResult = nullptr;  return; }
	va_list Args;
	va_start(Args, str);
	size_t uLen = StrLen_VA(str, Args);
	va_end(Args);
	sResult = (char*)malloc(sizeof(char) * (uLen + 1));
	va_start(Args, str);
	StrConcat_VA((char*)sResult, str, Args);
	va_end(Args);
}

CStrConcat::CStrConcat(const wchar_t* str, ...)
{
	if (!str) { sResult = nullptr;  return; }
	va_list Args;
	va_start(Args, str);
	size_t uLen = StrLen_VA(str, Args);
	va_end(Args);
	sResult = (wchar_t*)malloc(sizeof(wchar_t) * (uLen + 1));
	va_start(Args, str);
	StrConcat_VA((wchar_t*)sResult, str, Args);
	va_end(Args);
}

CStrConcat::operator char*() const { return (char*)sResult; }

CStrConcat::operator wchar_t*() const { return (wchar_t*)sResult; }

CStrConcat::~CStrConcat() { free(sResult); }

// **************************************** CIntToStr class ****************************************

CIntToStr::CIntToStr(const int64_t iValue, const unsigned uRadix)
{
	this->iValue = iValue;  this->uRadix = uRadix;
	sResult = nullptr;
}

CIntToStr::operator char* const()
{
	if (!sResult) sResult = (void*)IntToStr(iValue, (char*)nullptr, uRadix);
	return (char*)sResult;
}

CIntToStr::operator wchar_t* const()
{
	if (!sResult) sResult = (void*)IntToStr(iValue, (wchar_t*)nullptr, uRadix);
	return (wchar_t*)sResult;
}

CIntToStr::~CIntToStr() { free(sResult); }

// **************************************** CDblToStr class ****************************************

CDblToStr::CDblToStr(const double dValue, unsigned uDecNum)
{
	this->dValue = dValue;  this->uDecNum = uDecNum;
	sResult = nullptr;
}

CDblToStr::operator char* const()
{
	if (!sResult) sResult = (void*)DblToStr(dValue, (char*)0, uDecNum);
	return (char*)sResult;
}

CDblToStr::operator wchar_t* const()
{
	if (!sResult) sResult = (void*)DblToStr(dValue, (wchar_t*)0, uDecNum);
	return (wchar_t*)sResult;
}

CDblToStr::~CDblToStr() { free(sResult); }

} // namespace LisStr
