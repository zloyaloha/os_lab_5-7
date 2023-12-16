#include "my_zmq.h"
#include <iostream>
#include <map>
#include <unistd.h>

long long node_id;

void make_node(std::pair<void *, void *> &contex_socket, bool &flag, long long &id, msg_t &token, msg_t *reply) {
    my_zmq::init_pair_socket(contex_socket.first, contex_socket.second);
    if (zmq_bind(contex_socket.second, ("tcp://*:" + std::to_string(PORT_BASE + token.id)).c_str()) != 0) {
        perror("Bind");
        exit(EXIT_FAILURE);
    }
    int fork_id = fork();
    if (fork_id == 0) {
        execl(NODE_EXECUTABLE_NAME, NODE_EXECUTABLE_NAME, std::to_string(token.id).c_str(), nullptr);
        perror("Execl");
        exit(EXIT_FAILURE);
    } else if (fork_id > 0) {
        flag = true;
        id = token.id;
        reply->action = success;
    } else {
        perror("Fork");
        exit(EXIT_FAILURE);
    }
}

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
            if (node_id > token.id && has_left) {
                std::cout << "Go left in " << node_id << std::endl;
                auto *token_left = new msg_t(token);
                msg_t reply_left = *reply;
                my_zmq::send_receive_wait(token_left, reply_left, left.second);
                if (reply_left.action == success) {
                    *reply = reply_left;
                }
            } else if (node_id < token.id && has_right) {
                std::cout << "Go right in " << node_id << std::endl;
                auto *token_right = new msg_t(token);
                msg_t reply_right = *reply;
                my_zmq::send_receive_wait(token_right, reply_right, right.second);
                if (reply_right.action == success) {
                    *reply = reply_right;
                }   
            }
            if (has_left == false && node_id > token.id) {
                std::cout << "Making left in " << node_id << std::endl;
                make_node(left, has_left, token.id, token, reply);
            }
            if (has_right == false && node_id < token.id) {
                std::cout << "Making right in "<< node_id << std::endl;
                make_node(right, has_right, token.id, token, reply);
            }
            my_zmq::send_msg_no_wait(reply, node_parent_socket);
        } else if (token.action == ping) {
            // std::cout << std::endl;
            // std::cout << "Hi i am: " << node_id << std::endl;
            msg_t ping_left = msg_t({fail, node_id, node_id}), ping_right = msg_t({fail, node_id, node_id});
            auto *msg_to_parent = new msg_t({success, 0, node_id});
            my_zmq::send_msg_no_wait(msg_to_parent, node_parent_socket);
            if (has_left) {
                std::cout << "Go left in " << node_id << std::endl;
                auto *token_left = new msg_t({ping, node_id, token.id});
                my_zmq::send_msg_no_wait(token_left, left.second);
            }
            if (has_right) {
                std::cout << "Go right in " << node_id << std::endl;
                auto *token_right = new msg_t({ping, node_id, token.id});
                my_zmq::send_msg_no_wait(token_right, right.second);
            }
            if (has_left) {
                while (my_zmq::test_recv(ping_left, left.second)) {
                    std::cout << "ok" << ping_left.id;         
                    // my_zmq::send_msg_no_wait(&ping_left, node_parent_socket);
                }
            }
            if (has_right) {
                while (my_zmq::test_recv(ping_right, right.second)) {   
                    // my_zmq::send_msg_no_wait(&ping_right, node_parent_socket);
                }
            }
            // std::cout << std::endl;
        }
    }
}