#!/bin/bash

HOST=127.0.0.1
PORT=6667

{
  echo "PASS pass"
  echo "NICK crash"
  echo "USER crash 0 * :Crash Test"
  echo "JOIN #test"

  # malformed spacing
  echo "PRIVMSG#test:badformat"
  echo "PRIVMSG"
  echo "PRIVMSG #test"
  echo "PRIVMSG :missingtarget"
  echo "PRIVMSG #test :"

  # correct again after garbage
  echo "PRIVMSG #test :recovery test still alive"

} | nc -C $HOST $PORT