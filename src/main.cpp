#include "../include/node.hpp"
#include <bits/stdc++.h>
using namespace std;

int main() {
    map<int, unique_ptr<Node>> nodes;
    map<int, thread> server_threads;
    map<int, thread> refresh_threads;
    int next_id = 2; // node1 is always id=1

    // Start bootstrap node1
    nodes[1] = make_unique<Node>("0.0.0.0", "50051");
    server_threads[1] = thread([&](){ nodes[1]->RunServer(); });
    refresh_threads[1] = thread([&](){ nodes[1]->periodicBucketRefresh(); });

    cout << "Bootstrap node1 started at 0.0.0.0:50051 (id=1)" << endl;

    string cmd;
    while (true) {
        cout << "\nCommands: addnode <port> | stop <id> | store <id> <filename> | retrieve <id> <filename> <outfile> | print <id> | exit\n> ";
        cin >> cmd;
        if (cmd == "addnode") {
            string port;
            cin >> port;
            int id = next_id++;
            nodes[id] = make_unique<Node>("0.0.0.0", port);
            server_threads[id] = thread([&, id](){ nodes[id]->RunServer(); });
            refresh_threads[id] = thread([&, id](){ nodes[id]->periodicBucketRefresh(); });
            // Bootstrap to node1
            this_thread::sleep_for(chrono::milliseconds(500));
            nodes[id]->bootstrap("0.0.0.0", 50051);
            cout << "Node" << id << " started at 0.0.0.0:" << port << endl;
        } else if (cmd == "stop") {
    int id;
    cin >> id;
    if (nodes.count(id)) {
        nodes[id]->stop();
        cout << "Node" << id << " stopped." << endl;
        // Clean up threads
        if (server_threads.count(id) && server_threads[id].joinable()) server_threads[id].detach();
        if (refresh_threads.count(id) && refresh_threads[id].joinable()) refresh_threads[id].detach();
        // Remove node from maps
        nodes.erase(id);
        server_threads.erase(id);
        refresh_threads.erase(id);
    }
    } else if (cmd == "store") {
            int id;
            string filename;
            cin >> id >> filename;
            if (nodes.count(id)) {
                nodes[id]->storeFile(filename);
            } else {
                cout << "Node" << id << " does not exist." << endl;
            }
        }
        else if (cmd == "retrieve") {
            int id;
            string filename, outfile;
            cin >> id >> filename >> outfile;
            if (nodes.count(id)) {
                nodes[id]->retrieveFile(filename, outfile);
            } else {
                cout << "Node" << id << " does not exist." << endl;
            }
        } else if (cmd == "print") {
            int id;
            cin >> id;
            if (nodes.count(id)) {
                nodes[id]->printBuckets();
            } else {
                cout << "Node" << id << " does not exist." << endl;
            }
        } else if (cmd == "exit") {
            for (auto& [id, node] : nodes) node->stop();
            for (auto& [id, t] : server_threads) if (t.joinable()) t.detach();
            for (auto& [id, t] : refresh_threads) if (t.joinable()) t.detach();
            break;
        } else {
            cout << "Unknown command." << endl;
        }
    }
    return 0;
}