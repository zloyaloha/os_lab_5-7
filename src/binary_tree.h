#pragma once 
#include <iostream>
template <typename T>
class TNode {
    public:
        TNode(T data) : _data(data), _left(nullptr), _right(nullptr) {}
        T _data;
        TNode<T> *_left;
        TNode<T> *_right;
        void set_data(const T &data) {
            _data = data;
        }
};

template <typename T>
class BinaryTree {
    private:
        static void DestroyNode(TNode<T>* node) {
            if (node) {
                DestroyNode(node->_left);
                DestroyNode(node->_right);
                delete node;
            }
        }
    private:
        TNode<T> *_root;
    public:
        BinaryTree(T key) {
            _root = new TNode(key);
        }
        ~BinaryTree() { DestroyNode(_root); }
        void insert(T x) {
            TNode<T>** cur = &_root;
            while (*cur) {
                TNode<T>& node = **cur;
                if (x < node._data) {
                    cur = &node._left;
                } else if (x > node._data) {
                    cur = &node._right;
                } else {
                    return;
                }
            }
            *cur = new TNode(x);
        }
        std::pair<TNode<T>*, bool> find_insert(T key) {
            TNode<T> *curr = _root;
            TNode<T> *parent = nullptr;
            std::pair<TNode<T>*, bool> res{nullptr, true};
            while (curr && curr->_data != key) {
                parent = curr;
                if (curr->_data > key){
                    curr = curr->_left;
                }
                if (curr->_data < key) {
                    curr = curr->_right;
                } else {
                    return res;
                }
            }
            res.first = parent;
            if (parent->_data > key) {
                res.second = false;
            }
            return res;
        }
        void print() {
            print_tree(_root);
            std::cout << std::endl;
        }
        void print_tree(TNode<T> *curr) {
            if (curr) {
                print_tree(curr->_left);
                std::cout << curr->_data << " ";
                print_tree(curr->_right);
            }
        }
        TNode<T> *get_root() {
            return _root;
        }
        TNode<T> *find(T key) {
            TNode<T> *curr = _root;
            while (curr && curr->_data != key) {
                if (curr->_data > key)
                    curr = curr->_left;
                else
                    curr = curr->_right;
            }
            return curr;
        }
        void erase(T key) {
            TNode<T> * curr = _root;
            TNode<T> * parent = NULL;
            while (curr && curr->_data != key) {
                parent = curr;
                if (curr->_data > key) {
                    curr = curr->_left;
                } else {
                    curr = curr->_right;
                }
            }
            if (!curr) return;
            if (curr->_left == NULL) {
                if (parent && parent->_left == curr)
                    parent->_left = curr->_right;
                if (parent && parent->_right == curr)
                    parent->_right = curr->_right;
                delete curr;
                return;
            }
            if (curr->_right == NULL) {
                if (parent && parent->_left == curr)
                    parent->_left = curr->_left;
                if (parent && parent->_right == curr)
                    parent->_right = curr->_left;
                delete curr;
                return;
            }
            TNode<T> *replace = curr->_right;
            while (replace->_left)
                replace = replace->_left;
            int replace_value = replace->_data;
            this->erase(replace_value);
            curr->_data = replace_value;
        }
};