#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define MAX_MOVES 100
#define MAX_SIZE 10
#define BUFSZ 1024

struct action {
    int type;
    int moves[MAX_MOVES];
    int board[MAX_SIZE][MAX_SIZE];
};

// Representações
enum Actions { START, MOVE, MAP, HINT, UPDATE, WIN, RESET, EXIT };

void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port>\n", argv[0]);
    printf("example: %s v4 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

void load_maze(const char *filename, int maze[MAX_SIZE][MAX_SIZE]) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir arquivo do labirinto");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < MAX_SIZE; i++) {
        for (int j = 0; j < MAX_SIZE; j++) {
            if (fscanf(file, "%d", &maze[i][j]) != 1) {
                maze[i][j] = -1; // Preenche áreas não configuradas
            }
        }
    }

    fclose(file);
}

void send_update(int client_socket, int maze[MAX_SIZE][MAX_SIZE]) {
    struct action update = {0};
    update.type = UPDATE;
    memcpy(update.board, maze, MAX_SIZE * MAX_SIZE * sizeof(int));
    send(client_socket, &update, sizeof(update), 0);
}

void handle_client(int client_socket, int maze[MAX_SIZE][MAX_SIZE]) {
    struct action request = {0};
    while (recv(client_socket, &request, sizeof(request), 0) > 0) {
        switch (request.type) {
            case START:
                printf("Cliente conectado. Enviando estado inicial.\n");
                send_update(client_socket, maze);
                break;

            case MOVE:
                printf("Cliente realizou movimento: %d\n", request.moves[0]);
                // Processar movimento no labirinto (a lógica pode ser adicionada aqui)
                send_update(client_socket, maze);
                break;

            case MAP:
                printf("Cliente solicitou mapa completo.\n");
                send_update(client_socket, maze);
                break;

            case RESET:
                printf("Cliente solicitou reset do jogo.\n");
                // Recarrega o labirinto
                send_update(client_socket, maze);
                break;

            case EXIT:
                printf("Cliente desconectado.\n");
                close(client_socket);
                return;

            default:
                printf("Ação desconhecida recebida.\n");
                break;
        }
    }

    printf("Conexão encerrada pelo cliente.\n");
    close(client_socket);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    int enable = 1;
    if (0 != setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int))) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != bind(s, addr, sizeof(storage))) {
        logexit("bind");
    }

    if (0 != listen(s, 10)) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);

    int maze[MAX_SIZE][MAX_SIZE] = {0};
    //load_maze(maze_file, maze);
    
    while (1) {
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *)(&cstorage);
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        if (csock == -1) {
            logexit("accept");
        }

        char caddrstr[BUFSZ];
        addrtostr(caddr, caddrstr, BUFSZ);
        printf("[log] connection from %s\n", caddrstr);

        char buf[BUFSZ];
        memset(buf, 0, BUFSZ);
        size_t count = recv(csock, buf, BUFSZ - 1, 0);
        printf("[msg] %s, %d bytes: %s\n", caddrstr, (int)count, buf);

        sprintf(buf, "remote endpoint: %.1000s\n", caddrstr);
        count = send(csock, buf, strlen(buf) + 1, 0);
        if (count != strlen(buf) + 1) {
            logexit("send");
        }
        close(csock);
    }

    exit(EXIT_SUCCESS);
}
