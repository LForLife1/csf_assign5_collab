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

  Message response;
  // send rlogin and join messages (expect a response from
  // the server for each one)
  Message rlogin_msg(TAG_RLOGIN, username);
  conn.send(rlogin_msg);
  if (!conn.receive(response)) {
    fprintf(stderr, response.data.c_str());
    conn.close();
    exit(1);
  }

  Message join_msg(TAG_JOIN, room_name);
  conn.send(join_msg);
  if (!conn.receive(response)) {
    fprintf(stderr, response.data.c_str());
    conn.close();
    exit(1);
  }

  // loop waiting for messages from server
  // (which should be tagged with TAG_DELIVERY)
  while (true) {
    if (!conn.receive(response) || response.tag == TAG_ERR) {
      fprintf(stderr, response.data.c_str());
      conn.close();
      break;
    }
    if (response.tag == TAG_DELIVERY) {
      int sender_index = response.data.find(":");
      int message_index = response.data.find(":", sender_index + 1);
      int sender_len = message_index - sender_index - 1;
      std::cout << response.data.substr(sender_index + 1, sender_len) << ": " << response.data.substr(message_index + 1);
    }
  }


  return 0;
}
