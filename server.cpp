#include <iostream>
#include <cassert>
#include <memory>
#include <iostream>
#include "csapp.h"
#include "exceptions.h"
#include "guard.h"
#include "server.h"
#include "client_connection.h"

Server::Server() : listen_fd(-1) {}

Server::~Server()
{
  if (listen_fd >= 0) {
    Close(listen_fd);
  }
}

void Server::listen(const std::string &port) {
  listen_fd = Open_listenfd(port.c_str());
  if (listen_fd < 0) {
    throw std::runtime_error("Failed to open listening socket on port: " + port);
  }
}

void Server::server_loop()
{
  while (true) {
    struct sockaddr_storage client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd = Accept(listen_fd, (struct sockaddr *) &client_addr, &client_len);

    try {
      ClientConnection *client = new ClientConnection(this, client_fd);
      pthread_t thr_id;

      if (pthread_create(&thr_id, nullptr, client_worker, client) != 0) {
        log_error("Could not create client thread");
        delete client;
      } else {
        pthread_detach(thr_id);
      }
    } catch (const std::exception &e) {
      log_error(e.what());
      Close(client_fd);
    }
  }
  // Note that your code to start a worker thread for a newly-connected
  // client might look something like this:
/*
  ClientConnection *client = new ClientConnection( this, client_fd );
  pthread_t thr_id;
  if ( pthread_create( &thr_id, nullptr, client_worker, client ) != 0 )
    log_error( "Could not create client thread" );
*/
}


void *Server::client_worker( void *arg )
{
  std::unique_ptr<ClientConnection> client(static_cast<ClientConnection *>(arg));

  try {
    client->chat_with_client();
  } catch (const std::exception &e) {
    client->get_server()->log_error("Client thread error: " + std::string(e.what()));
  }
  return nullptr;
  // Assuming that your ClientConnection class has a member function
  // called chat_with_client(), your implementation might look something
  // like this:
/*
  std::unique_ptr<ClientConnection> client( static_cast<ClientConnection *>( arg ) );
  client->chat_with_client();
  return nullptr;
*/
}

void Server::log_error( const std::string &what )
{
  std::lock_guard<std::mutex> lock(log_mutex); // not sure if this should be here lol
  // also where the heck do I do unlocking again
  std::cerr << "Error: " << what << "\n";
}

// TODO: implement member functions
void Server::create_table(const std::string &name) {
  std::lock_guard<std::mutex> lock(tables_mutex);
  if (tables.find(name) != tables.end()) {
    throw std::runtime_error("This table already exists: " + name);
  }
  tables[name] = std::make_unique<Table>(name);
  print_tables();
}

Table *Server::find_table(const std::string &name) {
  std::lock_guard<std::mutex> lock(tables_mutex);
  auto it = tables.find(name);
  if (it == tables.end()) {
    throw std::runtime_error("This table could not be found: " + name);
  }
  print_tables();
  return it->second.get();
}

void Server::print_tables()
{
  std::lock_guard<std::mutex> lock(tables_mutex);

  std::cout << "Server Tables:" << std::endl;
  for (const auto& table_entry : tables) {
    // table_entry.first is the table name (key in the map)
    // table_entry.second is a unique_ptr<Table>
    Table* table = table_entry.second.get();
    table->print_contents();
  }
}