#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSZ 1024
#define TAM_MAX_BOARD 10

// Estrutura de dados 'action'
struct action {
    int type;
    int moves[100];
    int board[TAM_MAX_BOARD][TAM_MAX_BOARD];
};

void usage(int argc, char **argv) {
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (addrparse(argv[1], argv[2], &storage) != 0) {
        usage(argc, argv);
    }

    int s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (connect(s, addr, sizeof(storage)) != 0) {
        logexit("connect");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    bool comecouJogo = 0;
    int moves[4] = {0};

    while (1) {
        struct action enviarMensagem;
        char comando[100];
        memset(&enviarMensagem, 0, sizeof(enviarMensagem));
        printf("> ");
        scanf("%s", comando);

        switch (comecouJogo) {
            case 0:
                switch (strcmp(comando, "start")) {
                    case 0:
                        enviarMensagem.type = 0;
                        comecouJogo = 1;
                        break;

                    default:
                        printf("error: start the game first\n");
                        printf("> ");
                        scanf("%s", comando);
                        break;
                }
                break;

            case 1:
                switch (mapearComando(comando)) {
                    case UP:
                        if (validaMovimento(moves, 1)) {
                            enviarMensagem.type = 1;
                            enviarMensagem.moves[0] = 1;
                            break;
                        } else {
                            printf("error: you cannot go this way\n");
                            scanf("%s", comando);
                        }
                        break;

                    case RIGHT:
                        if (validaMovimento(moves, 2)) {
                            enviarMensagem.type = 1;
                            enviarMensagem.moves[0] = 2;
                            break;
                        } else {
                            printf("error: you cannot go this way\n");
                            scanf("%s", comando);
                        }
                        break;

                    case DOWN:
                        if (validaMovimento(moves, 3)) {
                            enviarMensagem.type = 1;
                            enviarMensagem.moves[0] = 3;
                            break;
                        } else {
                            printf("error: you cannot go this way\n");
                            scanf("%s", comando);
                        }
                        break;

                    case LEFT:
                        if (validaMovimento(moves, 4)) {
                            enviarMensagem.type = 1;
                            enviarMensagem.moves[0] = 4;
                            break;
                        } else {
                            printf("error: you cannot go this way\n");
                            scanf("%s", comando);
                        }
                        break;

                    case MAP:
                        enviarMensagem.type = 2;
                        break;

                    case RESET:
                        enviarMensagem.type = 6;
                        break;

                    case EXIT:
                        enviarMensagem.type = 7;
                        return 0;
                        break;

                    case INVALID:
                        printf("error: command not found\n");
                        break;

                    default:
                        break;
                }
                break;
        }
        send(s, &enviarMensagem, sizeof(enviarMensagem), 0);

        struct action receberMensagem;
        memset(&receberMensagem, -1, sizeof(receberMensagem));

        recv(s, &receberMensagem, sizeof(receberMensagem), 0);
        for (int i = 0; i < 4; i++) {
            moves[i] = receberMensagem.moves[i];
        }
        
        if (enviarMensagem.type == 2) {
            /*Exibir mapa*/
            for (int i = 0; i < TAM_MAX_BOARD; i++) {
                int cont = 0;
                for (int j = 0; j < TAM_MAX_BOARD; j++) {
                    switch (receberMensagem.board[i][j]) {
                        case 0:
                            printf("# ");
                            break;
                        case 1:
                            printf("_ ");
                            break;
                        case 2:
                            printf("> ");
                            break;
                        case 3:
                            printf("X ");
                            break;
                        case 4:
                            printf("? ");
                            break;
                        case 5:
                            printf("+ ");
                            break;
                        default:
                            cont++;
                            break;
                    }
                }

                if (cont != TAM_MAX_BOARD)
                    printf("\n");
            }
        }

        if (receberMensagem.type == 4) {
            /*Exibir movimentos possíveis*/
            printf("Possible moves: ");
            for (int i = 0; i < 4; i++) {
                switch (moves[i]) {
                    case 1:
                        printf("up");
                        break;
                    case 2:
                        printf("right");
                        break;
                    case 3:
                        printf("down");
                        break;
                    case 4:
                        printf("left");
                        break;
                    default:
                        break;
                }

                if (moves[i + 1] == 0) {
                    printf(".");
                    break;
                } else {
                    printf(", ");
                }
            }
            printf("\n");
        }

        if (receberMensagem.type == 5) {
            printf("You escaped!\n");
            /*Exibir labirinto completo*/
            for (int i = 0; i < TAM_MAX_BOARD; i++) {
                int cont = 0;
                for (int j = 0; j < TAM_MAX_BOARD; j++) {
                    switch (receberMensagem.board[i][j]) {
                        case 0:
                            printf("# ");
                            break;
                        case 1:
                            printf("_ ");
                            break;
                        case 2:
                            printf("> ");
                            break;
                        case 3:
                            printf("X ");
                            break;
                        default:
                            cont++;
                            break;
                    }
                }

                if (cont != TAM_MAX_BOARD)
                        printf("\n");
            }
            enviarMensagem.type = 6;
            send(s, &enviarMensagem, sizeof(enviarMensagem), 0);
        }
    }

    exit(EXIT_SUCCESS);
} 