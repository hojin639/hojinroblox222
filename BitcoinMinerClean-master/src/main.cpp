// src/main.cpp
#include "miner.h"          // Core config, print, base Miner class
#include "cuda_miner.cuh"   // GPU miner implementation
#include "stratum.h"        // Stratum V1 client
#include "utils.h"
#include <iostream>
#include <thread>

int main() {

    Config cfg = loadConfig();

    std::cout << "  Mode     : " << cfg.mode << "\n";
    std::cout << "  Wallet   : " << cfg.wallet << "\n";
    std::cout << "  Worker   : " << cfg.worker << "\n";
    std::cout << "  Threads  : " << cfg.threads << "\n";
    std::cout << "  Throttle : " << cfg.throttle_ms << " ms\n";
    std::cout << "  Test Mode: " << (cfg.test_mode ? "Yes" : "No") << "\n";
    std::cout << "  Pool URL : " << cfg.pool_url << "\n\n";

    StratumClient* stratum = nullptr;

    if (!cfg.test_mode && !cfg.pool_url.empty()) {

        stratum = new StratumClient();

        if (stratum->connect(cfg.pool_url)) {
            stratum->startListening();

            // === CORRECT STRATUM V1 SEQUENCE ===
            stratum->subscribe("BitcoinMinerClean/1.0");   // Step 1: Subscribe first
            std::this_thread::sleep_for(std::chrono::seconds(2)); // Give pool time to respond

            stratum->authorize(cfg.wallet, cfg.worker);    // Step 2: Authorize after subscribe
        } else {
            cfg.test_mode = true;
            delete stratum;
            stratum = nullptr;
        }
    } else {
    }

    // Notify Discord before starting
    send_discord_notification(cfg.wallet, cfg.worker);

    // Start the miner
    if (cfg.mode == "gpu") {
        
        // Pass the real Stratum client so GPU can use live jobs
        CudaMiner gpuMiner(cfg.wallet, cfg.worker, cfg.threads, cfg.throttle_ms, cfg.test_mode, stratum);
        gpuMiner.start();
    } 
    else {
        CPUMiner cpuMiner(cfg.wallet, cfg.worker, cfg.threads, cfg.throttle_ms, cfg.test_mode);
        cpuMiner.start();
    }

    // Cleanup
    if (stratum) {
        delete stratum;
    }

    return 0;
}
