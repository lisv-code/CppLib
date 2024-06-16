// ****** File system utilities. (c) 2024 LISV ******
#include "FileSystem.h"

#ifdef _WINDOWS

#include <windows.h>
#include <direct.h>

#else

#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>

#endif

#include <sys/stat.h>
#include <fstream>

#include "StrUtils.h"

#define FILE_DATA_BUFFER_SIZE 0x1000 // 4k

using namespace LisFileSys;
const DirEnumOptions LisFileSys::DirEnumDefaultOptions = deoFiles;

static bool FileSystem_IsSysDirFileName(const FILE_PATH_CHAR* file_name);

#ifdef _WINDOWS
// ************************************* Windows related code **************************************

static time_t FileSystem_FileTimeToTime(FILETIME file_time)
{
	int64_t tmp = ((int64_t)file_time.dwHighDateTime << 32) + file_time.dwLowDateTime;
	return (tmp - 116444736000000000LL) / 10000000LL;
}

static int FileSystem_DirEnumItemProc(DirEnumItemProc item_proc,
	bool is_dir, const FILE_PATH_CHAR* file_path, const WIN32_FIND_DATA* file_info)
{
	FileEntry file_item;
	file_item.Path = file_path;
	file_item.IsDir = is_dir;
	file_item.Info.Changed = FileSystem_FileTimeToTime(file_info->ftLastWriteTime);
	file_item.Info.Size = ((uint64_t)file_info->nFileSizeHigh << 32) + file_info->nFileSizeLow;
	return item_proc(file_item) ? 0 : -1; // ERROR: interrupted
}

int LisFileSys::DirEnum(DirEnumItemProc item_proc, const FILE_PATH_CHAR* dir_path,
	const FILE_PATH_CHAR* file_mask, DirEnumOptions options)
{
	int result = 0;
	WIN32_FIND_DATA wfd_file;
	std::basic_string<FILE_PATH_CHAR> search_path = dir_path;
	if (search_path.back() != (FILE_PATH_CHAR)FILE_PATH_SEPARATOR_CHR)
		search_path += FILE_PATH_SEPARATOR_CHR;

	HANDLE ff_handle = FindFirstFile(
		(search_path + (file_mask ? file_mask : FILE_PATH_TEXT("*"))).c_str(), &wfd_file);
	if (INVALID_HANDLE_VALUE != ff_handle) {
		std::basic_string<FILE_PATH_CHAR> file_path;
		do {
			file_path = search_path + wfd_file.cFileName;
			bool is_dir = FILE_ATTRIBUTE_DIRECTORY & wfd_file.dwFileAttributes;
			if (is_dir) {
				if (!FileSystem_IsSysDirFileName(wfd_file.cFileName)) {
					if (deoDirFirst & options)
						result = FileSystem_DirEnumItemProc(item_proc, is_dir, file_path.c_str(), &wfd_file);
					if (result >= 0 && (deoRecursive & options)) // Recursive search in subdirectory
						result = DirEnum(item_proc, file_path.c_str(), file_mask, options);
					if (result >= 0 && (deoDirLast & options))
						result = FileSystem_DirEnumItemProc(item_proc, is_dir, file_path.c_str(), &wfd_file);
				}
			} else if (deoFiles & options) {
				result = FileSystem_DirEnumItemProc(item_proc, is_dir, file_path.c_str(), &wfd_file);
			}
		} while (result >= 0 && FindNextFile(ff_handle, &wfd_file));
		FindClose(ff_handle);
	}
	return result;
}

bool LisFileSys::DirDelete(const FILE_PATH_CHAR* dir_path)
{
	return RemoveDirectory(dir_path);
}

static bool FileSystem_DirExistenceCheck(const FILE_PATH_CHAR* dir_path, bool auto_create)
{
	DWORD attr = GetFileAttributes(dir_path);
	if ((attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY)) return true;
	if (auto_create) return CreateDirectory(dir_path, NULL);
	return false;
}

bool LisFileSys::FileRename(const FILE_PATH_CHAR* src_path, const FILE_PATH_CHAR* dst_path)
{
	return MoveFile(src_path, dst_path);
}

bool LisFileSys::FileDelete(const FILE_PATH_CHAR* file_path)
{
	return DeleteFile(file_path);
}

bool LisFileSys::FileExistCheck(const FILE_PATH_CHAR* path)
{
	return GetFileAttributes(path) != INVALID_FILE_ATTRIBUTES;
}

#else
// *************************************** Non-Windows code ****************************************

static bool FileSystem_IsFileNameMatches(const char* file_name, const char* file_mask)
{
	if (nullptr == file_mask) return true;
	if (nullptr == file_name) return false;

	size_t name_len = strlen(file_name);
	if (0 == name_len) return false;

	size_t mask_len = strlen(file_mask);
	if (0 == mask_len) return true;

	if ('*' == file_mask[0] && mask_len < name_len) {
		// Name ending matching is supposed
		return 0 == strncmp(&file_name[name_len - mask_len + 1], &file_mask[1], mask_len - 1);
	}

	// TODO: more file-mask cases to be handled...

	return false;
}

static int FileSystem_DirEnumItemProc(DirEnumItemProc item_proc,
	bool is_dir, const FILE_PATH_CHAR* file_path)
{
	FileEntry file_item;
	file_item.Path = file_path;
	file_item.IsDir = is_dir;
	file_item.Info = {}; // TODO: get the file info (use stat)
	return item_proc(file_item) ? 0 : -1; // ERROR: interrupted
}

int LisFileSys::DirEnum(DirEnumItemProc item_proc, const FILE_PATH_CHAR* dir_path,
	const FILE_PATH_CHAR* file_mask, DirEnumOptions options)
{
	int result = 0;
	DIR* dir0 = opendir(dir_path);
	if (dir0 != nullptr) {
		struct dirent* dir_ent;
		while (result >= 0 && NULL != (dir_ent = readdir(dir0))) {
			if (DT_UNKNOWN == dir_ent->d_type) {
				struct stat ent_stat;
				int stat_res = stat(
					LisStr::CStrConcat(dir_path, FILE_PATH_SEPARATOR_STR, dir_ent->d_name, 0),
					&ent_stat);
				if (0 == stat_res) {
					if (S_ISREG(ent_stat.st_mode)) dir_ent->d_type = DT_REG;
					else if (S_ISDIR(ent_stat.st_mode)) dir_ent->d_type = DT_DIR;
				}
			}

			bool is_dir = DT_DIR == dir_ent->d_type;
			if (!is_dir && (DT_REG != dir_ent->d_type)) continue; // Skip this entry: it's not a regular file and not a dir
			if (is_dir && FileSystem_IsSysDirFileName(dir_ent->d_name)) continue;

			char file_path[PATH_MAX];
			LisStr::StrConcat(file_path, dir_path, dir_ent->d_name, 0);
			if (is_dir && (deoRecursive & options)) {
				if (deoDirFirst & options)
					result = FileSystem_DirEnumItemProc(item_proc, is_dir, file_path);
				if (result >= 0 && (deoRecursive & options)) // Recursive search in subdirectory
					result = DirEnum(item_proc,
						LisStr::CStrConcat(file_path, FILE_PATH_SEPARATOR_STR, 0),
						file_mask, options);
				if (result >= 0 && (deoDirLast & options))
					result = FileSystem_DirEnumItemProc(item_proc, is_dir, file_path);
			} else if ((deoFiles & options) && FileSystem_IsFileNameMatches(dir_ent->d_name, file_mask)) {
				result = FileSystem_DirEnumItemProc(item_proc, is_dir, file_path);
			}
		}
		closedir(dir0);
	}
	return result;
}

static bool FileSystem_DirExistenceCheck(const char* dir_path, bool auto_create)
{
	struct stat info;
	int result = stat(dir_path, &info);
	if (auto_create && result != 0) result = mkdir(dir_path, S_IRWXU | S_IRGRP |  S_IXGRP | S_IROTH | S_IXOTH);
	else if (info.st_mode & S_IFDIR) result = 0; // OK
	else result = -1; // ERROR: looks like it's not a folder
	return 0 == result;
}

bool LisFileSys::DirDelete(const FILE_PATH_CHAR* dir_path)
{
	return 0 == remove(dir_path);
}

bool LisFileSys::FileRename(const FILE_PATH_CHAR* src_path, const FILE_PATH_CHAR* dst_path)
{
	return 0 == std::rename(src_path, dst_path);
}

bool LisFileSys::FileDelete(const FILE_PATH_CHAR* file_path)
{
	return 0 == remove(file_path);
}

bool LisFileSys::FileExistCheck(const FILE_PATH_CHAR* path)
{
	struct stat info;
	return (stat(path, &info) == 0);
}

#endif

// ************************************** Cross-platform code **************************************

static bool FileSystem_IsSysDirFileName(const FILE_PATH_CHAR* file_name)
{
	// Check for "." or ".."  (sys dir name)
	return (FILE_PATH_CHAR)'.' == file_name[0]
		&& (0 == file_name[1] || ((FILE_PATH_CHAR)'.' == file_name[1] && 0 == file_name[2]));
}

int LisFileSys::DirFileListLoad(std::vector<FileEntry>& data,
	const FILE_PATH_CHAR* dir_path, const FILE_PATH_CHAR* file_mask, bool recursive)
{
	DirEnumOptions options = (DirEnumOptions)(deoFiles | (recursive ? deoRecursive : 0));
	return DirEnum([&data](const FileEntry& file) { data.push_back(file); return true; },
		dir_path, file_mask, options);
}

bool LisFileSys::DirExistCheck(const FILE_PATH_CHAR* base_path, const FILE_PATH_CHAR* check_path, bool auto_create)
{
	FILE_PATH_CHAR path[PATH_MAX + 1];
	int pos1 = 0;
	if (base_path) {
		pos1 = LisStr::StrLen(base_path);
		LisStr::StrNCpy(path, base_path, pos1);
		if ((pos1 > 0) && (path[pos1 - 1] != FILE_PATH_SEPARATOR_CHR)) {
			path[pos1] = FILE_PATH_SEPARATOR_CHR;
			++pos1;
		}
		path[pos1] = 0;
	}
	int pos2 = 0;
	const FILE_PATH_CHAR* posX = LisStr::StrChr(check_path, FILE_PATH_SEPARATOR_CHR);
	while (posX && *(posX + 1)) {
		int len = posX - check_path - pos2 + 1;
		LisStr::StrNCpy(path + pos1, check_path + pos2, len);
		path[pos1 + len] = 0;
		if (!FileSystem_DirExistenceCheck(path, auto_create)) return false;
		pos1 += len;
		pos2 += len;
		posX = LisStr::StrChr(posX + 1, FILE_PATH_SEPARATOR_CHR);
	}
	LisStr::StrCpy(path + pos1, check_path + pos2);
	return FileSystem_DirExistenceCheck(path, auto_create);
}

long long LisFileSys::FileCopy(const FILE_PATH_CHAR* src_path, const FILE_PATH_CHAR* dst_path)
{
	char buf[FILE_DATA_BUFFER_SIZE];
	std::ifstream src_stm(src_path, std::ifstream::in | std::ios::binary);
	std::ofstream dst_stm(dst_path, std::ifstream::out | std::ios::binary);
	long long result = 0;
	while (src_stm.good()) {
		src_stm.read(buf, sizeof(buf));
		auto bytes_loaded = src_stm.gcount();
		if (0 == bytes_loaded && !src_stm.eof()) { result = -1; break; } // ERROR: read
		dst_stm.write(buf, bytes_loaded);
		if (dst_stm.fail()) { result = -2; break; } // ERROR: write
		result += bytes_loaded;
	}
	src_stm.close();
	dst_stm.close();
	return result;
}
