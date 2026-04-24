// src/stratum.h
#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#endif

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include "miner.h"

class StratumClient {
public:
    StratumClient();
    ~StratumClient();
    std::string getExtranonce1() const { return extranonce1; }
    uint32_t getExtranonce2Size() const { return extranonce2_size; }
    // Inside the public: section of class StratumClient
    void clearNewJobFlag();
    bool connect(const std::string& poolUrl);
    bool authorize(const std::string& wallet, const std::string& worker);
    bool subscribe(const std::string& userAgent = "cheat loader"); // NEW: Added subscribe method for proper Stratum V1 flow
    void startListening();

    StratumJob getCurrentJob();
    bool hasNewJob();
    void submitShare(const std::string& job_id, uint32_t nonce, const uint8_t* hash);

    bool isConnected() const { return connected; }

private:
    void parseMessage(const std::string& msg);
    void sendJson(const std::string& json);
    
    std::string wallet;     // NEW
    std::string worker;     // NEW
    // NEW: Required for real Stratum V1
    std::string extranonce1;
    uint32_t extranonce2_size = 4;   // default 4 bytes

    SOCKET sock = INVALID_SOCKET;
    std::thread listenerThread;
    std::mutex mutex;
    std::atomic<bool> connected{false};
    std::atomic<bool> running{true};

    StratumJob currentJob;
    bool newJobAvailable = false;
    int messageId = 1;
};