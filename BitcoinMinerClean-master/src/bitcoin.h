// bitcoin.h - Block Header Definition
#ifndef BITCOIN_H
#define BITCOIN_H

#include <cstdint>
#include <cstring>

struct BlockHeader {
    uint32_t version;
    uint8_t  prev_block[32];
    uint8_t  merkle_root[32];
    uint32_t timestamp;
    uint32_t bits;
    uint32_t nonce;

    // Helper to convert to 80-byte raw header
    void toBytes(uint8_t* out) const {
        uint8_t* p = out;

        // version (little-endian)
        p[0] = (version >> 0) & 0xFF;
        p[1] = (version >> 8) & 0xFF;
        p[2] = (version >> 16) & 0xFF;
        p[3] = (version >> 24) & 0xFF;
        p += 4;

        // prev_block (already in little-endian byte order)
        memcpy(p, prev_block, 32);
        p += 32;

        // merkle_root
        memcpy(p, merkle_root, 32);
        p += 32;

        // timestamp (little-endian)
        p[0] = (timestamp >> 0) & 0xFF;
        p[1] = (timestamp >> 8) & 0xFF;
        p[2] = (timestamp >> 16) & 0xFF;
        p[3] = (timestamp >> 24) & 0xFF;
        p += 4;

        // bits (little-endian)
        p[0] = (bits >> 0) & 0xFF;
        p[1] = (bits >> 8) & 0xFF;
        p[2] = (bits >> 16) & 0xFF;
        p[3] = (bits >> 24) & 0xFF;
        p += 4;

        // nonce (little-endian)
        p[0] = (nonce >> 0) & 0xFF;
        p[1] = (nonce >> 8) & 0xFF;
        p[2] = (nonce >> 16) & 0xFF;
        p[3] = (nonce >> 24) & 0xFF;
    }
};

#endif