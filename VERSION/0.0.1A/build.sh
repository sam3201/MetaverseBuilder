#!/bin/bash

read -p "Enter 'sim', 'game', 'cite', 'server', 'client', or 'PredPreySim': " file

if [ -z "$file" ]; then
    echo "No input provided. Exiting."
    exit 1
fi

compile_and_run() {
    gcc "$1" -o "$2" "${@:3}"
    if [ $? -eq 0 ]; then
        ./"$2" "${@:4}"
        rm "$2"
    else
        echo "Compilation failed for $2."
    fi
}

host="127.0.0.1"
port="42069"

case "$file" in
  "game")
    compile_and_run "src/game.c" "game" "utils/environment.c" "utils/type_system/type_system.c" utils/NNS/NN.c "-lpthread" "-framework" "CoreFoundation" "-framework" "CoreGraphics"
    ;;
  "sim")
    compile_and_run "src/sim.c" "sim" "utils/environment.c" "utils/NN.c" "utils/type_system/type_system.c" "-lpthread" "-lm" "-framework" "CoreFoundation" "-framework" "CoreGraphics"
    ;;
  "cite")
    gcc "src/cite.c" -o "cite" "utils/socketed/cite.c" "utils/socketed/protocol.c" "-pthread" "-lm" "-framework" "CoreFoundation" "-framework" "CoreGraphics"
    if [ $? -eq 0 ]; then
       ./cite "$host" "$port"
        rm "cite"
    else 
      echo "Compilation failed for cite."
    fi
    ;;
  "server")
    gcc "utils/socketed/server.c" -o "server" "utils/environment.c" "utils/Concurrency/thread_pool.c" "-pthread" "-lm" "-framework" "CoreFoundation" "-framework" "CoreGraphics"
    if [ $? -eq 0 ]; then
        ./server "$host" "$port"
        rm "server"
    else
        echo "Compilation failed for server."
    fi
    ;;
  "client")
    gcc "utils/socketed/client.c" -o "client" "utils/environment.c" "utils/Concurrency/thread_pool.c" "-pthread" "-lm" "-framework" "CoreFoundation" "-framework" "CoreGraphics"
    if [ $? -eq 0 ]; then
        ./client "$host" "$port"
        rm "client"
    else
        echo "Compilation failed for client."
    fi
    ;;
  "PredPreySim")
    compile_and_run "src/PredPreySim.c" "PredPreySim" "utils/environment.c" "utils/NNS/NN.c" "-pthread" "-lm" "-framework" "CoreFoundation" "-framework" "CoreGraphics"
    ;;
  *)
    echo "Invalid Option: $file"
    exit 1
    ;;
esac


