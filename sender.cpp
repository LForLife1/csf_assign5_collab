#include <iostream>
#include <string>
#include <sstream>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv) {
  if (argc != 4) {
    std::cerr << "Usage: ./sender [server_address] [port] [username]\n";
    return 1;
  }

  std::string server_hostname;
  int server_port;
  std::string username;

  server_hostname = argv[1];
  server_port = std::stoi(argv[2]);
  username = argv[3];

  Connection conn;

  // connect to server
  conn.connect(server_hostname, server_port);
  if (!conn.is_open()) {
    fprintf(stderr, "Could not connect to the server\n");
    return 1;
  }

  // TODO: send slogin message
  Message slogin_msg(TAG_SLOGIN, username);
  Message login_response;
  conn.send(slogin_msg);
  conn.receive(login_response);
  if (login_response.tag == TAG_ERR) {
    fprintf(stderr, login_response.data.c_str());
    return 1;
  }

  // TODO: loop reading commands from user, sending messages to
  //       server as appropriate

  return 0;
}
