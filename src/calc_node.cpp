#include "my_zmq.h"
#include <iostream>
#include <map>
#include <unistd.h>

long long node_id;

int main(int argc, char **argv) {
    if (argc != 2) {
        exit(EXIT_FAILURE);
    }
    node_id = std::stoll(std::string(argv[1]));
    long long left_id = -1, right_id = -1;
    void *node_parent_context = zmq_ctx_new();
    void *node_parent_socket = zmq_socket(node_parent_context, ZMQ_PAIR);
    if (zmq_connect(node_parent_socket, ("tcp://localhost:" + std::to_string(PORT_BASE + node_id)).c_str()) != 0) {
        perror("ZMQ_Connect");
        exit(EXIT_FAILURE);
    }
    std::pair<void *, void *> left, right; // <context, socket>
    std::cout << "OK: " << getpid() << std::endl;
    bool has_left = false, has_right = false, awake = true;
    while (awake) {
        msg_t token({fail, 0, 0});
	    my_zmq::receive_msg(token, node_parent_socket);
	    auto *reply = new msg_t({fail, node_id, node_id});
        if (token.action == create) {
            std::cout << "some info " << token.parent_id << ' ' << node_id << ' ' << token.id << std::endl;
            if (token.parent_id = node_id) {
                if (node_id < token.id) {
                    my_zmq::init_pair_socket(right.first, right.second);
                    if (zmq_bind(right.second, ("tcp://*:" + std::to_string(PORT_BASE + token.id)).c_str()) != 0) {
                        perror("Bind");
                        exit(EXIT_FAILURE);
                    }
                    int fork_id = fork();
                    if (fork_id == 0) {
                        execl(NODE_EXECUTABLE_NAME, NODE_EXECUTABLE_NAME, std::to_string(token.id).c_str(), nullptr);
                        perror("Execl");
                        exit(EXIT_FAILURE);
                    } else if (fork_id > 0) {
                        has_right = true;
                        right_id = token.id;
                        reply->action = success;
                    } else {
                        perror("Fork");
                        exit(EXIT_FAILURE);
                    }
                } else if (node_id > token.id) {
                    my_zmq::init_pair_socket(left.first, left.second);
                    if (zmq_bind(left.second, ("tcp://*:" + std::to_string(PORT_BASE + token.id)).c_str()) != 0) {
                        perror("Bind");
                        exit(EXIT_FAILURE);
                    }
                    int fork_id = fork();
                    if (fork_id == 0) {
                        execl(NODE_EXECUTABLE_NAME, NODE_EXECUTABLE_NAME, std::to_string(token.id).c_str(), nullptr);
                        perror("Execl");
                        exit(EXIT_FAILURE);
                    } else if (fork_id > 0) {
                        has_left = true;
                        left_id = token.id;
                        reply->action = success;
                    } else {
                        perror("Fork");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    perror("What the fuck create calc node");
                    exit(EXIT_FAILURE);
                }
            } else {
                auto *msg_to_descendants = new msg_t(token);
                msg_to_descendants->parent_id = node_id;
                msg_t reply_from_descendants = *reply;
                if (token.id > node_id) {
                    my_zmq::send_receive_wait(msg_to_descendants, reply_from_descendants, right.second);
                } else {
                    my_zmq::send_receive_wait(msg_to_descendants, reply_from_descendants, left.second);
                }
                if (reply_from_descendants.action == success) {
                    *reply = reply_from_descendants;
                }
            }
        }
        // } else if (token.action == destroy) {
            
        // }
        my_zmq::send_msg_no_wait(reply, node_parent_socket);
    }
}