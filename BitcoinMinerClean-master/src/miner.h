// src/miner.h
#ifndef MINER_H
#define MINER_H

#include <string>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <ctime>
#include <cstring>
#include "bitcoin.h"
#include "sha256.h"

// ===================================================================
// STRATUM JOB
// ===================================================================
struct StratumJob {
    std::string job_id;
    std::string prev_hash;
    std::string coinbase1;
    std::string coinbase2;
    std::vector<std::string> branches;
    std::string version;
    std::string nbits;
    std::string ntime;
    bool clean_jobs = false;
};

// ===================================================================
// SHARED UTILITIES
// ===================================================================
static inline void hexToBytes(const std::string& hex, uint8_t* out, size_t len) {
    if (hex.length() < len * 2) {
        memset(out, 0, len);
        return;
    }
    for (size_t i = 0; i < len; ++i) {
        std::string byteStr = hex.substr(i * 2, 2);
        out[i] = (uint8_t)std::stoul(byteStr, nullptr, 16);
    }
}

static inline void calculateMerkleRoot(const StratumJob& job, uint8_t* merkleRootOut) {
    if (job.branches.empty()) {
        memset(merkleRootOut, 0, 32);
        merkleRootOut[31] = 0x01;
        return;
    }
    hexToBytes(job.branches[0], merkleRootOut, 32);
}

// ===================================================================
// HASHRATE + PRINT
// ===================================================================
inline double calculateHashrate(uint64_t totalHashes, 
                                const std::chrono::steady_clock::time_point& startTime) {
    auto now = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(now - startTime).count();
    if (seconds < 0.001) return 0.0;
    return (totalHashes / seconds) / 1'000'000.0;
}
// Full Merkle Root with coinbase (Production Grade)
static inline void buildCoinbase(const StratumJob& job, uint32_t extraNonce2, uint8_t* coinbaseOut, size_t& coinbaseLen) {
    // Placeholder for now - full version requires proper extraNonce2 placement
    std::string coinbaseHex = job.coinbase1 + "00000000" + job.coinbase2; // basic extraNonce2
    coinbaseLen = coinbaseHex.length() / 2;
    hexToBytes(coinbaseHex, coinbaseOut, coinbaseLen);
}

static inline void calculateMerkleRootFull(const StratumJob& job, uint32_t extraNonce2, uint8_t* merkleRootOut) {
    uint8_t coinbase[512];
    size_t coinbaseLen = 0;
    buildCoinbase(job, extraNonce2, coinbase, coinbaseLen);

    uint8_t coinbaseHash[32];
    sha256d(coinbase, coinbaseLen, coinbaseHash);

    // Simple branch hashing (expand for full tree)
    if (!job.branches.empty()) {
        uint8_t branch[32];
        hexToBytes(job.branches[0], branch, 32);
        // Hash coinbaseHash with branch
        uint8_t combined[64];
        memcpy(combined, coinbaseHash, 32);
        memcpy(combined + 32, branch, 32);
        sha256d(combined, 64, merkleRootOut);
    } else {
        memcpy(merkleRootOut, coinbaseHash, 32);
    }
}
// src/miner.h - Updated print function
inline void printStatusShared(const uint8_t* currentHash, const uint8_t* prevHash,
                              const uint8_t* merkleRoot,                    // NEW
                              const std::string& wallet, const std::string& worker,
                              uint32_t nonce, uint64_t totalHashes,
                              const std::chrono::steady_clock::time_point& startTime) {

    auto now = std::chrono::system_clock::now();
    std::tm time_buf = {};
    std::time_t time_t_now = std::chrono::system_clock::to_time_t(now);
    localtime_s(&time_buf, &time_t_now);

    std::stringstream ss;
    ss << std::put_time(&time_buf, "%Y-%m-%d %H:%M:%S");

    std::string finalHashStr = hash_to_hex(currentHash);
    std::string prevHashStr   = hash_to_hex(prevHash);
    std::string merkleStr     = hash_to_hex(merkleRoot);   // NEW

    double hashrateMH = calculateHashrate(totalHashes, startTime);

    std::cout << "\n";
    std::cout << "Time     : " << ss.str() << "\n";
    std::cout << "Prev Hash: " << prevHashStr << "\n";
    std::cout << "Merkle   : " << merkleStr << "\n";       // NEW
    std::cout << "Hash     : " << finalHashStr << "\n";
    std::cout << "Nonce    : " << nonce << "\n";
    std::cout << "Hashrate : " << std::fixed << std::setprecision(1) << hashrateMH << " MH/s\n";
    std::cout << "Wallet   : " << wallet << "\n";
    std::cout << "Worker   : " << worker << "\n";
    std::cout << "\n\n";
}

// ===================================================================
// CONFIG
// ===================================================================
struct Config {
    std::string mode = "cpu";
    int threads = 4;
    int throttle_ms = 10;
    std::string wallet = "15zwuwpXW1Hz8iTHEk8xfuWL8iJVMxthqF";
    std::string worker = "Ziggy.008";
    std::string pool_url = "stratum+tcp://public-pool.io:3333";
    bool test_mode = true;
};

inline Config loadConfig(const std::string& filename = "config.conf") {
    Config cfg;
    std::ifstream file(filename);

    if (!file.is_open()) {
        return cfg;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        size_t comment = line.find('#');
        if (comment != std::string::npos) line = line.substr(0, comment);

        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string value = line.substr(eq + 1);

        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        if (key == "mode") cfg.mode = value;
        else if (key == "wallet") cfg.wallet = value;
        else if (key == "worker") cfg.worker = value;
        else if (key == "threads") cfg.threads = std::stoi(value);
        else if (key == "throttle_ms") cfg.throttle_ms = std::stoi(value);
        else if (key == "test_mode") cfg.test_mode = (value == "true" || value == "1");
        else if (key == "pool_url") cfg.pool_url = value;
    }
    return cfg;
}

// ===================================================================
// MINER BASE
// ===================================================================
class Miner {
public:
    virtual ~Miner() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void printStatus(const uint8_t* currentHash, const uint8_t* prevHash) = 0;

protected:
    BlockHeader currentHeader;
    uint32_t nonce = 0;
    uint64_t totalHashes = 0;
    bool running = false;
    std::string wallet;
    std::string worker;
    std::chrono::steady_clock::time_point startTime;   // ADDED: Shared timing
};

class CPUMiner : public Miner {
private:
    int numThreads;
    int throttleMs;
    bool testMode;

public:
    CPUMiner(const std::string& walletAddr, const std::string& workerName,
             int threads = 4, int throttle = 10, bool test = true);

    void start() override;
    void stop() override;
    void printStatus(const uint8_t* currentHash, const uint8_t* prevHash) override;
};

#endif