#!/bin/bash

read -p "Enter 'sim', 'game', 'server', or 'client': " file

if [ -z "$file" ]; then
    echo "No input provided. Exiting."
    exit 1
fi

case "$file" in
  "game")
    gcc game.c -o game utils/environment.c utils/NN.c -lpthread
    if [ $? -eq 0 ]; then
      ./game
      rm game
    else
      echo "Compilation failed for game."
    fi
    ;;
  "sim")
    gcc sim.c -o sim utils/environment.c utils/NN.c -lpthread -lm
    if [ $? -eq 0 ]; then
      ./sim
      rm sim
    else
      echo "Compilation failed for sim."
    fi
    ;;
  "server")
    gcc -server.c -o server - utils/server.c -pthread
    if [ $? -eq 0 ]; then
      ./server 127.0.0.1 42069
      rm server
    else
      echo "Compilation failed for server."
    fi
    ;;
  "client")
    gcc client.c -o client utils/client.c -pthread
    if [ $? -eq 0 ]; then
      ./client 127.0.0.1 42069
      rm client
    else
      echo "Compilation failed for client."
    fi
    ;;
  *)
    echo "Invalid Option: $file"
    exit 1
    ;;
esac

