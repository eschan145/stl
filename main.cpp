#include <iostream>

#include "ptr.h"


struct Node {
    Node() {
        std::cout << "Node created\n";
    }
    ~Node() {
        std::cout << "Node destroyed\n";
    }
}

int main() {
    Node node1;
    Node node2;
}
