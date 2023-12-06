#include <iostream>
#include <zmq.hpp>
#include "binary_tree.h"

int main() {
    BinaryTree<int> tree(5);
    tree.insert(4);
    tree.insert(7);
    tree.insert(6);
    tree.print();
    tree.erase(6);
    tree.print();
    std::cout << tree.find(4);
}