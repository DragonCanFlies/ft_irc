*This project has been created as part of the 42 curriculum by ltoscani and latabagl.*

## Description

ft\_irc is a small IRC server written in C++98. It accepts multiple TCP clients, uses non-blocking sockets with poll(), and supports authentication, nicknames, users, channels, private messages, and basic channel operator commands.

## Instructions

Compile:

```bash
make
```

Run:

```bash
./ircserv <port> <password>
```

Example:

```bash
./ircserv 6667 pass
```

## Features

* PASS, NICK, USER registration
* PING/PONG
* JOIN, PART, QUIT
* PRIVMSG to users and channels
* Channel operators
* KICK, INVITE, TOPIC
* MODE: i, t, k, o, l

## Testing

With netcat:

```bash
nc -C 127.0.0.1 6667
PASS pass
NICK max
USER max 0 \* :Max
JOIN #test
PRIVMSG #test :hello
```

Reference client used for manual checks: irssi.
Raw protocol checks were also done with nc.

## Resources

* IRC protocol documentation
* man pages: socket, bind, listen, accept, poll, recv, send, fcntl

## AI Usage

AI was used to plan the implementation order, create a minimal project structure, and review edge cases. All code was tested and reviewed manually before submission.
