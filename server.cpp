#include <pthread.h>
#include <iostream>
#include <sstream>
#include <memory>
#include <set>
#include <vector>
#include <cctype>
#include <cassert>
#include "message.h"
#include "connection.h"
#include "user.h"
#include "room.h"
#include "guard.h"
#include "server.h"

////////////////////////////////////////////////////////////////////////
// Server implementation data types
////////////////////////////////////////////////////////////////////////

struct ClientInfo
{
  Connection *conn;
  Server *serv;

  ~ClientInfo()
  {
    delete conn;
  }
};

////////////////////////////////////////////////////////////////////////
// Client thread functions
////////////////////////////////////////////////////////////////////////

namespace
{

  void clientReceiverLoop() {

  }

  void clientSenderLoop() {

  }

  void *worker(void *arg)
  {
    pthread_detach(pthread_self());

    // use a static cast to convert arg from a void* to
    // whatever pointer type describes the object(s) needed
    // to communicate with a client (sender or receiver)
    ClientInfo* clientInfo = static_cast<ClientInfo*>(arg);

    // read login message (should be tagged either with
    // TAG_SLOGIN or TAG_RLOGIN), send response
    Message msg;

    if (!info->conn->receive(msg)) { //could not receive message
      if (info->conn->get_last_result() == Connection::INVALID_MSG) {
        info->conn->send(Message(TAG_ERR, "Invalid message"));
      }
      return nullptr;
    }

    int isReceiver = 0; //0 be false, 1 be true
    if(msg.tag == TAG_RLOGIN) {
      isReceiver == 1;
    } else if (msg.tag != TAG_SLOGIN) { //invalid tag
      info->conn->send(Message(TAG_ERR, "Invalid login attempt"));
      return nullptr;
    } //will be 0 if sender login
    
    // depending on whether the client logged in as a sender or
    // receiver, communicate with the client (implementing
    // separate helper functions for each of these possibilities
    // is a good idea)

    //get username to make user
    std::string username = msg.data;
    username = username.substr(0, username.size() -1);

    if (isReceiver == 1) {
      clientReceiverLoop();
    } else {
      clientSenderLoop();
    }

    return nullptr;
  }

}

////////////////////////////////////////////////////////////////////////
// Server member function implementation
////////////////////////////////////////////////////////////////////////

Server::Server(int port)
    : m_port(port), m_ssock(-1)
{
  // initialize mutex
  pthread_mutex_init(&m_lock, nullptr);
}

Server::~Server()
{
  // destroy mutex
  pthread_mutex_destroy(&m_lock);
}

bool Server::listen()
{
  // use open_listenfd to create the server socket
  // return true if successful, false if not
  std::string port_as_str = std::to_string(m_port);
  int result = open_listenfd(port_as_str.c_str());
  return result >= 0;
}

void Server::handle_client_requests()
{
  // infinite loop calling accept or Accept, starting a new
  // pthread for each connected client
  while (true)
  {
    int fd = Accept(m_ssock, nullptr, nullptr);
    if (fd == -1) {
      fprintf(stderr, "Error accepting connection\n");
      return;
    }

    ClientInfo *clientInfo = new ClientInfo();
    clientInfo->conn = new Connection(fd);
    clientInfo->serv = this;
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, worker, clientInfo) != 0) {
      fprintf(stderr, "pthread_create failed");
      return;
    }
  }
}

Room *Server::find_or_create_room(const std::string &room_name)
{
  // return a pointer to the unique Room object representing
  // the named chat room, creating a new one if necessary
  Guard g(m_lock);
  Room *room_ret;

  //if it doesn't exist, it will return end and make new room
  //otherwise return the room pointer
  RoomMap::iterator room_location = m_rooms.find(room_name);
  if (room_location == m_rooms.end()) {
    room_ret = new Room(room_name);
    m_rooms.insert({room_name, room_ret});
  } else {
    room_ret = room_location->second;
  }
  return room_ret;
}
