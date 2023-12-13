#include "guard.h"
#include "message.h"
#include "message_queue.h"
#include "user.h"
#include "room.h"

Room::Room(const std::string &room_name)
  : room_name(room_name) {
  // initialize the mutex
  pthread_mutex_init(&lock, nullptr);
}

Room::~Room() {
  // destroy the mutex
  pthread_mutex_destroy(&lock);
}

void Room::add_member(User *user) {
  // add User to the room
  Guard g(lock);
  members.insert(user);
}

void Room::remove_member(User *user) {
  // remove User from the room
  Guard g(lock);
  members.erase(user);
}

void Room::broadcast_message(const std::string &sender_username, const std::string &message_text) {
  
  Guard g(lock);
  //Create the text for the message, and the message itself
  //Make new object to put on heap
  std::string message_text_tot = room_name + ":" + sender_username + ":" + message_text;

  //For each user in the Room, enqueue the message to their message queue
  for(User* user : members) {
    if (user->username != sender_username)
    {
      Message* message = new Message(TAG_DELIVERY, message_text_tot);
      user->mqueue.enqueue(message);
    }
  }
  
}
