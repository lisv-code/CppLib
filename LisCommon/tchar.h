
#ifndef _TCHAR_H_

#define _TCHAR_H_

#if defined(UNICODE) || defined(_UNICODE)

#define TCHAR wchar_t
#define _TEXT(x) L ## x

#include <wchar.h>

#define _tcscmp wcscmp
#define _tcsftime wcsftime
#define _tcsicmp wcsicmp
#define _tcslen wcslen
#define _tcsncmp wcsncmp

#else

#define TCHAR char
#define _TEXT(x) x

#include <string.h>

#define _tcscmp strcmp
#define _tcsftime strftime
#define _tcsicmp stricmp
#define _tcslen strlen
#define _tcsncmp strncmp

#endif

#endif // _TCHAR_H_
