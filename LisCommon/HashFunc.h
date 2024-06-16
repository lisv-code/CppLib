#pragma once
#ifndef _LIS_HASH_UTILS_H_
#define _LIS_HASH_UTILS_H_

#include <stdint.h>
#include <istream>

// CRC (cyclic redundancy check)

uint16_t hash_crc16(const unsigned char* data, size_t len);

uint32_t hash_crc24(const unsigned char* data, size_t len);

// FNV-1a (Fowler/Noll/Vo)

uint32_t hash_fnv32(const unsigned char* data, size_t len);

#define FNV64_OFFSET 0xCBF29CE484222325ULL

uint64_t hash_fnv64(const unsigned char* data, size_t len, uint64_t offset = FNV64_OFFSET);

long long hash_fnv64(std::istream data, uint64_t& hash);

#endif // #ifndef _LIS_HASH_UTILS_H_
