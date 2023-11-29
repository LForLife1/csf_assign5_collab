#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "csapp.h"
#include "message.h"
#include "connection.h"
#include "client_util.h"

int main(int argc, char **argv) {
  if (argc != 5) {
    std::cerr << "Usage: ./receiver [server_address] [port] [username] [room]\n";
    return 1;
  }

  std::string server_hostname = argv[1];
  int server_port = std::stoi(argv[2]);
  std::string username = argv[3];
  std::string room_name = argv[4];

  Connection conn;

  // connect to server
  conn.connect(server_hostname, server_port);
  if (!conn.is_open()) {
    fprintf(stderr, "Could not connect to the server\n");
    return 1;
  }

  // attempt to login
  Message rlogin_msg(TAG_RLOGIN, username);
  Message login_response;
  conn.send(rlogin_msg);
  conn.receive(login_response);
  if (login_response.tag == TAG_ERR) {
    fprintf(stderr, login_response.data.c_str());
    return 1;
  }

  //attempt to join
  Message join_msg(TAG_JOIN, room_name);
  Message join_response;
  conn.send(join_msg);
  conn.receive(join_response);
  if (join_response.tag == TAG_ERR) {
    fprintf(stderr, join_response.data.c_str());
    conn.close();
    return 1;
  }

  // loop waiting for messages from server
  // (which should be tagged with TAG_DELIVERY)
  Message response;
  while (true) {
    conn.receive(response);
    if (response.tag == TAG_ERR) {
      fprintf(stderr, response.data.c_str());
      conn.close();
      return 1;
    }
    else if (response.tag == TAG_DELIVERY) {
      int sender_index = response.data.find(":");
      int message_index = response.data.find(":", sender_index + 1);
      int sender_len = message_index - sender_index - 1;
      std::cout << response.data.substr(sender_index + 1, sender_len) << ": " << response.data.substr(message_index + 1);
    }
  }
  return 0;
}
