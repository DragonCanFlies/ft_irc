#!/bin/bash

HOST=127.0.0.1
PORT=6667
CHANNEL="#test"

{
  echo "PASS pass"
  echo "NICK flooder"
  echo "USER flooder 0 * :Flood Bot"
  echo "JOIN $CHANNEL"

  for i in $(seq 1 5000); do
    echo "PRIVMSG $CHANNEL :message $i $(head /dev/urandom | tr -dc A-Za-z0-9 | head -c 10)"
  done
} | nc -C $HOST $PORT