#include <utility>
#include <sstream>
#include <cassert>
#include <map>
#include <sstream>
#include <string>
#include <iostream>
#include "exceptions.h"
#include "message_serialization.h"

std::string message_type_to_string(MessageType type) {
  switch (type) {
    case MessageType::NONE: return "NONE";
    case MessageType::LOGIN: return "LOGIN";
    case MessageType::CREATE: return "CREATE";
    case MessageType::PUSH: return "PUSH";
    case MessageType::POP: return "POP";
    case MessageType::TOP: return "TOP";
    case MessageType::SET: return "SET";
    case MessageType::GET: return "GET";
    case MessageType::ADD: return "ADD";
    case MessageType::SUB: return "SUB";
    case MessageType::MUL: return "MUL";
    case MessageType::DIV: return "DIV";
    case MessageType::BEGIN: return "BEGIN";
    case MessageType::COMMIT: return "COMMIT";
    case MessageType::BYE: return "BYE";
    case MessageType::OK: return "OK";
    case MessageType::FAILED: return "FAILED";
    case MessageType::ERROR: return "ERROR";
    case MessageType::DATA: return "DATA";
    default: throw std::invalid_argument("Unknown MessageType");
  }
}

MessageType string_to_message_type(const std::string& str) {
    if (str == "NONE") return MessageType::NONE;
    if (str == "LOGIN") return MessageType::LOGIN;
    if (str == "CREATE") return MessageType::CREATE;
    if (str == "PUSH") return MessageType::PUSH;
    if (str == "POP") return MessageType::POP;
    if (str == "TOP") return MessageType::TOP;
    if (str == "SET") return MessageType::SET;
    if (str == "GET") return MessageType::GET;
    if (str == "ADD") return MessageType::ADD;
    if (str == "SUB") return MessageType::SUB;
    if (str == "MUL") return MessageType::MUL;
    if (str == "DIV") return MessageType::DIV;
    if (str == "BEGIN") return MessageType::BEGIN;
    if (str == "COMMIT") return MessageType::COMMIT;
    if (str == "BYE") return MessageType::BYE;
    if (str == "OK") return MessageType::OK;
    if (str == "FAILED") return MessageType::FAILED;
    if (str == "ERROR") return MessageType::ERROR;
    if (str == "DATA") return MessageType::DATA;
    throw std::invalid_argument("Unknown MessageType"); // Throw exception for unknown types
}

void MessageSerialization::encode( const Message &msg, std::string &encoded_msg )
{
  ;
  MessageType message_type = msg.get_message_type();
  std::string message = message_type_to_string(message_type);
  
  int num_args = msg.get_num_args();
  for (int i = 0; i < num_args; i++) {
      message += " " + msg.get_arg(i);
  }

  message += '\n';
  encoded_msg = message;
  
  if (encoded_msg.length() > Message::MAX_ENCODED_LEN) {
    throw InvalidMessage("Message is too long");
  }
}

void MessageSerialization::decode(const std::string &encoded_msg_, Message &msg) {
    if (encoded_msg_.length() > Message::MAX_ENCODED_LEN || encoded_msg_.back() != '\n' || encoded_msg_.empty()) {
        throw InvalidMessage("Encoded message is invalid");
    }

    std::string encoded_msg = encoded_msg_;
    encoded_msg.pop_back();

    std::stringstream ss(encoded_msg);

    std::string message_type_str;
    ss >> message_type_str;

    if (message_type_str.empty()) {
        throw InvalidMessage("Message type is missing");
    }

    MessageType message_type;
    try {
        message_type = string_to_message_type(message_type_str);
    } catch (const std::invalid_argument &) {
        throw InvalidMessage("Invalid message type");
    }

    Message message;
    message.set_message_type(message_type);

    std::string arguments;
    std::getline(ss, arguments);

    std::vector<std::string> args;
    size_t i = 0;
    while (i < arguments.length()) {
        while (i < arguments.length() && std::isspace(arguments[i])) {
            ++i;
        }
        if (i >= arguments.length()) break;

        if (arguments[i] == '"') { //quoted text
            size_t start = i++;
            while (i < arguments.length() && arguments[i] != '"') {
                ++i;
            }
            if (i >= arguments.length() || arguments[i] != '"') {
                throw InvalidMessage("Unterminated quoted text");
            }
            ++i; //include closing quote
            args.push_back(arguments.substr(start + 1, i - start - 2)); //exclude quotes
        } else { //unquoted text
            size_t start = i;
            while (i < arguments.length() && !std::isspace(arguments[i])) {
                ++i;
            }
            args.push_back(arguments.substr(start, i - start));
        }
    }

    for (const std::string &arg : args) {
        message.push_arg(arg);
    }

    if (!message.is_valid()) {
        throw InvalidMessage("Created message is not valid");
    }

    msg = message;
}

