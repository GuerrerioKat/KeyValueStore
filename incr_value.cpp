#include <iostream>
#include <vector>
#include "csapp.h"
#include "message_serialization.h"
#include "message.h"
#include "exceptions.h"

void send_request(Message msg, int clientfd) {
  std::string encoded_message;
  MessageSerialization::encode(msg, encoded_message);

  int rc = rio_writen(clientfd, encoded_message.data(), encoded_message.size());

  if (rc <= 0) {
    throw CommException("Error in sending request to server");
  }
}

std::string recieve_response(rio_t &rio) {
  char response[Message::MAX_ENCODED_LEN];
  ssize_t rc = rio_readlineb(&rio, response, sizeof(response));

  if (rc <= 0) {
    throw CommException("Error in recieving request from server");
  }
  return response;
}

int main(int argc, char **argv) {
  if ( argc != 6 && (argc != 7 || std::string(argv[1]) != "-t") ) {
    std::cerr << "Usage: ./incr_value [-t] <hostname> <port> <username> <table> <key>\n";
    std::cerr << "Options:\n";
    std::cerr << "  -t      execute the increment as a transaction\n";
    return 1;
  }

  int count = 1;

  bool use_transaction = false;
  if ( argc == 7 ) {
    use_transaction = true;
    count = 2;
  }

  std::string hostname = argv[count++];
  std::string port = argv[count++];
  std::string username = argv[count++];
  std::string table = argv[count++];
  std::string key = argv[count++];

  int clientfd = open_clientfd(hostname.c_str(), port.c_str());
  if (clientfd > 0) {
    std::vector<Message> server_requests;

    Message msg_login(MessageType::LOGIN, {username});
    server_requests.push_back(msg_login);

    if (use_transaction) {
      Message msg_begin(MessageType::BEGIN, {});
      server_requests.push_back(msg_begin);
    }
    
    Message msg_get(MessageType::GET, {table, key});
    server_requests.push_back(msg_get);
    
    Message msg_push(MessageType::PUSH, {"1"});
    server_requests.push_back(msg_push);

    Message msg_add(MessageType::ADD, {});
    server_requests.push_back(msg_add);

    Message msg_set(MessageType::SET, {table, key});
    server_requests.push_back(msg_set);

    if (use_transaction) {
      Message msg_commit(MessageType::COMMIT, {});
      server_requests.push_back(msg_commit);
    }

    Message msg_bye(MessageType::BYE, {});
    server_requests.push_back(msg_bye);

    rio_t rio;
    rio_readinitb(&rio, clientfd);
    for (Message msg : server_requests) {
      std::string response;
      try {
        send_request(msg, clientfd);
        response = recieve_response(rio);
      } catch (const CommException& e) {
        std::cerr << "Error: " << "Error communicating with server" << std::endl;
        return 3;
      }
      
      
      Message response_msg;
      MessageSerialization::decode(response, response_msg);
      if (response_msg.get_message_type() == MessageType::FAILED) {
        std::cerr << "Error: " << response_msg.get_quoted_text() << std::endl;
        return 1;
      } else if (response_msg.get_message_type() == MessageType::ERROR) {
        std::cerr << "Error: " << response_msg.get_quoted_text() << std::endl;
        return 2;
      } else if (response_msg.get_message_type() == MessageType::DATA) {
        std::cout << response_msg.get_value() << std::endl;
      }
    }

    return 0;
  }
}