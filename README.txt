Sample README.txt

Eventually your report about how you implemented thread synchronization
in the server should go here

We have critical regions wherever multiple users deal with the same information.
These regions only appear in room.cpp and message_queue.cpp. This makes sense as
for room.cpp, the shared data is the list of members which must be modified 
every time a user joins/leaves. It must also 

//Some inspiration
