#include <iostream>

#include "ptr.h"


struct Node {
    int value = 1;
    Node() {
        std::cout << "Node created\n";
    }
    ~Node() {
        std::cout << "Node destroyed\n";
    }
};

int main() {
    stl::ptr<Node> node1 = stl::make_ptr<Node>();
    stl::ptr<Node> node2 = stl::make_ptr<Node>();
    stl::ptr<Node> node3 = nullptr;
    node->value;
}
