================================================
Build:
Start inside the project directory (the directory containing this Makefile)
and type one of the following commands:

- To build everything (both server and client): run "make" or "make all"

- To build just the server: "make server"
  This will create the executable file Project3Server

- To build just the client: "make client"
  This will create the executable file Project3Client

- To unbuild everything: "make clean"

================================================
Server usage

To run the server, type the command:
./server.out [-p <port>]

-p  (Optional) The port number for the server to listen to

================================================
Client usage

To run the client, type the command:
./client.out [-h <server>] [-p <port>]

-h  (Optional) The IP or domain name of the server (e.g 127.0.0.1 or mathcs01)
-p  (Optional) The port number of the server

the flags can be in any order.


This will create the directory clientdata, where all music/files should be stored.
