# Distributed File System Simulation

This project is a simulation of a Distributed Hash Table (DHT)-based file storage system, inspired by systems like Kademlia. Nodes in the network can store and replicate data, and the system uses SHA-1 hashing for node IDs and data keys.

## Features

- **Node ID Generation:** Each node generates a unique 160-bit ID using SHA-1 hashing.
- **K-Buckets:** Each node maintains k-buckets for efficient routing and neighbor management.
- **Data Storage & Replication:** Nodes can store data and replicate it to the k closest nodes.
- **File Storage:** Supports storing file contents in the distributed network.
- **Find Closest Nodes:** Efficient lookup for the k closest nodes to a given key.
- **OpenSSL Integration:** Uses OpenSSL for SHA-1 hashing.

## Directory Structure
├── include/ # Header files (Node, KBucket, NodeID, etc.) 
├── src/ # Source files (main.cpp, node.cpp, etc.) 
├── build/ # Build directory (ignored by git) 
├── CMakeLists.txt # Build configuration 
├── ReadMe.md # This file 
└── .gitignore


## Build Instructions

1. **Install dependencies:**  
   - C++17 compiler (e.g., g++ or clang++)
   - CMake (>= 3.10)
   - Clang
   - Ninja
   - OpenSSL development libraries

2. **Build the project:**
   ```bash
   mkdir build
   cd build
   cmake -G Ninja .. -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
   ninja      # or: make

3. **Run Simulation:**
    ./main_exec

## Usage
- The simulation creates several nodes, adds them to each other's buckets, and demonstrates storing and replicating data.
- Output will show which nodes store and replicate chunks, and the state of each node's data store.

## Customization
- Change K-bucket size: Edit the K constant in the header files.
- Change ID size: Edit the ID_BITS constant (default is 160 for SHA-1).
- Add more nodes or files: Modify src/main.cpp to simulate different scenarios.

### License
This project is for educational and research purposes.

### Author:
Liam