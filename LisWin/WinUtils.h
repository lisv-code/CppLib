// ****** MS Windows system utilities. (c) LISV 2008-2024 ******
#pragma once
#include <windows.h>

bool GetVersionInfo(const TCHAR* sFileName, const TCHAR** aVINames, TCHAR** aVIValues);

class CSysErrStr {
	DWORD dwErrorCode;
	void* sResult;
	DWORD dwFMFlags;
	LPVOID lpFMSource;
	void SetFMParams(void);
public:
	CSysErrStr(void);
	CSysErrStr(DWORD dwErrorCode);
	operator char* () const;
	operator wchar_t* () const;
	~CSysErrStr();
};
