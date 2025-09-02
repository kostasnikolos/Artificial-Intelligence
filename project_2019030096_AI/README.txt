TUC Hexthello v0.98b
--------------------------------------------------
Author: Ioannis Skoulakis
Affiliation: Technical University of Crete, Greece
--------------------------------------------------

Requirements:

* any Linux distribution

Compilation:

* make [all]  - to build client and server
* make client - to build just the client
* make server - to build just the server

Execution:

./guiServer
./server [-p port] [-g number_of_games] [-s (swap color after each game)]
./client [-i ip] [-p port]

--------------------------------------------------
To run a client with the algorithm you want  press ./client -i 127.0.0.1 -p 6002 -a (your algorithmi choice) 
Simple MiniMax: 0
A-B pruning: 1
A-B pruning with Ordering:2