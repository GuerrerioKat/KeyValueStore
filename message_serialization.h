#ifndef MESSAGE_SERIALIZATION_H
#define MESSAGE_SERIALIZATION_H

#include "message.h"

namespace MessageSerialization {
  void encode(const Message &msg, std::string &encoded_msg);
  void decode(const std::string &encoded_msg, Message &msg);
};

std::string message_type_to_string(MessageType type);
MessageType string_to_message_type(const std::string& str);

#endif // MESSAGE_SERIALIZATION_H
