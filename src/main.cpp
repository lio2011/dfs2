#include<bits/stdc++.h>
using namespace std;
#include"../include/node.hpp"

int main(){
    Node node1("node1");
    Node node2("node2");
    Node node3("node3");
    Node node4("node4");
    Node node5("node5");
    Node node6("node6");
    Node node7("node6");

    node1.addNodeToBucket(node2);
    node1.addNodeToBucket(node3);
    node1.addNodeToBucket(node4);
    node1.addNodeToBucket(node5);
    node1.addNodeToBucket(node6);
    node1.addNodeToBucket(node7);


    node1.store("Hello World");
    node1.storeFile("test.txt");
    cout << "Data store of node1 after storing:" << endl;
    node1.printdataStore();
    cout<<"----------------------"<<endl;
    cout<<"data store of node2 after storing:" << endl;
    node2.printdataStore();
    cout<<"----------------------"<<endl;
    cout<<"data store of node3 after storing:" << endl;
    node3.printdataStore();
    cout<<"----------------------"<<endl;
    cout<<"data store of node4 after storing:" << endl;
    node4.printdataStore();
    cout<<"----------------------"<<endl;
    cout<<"data store of node5 after storing:" << endl;
    node5.printdataStore();
    cout<<"----------------------"<<endl;
    cout<<"data store of node6 after storing:" << endl;
    node6.printdataStore();
    cout<<"----------------------"<<endl;
    cout<<"data store of node7 after storing:" << endl;
    node7.printdataStore();
    cout<<"----------------------"<<endl;
    return 0;
}