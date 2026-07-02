#!/bin/bash

HOST=127.0.0.1
PORT=6667
CHAN="#test"

{
  echo "PASS pass"
  echo "NICK edge"
  echo "USER edge 0 * :Edge Tester"
  echo "JOIN $CHAN"

  # 1. normal
  echo "PRIVMSG $CHAN :hello world"

  # 2. multiple spaces
  echo "PRIVMSG $CHAN :hello     world    spaced"

  # 3. special characters
  echo "PRIVMSG $CHAN :!@#$%^&*()_+-=[]{};':\",.<>/?"

  # 4. unicode-like noise
  echo "PRIVMSG $CHAN :áéíóú ñ ç ø æ"

  # 5. long message
  echo "PRIVMSG $CHAN :$(head /dev/urandom | tr -dc A-Za-z0-9 | head -c 300)"

  # 6. missing message (should error, not crash)
  echo "PRIVMSG $CHAN"

  # 7. missing target
  echo "PRIVMSG :no_target"

  # 8. invalid channel
  echo "PRIVMSG #doesnotexist :hello?"

  # 9. message starting with colon inside text
  echo "PRIVMSG $CHAN ::double colon test"

  # 10. weird formatting
  echo "PRIVMSG    $CHAN     :    spaced out message    "

} | nc -C $HOST $PORT