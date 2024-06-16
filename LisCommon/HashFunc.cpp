#include "HashFunc.h"

// CRC16 - simplified CCITT (poly 0x1021)
uint16_t hash_crc16(const unsigned char* data, size_t len)
{
	uint16_t result = 0xFFFF;
	uint16_t x;
	while (len--) {
		x = result >> 8 ^ *data++;
		x ^= x >> 4;
		result = (result << 8) ^ (x << 12) ^ (x << 5) ^ x;
	}
	return result;
}

#define CRC24_INIT 0xB704CEUL
// Standard (CRC24A) CRC Polynomial function
#define CRC24_POLY 0x1864CFBUL

uint32_t hash_crc24(const unsigned char* data, size_t len)
{
	uint32_t result = CRC24_INIT;
	while (len--) {
		result ^= ((uint32_t)(*data++)) << 16;
		for (int i = 0; i < 8; ++i) {
			result <<= 1;
			if (result & 0x1000000UL)
				result ^= CRC24_POLY;
		}
	}
	return result & 0xFFFFFFUL;
}

#define FNV32_OFFSET 0x811C9DC5UL
#define FNV32_PRIME 0x01000193UL

uint32_t hash_fnv32(const unsigned char* data, size_t len)
{
	uint32_t result = FNV32_OFFSET;
	for (size_t i = 0; i < len; ++i) {
		result ^= (uint32_t)(data[i]);
		result *= FNV32_PRIME;
	}
	return result;
}

#define FNV64_PRIME 0x00000100000001B3ULL

uint64_t hash_fnv64(const unsigned char* data, size_t len, uint64_t offset)
{
	uint64_t result = offset;
	for (size_t i = 0; i < len; ++i) {
		result ^= (uint64_t)(data[i]);
		result *= FNV64_PRIME;
	}
	return result;
}

long long hash_fnv64(std::istream data, uint64_t& hash)
{
	unsigned char buf[0xFF];
	hash = FNV32_OFFSET;
	long long result = 0;
	while (data.good()) {
		data.read((char*)buf, sizeof(buf));
		auto bytes_read = data.gcount();
		if (0 == bytes_read && !data.eof()) { result = -1; break; } // ERROR: read
		hash = hash_fnv64(buf, bytes_read, hash);
		result += bytes_read;
	}
	return result;
}
