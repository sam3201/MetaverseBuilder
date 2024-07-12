#!/bin/bash

read -p "Enter 'sim', 'game', 'cite' 'server', 'client': " file

if [ -z "$file" ]; then
    echo "No input provided. Exiting."
    exit 1
fi

case "$file" in
  "game")
    gcc src/game.c -o game utils/environment.c utils/type_system/type_system.c -lpthread
    if [ $? -eq 0 ]; then
      ./game
      rm game
    else
      echo "Compilation failed for game."
    fi
    ;;
  "sim")
    gcc src/sim.c -o sim utils/environment.c utils/NN.c utils/type_system/type_system.c -lpthread -lm
    if [ $? -eq 0 ]; then
      ./sim
      rm sim
    else
      echo "Compilation failed for sim."
    fi
    ;;
  "cite")
    gcc src/cite.c -o cite utils/environment.c utils/type_system/type_system.c utils/NN.c utils/server.c -pthread -lm 
    if [ $? -eq 0 ]; then
      ./cite 127.0.0.1 42069 
      rm cite 
    else
      echo "Compilation failed for cite."
    fi
    ;;
  "server")
    gcc utils/server.c -o server utils/environment.c -pthread -lm 
    if [ $? -eq 0 ]; then
      read -p "Start game loop? (yes/no): " start_game
      ./server 127.0.0.1 42069 $start_game
      rm server
    else
      echo "Compilation failed for server."
    fi
    ;;
  "client")
    gcc utils/client.c -o client -pthread -lm
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

