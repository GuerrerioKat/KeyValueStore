#include <iostream>
#include <cassert>
#include "csapp.h"
#include "message.h"
#include "message_serialization.h"
#include "server.h"
#include "exceptions.h"
#include "client_connection.h"

ClientConnection::ClientConnection( Server *server, int client_fd )
  : m_server( server )
  , m_client_fd( client_fd )
{
  rio_readinitb( &m_fdbuf, m_client_fd );
}

ClientConnection::~ClientConnection()
{
  // TODO: implement
  Close(m_client_fd);
}

void ClientConnection::chat_with_client()
{
  //TODO: implement
  char buf[MAXLINE];
  while (true) {
    ssize_t n = Rio_readlineb(&m_fdbuf, buf, MAXLINE);
    if (n <= 0) {
      break;
    }
    std::string request(buf, n);
    std::string response;

    try {
      if (request.substr(0, 13) == "CREATE TABLE ") {
        std::string table_name = request.substr(13);
        m_server->create_table(table_name);
        response = "Table created successfully\n";
      } else if (request.substr(0, 10) == "GET TABLE ") {
        std::string table_name = request.substr(10);
        Table *table = m_server->find_table(table_name);
        table->lock();
        response = "Table " + table_name + " found\n";
        table->unlock();
      } else {
        response = "ERROR: Invalid request\n";
      }
    } catch (const std::exception &e) {
      response = "ERROR: " + std::string(e.what()) + "\n";
    }

    Rio_writen(m_client_fd, response.c_str(), response.size());
  }
}

Server *ClientConnection::get_server() const {
  return m_server;
}

// TODO: additional member functions
