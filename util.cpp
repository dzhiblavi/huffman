//
//  author dzhiblavi
//

#include <cstdint>
#include <algorithm>
#include <iostream>

#include "util.hpp"

uint32_t crc32_hash(uint32_t hash, uint8_t c) {
    static CRC32_TABLE crc_table;
    return crc_table[(hash ^ c) & 0xFF] ^ (hash >> 8);
}