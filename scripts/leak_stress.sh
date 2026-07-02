#!/bin/bash

while true; do
  {
    echo "PASS pass"
    echo "NICK leakbot"
    echo "USER leakbot 0 * :Leak Bot"
    echo "JOIN #test"

    for i in $(seq 1 2000); do
      echo "PRIVMSG #test :$(head /dev/urandom | tr -dc A-Za-z0-9 | head -c 50)"
    done
  } | nc -C 127.0.0.1 6667
done