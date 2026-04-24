// src/cuda_miner.cu
// ========================================================
// LOCKED PRODUCTION BETA - STABLE & CLEAN
// Uses shared helpers from miner.h only
// No duplicate functions
// ========================================================

#include "cuda_miner.cuh"
#include "sha256.cuh"
#include "sha256.h"
#include "miner.h"
#include <iostream>
#include <chrono>
#include <thread>

// Constructor
CudaMiner::CudaMiner(const std::string& walletAddr, const std::string& workerName,
                     int threads, int throttle, bool test, StratumClient* stratumPtr)
    : numThreads(threads),
      throttleMs(throttle),
      testMode(test),
      stratum(stratumPtr)          // Save the pointer for real jobs
{
    wallet = walletAddr;
    worker = workerName;
}

__global__ void gpu_sha256d_kernel(const uint8_t* header, uint8_t* output) {
    device_sha256d(header, 80, output);
}

void CudaMiner::start() {
    running = true;
    startTime = std::chrono::steady_clock::now();

    uint8_t headerBytes[80];
    uint8_t hash[32];
    uint8_t prevHash[32] = {0};

    uint8_t* d_header = nullptr;
    uint8_t* d_hash = nullptr;
    cudaMalloc(&d_header, 80);
    cudaMalloc(&d_hash, 32);

    int gridSize = 65536;
    uint32_t currentNonce = 0;
    auto lastStatusTime = std::chrono::steady_clock::now();

    std::cout << "🚀 CUDA GPU Miner (RTX 5070) - PRODUCTION BETA\n";
    std::cout << "Grid Size: " << gridSize << " | Throttle: " << throttleMs << "ms\n\n";

    while (running) {
        if (stratum && stratum->hasNewJob() && !testMode) {
            StratumJob job = stratum->getCurrentJob();
            std::cout << "✅ Using real job | ID: " << job.job_id << "\n";

            // Fill real pool data
            if (!job.prev_hash.empty()) hexToBytes(job.prev_hash, currentHeader.prev_block, 32);
            if (!job.version.empty()) currentHeader.version = std::stoul(job.version, nullptr, 16);
            if (!job.nbits.empty()) currentHeader.bits = std::stoul(job.nbits, nullptr, 16);
            if (!job.ntime.empty()) currentHeader.timestamp = std::stoul(job.ntime, nullptr, 16);

            calculateMerkleRoot(job, currentHeader.merkle_root);

            currentNonce = 0;
            stratum->clearNewJobFlag();
        }

        currentHeader.nonce = currentNonce;
        currentHeader.toBytes(headerBytes);

        cudaMemcpy(d_header, headerBytes, 80, cudaMemcpyHostToDevice);
        gpu_sha256d_kernel<<<gridSize, 256>>>(d_header, d_hash);
        cudaDeviceSynchronize();
        cudaMemcpy(hash, d_hash, 32, cudaMemcpyDeviceToHost);

        totalHashes += (uint64_t)gridSize * 256;

        // Status every ~4 seconds
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<double>(now - lastStatusTime).count() >= 4.0) {
            printStatusShared(hash, prevHash, currentHeader.merkle_root,
                              wallet, worker, currentNonce, totalHashes, startTime);
            lastStatusTime = now;
        }

        // Share detection (real difficulty is higher, so this may take time)
        if (hash[31] == 0 && hash[30] == 0) {
            std::cout << "\n*** GPU VALID SHARE FOUND! ***\n"
                      << "Nonce : " << currentNonce << "\n"
                      << "Hash  : " << hash_to_hex(hash) << "\n\n";

            if (stratum && !testMode) {
                stratum->submitShare("current_job", currentNonce, hash);
            }
        }

        for (int i = 0; i < 32; ++i) prevHash[i] = hash[i];
        currentNonce += (uint32_t)gridSize * 256;
        nonce = currentNonce;

        if (throttleMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(throttleMs));
        }
    }

    cudaFree(d_header);
    cudaFree(d_hash);
}

void CudaMiner::stop() {
    running = false;
}
// Updated to include Merkle Root in status output (for GUI + better visibility)
void CudaMiner::printStatus(const uint8_t* currentHash, const uint8_t* prevHash) {
    // Pass current merkle root from the active header
    printStatusShared(currentHash, 
                      prevHash, 
                      currentHeader.merkle_root,   // NEW: Merkle Root passed through
                      wallet, 
                      worker, 
                      nonce, 
                      totalHashes, 
                      startTime);
}