#include<bits/stdc++.h>
using namespace std;
const int K = 20; // Max Number of nodes in the k-bucket
const int ID_BITS=160;
class Node { 
    bitset<ID_BITS> id;
    unordered_map<int, vector<Node*>> buckets; // Buckets for storing nodes
    // The key is the distance to the node, and the value is a vector of node ids
    // The distance is calculated as the number of differing bits in the id
    // The vector stores the ids of nodes in the bucket
    // The vector is limited to K nodes
    // The buckets are sorted by distance, with the closest nodes first

    unordered_map<bitset<ID_BITS>, string> dataStore;
    // The key is the id of the node, and the value is the data stored in the node
    public:
    Node(const string& input); // Constructor to initialize self node id
    void addNodeToBucket( Node& node);
    void removeNodeFromBucket( Node& node);
    void printBuckets();
    int distanceTo(const Node& other)const;
    void store(const string& content);
    string find(const string& keyContent);
    void storeFile(const string& filename);
    void replicateChunk(const bitset<ID_BITS>& key, const string& content);
    vector<Node*> findKClosestNodes(const bitset<ID_BITS>& key);
    Node* findNodeById(const bitset<ID_BITS>& id);
    bitset<ID_BITS> getId()const;
    void printdataStore();
};