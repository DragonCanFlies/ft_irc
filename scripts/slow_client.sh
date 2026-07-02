#!/bin/bash

HOST=127.0.0.1
PORT=6667
PASS=pass
NICK=slow
CHANNEL="#test"

{
  echo "PASS $PASS"
  echo "NICK $NICK"
  echo "USER slow 0 * :Slow Client"
  echo "JOIN $CHANNEL"

  # VERY slow reader
  while true; do
    sleep 1
  done
} | nc -C $HOST $PORT