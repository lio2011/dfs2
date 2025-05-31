#include "../include/node.hpp"
#include <bits/stdc++.h>
using namespace std;

int main() {
    // Create nodes
    Node node1("0.0.0.0", "50051");
    Node node2("0.0.0.0", "50052");
    Node node3("0.0.0.0", "50053");

    std::thread t1([&](){ node1.RunServer(); });
    std::thread t2([&](){ node2.RunServer(); });
    std::thread t3([&](){ node3.RunServer(); });

    // Give servers time to start
    std::this_thread::sleep_for(std::chrono::seconds(1));

    cout <<"Bootstrapping nodes..." << endl;

    // Bootstrap node2 and node3 to node1
    node2.bootstrap("0.0.0.0", 50051);
    node3.bootstrap("0.0.0.0", 50051);

    // Optionally, bootstrap node3 to node2 as well
    // node3.bootstrap("0.0.0.0", 50052);

    // Print buckets for each node
    cout << "Node 1 Buckets:" << endl;
    node1.printBuckets();
    cout << "\nNode 2 Buckets:" << endl;
    node2.printBuckets();
    cout << "\nNode 3 Buckets:" << endl;
    node3.printBuckets();

//     // Remove a node from a bucket
//     bitset<ID_BITS> id2 = node2.getId();
//     node1.removeNodeFromBucket(id2, "0.0.0.0:50052");
//     cout << "\nNode 1 Buckets after removing Node 2:" << endl;
//     node1.printBuckets();

//     // Find K closest nodes to a random key from node1's perspective
//     bitset<ID_BITS> randomKey;
//     randomKey.set(0); // Just an example, set bit 0
//     // Example usage after calling findKClosestNodes
//     auto closest = node1.findKClosestNodes(randomKey);
//     if (closest.empty()) {
//         std::cout << "No closest nodes found." << std::endl;
//     } else {
//     for (const auto& peer : closest) {
//         std::cout << "[" << peer.id << ", " << peer.ip << ", " << peer.port << "] ";
//     }
//     std::cout << std::endl;
// }
    t1.join();
    t2.join();
    t3.join();
    return 0;
}