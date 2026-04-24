// src/cuda_miner.cuh
#ifndef CUDA_MINER_CUH
#define CUDA_MINER_CUH

#include "miner.h"      // Base Miner class + Config + print functions
#include "stratum.h"    // StratumClient for real pool jobs

class CudaMiner : public Miner {
private:
    int numThreads;
    int throttleMs;
    bool testMode;
    
    // Pointer to Stratum client so GPU miner can receive real jobs
    StratumClient* stratum = nullptr;

    std::chrono::steady_clock::time_point startTime;

public:
    // Constructor - accepts StratumClient pointer (default nullptr for test mode)
    CudaMiner(const std::string& walletAddr, const std::string& workerName,
              int threads = 4, 
              int throttle = 10, 
              bool test = true,
              StratumClient* stratumPtr = nullptr);

    void start() override;
    void stop() override;
    void printStatus(const uint8_t* currentHash, const uint8_t* prevHash) override;
};

#endif