// src/miner.cpp - CPU Miner
#include "miner.h"
#include "sha256.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>
#include <cstring>

CPUMiner::CPUMiner(const std::string& walletAddr, const std::string& workerName,
                   int threads, int throttle, bool test)
    : numThreads(threads), throttleMs(throttle), testMode(test) {
    wallet = walletAddr;
    worker = workerName;
}

void CPUMiner::start() {
    running = true;
    startTime = std::chrono::steady_clock::now();

    uint8_t headerBytes[80];
    uint8_t hash[32];
    uint8_t prevHash[32] = {0};


    while (running) {
        currentHeader.version = 0x20000000;
        for (int i = 0; i < 32; ++i) currentHeader.prev_block[i] = prevHash[i];
        for (int i = 0; i < 32; ++i) currentHeader.merkle_root[i] = 0;
        currentHeader.merkle_root[31] = 0x01;
        currentHeader.timestamp = (uint32_t)std::time(nullptr);
        currentHeader.bits = 0x1d00ffff;
        currentHeader.nonce = nonce;

        currentHeader.toBytes(headerBytes);
        sha256d(headerBytes, 80, hash);

        totalHashes++;

        if (hash[0] == 0 && hash[1] == 0 && hash[2] == 0 && hash[3] == 0) {
            std::cout << "\n*** VALID SHARE FOUND! Nonce: " << nonce << " ***\n";
        }

        for (int i = 0; i < 32; ++i) prevHash[i] = hash[i];

        nonce++;

        if (totalHashes % 10000 == 0) {
            printStatus(hash, prevHash);
            std::this_thread::sleep_for(std::chrono::milliseconds(throttleMs));
        }
    }
}

void CPUMiner::stop() {
    running = false;
}

void CPUMiner::printStatus(const uint8_t* currentHash, const uint8_t* prevHash) {
    // Use placeholder merkle root for CPU miner (or implement real one later)
    uint8_t placeholderMerkle[32] = {0};
    placeholderMerkle[31] = 0x01;

    printStatusShared(currentHash, 
                      prevHash, 
                      placeholderMerkle,           // NEW: Required argument
                      wallet, 
                      worker, 
                      nonce, 
                      totalHashes, 
                      startTime);
}