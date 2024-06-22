read -p "sim or game " file 

case "$file" in "game") gcc -o game game.c utils/environment.c  -lpthread && ./game && rm game && exit 1;; esac 
case "$file" in "sim") gcc -o sim sim.c utils/environment.c -lpthread && ./sim && rm sim && exit 1 ;; esac 
case "$file" in "server") gcc -o server server.c -pthread && ./server 127.0.0.1 42069 && rm server && exit 1 ;; esac
case "$file" in "client") gcc -o client client.c -pthread && ./client 127.0.0.1 42069 && rm client && exit 1 ;; esac
case "$file" in *) echo "Invalid Option: $file" && exit 1 ;; esac
