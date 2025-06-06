#pragma once
#include <grpcpp/grpcpp.h>
#include<bits/stdc++.h>
#include "dht.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::Channel;
using grpc::ClientContext;
using dht::DHTNode;
using dht::NodeId;
using dht::PingRequest;
using dht::PingResponse;
using dht::StoreRequest;
using dht::StoreResponse;
using dht::FindNodeRequest;
using dht::FindNodeResponse;
using dht::ReplicateChunkRequest;
using dht::ReplicateChunkResponse;
using dht::FindNodeResponse;
using dht::AddPeerRequest;
using dht::AddPeerResponse;
using dht::GetValueRequest;
using dht::GetValueResponse;


using namespace std;
const int K = 20; // Max Number of nodes in the k-bucket
const int ID_BITS=160;
const size_t CHUNK_SIZE = 1024 * 64; // 64 KB chunk size

struct PeerInfo {
    std::bitset<ID_BITS> id;
    std::string ip;
    std::string port;
    PeerInfo() : id(0), ip(""), port("") {};
    PeerInfo(const std::bitset<ID_BITS>& nid, const std::string& nip, const std::string& nport)
        : id(nid), ip(nip), port(nport) {};
};

struct BitsetLess {
    bool operator()(const std::bitset<ID_BITS>& a, const std::bitset<ID_BITS>& b) const {
        for (int i = ID_BITS - 1; i >= 0; --i) {
            if (a[i] != b[i]) return a[i] < b[i];
        }
        return false;
    }
};

class Node : public DHTNode::Service { 
    bitset<ID_BITS> id;
    unordered_map<int, vector<PeerInfo>> buckets; // Buckets for storing nodes
    unordered_map<bitset<ID_BITS>, string> dataStore;
    string ip;
    string port;
    string address_; // Address of the node
    int interval_seconds = 5; // Interval for periodic tasks in seconds
    atomic<bool> running{true};
    std::unique_ptr<Server> server;

    public:
    Node(const string& ip, const string& port);

    // --- gRPC server-side implementation ---
    Status Ping(ServerContext* context, const PingRequest* request, PingResponse* response) override;
    Status AddPeer(ServerContext* context, const AddPeerRequest* request, AddPeerResponse* response) override;
    Status FindNode(ServerContext* context, const FindNodeRequest* request, FindNodeResponse* response) override;

    // --- gRPC client-side logic ---
    void PingPeer(const string& peer_address);
    void AddPeerToRemote(const std::string& peer_address);
    void periodicBucketRefresh();

    // Node logic
    vector<std::pair<std::string, bitset<ID_BITS>>> FindNodeRPC(const string& peer_address, const bitset<ID_BITS>& target_id);
    void bootstrap(const std::string& bootstrap_ip, int bootstrap_port);
    void addNodeToBucket(const std::bitset<ID_BITS>& id, const std::string& ip, const std::string& port);
    void removeNodeFromBucket( bitset<ID_BITS>& nodeId, const string& nodeAddress);
    void printBuckets();
    int distanceTo(const bitset<ID_BITS>& id) const;
    void store(const string& content);
    string find(const string& keyContent);
    void storeFile(const string& filename);
    Status ReplicateChunk(ServerContext* context, const ReplicateChunkRequest* request, ReplicateChunkResponse* response) override;
    vector<PeerInfo> findKClosestNodes(const bitset<ID_BITS>& key);
    Node* findNodeById(const bitset<ID_BITS>& id);
    bitset<ID_BITS> getId()const;
    void printdataStore();
    string getAddress() const { return ip + ":" + port; };
    bitset<ID_BITS> storeChunk(const std::string& chunk_data);
    void storeManifest(const std::string& filename, const std::string& manifest);
    std::string findDHT(const std::bitset<ID_BITS>& key);
    void retrieveFile(const std::string& filename, const std::string& out_filename);
    Status GetValue(ServerContext* context, const GetValueRequest* request, GetValueResponse* response) override;


    // Server runner
    void RunServer();
    void stop();
};