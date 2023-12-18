#include <unistd.h>
#include <iostream>
#include <ctime>
#include "my_zmq.h"
#include "binary_tree.h"

using node_id_type = int;

void tokenize(std::vector<int> &args, const std::string &input) {
    std::stringstream ss(input);
    std::string s;
    while (std::getline(ss, s, ' ')) {
        args.push_back(std::stoi(s));
    }
}

int main() {
    BinaryTree<node_id_type> *control_node = new BinaryTree<node_id_type>(-1);
    std::string s;
    node_id_type id;
    std::pair<void *, void *> child;
    long long child_id = -1;
    while (std::cin >> s >> id) {
        if (s == "create") {
            TNode<node_id_type> *node = control_node->find(id);
            if (node != nullptr) {
                std::cout << "Node with this id is already exist" << std::endl;
                continue;
            }
            if (control_node->get_root()->_data == -1) { // если это первый вычислительный узел.
                my_zmq::init_pair_socket(child.first, child.second);
                child_id = id;
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
            control_node->print();
        } else if (s == "heartbit") {
            auto *msg = new msg_t({ping, 0, id});
            msg_t reply = *msg;
            const int wait = 1000 * 4 * id;
            int counter = 0;
            zmq_setsockopt(child.second, ZMQ_RCVTIMEO, &wait, sizeof(int));
            my_zmq::send_msg_wait(msg, child.second);
            while (true) {
                if (counter == 3) {
                    break;
                }
                bool flag = my_zmq::recv_wait_for_time(reply, child.second);
                if (reply.action == success && flag) {
                    std::cout << "ok: " << reply.id << std::endl;
                    counter++;
                } else {
                    std::cout << "unbelievable but root node is unavailable: " << child_id <<std::endl;
                    counter++;
                }
            }
            zmq_setsockopt(child.second, ZMQ_RCVTIMEO, &WAIT_TIME, sizeof(int));
        } else if (s == "exec") {
            auto *terminate_msg = new msg_t({exec_add, -1, id});
            std::vector<int> buf;
            std::string num;
            while(true) {
                std::cin >> num;
                if (num == "!") {
                    break;
                }
                buf.push_back(std::stoi(num));
            }
            for (int num: buf){
                auto *msg_to_child = new msg_t({exec_add, num, id});
                my_zmq::send_msg_wait(msg_to_child, child.second);
            }
            my_zmq::send_msg_wait(terminate_msg, child.second);
        } else if (s == "remove") {
            std::pair<TNode<int> *, bool> res = control_node->find_insert(id);
            if (!res.second) {
                std::cout << "Error: Node with id " << id << " doesn't exists" << std::endl;
                continue;
            }
            auto *msg = new msg_t({destroy, -1, id});
            if (res.first != nullptr) {
                auto *msg = new msg_t({destroy, res.first->_data, id});
            }
            msg_t reply = *msg;
            my_zmq::send_receive_wait(msg, reply, child.second);
            if (reply.action == success) {
                std::cout << "Node " << id << " was deleted" << std::endl;
                control_node->deleteSubtree(control_node->get_root(), id);
                control_node->print();
            } else {
                std::cout << "Parent is unavailable" << std::endl;
            }
        } else {
            std::cout << "what";
            continue;
        }
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