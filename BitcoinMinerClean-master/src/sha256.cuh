#ifndef SHA256_CUH
#define SHA256_CUH

#include <cuda_runtime.h>
#include <stdint.h>

////////////////////////////////////////////////////////////////////////////////
// Constants and macros
////////////////////////////////////////////////////////////////////////////////

__constant__ uint32_t k_sha256_device[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

#define ROTR(x, n)  (((x) >> (n)) | ((x) << (32 - (n))))

#define Ch(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define Maj(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define Sigma0(x)    (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define Sigma1(x)    (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define sigma0(x)    (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define sigma1(x)    (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

////////////////////////////////////////////////////////////////////////////////
// Core transform
////////////////////////////////////////////////////////////////////////////////

__device__ __forceinline__
void device_sha256_transform(uint32_t state[8], const uint32_t block[16]) {
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    uint32_t f = state[5];
    uint32_t g = state[6];
    uint32_t h = state[7];

    uint32_t w[64];

#pragma unroll
    for (int i = 0; i < 16; ++i) {
        w[i] = block[i];
    }

#pragma unroll
    for (int i = 16; i < 64; ++i) {
        w[i] = sigma1(w[i - 2]) + w[i - 7] + sigma0(w[i - 15]) + w[i - 16];
    }

#pragma unroll
    for (int i = 0; i < 64; ++i) {
        uint32_t t1 = h + Sigma1(e) + Ch(e, f, g) + k_sha256_device[i] + w[i];
        uint32_t t2 = Sigma0(a) + Maj(a, b, c);

        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

////////////////////////////////////////////////////////////////////////////////
// Single SHA-256
////////////////////////////////////////////////////////////////////////////////

__device__ __forceinline__
void device_sha256(const uint8_t* __restrict__ input,
    uint32_t len,
    uint8_t* __restrict__ output) {
    // Initial state (OpenSSL / FIPS 180-4)
    uint32_t state[8] = {
        0x6a09e667U, 0xbb67ae85U, 0x3c6ef372U, 0xa54ff53aU,
        0x510e527fU, 0x9b05688cU, 0x1f83d9abU, 0x5be0cd19U
    };

    uint32_t block[16];
    uint32_t i = 0;
    uint32_t remaining = len;

    // Process full 64-byte blocks
    while (remaining >= 64) {
#pragma unroll
        for (int j = 0; j < 16; ++j) {
            uint32_t idx = i + j * 4;
            block[j] =
                ((uint32_t)input[idx + 0] << 24) |
                ((uint32_t)input[idx + 1] << 16) |
                ((uint32_t)input[idx + 2] << 8) |
                ((uint32_t)input[idx + 3]);
        }
        device_sha256_transform(state, block);
        i += 64;
        remaining -= 64;
    }

    // Padding
    uint8_t pad[128];  // enough for up to two blocks
#pragma unroll
    for (int j = 0; j < 128; ++j) pad[j] = 0;

#pragma unroll
    for (uint32_t j = 0; j < remaining; ++j) {
        pad[j] = input[i + j];
    }
    pad[remaining] = 0x80;

    uint64_t bitlen = (uint64_t)len * 8ULL;

    if (remaining + 1 <= 56) {
        // length fits in this block
        pad[56] = (uint8_t)((bitlen >> 56) & 0xFF);
        pad[57] = (uint8_t)((bitlen >> 48) & 0xFF);
        pad[58] = (uint8_t)((bitlen >> 40) & 0xFF);
        pad[59] = (uint8_t)((bitlen >> 32) & 0xFF);
        pad[60] = (uint8_t)((bitlen >> 24) & 0xFF);
        pad[61] = (uint8_t)((bitlen >> 16) & 0xFF);
        pad[62] = (uint8_t)((bitlen >> 8) & 0xFF);
        pad[63] = (uint8_t)(bitlen & 0xFF);

#pragma unroll
        for (int j = 0; j < 16; ++j) {
            uint32_t idx = j * 4;
            block[j] =
                ((uint32_t)pad[idx + 0] << 24) |
                ((uint32_t)pad[idx + 1] << 16) |
                ((uint32_t)pad[idx + 2] << 8) |
                ((uint32_t)pad[idx + 3]);
        }
        device_sha256_transform(state, block);
    }
    else {
        // need two blocks
        // first block: current pad[0..63]
#pragma unroll
        for (int j = 0; j < 16; ++j) {
            uint32_t idx = j * 4;
            block[j] =
                ((uint32_t)pad[idx + 0] << 24) |
                ((uint32_t)pad[idx + 1] << 16) |
                ((uint32_t)pad[idx + 2] << 8) |
                ((uint32_t)pad[idx + 3]);
        }
        device_sha256_transform(state, block);

        // second block: all zeros except length at the end
#pragma unroll
        for (int j = 0; j < 64; ++j) pad[j] = 0;

        pad[56] = (uint8_t)((bitlen >> 56) & 0xFF);
        pad[57] = (uint8_t)((bitlen >> 48) & 0xFF);
        pad[58] = (uint8_t)((bitlen >> 40) & 0xFF);
        pad[59] = (uint8_t)((bitlen >> 32) & 0xFF);
        pad[60] = (uint8_t)((bitlen >> 24) & 0xFF);
        pad[61] = (uint8_t)((bitlen >> 16) & 0xFF);
        pad[62] = (uint8_t)((bitlen >> 8) & 0xFF);
        pad[63] = (uint8_t)(bitlen & 0xFF);

#pragma unroll
        for (int j = 0; j < 16; ++j) {
            uint32_t idx = j * 4;
            block[j] =
                ((uint32_t)pad[idx + 0] << 24) |
                ((uint32_t)pad[idx + 1] << 16) |
                ((uint32_t)pad[idx + 2] << 8) |
                ((uint32_t)pad[idx + 3]);
        }
        device_sha256_transform(state, block);
    }

    // Output: big-endian, OpenSSL-compatible
#pragma unroll
    for (int j = 0; j < 8; ++j) {
        uint32_t v = state[j];
        output[j * 4 + 0] = (uint8_t)((v >> 24) & 0xFF);
        output[j * 4 + 1] = (uint8_t)((v >> 16) & 0xFF);
        output[j * 4 + 2] = (uint8_t)((v >> 8) & 0xFF);
        output[j * 4 + 3] = (uint8_t)(v & 0xFF);
    }
}

////////////////////////////////////////////////////////////////////////////////
// Double SHA-256 (Bitcoin-style internal)
////////////////////////////////////////////////////////////////////////////////

__device__ __forceinline__
void device_sha256d(const uint8_t* __restrict__ input,
    uint32_t len,
    uint8_t* __restrict__ output) {
    uint8_t mid[32];
    device_sha256(input, len, mid);
    device_sha256(mid, 32, output);
}
#endif // SHA256_CUH