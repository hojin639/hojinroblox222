// src/stratum.cpp
#include "stratum.h"
#include <iostream>
#include <sstream>
#include <WS2tcpip.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

StratumClient::StratumClient() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);
}

StratumClient::~StratumClient() {
    running = false;
    if (sock != INVALID_SOCKET) closesocket(sock);
    WSACleanup();
}

// CHANGED: Full implementation with proper return
bool StratumClient::connect(const std::string& poolUrl) {
    std::string host = poolUrl;
    int port = 3333;

    size_t pos = host.find("://");
    if (pos != std::string::npos) host = host.substr(pos + 3);

    pos = host.find(':');
    if (pos != std::string::npos) {
        port = std::stoi(host.substr(pos + 1));
        host = host.substr(0, pos);
    }

    struct addrinfo* result = nullptr;
    struct addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &result) != 0) {
        return false;
    }

    sock = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sock == INVALID_SOCKET) {
        freeaddrinfo(result);
        return false;
    }

    if (::connect(sock, result->ai_addr, (int)result->ai_addrlen) == SOCKET_ERROR) {
        closesocket(sock);
        sock = INVALID_SOCKET;
        freeaddrinfo(result);
        return false;
    }

    freeaddrinfo(result);
    connected = true;
    return true;
}

bool StratumClient::subscribe(const std::string& userAgent) {
    std::stringstream ss;
    ss << "{\"id\":" << messageId++ 
       << ",\"method\":\"mining.subscribe\""
       << ",\"params\":[\"" << userAgent << "\"]}\n";

    sendJson(ss.str());
    return true;
}

bool StratumClient::authorize(const std::string& walletAddr, const std::string& workerName) {
    wallet = walletAddr;
    worker = workerName;

    std::stringstream ss;
    ss << "{\"id\":" << messageId++ 
       << ",\"method\":\"mining.authorize\",\"params\":[\"" 
       << walletAddr << "." << workerName << "\",\"x\"]}\n";

    sendJson(ss.str());
    return true;
}

void StratumClient::startListening() {
    listenerThread = std::thread([this]() {
        std::string recvBuf;           // Persistent buffer
        char buffer[4096];

        while (running && connected) {
            int bytes = recv(sock, buffer, sizeof(buffer), 0);
            if (bytes <= 0) {
                connected = false;
                break;
            }

            recvBuf.append(buffer, bytes);

            // Split on newline - each line is one complete JSON message
            size_t pos;
            while ((pos = recvBuf.find('\n')) != std::string::npos) {
                std::string line = recvBuf.substr(0, pos);
                recvBuf.erase(0, pos + 1);
                if (!line.empty()) {
                    parseMessage(line);
                }
            }
        }
    });
}

// src/stratum.cpp - Updated with better job parsing
// ... keep your existing constructor, destructor, connect, authorize, etc.

void StratumClient::parseMessage(const std::string& msg) {

    std::lock_guard<std::mutex> lock(mutex);

    // === 1. Subscribe response (contains "result":[[...],"extranonce1",size]) ===
    if (msg.find("\"result\":[") != std::string::npos && msg.find("mining.notify") != std::string::npos) {

        // Extract extranonce1 (the string after the first ]] 
        size_t firstClose = msg.find("]]");
        if (firstClose != std::string::npos) {
            size_t quote1 = msg.find('"', firstClose + 2);
            size_t quote2 = msg.find('"', quote1 + 1);
            if (quote1 != std::string::npos && quote2 != std::string::npos) {
                extranonce1 = msg.substr(quote1 + 1, quote2 - quote1 - 1);
            }

            // Extract extranonce2_size (number after the next comma)
            size_t comma = msg.find(',', quote2 + 1);
            if (comma != std::string::npos) {
                extranonce2_size = std::stoul(msg.substr(comma + 1));
            }
        }
    }

    // === 2. mining.notify (real jobs) ===
    if (msg.find("\"method\":\"mining.notify\"") != std::string::npos) {
        newJobAvailable = true;

        size_t paramsStart = msg.find("\"params\":[");
        if (paramsStart != std::string::npos) {
            size_t idStart = msg.find('"', paramsStart + 10);
            size_t idEnd = msg.find('"', idStart + 1);
            if (idStart != std::string::npos && idEnd != std::string::npos) {
                currentJob.job_id = msg.substr(idStart + 1, idEnd - idStart - 1);
                std::cout << "   → Job ID: " << currentJob.job_id << "\n";
            }
        }
    }

    // === 3. Difficulty update ===
    if (msg.find("\"method\":\"mining.set_difficulty\"") != std::string::npos) {
    }
}

// src/stratum.cpp - Enhanced logging for GUI
void StratumClient::sendJson(const std::string& json) {
    if (sock != INVALID_SOCKET) {
        ::send(sock, json.c_str(), (int)json.length(), 0);
    }
}

StratumJob StratumClient::getCurrentJob() {
    std::lock_guard<std::mutex> lock(mutex);
    newJobAvailable = false;
    return currentJob;
}

bool StratumClient::hasNewJob() {
    return newJobAvailable;
}

void StratumClient::submitShare(const std::string& job_id, uint32_t nonce, const uint8_t* hash) {
    std::stringstream ss;
    ss << "{\"id\":" << messageId++ 
       << ",\"method\":\"mining.submit\""
       << ",\"params\":[\"" << wallet << "." << worker 
       << "\",\"" << job_id 
       << "\"," << nonce 
       << ",\"" << hash_to_hex(hash) << "\"]}\n";

    std::string submitJson = ss.str();


    // REAL SUBMISSION - ENABLED
    sendJson(submitJson);
}
void StratumClient::clearNewJobFlag() {
    std::lock_guard<std::mutex> lock(mutex);
    newJobAvailable = false;
}