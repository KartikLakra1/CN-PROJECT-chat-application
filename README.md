Have to do from STEP 4 : CONTINUOUS CHAT (LOOP).

right now the server and client send/receive only one message and exit. Let's
->Keep the sever running in a loop to handle continuous messages.
->Do the same for the client.

Update server.cpp - Add Message Loop

Inside the server (after connected!), replace the old recv(...) with :

CO/233
