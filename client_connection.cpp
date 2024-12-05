#include <iostream>
#include <cassert>
#include <stack>
#include <sstream>
#include "csapp.h"
#include "message.h"
#include "message_serialization.h"
#include "server.h"
#include "exceptions.h"
#include "client_connection.h"

//Return values of all calls to I/O functions must be checked!!!

ClientConnection::ClientConnection( Server *server, int client_fd )
  : m_server( server )
  , m_client_fd( client_fd )
{
  rio_readinitb( &m_fdbuf, m_client_fd );
}

ClientConnection::~ClientConnection()
{
  Close(m_client_fd);
}

void ClientConnection::chat_with_client() {
    char buf[MAXLINE];
    int num_requests = 0;
    while (true) {
        ssize_t n = rio_readlineb(&m_fdbuf, buf, MAXLINE);
        if (n <= 0) {
            break;
        }
        std::string request(buf, n);
        std::string response;
        Table *table = nullptr;
        try {
            Message request_msg;
            MessageSerialization::decode(request, request_msg);
            MessageType message_type = request_msg.get_message_type();
            std::string command = message_type_to_string(message_type);

            if (num_requests == 0 && command != "LOGIN") {
                Message msg(MessageType::ERROR, {"First command must be a LOGIN"});
                MessageSerialization::encode(msg, response);
                rio_writen(m_client_fd, response.c_str(), response.size()); //throw CommException
                break;
            }

            num_requests++;

            if (command == "LOGIN") {
                //do we have to do anything here?
                Message msg(MessageType::OK, {});
                MessageSerialization::encode(msg, response);
            } else if (command == "CREATE") {
                std::string table_name = request_msg.get_table();
                m_server->create_table(table_name);
                Message msg(MessageType::OK, {});
                MessageSerialization::encode(msg, response);
            } else if (command == "GET") {
                std::string table_name = request_msg.get_table();
                std::string key = request_msg.get_key();
                table = m_server->find_table(table_name);

                lock_table(table);

                if (!table->has_key(key)) {
                    throw OperationException("Key does not exist: " + key);
                }
                
                //how are we supposed to handle non-integer arithmetic? stack is currenlty only set to handle integers
                std::string s_value = table->get(key);
                int value;
                try {
                    value = std::stoi(s_value);
                } catch (const std::invalid_argument&) {
                    unlock_table(table);
                    throw OperationException("Value is not a valid integer: " + s_value);
                } catch (const std::out_of_range&) {
                    unlock_table(table);
                    throw OperationException("Value is out of range: " + s_value);
                }

                stack.push(value);

                unlock_table(table); //unlock the table if not in transaction

                Message msg(MessageType::OK, {});
                MessageSerialization::encode(msg, response);
            } else if (command == "SET") {
                std::string table_name = request_msg.get_table();
                std::string key = request_msg.get_key();
                table = m_server->find_table(table_name);

                lock_table(table);

                if (stack.empty()) {
                    throw OperationException("Operand stack is empty");
                }

                int value = stack.top();
                stack.pop();
                std::string s_value = std::to_string(value);

                table->set(key, s_value);

                //commit changes and unlock the table if not in transaction
                if (!in_transaction) {
                    table->commit_changes();
                    unlock_table(table);
                }

                Message msg(MessageType::OK, {});
                MessageSerialization::encode(msg, response);
            } else if (command == "PUSH") {
                std::string s_value = request_msg.get_value();
                int value;
                try {
                    value = std::stoi(s_value);
                } catch (const std::invalid_argument&) {
                    throw OperationException("PUSH argument is not a valid integer: " + s_value);
                } catch (const std::out_of_range&) {
                    throw OperationException("PUSH argument is out of range: " + s_value);
                }
                stack.push(value);
                Message msg(MessageType::OK, {});
                MessageSerialization::encode(msg, response);
            } else if (command == "POP") {
                if (stack.empty()) {
                    throw OperationException("Operand stack is empty");
                }
                stack.pop();
                Message msg(MessageType::OK, {});
                MessageSerialization::encode(msg, response);
            } else if (command == "TOP") {
                if (stack.empty()) {
                    throw OperationException("Operand stack is empty");
                }
                int value = stack.top();
                Message msg(MessageType::DATA, {std::to_string(value)});
                MessageSerialization::encode(msg, response);
            } else if (command == "ADD" || command == "SUB" ||
                       command == "MUL" || command == "DIV") {
                handle_arithmetic_operation(command);
                Message msg(MessageType::OK, {});
                MessageSerialization::encode(msg, response);
            } else if (command == "BEGIN") {
                if (in_transaction) {
                    throw FailedTransaction("Nested transactions are not allowed");
                }
                in_transaction = true;
                locked_tables.clear();
                Message msg(MessageType::OK, {});
                MessageSerialization::encode(msg, response);
            } else if (command == "COMMIT") {
                if (!in_transaction) {
                    throw OperationException("No transaction in progress");
                }
                //commit changes to all locked tables and unlock them
                for (Table* t : locked_tables) {
                    t->commit_changes();
                    t->unlock();
                }
                locked_tables.clear();
                in_transaction = false;
                Message msg(MessageType::OK, {});
                MessageSerialization::encode(msg, response);
            } else if (command == "BYE") {
                if (in_transaction) {
                    rollback_transaction(); //if no commit, rollback
                }
                Message msg(MessageType::OK, {});
                MessageSerialization::encode(msg, response);
                rio_writen(m_client_fd, response.c_str(), response.size());
                break; //end connection
            } else {
                throw InvalidMessage("Invalid command: " + command);
            }

            rio_writen(m_client_fd, response.c_str(), response.size());
            num_requests++;
        } catch (const InvalidMessage &e) {
            Message msg(MessageType::ERROR, {e.what()});
            MessageSerialization::encode(msg, response);
            rio_writen(m_client_fd, response.c_str(), response.size());
            break; //doesn't coninue on an error
        } catch (const FailedTransaction &e) {
            rollback_transaction();
            Message msg(MessageType::FAILED, {e.what()});
            MessageSerialization::encode(msg, response);
            Rio_writen(m_client_fd, response.c_str(), response.size());
        } catch (const OperationException &e) {
            Message msg(MessageType::FAILED, {e.what()});
            MessageSerialization::encode(msg, response);
            rio_writen(m_client_fd, response.c_str(), response.size());
        } catch (const std::exception &e) {
            rollback_transaction();

            Message msg(MessageType::ERROR, {e.what()});
            MessageSerialization::encode(msg, response);
            rio_writen(m_client_fd, response.c_str(), response.size());
            break; //doesn't coninue on an error
        } //catch CommException and release any held resources and end the thread
    }
}

//values are supposed to be strings, only need to integers here
void ClientConnection::handle_arithmetic_operation(const std::string& operation) {
    if (stack.size() < 2) {
        throw OperationException("Not enough operands on stack");
    }
    int value1 = stack.top();
    stack.pop();
    int value2 = stack.top();
    stack.pop();

    int result;
    if (operation == "ADD") {
        result = value1 + value2;
    } else if (operation == "SUB") {
        result = value2 - value1;
    } else if (operation == "MUL") {
        result = value1 * value2;
    } else if (operation == "DIV") {
        if (value1 == 0) {
            throw OperationException("Division by zero");
        }
        result = value2 / value1;
    } else {
        throw OperationException("Unknown operation");
    }

    stack.push(result);
}

void ClientConnection::lock_table(Table* table) {
    if (in_transaction) {
        // Try to lock the table if not already locked
        if (locked_tables.find(table) == locked_tables.end()) {
            if (!table->trylock()) {
                throw FailedTransaction("Could not acquire lock on table " + table->get_name());
            }
            locked_tables.insert(table);
        }
    } else {
        // Autocommit mode
        table->lock();
    }
}

void ClientConnection::unlock_table(Table* table) {
    if (!in_transaction) {
        table->unlock();
    }
    // In transaction mode, tables remain locked until COMMIT or ROLLBACK
}

void ClientConnection::rollback_transaction() {
    if (in_transaction) {
        for (Table* t : locked_tables) {
            t->rollback_changes();
            t->unlock();
        }
        locked_tables.clear();
        in_transaction = false;
    }
}

Server* ClientConnection::get_server() const {
    return m_server;
}