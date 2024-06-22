gcc -o main main.c utils/environment.c  -lpthread && ./main && rm main
#gcc -o server main.c server.c -pthread && ./server 127.0.0.1 42069 && rm server


