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

  // send slogin message
  Message slogin_msg(TAG_SLOGIN, username);
  Message login_response;
  conn.send(slogin_msg);
  conn.receive(login_response);
  if (login_response.tag == TAG_ERR) {
    fprintf(stderr, login_response.data.c_str());
    return 1;
  }

  Message response;
  std::string user_input;

  // loop reading commands from user, sending messages to server as appropriate
  while (true) {
    std::getline(std::cin, user_input);
    //Commands
    if(user_input.at(0) == '/'){
      if(user_input == "/quit"){  //quit
        response.tag = TAG_QUIT;
        response.data = " ";
        conn.send(response);
        conn.receive(response); //wait for server response before quitting
        if (response.tag == TAG_ERR) { 
          fprintf(stderr, response.data.c_str());
        }
        return 0;

      } else if(user_input == "/leave") { //leave
        response.tag = TAG_LEAVE;
        response.data = " ";
        conn.send(response);

      } else if(user_input == "/join"){   //join
        response.tag = TAG_JOIN;
        response.data = user_input.substr(6); //room name after "/join "
        conn.send(response);
      }

      else {
        fprintf(stderr, "Command not recognized\n");
      }

    //otherwise, trying to send message
    } else { 
      if(user_input.length() > response.MAX_LEN) {
        //TODO: WHAT HAPPENS IF INPUT MESSAGE IS TOO LONG
      } else{
        response.tag = TAG_SENDALL;
        response.data = user_input;
        conn.send(response);
      }
    }

    conn.receive(response); 
    if (response.tag == TAG_ERR) { 
      fprintf(stderr, response.data.c_str());
    }

  }
   //while loop
  //wait for stdin input (Commands start with the / character and may be one of the following, reject rest to stderr)
  //if quit, break loop
    // the sender must wait for a reply from the server before exiting with exit code 0.
  //if join, join room
  //if leave, leave the room
  //if message, send message in room

  return 0;
}
