Have to do from STEP 4 : CONTINUOUS CHAT (LOOP).

right now the server and client send/receive only one message and exit. Let's
->Keep the sever running in a loop to handle continuous messages.
->Do the same for the client.

Update server.cpp - Add Message Loop

Inside the server (after connected!), replace the old recv(...) with :

CO/233

IN COMMAND PROMPT
g++ server.cpp -o server.exe -lws2_32 -std=c++11
server.exe

g++ client.cpp -o client.exe -lws2_32 -std=c++11
client.exe
