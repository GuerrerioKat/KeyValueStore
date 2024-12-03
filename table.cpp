#include <cassert>
#include <iostream>
#include "table.h"
#include "exceptions.h"
#include "guard.h"

Table::Table( const std::string &name )
  : m_name( name )
{
  pthread_mutex_init(&m_mutex, nullptr);
}

Table::~Table()
{
  pthread_mutex_destroy(&m_mutex);
}

void Table::lock()
{
  pthread_mutex_lock(&m_mutex);
}

void Table::unlock()
{
  pthread_mutex_unlock(&m_mutex);
}

bool Table::trylock()
{
  return pthread_mutex_trylock(&m_mutex) == 0;
}

void Table::set( const std::string &key, const std::string &value )
{
  suggestions[key] = value;
}

std::string Table::get( const std::string &key )
{

  if (body.find(key) != body.end()) {
    return body[key];
  }

  if (suggestions.find(key) != suggestions.end()) {
    return suggestions[key];
  }

  return "";

}

bool Table::has_key( const std::string &key )
{
  return body.find(key) != body.end() || suggestions.find(key) != suggestions.end();
}

void Table::commit_changes()
{
  for (std::map<std::string, std::string>::iterator it = suggestions.begin(); it != suggestions.end(); ++it) {
    body[it->first] = it->second;
  }

  suggestions.clear();
}

void Table::rollback_changes()
{
  suggestions.clear();
}

void Table::print_contents()
{
  // Use Guard to lock the mutex (assuming Guard is properly implemented)
  Guard lock(m_mutex);

  std::cout << "Table: " << m_name << std::endl;

  // Print committed entries
  std::cout << "  Committed entries:" << std::endl;
  for (const auto& entry : body) {
    std::cout << "    Key: " << entry.first << ", Value: " << entry.second << std::endl;
  }

  // Print tentative (uncommitted) changes
  if (!suggestions.empty()) {
    std::cout << "  Tentative entries:" << std::endl;
    for (const auto& entry : suggestions) {
      std::cout << "    Key: " << entry.first << ", Value: " << entry.second << std::endl;
    }
  }
}