#include <sstream>
#include <cctype>
#include <cassert>
#include "csapp.h"
#include "message.h"
#include "connection.h"

Connection::Connection()
  : m_fd(-1)
  , m_last_result(SUCCESS) {
}

Connection::Connection(int fd)
  : m_fd(fd)
  , m_last_result(SUCCESS) {
  // call rio_readinitb to initialize the rio_t object
  rio_readinitb(&m_fdbuf, fd);
}

void Connection::connect(const std::string &hostname, int port) {
  // call open_clientfd to connect to the server
  std::string port_as_str = std::to_string(port);
  m_fd = open_clientfd(hostname.c_str(), port_as_str.c_str());

  if (m_fd < 0) {
    fprintf(stderr, "Could not connect to the host\n");
    exit(1);
  }

  // call rio_readinitb to initialize the rio_t object
  rio_readinitb(&m_fdbuf, m_fd);
}

Connection::~Connection() {
  // close the socket if it is open
   if(is_open()) {
     Close(m_fd);
   }
}

bool Connection::is_open() const {
  // return true if the connection is open
  return m_fd >= 0;
}

void Connection::close() {
  // close the connection if it is open
  if (is_open()){
    Close(m_fd);
    m_fd = -1;
  }
}

bool Connection::send(const Message &msg) {
  if (!isValidTag(msg.tag)) {
    m_last_result = INVALID_MSG;
    return false;
  }

  std::string formatted_message = msg.tag + ":" + msg.data + "\n";
  char c_formatted_message[formatted_message.length() + 1]; 
  strcpy(c_formatted_message, formatted_message.c_str());

  // send a message
  ssize_t result = rio_writen(m_fd, &c_formatted_message, formatted_message.length());
 
  // return true if successful, false if not
  if (result < 0) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }
  m_last_result = SUCCESS;
  return true;
}

bool Connection::receive(Message &msg) {
  // receive a message, storing its tag and data in msg
  char receive_buffer[msg.MAX_LEN] = "";
  ssize_t result = rio_readlineb(&m_fdbuf, receive_buffer, msg.MAX_LEN);

  // return true if successful, false if not
  if (result < 0) {
    m_last_result = EOF_OR_ERROR;
    return false;
  }
  
  if(receive_buffer != NULL) {
    std::string rec_str = receive_buffer;
    int colon_index = rec_str.find(":");
    std::string tag = rec_str.substr(0,colon_index);
    std::string data = rec_str.substr(colon_index + 1);

    if (!isValidTag(tag)) {
      m_last_result = INVALID_MSG;
      return false;
    }

    msg.tag = tag;
    msg.data = data;
    m_last_result = SUCCESS;

  } else { //recieve buffer was NULL
    m_last_result = EOF_OR_ERROR;
    return false;
  }
  
  return true;
}

bool Connection::isValidTag(const std::string tag) const{
  if (tag == TAG_ERR || tag == TAG_OK || tag == TAG_SLOGIN || tag == TAG_RLOGIN || tag == TAG_JOIN || tag == TAG_LEAVE || 
      tag == TAG_SENDALL || tag == TAG_SENDUSER || tag == TAG_QUIT || tag == TAG_DELIVERY || tag == TAG_EMPTY) {
    return true;
  }

  return false;
}
