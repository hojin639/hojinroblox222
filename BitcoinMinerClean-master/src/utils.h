#pragma once
#include <string>
#include <cstdint>

std::string get_current_timestamp();
std::string bytes_to_hex(const uint8_t* data, size_t len);
void hex_to_bytes(const std::string& hex, uint8_t* out);
uint32_t swap_endian32(uint32_t val);
uint64_t get_current_time_ms();