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

int main(int argc, char **argv)
{
  if (argc != 7) {
    std::cerr << "Usage: ./set_value <hostname> <port> <username> <table> <key> <value>\n";
    return 1;
  }

  std::string hostname = argv[1];
  std::string port = argv[2];
  std::string username = argv[3];
  std::string table = argv[4];
  std::string key = argv[5];
  std::string value = argv[6];

  int clientfd = open_clientfd(hostname.c_str(), port.c_str());
  if (clientfd > 0) {
    Message msg_login(MessageType::LOGIN, {username});
    Message msg_push(MessageType::PUSH, {value});
    Message msg_set(MessageType::SET, {table, key});
    Message msg_bye(MessageType::BYE, {});
    
    std::vector<Message> server_requests = {msg_login, msg_push, msg_set, msg_bye};

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
  }
  return 0;
}
