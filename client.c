#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// Estrutura de dados 'action'
struct action {
    int type;
    int moves[100];
    int board[10][10];
};

void usage(int argc, char **argv) {
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

// função para verificar se no movimentos possiveis tem um movimento especifico
int verificaMove(int moves[4], int num) {
    for (int i = 0; i < 4; i++) {
        if (num == moves[i])
            return 1;
    }
    return 0;
}

#define BUFSZ 1024

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
    if (0 != addrparse(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if (s == -1) {
        logexit("socket");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (0 != connect(s, addr, sizeof(storage))) {
        logexit("connect");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);

    // printf("connected to %s\n", addrstr);

    int local = 0;
    int moves[4] = {0};

    while (1) {

        // Cria a mensagem struct para enviar
        // int possibleMoves[4];
        //--------------------------------------------------------------------
        struct action send_action;
        memset(&send_action, 0, sizeof(send_action)); // preenche tudo com 0
        //--------------------------------------------------------------------

        // logica cliente
        //------------------------------------------------------------------------
        // recebe o que o cliente digitar;
        char msg[100];
        printf("> ");
        scanf("%s", msg);

        // fica no loop esperando ser digitado 'start'
        if (local == 0) {
            while (local == 0) {
                if (strcmp(msg, "start") == 0) {
                    send_action.type = 0;
                    local = 1;
                }
                else {
                    printf("error: start the game first\n> ");
                    scanf("%s", msg);
                }
            }
        }

        // fica no loop esperando ser digitado algum movimento valido
        else if (local == 1) {
            while (1) {
                //testa se o comando enviado existe no jogo
                //testa se o movimento escolhido é possivel
                if (strcmp(msg, "up") == 0) {
                    if (verificaMove(moves, 1)) {
                        send_action.type = 1;
                        send_action.moves[0] = 1;
                        break;
                    }
                    else {
                        printf("error: you cannot go this way\n> ");
                        scanf("%s", msg);
                    }
                }
                else if ((strcmp(msg, "right") == 0)) {
                    if (verificaMove(moves, 2)) {
                        send_action.type = 1;
                        send_action.moves[0] = 2;
                        break;
                    }
                    else {
                        printf("error: you cannot go this way\n> ");
                        scanf("%s", msg);
                    }
                }
                else if ((strcmp(msg, "down") == 0)) {
                    if (verificaMove(moves, 3)) {
                        send_action.type = 1;
                        send_action.moves[0] = 3;
                        break;
                    }
                    else {
                        printf("error: you cannot go this way\n> ");
                        scanf("%s", msg);
                    }
                }
                else if ((strcmp(msg, "left") == 0)) {
                    if (verificaMove(moves, 4)) {
                        send_action.type = 1;
                        send_action.moves[0] = 4;
                        break;
                    }
                    else {
                        printf("error: you cannot go this way\n> ");
                        scanf("%s", msg);
                    }
                }
                else if ((strcmp(msg, "map") == 0)) {
                    send_action.type = 2;
                    break;
                }
                else if ((strcmp(msg, "reset") == 0)) {
                    send_action.type = 6;
                    break;
                }
                else if ((strcmp(msg, "exit") == 0)) {
                    send_action.type = 7;
                    break;
                }
                else {
                    printf("error: command not found\n> ");
                    scanf("%s", msg);
                }
            }
        }        

    //------------------------------------------------------------------------

        // Envia a mensagem struct
        //--------------------------------------------------------------------
        size_t count = send(s, &send_action, sizeof(send_action), 0);

        if (count != sizeof(send_action)) {
            logexit("send");
        }

        if (send_action.type == 7) {
            exit(1);
        }
        //--------------------------------------------------------------------

        // recebe a mensagem action
        //--------------------------------------------------------------------
        struct action recv_action;
        memset(&recv_action, -1, sizeof(recv_action)); // preenche a struct toda com -1

        count = recv(s, &recv_action, sizeof(recv_action), 0); // recebe a mensagem
        for (int i = 0; i < 4; i++) {
            moves[i] = recv_action.moves[i];
        }

        //--------------------------------------------------------------------

        //close(s);

        // logica cliente
        //--------------------------------------------------------------------

        // imprime os movimentos possiveis
        if (recv_action.type == 4) {
            printf ("Possible moves:");
            for (int i = 0; i < 4; i++) {
                if (moves[i] == 1)
                    printf (" up");
                else if (moves[i] == 2)
                    printf (" right");
                else if (moves[i] == 3)
                    printf (" down");
                else if (moves[i] == 4)
                    printf (" left");

                if(moves[i+1] == 0)
                {
                    printf(".");
                    break;
                }
                else
                {
                    printf(",");
                }
            }
            printf("\n");
        }
        
        if (send_action.type == 2) {
            for (int i = 0; i < 10; i++) {
                int cont = 0;
                for (int j = 0; j < 10; j++) {
                    if (recv_action.board[i][j] == 0)
                        printf("# ");
                    else if (recv_action.board[i][j] == 1)
                        printf("_ ");
                    else if (recv_action.board[i][j] == 2)
                        printf("> ");
                    else if (recv_action.board[i][j] == 3)
                        printf("X ");
                    else if (recv_action.board[i][j] == 4)
                        printf("? ");
                    else if (recv_action.board[i][j] == 5)
                        printf("+ ");
                    else
                        cont++;
                }
                if (cont != 10)
                    printf("\n");
            }
        }

        if (recv_action.type == 5) {
            printf("You escaped!\n");
                for (int i = 0; i < 10; i++) {
                int cont = 0;
                    for (int j = 0; j < 10; j++) {
                        if (recv_action.board[i][j] == 0)
                            printf("# ");
                        else if (recv_action.board[i][j] == 1)
                            printf("_ ");
                        else if (recv_action.board[i][j] == 2)
                            printf("> ");
                        else if (recv_action.board[i][j] == 3)
                            printf("X ");
                        else
                            cont++;
                    }
                    if (cont != 10)
                        printf("\n");
                }

            scanf("%s", msg);
        }
        //--------------------------------------------------------------------
    }

    exit(EXIT_SUCCESS);
} 