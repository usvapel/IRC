<<<<<<< Updated upstream
# IRC
=======
*This project has been created as part of the 42 curriculum by jpelline, anpollan and nraatika.*
# IRC - a simple server in C++20
The project brief was to create a simple IRC server in C++, implementing only a portion of the server-to-client communication defined in the protocol, none of the server-to-server comms. 
## Architechture
We wanted to use as modern a version of C++ as the school infrastructure allows, so we chose `C++20` as the standard. At it's core, the server uses `epoll`to maintain and communicate to an `unordered map` of known clients. 
## Core functionality
In simple terms, an IRC server listens for incoming connections, and provides a way for users to send messages to each other, or to selected groups of users.
- The `server` listens for new connections on a port specified when started
- The `server` can accept new `clients` that send the appropriate handshake sequence of messages to register their information, most important being their `nickname`, which is how messages are addressed in IRC
- Each `client`can create and join `channels`by sending the appropriate message to the server
- Private messages can be sent to `clients` or `channels`; when a message is sent to a channel, the `server`relays the message to all `clients`that have joined that channel, except the sender
- The `server` maintains a list of `clients` with operator rights for each channel, which confer certain privileges regular uses don't have
- `clients`with operator rights (which are channel-specific) can set `modes` for each channel: 
	- toggle invite only (when on, only users invited by channel operators can join)
	- toggle topic lock (when on, only operators can change the channel topic)
	- set capacity limit (how many users can join the channel)
	- password (whether a password is needed to join the channel)
	and confer operator rights to another `client` present on the channel
- The `server` writes all important events to a log-file
- The `server`keeps track of when the last message was received from each `client`, pings them if they have been idle too long, and removes them if they still don't respond.
- The `server`quits cleanly when receiving a Ctrl-C signal

## Team & contributions
 - [Julius Pellinen](https://github.com/etherstep) : 
	 - Makefile
	 - Wrote the first functioning core server loop
	 - Mode handling for channels
- [Antti Pöllänen](https://github.com/Mursupaani):
	- Refined the server loop to handle incoming messages
	- Implemented Channel class and User class which differentiates operators and regular users
 - [Niklas Raatikainen](https://github.com/patsastus)
	- Refined the server loop to handle incoming and outgoing messages, and handle errors
	- Incoming and outgoing buffers in Client class and Socket class
	- Message parsing
	- Logging
	- Handling handshake sequence of messages

## Project choices and reflections
- Not strictly a technical choice, but we had everyone working on getting the core loop functional at the start, so everyone could get a good grasp of the core of the project, before branching out and implementing features more independently. I think it was a good choice.
- We utilized the **RAII**-idiom (**R**esource **A**cquisition **I**s **I**nitialization) as  our resource management technique (e.g. using `try_emplace`to create objects inside the containers where they will live). This meant no memory leaks or dangling open file descriptors ever appeared.
- We implemented **factory methods** to have a single Socket class for two different use cases: the servers listening socket and the client communication sockets
- We split the Socket class from the Client and kept separate maps  of Sockets and Clients, to handle cases where the remote Socket unexpectedly closes, while the Client exists long enough to inform users sharing a channel with them that they've unexpectedly quit. 
- We preempted many `Git`merge conflicts by sharing and using a `.clangd-format`file
- We mostly managed to keep pull requests to a reasonable size, so all team members were able to keep up with implementation choices.
- Communication was good, both in-person and online over Trello and Discord.

## Build and run
To test the project out, you first need to clone this repository.
Because we are using `epoll`, the server will only run on Linux.  On Linux, building and running it is quite simple:
```
make
./ircserv 6667 pass
```
the server is now running on the default port with the password `pass`, and writing logs to `irc_server.log`. To check what is going on internally, you can run 
```
tail -f irc_server.log
```
and connect to the server with any IRC-client (our testing workhorse was `irssi`, a text-based implementation of an IRC client). 
```
irssi usage?
```
Happy Internet Relay Chatting!
>>>>>>> Stashed changes
