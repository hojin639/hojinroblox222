cmake_minimum_required(VERSION 3.18)
project(BitcoinMinerClean LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CUDA_STANDARD 17)

set(CMAKE_CUDA_ARCHITECTURES 75 80 86 89 90 120)

find_package(CUDAToolkit REQUIRED)

add_executable(BitcoinMinerClean
    src/main.cpp
    src/miner.cpp
    src/bitcoin.cpp
    src/utils.cpp
    src/cuda_miner.cu
    src/stratum.cpp          # ADDED
)

target_include_directories(BitcoinMinerClean PRIVATE src)
target_link_libraries(BitcoinMinerClean PRIVATE CUDA::cudart ws2_32)   # ADDED: ws2_32 for Windows sockets

set_target_properties(BitcoinMinerClean PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
)

message(STATUS "BitcoinMinerClean configured for RTX 5070")