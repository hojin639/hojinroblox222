# BitcoinMinerClean

A lightweight, throttled Bitcoin miner in C++/CUDA focused on **"Lottery Mode"** — running continuously in the background with minimal system resource usage.

## Project Goals

- Full CPU and GPU (CUDA) mining capability
- "Lottery Mode": Very low resource usage with configurable throttling
- Clean modular architecture
- Cross-platform support (Windows + Linux) via CMake
- Structured, machine-parseable status output for GUI integration
- Easy configuration via `config.conf`
- Future support for Stratum v1 pool mining

## Features

- CPU mining with thread control and sleep throttling
- GPU mining via CUDA (planned next phase)
- Real-time structured status output
- Simple configuration file
- SHA-256d implementation (verified on both CPU and GPU)

## Project Structure
- 📁 BitcoinMinerClean.
    - 📄 .gitignore
    - 📄 README.md
    - 📁 src
        - 📄 bitcoin.cpp
        - 📄 bitcoin.h
        - 📄 config.conf
        - 📄 config.h
        - 📄 cuda_miner.cu
        - 📄 main.cpp
        - 📄 miner.cpp
        - 📄 miner.h
        - 📄 sha256.cuh
        - 📄 sha256.h
        - 📄 utils.cpp
        - 📄 utils.h


## Flowchart (High-Level)
---

```mermaid
flowchart TD
    A[User Starts Miner] --> B[Load config.conf]
    
    B --> C{Mode Selection}
    C -->|CPU| D[Initialize CPUMiner]
    C -->|GPU| E[Initialize CudaMiner<br/>planned]
    
    D --> F[Build Block Header]
    E --> F
    
    F --> G[Main Mining Loop]
    
    G --> H[Compute SHA256d<br/>header + nonce]
    H --> I[Check Hash vs Target]
    
    I -->|Valid Share| J[Log Share Found<br/>Future: Submit to Pool]
    I -->|No Share| K[Increment Nonce]
    
    K --> L[Update Statistics<br/>Hash Rate + Status]
    L --> M[Print Structured Output]
    M --> N[Apply Throttle<br/>Sleep / Yield]
    
    N --> G

    subgraph Configuration
        B
    end

    subgraph Output
        M
    end

    style A fill:#4ade80,stroke:#166534,stroke-width:2px
    style J fill:#f59e0b,stroke:#b45309,stroke-width:2px
    style G fill:#3b82f6,stroke:#1e40af,stroke-width:2px
    ```