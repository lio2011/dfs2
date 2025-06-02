#include"../include/node.hpp"
#include<bits/stdc++.h>
#include<openssl/sha.h>

using namespace std;

// --- Node constructor ---
Node::Node(const string& input_ip, const string& input_port) : ip(input_ip), port(input_port) {
    address_ = input_ip + ":" + input_port;
    string input=address_;
    unsigned char hash[SHA_DIGEST_LENGTH]; // SHA_DIGEST_LENGTH == 20 bytes == 160 bits
    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);

    // Convert hash bytes to bitset
    for (int byte = 0; byte < SHA_DIGEST_LENGTH; ++byte) {
        for (int bit = 0; bit < 8; ++bit) {
            id[(SHA_DIGEST_LENGTH - 1 - byte) * 8 + bit] = (hash[byte] >> bit) & 1;
        }
    }
}

// --- gRPC server-side implementation ---
Status Node::Ping(ServerContext* context, const PingRequest* request, PingResponse* response) {
    // cout<<"Server Side------"<<endl;
    // cout << "Received ping from: " << request->sender().ip() <<":"<<request->sender().port() << endl;
    response->set_message("Ping received by " + address_);
    // cout<<"------"<<endl;
    return Status::OK;
}

// --- gRPC client-side logic ---


void Node::PingPeer(const string& peer_address) {
    auto stub = DHTNode::NewStub(grpc::CreateChannel(peer_address, grpc::InsecureChannelCredentials()));
    PingRequest request;
    NodeId* sender = request.mutable_sender();
    sender->set_id(id.to_string());
    sender->set_ip(ip);
    sender->set_port(stoi(port));

    PingResponse response;
    ClientContext context;
    Status status = stub->Ping(&context, request, &response);
    cout<<"Client Side------"<<endl;
    if (status.ok()) {
        cout << "Ping response: " << response.message() << endl;
    } else {
        cerr << "RPC failed: " << status.error_message() << endl;
    }
    cout<<"------"<<endl;
}

// --- Run the gRPC server ---
void Node::RunServer() {
    ServerBuilder builder;
    builder.AddListeningPort(address_, grpc::InsecureServerCredentials());
    builder.RegisterService(this);
    server = builder.BuildAndStart();
    cout << "Server listening on " << address_ << endl;
    server->Wait();
}

void Node::stop() {
    if (server) {
        server->Shutdown();
        cout << "Server at " << address_ << " stopped." << endl;
    }
}

// --- Node logic ---
vector<pair<string, bitset<ID_BITS>>> Node::FindNodeRPC(const string& peer_address, const bitset<ID_BITS>& target_id) {
    auto stub = DHTNode::NewStub(grpc::CreateChannel(peer_address, grpc::InsecureChannelCredentials()));
    FindNodeRequest request;
    NodeId* sender = request.mutable_sender();
    sender->set_id(id.to_string());
    sender->set_ip(ip);
    sender->set_port(stoi(port));
    // Set target_id as bytes
    string target_id_str = target_id.to_string();
    request.set_target_id(target_id_str);

    FindNodeResponse response;
    ClientContext context;
    Status status = stub->FindNode(&context, request, &response);

    vector<pair<string, bitset<ID_BITS>>> result;
    if (status.ok()) {
        for (const auto& node : response.closest_nodes()) {
            bitset<ID_BITS> nid(node.id());
            string addr = node.ip() + ":" + to_string(node.port());
            result.push_back({addr, nid});
        }
    }
    return result;
}

Status Node::FindNode(ServerContext* context, const FindNodeRequest* request, FindNodeResponse* response) {
    // Get the target ID from the request
    string target_id_str(reinterpret_cast<const char*>(request->target_id().data()), request->target_id().size());
    bitset<ID_BITS> target_id(target_id_str);

    // Find K closest nodes to the target_id
    auto closest = findKClosestNodes(target_id);

    // Fill the response with NodeId messages
    for (const auto& peer : closest) {
        NodeId* node = response->add_closest_nodes();
        node->set_id(peer.id.to_string());
        node->set_ip(peer.ip);
        node->set_port(stoi(peer.port));
    }
    return Status::OK;
}

Status Node::AddPeer(ServerContext* context, const AddPeerRequest* request, AddPeerResponse* response) {
    const auto& sender = request->sender();

    // Convert sender.id() (string of '0' and '1') to bitset
    bitset<ID_BITS> peer_id(sender.id());
    string peer_ip = sender.ip();
    string peer_port = to_string(sender.port());

    addNodeToBucket(peer_id, peer_ip, peer_port);
    response->set_status("OK");
    return Status::OK;
}

void Node::AddPeerToRemote(const string& peer_address) {
    auto stub = DHTNode::NewStub(grpc::CreateChannel(peer_address, grpc::InsecureChannelCredentials()));
    AddPeerRequest request;
    NodeId* sender = request.mutable_sender();
    sender->set_id(id.to_string());
    sender->set_ip(ip);
    sender->set_port(stoi(port));

    AddPeerResponse response;
    ClientContext context;
    Status status = stub->AddPeer(&context, request, &response);
    if (status.ok()) {
        cout << "AddPeer to " << peer_address << ": " << response.status() << endl;
    } else {
        cerr << "AddPeer RPC failed to " << peer_address << ": " << status.error_message() << endl;
    }
}

void Node::bootstrap(const string& bootstrap_ip, int bootstrap_port) {
    string bootstrap_address = bootstrap_ip + ":" + to_string(bootstrap_port);

    set<bitset<ID_BITS>, BitsetLess> seen;
    queue<pair<string, bitset<ID_BITS>>> to_visit;

    // Start with the bootstrap node (ID unknown at first)
    to_visit.push({bootstrap_address, bitset<ID_BITS>()});

    //Add bootstrap node to the bucket if possible
    if (bootstrap_address != this->getAddress()) {
        // Get the bootstrap node's ID by hashing its address (same as Node constructor)
        string input = bootstrap_address;
        unsigned char hash[SHA_DIGEST_LENGTH];
        SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);
        bitset<ID_BITS> bootstrap_id;
        for (int byte = 0; byte < SHA_DIGEST_LENGTH; ++byte) {
            for (int bit = 0; bit < 8; ++bit) {
                bootstrap_id[(SHA_DIGEST_LENGTH - 1 - byte) * 8 + bit] = (hash[byte] >> bit) & 1;
            }
        }
        addNodeToBucket(bootstrap_id, bootstrap_ip, to_string(bootstrap_port));
    }

    while (!to_visit.empty() && buckets.size() < ID_BITS) {
        auto [peer_address, peer_id] = to_visit.front();
        to_visit.pop();

        // --- Add self to this peer's bucket if local (for testing) ---
        AddPeerToRemote(peer_address);

        // Skip if already seen
        if (seen.count(peer_id)) continue;
        seen.insert(peer_id);

        // Ping the peer to check if alive
        PingPeer(peer_address);

        // FindNode: ask this peer for nodes close to our ID
        auto found_nodes = FindNodeRPC(peer_address, id);

        for (const auto& [found_addr, found_id] : found_nodes) {
            if (found_id != id && !seen.count(found_id)) {
                // Split found_addr into ip and port
                auto pos = found_addr.find(':');
                string found_ip = found_addr.substr(0, pos);
                string found_port = found_addr.substr(pos + 1);

                addNodeToBucket(found_id, found_ip, found_port);

                // Add to queue for further traversal
                to_visit.push({found_addr, found_id});
            }
        }
    }

    cout << "Bootstrapping complete. Buckets filled: " << buckets.size() << endl;
}

int Node::distanceTo(const bitset<ID_BITS>& found_id) const {
    int distance = 0;
    for (int i = 0; i < ID_BITS; ++i) {
        if (id[i] != found_id[i]) {
            ++distance;
        }
    }
    return distance;
}

bitset<ID_BITS> Node::getId() const {
    return id;
}

void Node::addNodeToBucket(const bitset<ID_BITS>& node_id, const string& node_ip, const string& node_port) {
    int distance = distanceTo(node_id);
    auto& bucket = buckets[distance];

    // Check if the bucket already contains the node
    for (const auto& peer : bucket) {
        if (peer.id == node_id && peer.ip == node_ip && peer.port == node_port) {
            // Node already exists in this bucket
            return;
        }
    }

    if (bucket.size() < K) {
        bucket.push_back(PeerInfo(node_id, node_ip, node_port));
    } else {
        cout << "Bucket full, cannot add node." << endl;
    }
}

void Node::printBuckets() {
    for (const auto& bucket : buckets) {
        cout << "Distance: " << bucket.first << " -> ";
        for (const auto& peer : bucket.second) {
            cout << "[" << peer.id << ", " << peer.ip << ", " << peer.port << "] ";
        }
        cout << endl;
    }
}

void Node::removeNodeFromBucket( bitset<ID_BITS>& nodeId, const string& nodeAddress) {
    // if node is not responding to ping, then remove it from the bucket
    int distance = distanceTo(nodeId);
    auto& bucket = buckets[distance];
    auto it = find_if(bucket.begin(), bucket.end(),
    [&](const PeerInfo& peer) { return peer.id == nodeId; });
    if (it != bucket.end()) {
        bucket.erase(it);
    } else {
        cout << "Node not found in the bucket." << endl;
    }
}

void Node::store(const string& content) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(content.c_str()), content.size(), hash);

    bitset<ID_BITS> key;
    for (int byte = 0; byte < SHA_DIGEST_LENGTH; ++byte)
        for (int bit = 0; bit < 8; ++bit)
            key[(SHA_DIGEST_LENGTH - 1 - byte) * 8 + bit] = (hash[byte] >> bit) & 1;

    // Store locally
    dataStore[key] = content;

    // Replicate to k closest nodes using gRPC
    auto closest = findKClosestNodes(key);
    for (const auto& peer : closest) {
        string peer_address = peer.ip + ":" + peer.port;

        // Create a gRPC stub for the peer
        auto stub = DHTNode::NewStub(grpc::CreateChannel(peer_address, grpc::InsecureChannelCredentials()));

        // Prepare ReplicateChunkRequest
        ReplicateChunkRequest req;
        ReplicateChunkResponse resp;
        grpc::ClientContext ctx;

        NodeId* sender = req.mutable_sender();
        sender->set_id(id.to_string());
        sender->set_ip(ip);
        sender->set_port(stoi(port));
        // Set key as bytes
        string key_bytes(reinterpret_cast<const char*>(hash), SHA_DIGEST_LENGTH);
        req.set_key(key_bytes);
        req.set_value(content);

        // Call ReplicateChunk RPC
        auto status = stub->ReplicateChunk(&ctx, req, &resp);
        if (status.ok()) {
            cout << "Replicated chunk to " << peer_address << ": " << resp.status() << endl;
        } else {
            cerr << "Replication failed to " << peer_address << ": " << status.error_message() << endl;
        }
    }
}

vector<PeerInfo> Node::findKClosestNodes(const bitset<ID_BITS>& key) {
    vector<PeerInfo> closestNodes;
    for (const auto& bucket : buckets) {
        for (const auto& peer : bucket.second) {
            closestNodes.push_back(peer);
        }
    }
    // Sort by XOR distance to the key
    sort(closestNodes.begin(), closestNodes.end(), [&](const PeerInfo& a, const PeerInfo& b) {
        bitset<ID_BITS> dist_a = a.id ^ key;
        bitset<ID_BITS> dist_b = b.id ^ key;
        // Compare as unsigned long long (for large bitsets, you may need a custom comparator)
        for (int i = ID_BITS - 1; i >= 0; --i) {
            if (dist_a[i] != dist_b[i])
                return dist_a[i] < dist_b[i];
        }
        return false;
    });
    // Return the k closest nodes
    if (closestNodes.size() > K) {
        closestNodes.resize(K);
    }
    return closestNodes;
}

string Node::find(const string& keyContent) {
    // Convert keyContent (string of '0' and '1') to bitset
    bitset<ID_BITS> key(keyContent);
    auto it = dataStore.find(key);
    if (it != dataStore.end()) {
        return it->second;
    } else {
        return "Key not found";
    }
}

Status Node::ReplicateChunk(ServerContext* context, const ReplicateChunkRequest* request, ReplicateChunkResponse* response) {
    // Store the chunk in the local dataStore
    string value(reinterpret_cast<const char*>(request->value().data()), request->value().size());
    bitset<ID_BITS> key;
    // Convert bytes to bitset
    for (size_t i = 0; i < request->key().size() && i < ID_BITS/8; ++i) {
        for (int b = 0; b < 8; ++b) {
            key[(request->key().size() - 1 - i) * 8 + b] = (request->key()[i] >> b) & 1;
        }
    }
    dataStore[key] = value;
    response->set_status("OK");
    cout << "Replicated chunk stored for key: " << key << endl;
    return Status::OK;
}

void Node::storeFile(const string& filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }

    vector<bitset<ID_BITS>> chunk_keys;
    size_t chunk_idx = 0;
    while (file) {
        vector<char> buffer(CHUNK_SIZE);
        file.read(buffer.data(), CHUNK_SIZE);
        streamsize bytesRead = file.gcount();
        if (bytesRead <= 0) break;

        string chunk_data(buffer.data(), bytesRead);

        // Store chunk and get its key
        bitset<ID_BITS> chunk_key = storeChunk(chunk_data);
        chunk_keys.push_back(chunk_key);

        ++chunk_idx;
    }
    file.close();

    // Store the manifest (list of chunk keys) under a file key
    string manifest;
    for (const auto& key : chunk_keys) {
        manifest += key.to_string() + "\n";
    }
    // Use filename as the manifest key (or hash it)
    storeManifest(filename, manifest);
    cout << "Stored file '" << filename << "' as " << chunk_keys.size() << " chunks." << endl;
}

bitset<ID_BITS> Node::storeChunk(const string& chunk_data) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(chunk_data.c_str()), chunk_data.size(), hash);

    bitset<ID_BITS> chunk_key;
    for (int byte = 0; byte < SHA_DIGEST_LENGTH; ++byte)
        for (int bit = 0; bit < 8; ++bit)
            chunk_key[(SHA_DIGEST_LENGTH - 1 - byte) * 8 + bit] = (hash[byte] >> bit) & 1;

    // Store locally
    dataStore[chunk_key] = chunk_data;

    // Replicate to k closest nodes using gRPC (reuse your existing logic)
    auto closest = findKClosestNodes(chunk_key);
    for (const auto& peer : closest) {
        string peer_address = peer.ip + ":" + peer.port;
        auto stub = DHTNode::NewStub(grpc::CreateChannel(peer_address, grpc::InsecureChannelCredentials()));
        ReplicateChunkRequest req;
        ReplicateChunkResponse resp;
        grpc::ClientContext ctx;

        NodeId* sender = req.mutable_sender();
        sender->set_id(id.to_string());
        sender->set_ip(ip);
        sender->set_port(stoi(port));
        string key_bytes(reinterpret_cast<const char*>(hash), SHA_DIGEST_LENGTH);
        req.set_key(key_bytes);
        req.set_value(chunk_data);

        auto status = stub->ReplicateChunk(&ctx, req, &resp);
        if (status.ok()) {
            cout << "Replicated chunk to " << peer_address << ": " << resp.status() << endl;
        } else {
            cerr << "Replication failed to " << peer_address << ": " << status.error_message() << endl;
        }
    }
    return chunk_key;
}

void Node::storeManifest(const string& filename, const string& manifest) {
    // Hash the filename for the manifest key
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(filename.c_str()), filename.size(), hash);

    bitset<ID_BITS> manifest_key;
    for (int byte = 0; byte < SHA_DIGEST_LENGTH; ++byte)
        for (int bit = 0; bit < 8; ++bit)
            manifest_key[(SHA_DIGEST_LENGTH - 1 - byte) * 8 + bit] = (hash[byte] >> bit) & 1;

    // Store locally
    dataStore[manifest_key] = manifest;

    // Replicate to k closest nodes
    auto closest = findKClosestNodes(manifest_key);
    for (const auto& peer : closest) {
        string peer_address = peer.ip + ":" + peer.port;
        auto stub = DHTNode::NewStub(grpc::CreateChannel(peer_address, grpc::InsecureChannelCredentials()));
        ReplicateChunkRequest req;
        ReplicateChunkResponse resp;
        grpc::ClientContext ctx;

        NodeId* sender = req.mutable_sender();
        sender->set_id(id.to_string());
        sender->set_ip(ip);
        sender->set_port(stoi(port));
        string key_bytes(reinterpret_cast<const char*>(hash), SHA_DIGEST_LENGTH);
        req.set_key(key_bytes);
        req.set_value(manifest);

        auto status = stub->ReplicateChunk(&ctx, req, &resp);
        if (status.ok()) {
            cout << "Replicated manifest to " << peer_address << ": " << resp.status() << endl;
        } else {
            cerr << "Manifest replication failed to " << peer_address << ": " << status.error_message() << endl;
        }
    }
}

void Node::retrieveFile(const string& filename, const string& out_filename) {
    // Get manifest key
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(filename.c_str()), filename.size(), hash);

    bitset<ID_BITS> manifest_key;
    for (int byte = 0; byte < SHA_DIGEST_LENGTH; ++byte)
        for (int bit = 0; bit < 8; ++bit)
            manifest_key[(SHA_DIGEST_LENGTH - 1 - byte) * 8 + bit] = (hash[byte] >> bit) & 1;

    // Retrieve manifest using DHT-wide lookup
    string manifest = findDHT(manifest_key);
    if (manifest == "Key not found") {
        cerr << "Manifest not found for file: " << filename << endl;
        return;
    }

    // Parse chunk keys
    vector<bitset<ID_BITS>> chunk_keys;
    istringstream iss(manifest);
    string line;
    while (getline(iss, line)) {
        if (!line.empty()) {
            chunk_keys.push_back(bitset<ID_BITS>(line));
        }
    }

    // Retrieve each chunk and write to output file
    ofstream outfile(out_filename, ios::binary);
    for (const auto& chunk_key : chunk_keys) {
        string chunk = findDHT(chunk_key);
        if (chunk == "Key not found") {
            cerr << "Missing chunk for key: " << chunk_key << endl;
            continue;
        }
        outfile.write(chunk.data(), chunk.size());
    }
    outfile.close();
    cout << "File '" << out_filename << "' reconstructed from chunks." << endl;
}

void Node::printdataStore() {
    for (const auto& pair : dataStore) {
        cout << "Key: " << pair.first << ", Content: " << pair.second << endl;
    }
}

Status Node::GetValue(ServerContext* context, const GetValueRequest* request, GetValueResponse* response) {
    // Convert bytes to bitset
    bitset<ID_BITS> key;
    for (size_t i = 0; i < request->key().size() && i < ID_BITS/8; ++i) {
        for (int b = 0; b < 8; ++b) {
            key[(request->key().size() - 1 - i) * 8 + b] = (request->key()[i] >> b) & 1;
        }
    }
    auto it = dataStore.find(key);
    if (it != dataStore.end()) {
        response->set_value(it->second);
        response->set_found(true);
    } else {
        response->set_found(false);
    }
    return Status::OK;
}

string Node::findDHT(const bitset<ID_BITS>& key) {
    const int ALPHA = 3; // Number of parallel queries
    set<bitset<ID_BITS>, BitsetLess> queried;
    vector<PeerInfo> shortlist = findKClosestNodes(key);
    map<bitset<ID_BITS>, PeerInfo, BitsetLess> all_candidates;

    // Add initial shortlist to candidate set
    for (const auto& peer : shortlist) {
        all_candidates[peer.id] = peer;
    }

    // If we have the value locally, return it
    auto it = dataStore.find(key);
    if (it != dataStore.end()) {
        return it->second;
    }

    bool value_found = false;
    string value;

    while (!shortlist.empty() && !value_found) {
        // Query up to ALPHA closest unqueried nodes
        vector<PeerInfo> to_query;
        for (const auto& peer : shortlist) {
            if (queried.count(peer.id) == 0 && to_query.size() < ALPHA) {
                to_query.push_back(peer);
            }
        }
        if (to_query.empty()) break;

        // For each peer, query GetValue
        for (const auto& peer : to_query) {
            queried.insert(peer.id);
            string peer_address = peer.ip + ":" + peer.port;
            auto stub = DHTNode::NewStub(grpc::CreateChannel(peer_address, grpc::InsecureChannelCredentials()));
            GetValueRequest req;
            GetValueResponse resp;
            grpc::ClientContext ctx;

            // Convert bitset to raw bytes
            string key_bytes;
            for (int i = 0; i < ID_BITS; i += 8) {
                unsigned char byte = 0;
                for (int b = 0; b < 8 && (i + b) < ID_BITS; ++b) {
                    if (key[i + b]) byte |= (1 << (7 - b));
                }
                key_bytes.push_back(byte);
            }
            req.set_key(key_bytes);

            auto status = stub->GetValue(&ctx, req, &resp);
            if (status.ok() && resp.found()) {
                value_found = true;
                value = resp.value();
                break;
            }

            // If not found, ask for closest nodes (FindNode)
            FindNodeRequest fn_req;
            NodeId* sender = fn_req.mutable_sender();
            sender->set_id(id.to_string());
            sender->set_ip(ip);
            sender->set_port(stoi(port));
            fn_req.set_target_id(key.to_string());
            FindNodeResponse fn_resp;
            grpc::ClientContext fn_ctx;
            auto fn_status = stub->FindNode(&fn_ctx, fn_req, &fn_resp);
            if (fn_status.ok()) {
                for (const auto& node : fn_resp.closest_nodes()) {
                    bitset<ID_BITS> nid(node.id());
                    PeerInfo new_peer(nid, node.ip(), to_string(node.port()));
                    if (all_candidates.count(nid) == 0) {
                        all_candidates[nid] = new_peer;
                    }
                }
            }
        }

        // Update shortlist: K closest unqueried nodes
        shortlist.clear();
        for (const auto& [nid, peer] : all_candidates) {
            if (queried.count(nid) == 0) {
                shortlist.push_back(peer);
            }
            if (shortlist.size() >= K) break;
        }
    }

    if (value_found) {
        return value;
    } else {
        return "Key not found";
    }
}

void Node::periodicBucketRefresh() {
    while (true) {
        // For each bucket
        for (auto& bucket_pair : buckets) {
            auto& bucket = bucket_pair.second;
            for (auto it = bucket.begin(); it != bucket.end(); ) {
                std::string peer_address = it->ip + ":" + it->port;
                // Ping the peer
                auto stub = DHTNode::NewStub(grpc::CreateChannel(peer_address, grpc::InsecureChannelCredentials()));
                PingRequest request;
                NodeId* sender = request.mutable_sender();
                sender->set_id(id.to_string());
                sender->set_ip(ip);
                sender->set_port(stoi(port));
                PingResponse response;
                grpc::ClientContext context;
                Status status = stub->Ping(&context, request, &response);
                if (!status.ok()) {
                    std::cerr << "Peer " << peer_address << " not responding. Removing from bucket." << std::endl;
                    it = bucket.erase(it); // Remove dead peer
                } else {
                    ++it;
                }
            }
        }
        // Sleep for the specified interval
        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
    }
}