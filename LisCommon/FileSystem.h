// ****** File system utilities. (c) 2024 LISV ******
#pragma once
#ifndef _LIS_FILE_SYSTEM_H_
#define _LIS_FILE_SYSTEM_H_

#ifdef _WINDOWS

#include <tchar.h>

#define FILE_PATH_CHAR TCHAR
#define FILE_PATH_TEXT(quote) _TEXT(quote)
#define FILE_PATH_SEPARATOR_CHR '\\'
#define FILE_PATH_SEPARATOR_STR "\\"
#define PATH_MAX MAX_PATH

#else

#define FILE_PATH_CHAR char
#define FILE_PATH_TEXT(quote) quote
#define FILE_PATH_SEPARATOR_CHR '/'
#define FILE_PATH_SEPARATOR_STR "/"

#endif

#include <stdint.h>
#include <string>
#include <ctime>
#include <vector>
#include <functional>

namespace LisFileSys {

struct FileEntry {
	std::basic_string<FILE_PATH_CHAR> Path; bool IsDir;
	struct FileInfo {
		std::time_t Changed; uint64_t Size;
	} Info;
};
typedef std::function<bool(const FileEntry& file)> DirEnumItemProc;

enum DirEnumOptions {
	deoRecursive = 1, deoFiles = 1 << 1, deoDirFirst = 1 << 2, deoDirLast = 1 << 3
};
extern const DirEnumOptions DirEnumDefaultOptions; // deoFiles

// DirEnum behaviour differs: file_mask is used for directories under Windows, not for other OSes;
// file info (time, size) is retrieved under Windows only as for now.
int DirEnum(DirEnumItemProc item_proc, const FILE_PATH_CHAR* dir_path,
	const FILE_PATH_CHAR* file_mask = NULL, DirEnumOptions options = DirEnumDefaultOptions);

int DirFileListLoad(std::vector<FileEntry>& data,
	const FILE_PATH_CHAR* dir_path, const FILE_PATH_CHAR* file_mask = NULL, bool recursive = false);

bool DirExistCheck(const FILE_PATH_CHAR* base_path, const FILE_PATH_CHAR* check_path, bool auto_create);

bool DirDelete(const FILE_PATH_CHAR* dir_path);

long long FileCopy(const FILE_PATH_CHAR* src_path, const FILE_PATH_CHAR* dst_path);

bool FileRename(const FILE_PATH_CHAR* src_path, const FILE_PATH_CHAR* dst_path);

bool FileDelete(const FILE_PATH_CHAR* file_path);

bool FileExistCheck(const FILE_PATH_CHAR* path);

} // namespace LisFileSys

#endif // #ifndef _LIS_FILE_SYSTEM_H_
