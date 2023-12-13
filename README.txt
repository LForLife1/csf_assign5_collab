Sample README.txt

Eventually your report about how you implemented thread synchronization
in the server should go here

We have critical regions wherever multiple users deal with the same information.
These regions only appear in server.cpp, room.cpp and message_queue.cpp.

For server.cpp, we have the room map variable which is shared by all threads
on the server. So each time it is modified, (a room is created) the mutex must
be locked. Therefore there is a guard in find_or_create_room.

For room.cpp, the shared data is the list of members which must be modified 
every time a user joins/leaves or a message is broadcast. All functions modify
the members list in some way, so they all have guards.

For message_queue.cpp, the shared data is the queue of messages itself, so each
time that is modified (in enqueue and dequeue) there must be a guard.