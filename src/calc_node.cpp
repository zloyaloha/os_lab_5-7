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
    int sum = 0;
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
            msg_t ping_left = msg_t({fail, node_id, node_id}), ping_right = msg_t({fail, node_id, node_id});
            auto *msg_to_parent = new msg_t({success, 0, node_id});
            const int wait = 1000 * token.id * 4;
            for (int i = 0; i < 3; i++) {
                my_zmq::send_msg_wait(msg_to_parent, node_parent_socket);
                sleep(token.id);
            }
            if (has_left) {
                int counter = 0;
                auto *token_left = new msg_t({ping, node_id, token.id});
                zmq_setsockopt(left.second, ZMQ_RCVTIMEO, &wait, sizeof(int));
                my_zmq::send_msg_wait(token_left, left.second);
                while (true) {
                    if (counter == 3) {
                        break;
                    }
                    bool flag_l = my_zmq::recv_wait_for_time(ping_left, left.second);
                    if (ping_left.action == success && flag_l) {
                        std::cout << "ok: " << ping_left.id << std::endl;
                        counter++;
                    } else {
                        std::cout << "unbelievable but left node is unavailable: " << left_id <<std::endl;
                    }
                }
            }
            if (has_right) {
                int counter = 0;
                auto *token_right = new msg_t({ping, node_id, token.id});
                zmq_setsockopt(right.second, ZMQ_RCVTIMEO, &wait, sizeof(int));
                my_zmq::send_msg_wait(token_right, right.second);
                while (true) {
                    if (counter == 3) {
                        break;
                    }
                    bool flag_r = my_zmq::recv_wait_for_time(ping_right, right.second);
                    if (ping_right.action == success && flag_r) {
                        std::cout << "ok: " << ping_right.id << std::endl;
                        counter++;
                    } else {
                        std::cout << "unbelievable but right node is unavailable: " << right_id <<std::endl;
                    }
                }
            }
        } else if (token.action == exec_add) {
            if (node_id == token.id) {
                if (token.parent_id == -1) {
                    std::cout << "Summary equal to " << sum << std::endl;
                    sum = 0;
                    continue;
                }
                sum += token.parent_id;
            }
            if (node_id > token.id && has_left) {
                auto *token_left = new msg_t({exec_add, token.parent_id, token.id});
                msg_t reply_left = *reply;
                my_zmq::send_msg_no_wait(token_left, left.second);
            } else if (node_id < token.id && has_right) {
                auto *token_right = new msg_t({exec_add, token.parent_id, token.id});
                msg_t reply_right = *reply;
                my_zmq::send_msg_no_wait(token_right, right.second);
            }
        }
    }
}