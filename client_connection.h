#ifndef CLIENT_CONNECTION_H
#define CLIENT_CONNECTION_H

#include <set>
#include <stack>
#include "message.h"
#include "csapp.h"

class Server; // forward declaration
class Table; // forward declaration

class ClientConnection {
  private:
    void lock_table(Table* table);
    void unlock_table(Table* table);
    void rollback_transaction();
    void handle_arithmetic_operation(const std::string& operation);

    Server *m_server;
    int m_client_fd;
    rio_t m_fdbuf;
    std::stack<int> stack;
    bool in_transaction = false;
    std::set<Table*> locked_tables;

  // copy constructor and assignment operator are prohibited
  ClientConnection( const ClientConnection & );
  ClientConnection &operator=( const ClientConnection & );

  public:
    ClientConnection( Server *server, int client_fd );
    ~ClientConnection();

    void chat_with_client();

    // TODO: additional member functions
    Server* get_server() const;
    std::vector<std::string> splitString(const std::string& str);
};

#endif // CLIENT_CONNECTION_H
