#include <set>
#include <map>
#include <regex>
#include <cassert>
#include <iostream>
#include "message.h"

Message::Message()
  : m_message_type(MessageType::NONE)
{
}

Message::Message( MessageType message_type, std::initializer_list<std::string> args )
  : m_message_type( message_type )
  , m_args( args )
{
}

Message::Message( const Message &other )
  : m_message_type( other.m_message_type )
  , m_args( other.m_args )
{
}

Message::~Message()
{
}

Message &Message::operator=( const Message &rhs )
{
  m_message_type = rhs.m_message_type;
  m_args = rhs.m_args;
  return *this;
}

MessageType Message::get_message_type() const
{
  return m_message_type;
}

void Message::set_message_type(MessageType message_type)
{
  m_message_type = message_type;
}

std::string Message::get_username() const
{
  if (m_message_type == MessageType::LOGIN) {
    return m_args[0];
  }
  return "";
}

std::string Message::get_table() const
{
  if (m_message_type == MessageType::CREATE || m_message_type == MessageType::SET || m_message_type == MessageType::GET) {
    return m_args[0];
  }
  return "";
}

std::string Message::get_key() const
{
  if (m_message_type == MessageType::SET || m_message_type == MessageType::GET) {
    return m_args[1];
  }
  return "";
}

std::string Message::get_value() const
{
  if (m_message_type == MessageType::PUSH || m_message_type == MessageType::DATA) {
    return m_args[0];
  }
  return "";
}

std::string Message::get_quoted_text() const
{
  if (m_message_type == MessageType::FAILED || m_message_type == MessageType::ERROR) {
    return m_args[0];
  }
  return "";
}

void Message::push_arg( const std::string &arg )
{
  m_args.push_back( arg );
}

bool Message::is_valid() const
{
  //checking num_arguments

  if (m_message_type == MessageType::DATA || m_message_type == MessageType::PUSH || m_message_type == MessageType::LOGIN || m_message_type == MessageType::CREATE || m_message_type == MessageType::FAILED || m_message_type == MessageType::ERROR) {
    if(m_args.size() != 1) {
      return false;
    }
  } else if (m_message_type == MessageType::GET || m_message_type == MessageType::SET) {
    if(m_args.size() != 2) {
      return false;
    }
  } else {
    return m_args.empty();
  }
  
  //checking text valididty
  if (m_message_type == MessageType::LOGIN || m_message_type == MessageType::CREATE || m_message_type == MessageType::GET || m_message_type == MessageType::SET) {
    for (std::string arg : m_args) {
      if(!std::isalpha(arg[0])) {
        return false;
      }
      
      for (size_t i = 1; i < arg.size(); i++) {
        if (!std::isalnum(arg[i]) && arg[i] != '_') {
          return false;
        }
      }
    }
  } else if (m_message_type == MessageType::DATA || m_message_type == MessageType::PUSH) { // value
    for (char c : m_args[0]) {
      if (std::isspace(c)) {
        return false;
      }
    }
  } else if (m_message_type == MessageType::FAILED || m_message_type == MessageType::ERROR) { // quoted_text
    for (size_t i = 0; i < m_args[0].length(); i++) {
      if (m_args[0][i] == '"') { // 0+ non-quote characters
        return false;
      }
    }
  } 

  return true;
}
