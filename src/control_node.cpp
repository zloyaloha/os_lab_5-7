#include <unistd.h>
#include <iostream>
#include <ctime>
#include "my_zmq.h"
#include "binary_tree.h"

using node_id_type = int;

int main() {
    BinaryTree<node_id_type> *control_node = new BinaryTree<node_id_type>(-1);
    std::string s;
    node_id_type id;
    std::pair<void *, void *> child;
    while (std::cin >> s >> id) {
        if (s == "create") {
            TNode<node_id_type> *node = control_node->find(id);
            if (node != nullptr) {
                std::cout << "Node with this id is already exist" << std::endl;
                continue;
            }
            if (control_node->get_root()->_data == -1) { // если это первый вычислительный узел.
                my_zmq::init_pair_socket(child.first, child.second);
                if (zmq_bind(child.second, ("tcp://*:" + std::to_string(PORT_BASE + id)).c_str()) != 0) {
                    perror("ZMQ_Bind");
                    exit(EXIT_FAILURE);
                }
                int pid = fork();
                if (pid == 0) { //son
                    execl(NODE_EXECUTABLE_NAME, NODE_EXECUTABLE_NAME, std::to_string(id).c_str(), nullptr);
                    perror("Execl");
                    exit(EXIT_FAILURE);
                } else if (pid > 0) { // parent
                    control_node->get_root()->set_data(id);
                } else { // error
                    perror("Fork");
                    exit(EXIT_FAILURE);
                }
            } else { // если это не первый вычислительный узел
                auto *msg = new msg_t({create, 0, id});
                msg_t reply = *msg;
                my_zmq::send_receive_wait(msg, reply, child.second);
                if (reply.action == success) {
                    control_node->insert(id);
                } else {
                    std::cout << "Error: Parent is unavailable" << std::endl;
                }
            }
        } else if (s == "remove") {
            TNode<node_id_type> *node = control_node->find(id);
            if (node != nullptr) {
                std::cout << "Node with this id is doesn't exist" << std::endl;
                continue;
            }
        }
        control_node->print();
    }
    std::cout << "Out tree:" << std::endl;
    control_node->print();
    auto *msg = new msg_t({destroy, -1, control_node->get_root()->_data});
    msg_t reply = *msg;
    my_zmq::send_receive_wait(msg, reply, child.second);
    zmq_close(child.second);
    zmq_ctx_destroy(child.first);
    return 0;
}