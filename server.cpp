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
// Helper functions
////////////////////////////////////////////////////////////////////////

//Removes the trailing newline from a string if it exists
//Doesn't modify otherwise
std::string remove_trailing_newline(std::string in) {
  std::string ret = in;
  if (ret.back() == '\n') {
    ret.pop_back();
  }
  return ret;
}

////////////////////////////////////////////////////////////////////////
// Client thread functions
////////////////////////////////////////////////////////////////////////

namespace
{

  void chat_with_receiver(Connection *conn, Server *serv, User *user, Message &msg) {
    
    //attempt to join user to room
    //first get user message, see if it is join attempt
    if (!conn->receive(msg)) {
      if (conn->get_last_result() == Connection::INVALID_MSG) {
        conn->send(Message(TAG_ERR, "invalid message"));
      }
      return;
    }
    if (msg.tag != TAG_JOIN) {
      conn->send(Message(TAG_ERR, "not joined a room"));
      return;
    }

    std::string roomName = remove_trailing_newline(msg.data);
    Room *room = serv->find_or_create_room(roomName);
    room->add_member(user);
    conn->send(Message(TAG_OK, user->username + " joined " + room->get_room_name()));

    //loop while in room and no failure message
    //see if there is a new message for the user to receieve
    //if so and no error in it, send the message to the user
    while (true) {
      Message *message_r = user->mqueue.dequeue();
      if (message_r != nullptr) {
        if(!conn->send(*message_r)) {
          delete message_r;
          break;
        }
        delete message_r;
      }
    }

    //remove user from room
    room->remove_member(user);

  }

  void chat_with_sender(Connection *conn, Server *serv, User *user, Message &msg) {
    
    Room *room = nullptr;

    while (true) {
      //make sure last message received was valid
      if(!conn->receive(msg)) {
        if(conn->get_last_result() == Connection::INVALID_MSG) {
          conn->send(Message(TAG_ERR, "Invalid message"));
        }
        break;
      }

      //TAGS: JOIN, LEAVE, QUIT, SENDALL
      if (msg.tag == TAG_QUIT) {           //User wants to exit
        conn->send(Message(TAG_OK, "Quitting"));
        if (room != nullptr) {
          room->remove_member(user);
        }
        return;

      } else if (msg.tag == TAG_JOIN) {    //User wants to join room
        if (room != nullptr) {            //if already in one leave
          room->remove_member(user);
        }
        room = serv->find_or_create_room(remove_trailing_newline(msg.data));
        room->add_member(user);
        conn->send(Message(TAG_OK, "Joined room"));

      } else if (msg.tag == TAG_LEAVE) {   //User wants to leave
        if (room == nullptr) {            //if already in one leave
          conn->send(Message(TAG_ERR, "cannot leave - not in a room"));
          continue;
        }
        room->remove_member(user);
        conn->send(Message(TAG_OK, "Left room"));
        room = nullptr; //set the local variable to null

      } else if (msg.tag == TAG_SENDALL) { //User sending a message
        if(room == nullptr) {
          conn->send(Message(TAG_ERR, "join room to send message"));
          continue;
        }
        std::string msg_data = remove_trailing_newline(msg.data);
        room->broadcast_message(user->username, msg_data);
        conn->send(Message(TAG_OK, "Sent message"));
      }
    }
  }

  void *worker(void *arg)
  {
    pthread_detach(pthread_self());

    // use a static cast to convert arg from a void* to
    // whatever pointer type describes the object(s) needed
    // to communicate with a client (sender or receiver)
    ClientInfo* clientInfo = static_cast<ClientInfo*>(arg);
    std::unique_ptr<ClientInfo> info(clientInfo); //use for automatic management of info

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
      isReceiver = 1;
    } else if (msg.tag != TAG_SLOGIN) { //invalid tag
      info->conn->send(Message(TAG_ERR, "Invalid login attempt"));
      return nullptr;
    } //will be 0 if sender login

    //We logged in, (try to) send ok message 
    if (!info->conn->send(Message(TAG_OK, "logged in"))) {
      return nullptr;
    }
    
    // depending on whether the client logged in as a sender or
    // receiver, communicate with the client (implementing
    // separate helper functions for each of these possibilities
    // is a good idea)

    //get username to make user
    std::string username = remove_trailing_newline(msg.data);
    User *user = new User(username);

    if (isReceiver == 1) {
      chat_with_receiver(info->conn, info->serv, user, msg);
    } else {
      chat_with_sender(info->conn, info->serv, user, msg);
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
  const char* port_c_str = port_as_str.c_str();
  m_ssock = open_listenfd(port_c_str);
  return m_ssock >= 0;
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
