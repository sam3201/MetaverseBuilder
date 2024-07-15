#include "cite.h"
#include <sys/wait.h>
#include <util.h>

#define BUFFER_SIZE 4096

static int server_socket;
static struct sockaddr_in server_addr;

static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char* base64_encode(const unsigned char* input, int length) {
    int output_length = 4 * ((length + 2) / 3);
    char* encoded_data = malloc(output_length + 1);
    if (encoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < length;) {
        uint32_t octet_a = i < length ? input[i++] : 0;
        uint32_t octet_b = i < length ? input[i++] : 0;
        uint32_t octet_c = i < length ? input[i++] : 0;

        uint32_t triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        encoded_data[j++] = base64_table[(triple >> 18) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 12) & 0x3F];
        encoded_data[j++] = base64_table[(triple >> 6) & 0x3F];
        encoded_data[j++] = base64_table[triple & 0x3F];
    }

    for (int i = 0; i < (3 - length % 3) % 3; i++)
        encoded_data[output_length - 1 - i] = '=';

    encoded_data[output_length] = '\0';

    return encoded_data;
}

void init_cite_server(const char* host, const char* port) {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Failed to create socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(port));
    server_addr.sin_addr.s_addr = inet_addr(host);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) < 0) {
        perror("Failed to listen on socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server initialized on %s:%s\n", host, port);
}

void send_http_response(int client_socket, const char* content_type, const char* content) {
    char header[BUFFER_SIZE];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %zu\r\n"
             "\r\n",
             content_type, strlen(content));

    send(client_socket, header, strlen(header), 0);
    send(client_socket, content, strlen(content), 0);
}

void handle_websocket_handshake(int client_socket, const char* buffer) {
    char *key_start = strstr(buffer, "Sec-WebSocket-Key: ") + 19;
    char *key_end = strstr(key_start, "\r\n");
    int key_length = key_end - key_start;
    char key[256];
    strncpy(key, key_start, key_length);
    key[key_length] = '\0';

    char concat_key[512];
    sprintf(concat_key, "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11", key);
    
    unsigned char sha1_hash[20] = {0};
    for (int i = 0; i < strlen(concat_key); i++) {
        sha1_hash[i % 20] ^= concat_key[i];
    }

    char* accept_key = base64_encode(sha1_hash, 20);

    char response[1024];
    sprintf(response, 
            "HTTP/1.1 101 Switching Protocols\r\n"
            "Upgrade: websocket\r\n"
            "Connection: Upgrade\r\n"
            "Sec-WebSocket-Accept: %s\r\n\r\n",
            accept_key);
    send(client_socket, response, strlen(response), 0);

    free(accept_key);
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';

        if (strstr(buffer, "GET /ws HTTP/1.1") != NULL) {
            handle_websocket_handshake(client_socket, buffer);

            int master;
            char *slave_name;
            pid_t pid = forkpty(&master, &slave_name, NULL, NULL);

            if (pid < 0) {
                perror("Fork failed");
                return;
            } else if (pid == 0) {
                // Child process
                execl("/bin/sh", "/bin/sh", NULL);
                exit(1);
            } else {
                // Parent process
                fcntl(master, F_SETFL, O_NONBLOCK);

                fd_set readfds;
                struct timeval tv;

                while (1) {
                    FD_ZERO(&readfds);
                    FD_SET(client_socket, &readfds);
                    FD_SET(master, &readfds);

                    tv.tv_sec = 1;
                    tv.tv_usec = 0;

                    int max_fd = (client_socket > master) ? client_socket : master;

                    int activity = select(max_fd + 1, &readfds, NULL, NULL, &tv);

                    if (activity < 0) {
                        perror("Select error");
                        break;
                    }

                    if (FD_ISSET(client_socket, &readfds)) {
                        bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
                        if (bytes_received <= 0) break;

                        // Decode WebSocket frame (simplified)
                        unsigned char *payload = (unsigned char *)buffer + 6;
                        int payload_length = bytes_received - 6;
                        unsigned char mask[4];
                        memcpy(mask, buffer + 2, 4);

                        for (int i = 0; i < payload_length; i++) {
                            payload[i] ^= mask[i % 4];
                        }

                        write(master, payload, payload_length);
                    }

                    if (FD_ISSET(master, &readfds)) {
                        bytes_received = read(master, buffer + 2, BUFFER_SIZE - 3);
                        if (bytes_received > 0) {
                            buffer[0] = 0x81;  // Text frame
                            buffer[1] = bytes_received;
                            send(client_socket, buffer, bytes_received + 2, 0);
                        }
                    }
                }

                close(master);
                waitpid(pid, NULL, 0);
            }
        } else if (strstr(buffer, "GET") != NULL) {
            const char* html = "<!DOCTYPE html>\n"
                               "<html lang=\"en\">\n"
                               "<head>\n"
                               "    <meta charset=\"UTF-8\">\n"
                               "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                               "    <title>Cite Terminal</title>\n"
                               "    <link rel=\"stylesheet\" href=\"https://unpkg.com/xterm/css/xterm.css\" />\n"
                               "    <script src=\"https://unpkg.com/xterm/lib/xterm.js\"></script>\n"
                               "    <script src=\"https://unpkg.com/xterm-addon-fit/lib/xterm-addon-fit.js\"></script>\n"
                               "</head>\n"
                               "<body style=\"margin: 0; padding: 0; height: 100vh;\">\n"
                               "    <div id=\"terminal\" style=\"width: 100%; height: 100%;\"></div>\n"
                               "    <script>\n"
                               "        const term = new Terminal();\n"
                               "        const fitAddon = new FitAddon.FitAddon();\n"
                               "        term.loadAddon(fitAddon);\n"
                               "        term.open(document.getElementById('terminal'));\n"
                               "        fitAddon.fit();\n"
                               "\n"
                               "        const socket = new WebSocket(`ws://${window.location.host}/ws`);\n"
                               "\n"
                               "        socket.onopen = () => {\n"
                               "            term.write('Connected to the server\\r\\n');\n"
                               "        };\n"
                               "\n"
                               "        socket.onmessage = (event) => {\n"
                               "            term.write(event.data);\n"
                               "        };\n"
                               "\n"
                               "        term.onData((data) => {\n"
                               "            socket.send(data);\n"
                               "        });\n"
                               "\n"
                               "        window.addEventListener('resize', () => fitAddon.fit());\n"
                               "    </script>\n"
                               "</body>\n"
                               "</html>";
            send_http_response(client_socket, "text/html", html);
        }
    }
}

void start_cite_server() {
    printf("Server started. Waiting for connections...\n");

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);

        if (client_socket < 0) {
            perror("Failed to accept connection");
            continue;
        }

        printf("New connection accepted\n");
        handle_client(client_socket);
        close(client_socket);
    }
}
