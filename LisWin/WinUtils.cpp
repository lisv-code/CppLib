// ****** MS Windows system utilities. (c) LISV 2008-2024 ******
#include <stdio.h>
#include "WinUtils.h"
#include "../LisCommon/StrUtils.h"
#pragma comment(lib,"version.lib")

#ifdef UNICODE
#define sprintf swprintf
#endif

bool GetVersionInfo(const TCHAR* sFileName, const TCHAR** aVINames, TCHAR** aVIValues)
{
	bool bResult = false;
	DWORD hVer;
	UINT uSize, uVVLen;
	void *pVer, *pVerVal;
	uSize = GetFileVersionInfoSize(sFileName, &hVer);
	if (0 < uSize) {
		pVer = malloc(uSize);
		if (GetFileVersionInfo(sFileName, hVer, uSize, pVer)) {
			VerQueryValue(pVer, TEXT("\\VarFileInfo\\Translation"), &pVerVal, &uVVLen); // get translations list
			if (NULL != pVerVal) {
				TCHAR sTxtBuf[0xFF];
				sprintf(sTxtBuf, TEXT("%.4X%.4X"), LOWORD(*(DWORD*)pVerVal)/*language*/, HIWORD(*(DWORD*)pVerVal)/*codepage*/);
				TCHAR* sLngBlk = LisStr::StrConcat(0, TEXT("\\StringFileInfo\\"), sTxtBuf, TEXT("\\"), 0);
				int i = 0;
				while (NULL != aVINames[i]) {
					if (VerQueryValue(pVer, LisStr::CStrConcat(sLngBlk, aVINames[i], 0), &pVerVal, &uVVLen))
						aVIValues[i] = LisStr::StrCopy((TCHAR*)pVerVal);
					++i;
				}
				free(sLngBlk);
				bResult = true;
			}
		}
		free(pVer);
	}
	return bResult;
}

// *********************************** CSysErrStr implementation ***********************************

CSysErrStr::CSysErrStr(void)
{
	dwErrorCode = 0;
	sResult = NULL;
	lpFMSource = NULL;
	SetFMParams();
}

CSysErrStr::CSysErrStr(const DWORD dwErrorCode)
{
	this->dwErrorCode = dwErrorCode;
	sResult = NULL;
	lpFMSource = NULL;
	SetFMParams();
}

void CSysErrStr::SetFMParams(void)
{
	dwFMFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;
	if (lpFMSource) FreeLibrary((HMODULE)lpFMSource);
	lpFMSource = NULL;
	if (12000 <= dwErrorCode && dwErrorCode <= 12174) {
		lpFMSource = LoadLibrary(TEXT("WinInet.dll"));
		if (lpFMSource) dwFMFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE;
	}
}

CSysErrStr::operator char* () const
{
	FormatMessageA(dwFMFlags, lpFMSource, dwErrorCode, 0, (LPSTR)&sResult, 0, 0);
	return (char*)sResult;
}

CSysErrStr::operator wchar_t* () const
{
	FormatMessageW(dwFMFlags, lpFMSource, dwErrorCode, 0, (LPWSTR)&sResult, 0, 0);
	return (wchar_t*)sResult;
}

CSysErrStr::~CSysErrStr()
{
	if (lpFMSource) FreeLibrary((HMODULE)lpFMSource);
	if (sResult) LocalFree(sResult);
}
