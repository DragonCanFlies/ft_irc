#!/bin/bash

HOST=127.0.0.1
PORT=6667
PASS=pass
NICK=${1:-user}
CHANNEL="#test"

{
  echo "PASS $PASS"
  echo "NICK $NICK"
  echo "USER $NICK 0 * :Real Name"
  echo "JOIN $CHANNEL"

  # keep client alive and interactive
  while read line; do
    echo "$line"
  done
} | nc -C $HOST $PORT