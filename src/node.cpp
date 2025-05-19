#include"../include/node.hpp"
#include<bits/stdc++.h>
#include<openssl/sha.h>
using namespace std;

Node::Node(const std::string& input) {
    unsigned char hash[SHA_DIGEST_LENGTH]; // SHA_DIGEST_LENGTH == 20 bytes == 160 bits
    SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);

    // Convert hash bytes to bitset
    for (int byte = 0; byte < SHA_DIGEST_LENGTH; ++byte) {
        for (int bit = 0; bit < 8; ++bit) {
            id[(SHA_DIGEST_LENGTH - 1 - byte) * 8 + bit] = (hash[byte] >> bit) & 1;
        }
    }
}

int Node::distanceTo(const Node& other) const {
    int distance = 0;
    for (int i = 0; i < ID_BITS; ++i) {
        if (id[i] != other.id[i]) {
            ++distance;
        }
    }
    return distance;
}

bitset<ID_BITS> Node::getId() const {
    return id;
}

void Node::addNodeToBucket( Node& node){
    int distance = distanceTo(node);
    auto& bucket = buckets[distance]; // This will create the vector if it doesn't exist
    if (bucket.size() < K) {
        bucket.push_back(&node);
    } else {
        std::cout << "Bucket full, cannot add node." << std::endl;
    }
}

void Node::printBuckets() {
    for (const auto& bucket : buckets) {
        std::cout << "Distance: " << bucket.first << " -> ";
        for (const auto& nodePtr : bucket.second) {
            std::cout << nodePtr->getId() << " ";
        }
        std::cout << std::endl;
    }
}

void Node::removeNodeFromBucket( Node& node) {
    // if node is not responding to ping, then remove it from the bucket
    int distance = distanceTo(node);
    auto& bucket = buckets[distance];
    auto it = std::find(bucket.begin(), bucket.end(), &node);
    if (it != bucket.end()) {
        bucket.erase(it);
    } else {
        std::cout << "Node not found in the bucket." << std::endl;
    }
}

void Node::store(const std::string& content) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(content.c_str()), content.size(), hash);

    bitset<ID_BITS> key;
    for (int byte = 0; byte < SHA_DIGEST_LENGTH; ++byte)
        for (int bit = 0; bit < 8; ++bit)
            key[(SHA_DIGEST_LENGTH - 1 - byte) * 8 + bit] = (hash[byte] >> bit) & 1;

    // Store locally
    dataStore[key] = content;
    // Replicate to k closest nodes
    auto closest = findKClosestNodes(key);
    for (Node* neighbor : closest) {
        neighbor->replicateChunk(key, content);
    }

}

// Node* Node::findNodeById(const bitset<ID_BITS>& id) {
//     for (const auto& bucket : buckets) {
//         for (const auto& nodePtr : bucket.second) {
//             if (nodePtr->getId()== id) {
//                 return this; // Return the current node if found
//             }
//         }
//     }
//     return nullptr; // Not found
// }

vector<Node*> Node::findKClosestNodes(const bitset<ID_BITS>& key) {
    vector<Node*> closestNodes;
    for (const auto& bucket : buckets) {
        for (const auto& nodePtr : bucket.second) {
                closestNodes.push_back(nodePtr);
        }
    }
    // Sort by distance to the key
    sort(closestNodes.begin(), closestNodes.end(), [&](Node* a, Node* b) {
        return distanceTo(*a) < distanceTo(*b);
    });
    // Return the k closest nodes
    if (closestNodes.size() > K) {
        closestNodes.resize(K);
    }
    return closestNodes;
}

string Node::find(const std::string& keyContent) {
    auto it = dataStore.find(id);
    if (it != dataStore.end()) {
        return it->second;
    } else {
        return "Key not found";
    }
}

void Node::replicateChunk(const bitset<ID_BITS>& key, const string& content) {
    // Example implementation: store the content in the dataStore
    dataStore[key] = content;
    cout<<"--Replicating chunk--"<<endl;
    cout<<"storing in node with id: "<<id<<endl;
    
}

void Node::storeFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Error opening file: " << filename << endl;
        return;
    }
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    store(content);
    file.close();
}
void Node::printdataStore() {
    for (const auto& pair : dataStore) {
        cout << "Key: " << pair.first << ", Content: " << pair.second << endl;
    }
}