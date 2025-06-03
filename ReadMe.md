# Distributed File System Simulation

This project is a simulation of a Distributed Hash Table (DHT)-based file storage system, inspired by systems like Kademlia. Nodes in the network can store and replicate data, and the system uses SHA-1 hashing for node IDs and data keys.

## Features

- **Node ID Generation:** Each node generates a unique 160-bit ID using SHA-1 hashing.
- **K-Buckets:** Each node maintains k-buckets for efficient routing and neighbor management.
- **Data Storage & Replication:** Nodes can store data and replicate it to the k closest nodes.
- **File Storage:** Supports storing file contents in the distributed network.
- **Find Closest Nodes:** Efficient lookup for the k closest nodes to a given key.
- **OpenSSL Integration:** Uses OpenSSL for SHA-1 hashing.
- **Interactive CLI:** Add, stop, and inspect nodes; store and retrieve files from any node.

## Directory Structure
├── include/ # Header files (Node, KBucket, NodeID, etc.)  
├── src/     # Source files (main.cpp, node.cpp, etc.)  
├── build/   # Build directory (ignored by git)  
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
   ```

3. **Run the CLI:**
   ```bash
   ./main_exec
   ```

## Data Directory for File Operations

All file operations (store/retrieve) are performed relative to a `data` directory in your project root.  
**Always place files you want to store in the DHT inside the `data` folder.**  
When retrieving files, specify the output path as `data/<outfile>`.

### Why?
- This ensures portability and makes it easy to use with Docker or on any system.
- When running in Docker, you can mount the `data` directory as a volume for real-time file sharing between your host and the container.

### Example Workflow with Docker

1. **Prepare your file:**  
   Place `test.txt` in the `data` directory:
   ```
   cp /path/to/your/test.txt ./data/
   ```

2. **Store the file using the CLI:**  
   ```
   store 2 /app/data/test.txt
   ```
   The file will be fetched from data folder in the project which is mounted in docker at /app/. .Hence /app/data/test.txt

3. **Retrieve the file using the CLI:**  
   ```
   retrieve 3 /app/data/test.txt /app/data/result.txt
   ```
   The retrieved file will appear as `./data/result.txt`. You can view this file in real time.

### Docker Usage

When running in Docker, mount the `data` directory as a volume:
```bash
docker run --rm -it -p 50051-50060:50051-50060 -v ${PWD}/data:/app/data dht-cpp-app
```
- Inside the container, use `/app/data/filename` for file paths.
- Any changes in `./data` on your host are instantly visible in the container and vice versa.
- the ```-p 50051-50060:50051-50060``` exposes all ports in the range and maps them to your real machine, so that you can create new nodes in that range. The range can be extended as per requirement.

---

## Usage

When you run the program, you get an interactive prompt.  
**Available commands:**
- `addnode <port>` — Add a new node listening on the given port (joins the DHT via node1).
- `stop <id>` — Stop and remove the node with the given id.
- `store <id> <filename>` — Store a file in the DHT using the specified node.
- `retrieve <id> <filename> <outfile>` — Retrieve a file from the DHT using the specified node.
- `print <id>` — Print the k-buckets of the specified node.
- `exit` — Stop all nodes and exit.

**Example session:**
```
addnode 50052
addnode 50053
store 2 test.txt
retrieve 3 test.txt result.txt
print 2
stop 2
exit
```

- Node IDs are assigned incrementally (node1 is always id=1).
- After stopping a node, its id is removed from the CLI and cannot be used.

## Customization
- Change K-bucket size: Edit the `K` constant in the header files.
- Change ID size: Edit the `ID_BITS` constant (default is 160 for SHA-1).
- Add more nodes or files: Use the CLI to simulate different scenarios.

### License
This project is for educational and research purposes.

### Author:
Liam