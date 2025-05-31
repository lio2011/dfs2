#include "../include/node.hpp"
#include <bits/stdc++.h>
using namespace std;

int main() {
    // Create nodes
    Node node1("0.0.0.0", "50051");
    Node node2("0.0.0.0", "50052");
    Node node3("0.0.0.0", "50053");
    Node node4("0.0.0.0", "50054");

    std::thread t1([&](){ node1.RunServer(); });
    std::thread t2([&](){ node2.RunServer(); });
    std::thread t3([&](){ node3.RunServer(); });
    std::thread t4([&](){ node4.RunServer(); });

    // Give servers time to start
    std::this_thread::sleep_for(std::chrono::seconds(1));

    cout <<"Bootstrapping nodes..." << endl;

    // Bootstrap other nodes using a known node 1
    node2.bootstrap("0.0.0.0", 50051);
    node3.bootstrap("0.0.0.0", 50051);
    node4.bootstrap("0.0.0.0", 50051);


    // Print buckets for each node
    cout << "Node 1 Buckets:" << endl;
    node1.printBuckets();
    cout << "\nNode 2 Buckets:" << endl;
    node2.printBuckets();
    cout << "\nNode 3 Buckets:" << endl;
    node3.printBuckets();
    cout << "\nNode 4 Buckets:" << endl;
    node4.printBuckets();

    // Remove a node from a bucket
    bitset<ID_BITS> id2 = node2.getId();
    node1.removeNodeFromBucket(id2, "0.0.0.0:50052");
    cout << "\nNode 1 Buckets after removing Node 2:" << endl;
    node1.printBuckets();
    cout<<endl;
    
    string test_filename = "test.txt";
    string restored_filename = "restored_testfile.txt";

    node1.storeFile(test_filename);
    node3.retrieveFile(test_filename, restored_filename);

    std::cout << "File storage and retrieval test complete. Check '" << restored_filename << "'." << std::endl;

    t1.join();
    t2.join();
    t3.join();
    return 0;
}