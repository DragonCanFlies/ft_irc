#!/bin/bash

for i in $(seq 1 5); do
(
  HOST=127.0.0.1
  PORT=6667
  CHANNEL="#test"

  {
    echo "PASS pass"
    echo "NICK bot$i"
    echo "USER bot$i 0 * :Bot $i"
    echo "JOIN $CHANNEL"

    for j in $(seq 1 5000); do
      echo "PRIVMSG $CHANNEL :bot$i msg$j"
    done
  } | nc -C $HOST $PORT

) &
done

wait