#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 1024

// Estrutura de dados 'action'
struct action {
    int type;
    int moves[100];
    int board[10][10];
};

void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port> -i <input_file>\n", argv[0]);
    printf("example: %s v4 51511 -i input/in.txt\n", argv[0]);
    exit(EXIT_FAILURE);
}

void verificaEntorno(int* resultado, int linhas, int colunas, int matriz[linhas][colunas], int posX, int posY, int num)
{
    memset(resultado, 0, 100 * sizeof(int)); // Inicializa os primeiros 4 elementos com 0

    int count = 0;
    if (posX > 0 && !(matriz[posX - 1][posY] == num)) { // Cima
        resultado[count] = 1;
        count++;
    }
    if (posY < (colunas - 1) && !(matriz[posX][posY + 1] == num)) { // Direita
        resultado[count] = 2;
        count++;
    }
    if (posX < (linhas - 1) && !(matriz[posX + 1][posY] == num)) { // Baixo
        resultado[count] = 3;
        count++;
    }
    if (posY > 0 && !(matriz[posX][posY - 1] == num)) { // Esquerda
        resultado[count] = 4;
        count++;
    }
}

int main(int argc, char **argv) {
    if (argc < 5) {
        usage(argc, argv);
    }

    if (strcmp(argv[3], "-i") != 0) {
        usage(argc, argv);
    }

    // Abre o arquivo passado no comando
    FILE *file = fopen(argv[4], "r");
    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_storage storage;
    if (0 != server_sockaddr_init(argv[1], argv[2], &storage)) {
        usage(argc, argv);
    }

    int s;
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

    // end base code
    // ---------------------------------------------------------------------------------

    // Conta cada linha lida
    int linhas = 0;
    char bufferl[100];
    while (fgets(bufferl, sizeof(bufferl), file)) {
        linhas++;
    }
    rewind(file); // Rebobina o arquivo para reutilizá-lo

    // Lê a primeira linha e conta os valores separados por espaços/tabs
    int colunas = 0;
    char bufferc[100];
    if (fgets(bufferc, sizeof(bufferc), file)) {
        char *token = strtok(bufferc, " \t\n");
        while (token) {
            colunas++;
            token = strtok(NULL, " \t\n");
        }
    }
    rewind(file);

    // posição atual do jogador
    int posX = -1;
    int posY = -1;

    // criação de matrizes
    int matriz[linhas][colunas];    //labirinto
    int mapa[10][10];      //mapa do labirinto
    //memset(&mapa, 4, sizeof(mapa)); // inicializa o mapa todo como não descoberto

    // copia o arquivo para a matriz[][] e Encontra a posição inicial (onde o valor é 2)
    for (int i = 0; i < linhas; i++) {
        for (int j = 0; j < colunas; j++) {
            fscanf(file, "%d", &matriz[i][j]);  // Lê cada elemento da matriz
            if(matriz[i][j] == 2) {
                posX = i;  // Coluna (x)
                posY = j;  // Linha (y)
            }
        }
    }

    for (int i = 0; i < 10; i++)
        for (int j = 0; j < 10; j++)
            mapa[i][j] = 4;

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
        printf("client connected\n");
        while(1) {
            // Recebe a mensagem struct
            //---------------------------------------------------------------------------------
            struct action recv_action;
            memset(&recv_action, 0, sizeof(recv_action)); // preenche a struct toda com 0

            size_t count = recv(csock, &recv_action, sizeof(recv_action), 0);
            if (count == -1) {
                logexit("recv");
            }
            //---------------------------------------------------------------------------------


            // imprime tudo que recebeu
            //---------------------------------------------------------------------------------
            // printf("[msg] Received action with type: %d\n", recv_action.type);
            for (int i = 0; i < 100; i++) {
                //printf("Move %d: %d\n", i, recv_action.moves[i]);
            }
            for (int i = 0; i < 10; i++) {
                for (int j = 0; j < 10; j++) {
                    //printf("Board[%d][%d]: %d\n", i, j, recv_action.board[i][j]);
                }
            }
            
            // cria a mensagem struct para enviar
            struct action send_action;
            memset(&send_action, 0, sizeof(send_action)); // preenche tudo com 0
            for (int i = 0; i < 10; i++)
                for (int j = 0; j < 10; j++)
                    send_action.board[i][j] = -1;
            //---------------------------------------------------------------------------------

            // logica do codigo
            //-----------------------------------------------------------------------------------------
            // atualiza o mapa por onde o jogador for passando
            mapa[posX][posY] = matriz[posX][posY];
            if (posX > 0)
                mapa[posX - 1][posY] = matriz[posX - 1][posY];
            if (posY > 0)
                mapa[posX][posY - 1] = matriz[posX][posY - 1];
            if (posY < (colunas-1))
                mapa[posX][posY + 1] = matriz[posX][posY + 1];
            if (posX < (linhas-1))
                mapa[posX + 1][posY] = matriz[posX + 1][posY];

            if (recv_action.type == 2) {
                for (int i = 0; i < linhas; i++)
                    for (int j = 0; j < colunas; j++)
                        send_action.board[i][j] = mapa[i][j];
                send_action.board[posX][posY] = 5;
            }

            
            //realiza o movimento se os movimento enviado
            if (recv_action.type == 1) {
                if (posX > 0 && recv_action.moves[0] == 1 && !(matriz[posX-1][posY] == 0)) {
                    posX -= 1;
                }
                else if(posY < (colunas-1) && recv_action.moves[0] == 2 && !(matriz[posX][posY+1] == 0)) {
                    posY += 1;
                }
                else if(posX < (linhas-1) && recv_action.moves[0] == 3 && !(matriz[posX+1][posY] == 0)) {
                    posX += 1;
                }
                else if(posY > 0 && recv_action.moves[0] == 4 && !(matriz[posX][posY-1] == 0)) {
                    posY -= 1;
                }
            }

            if (recv_action.type == 6) {
                posX = 0;
                posY = 0;
                matriz[posX][posY] = 2;
                for (int i = 0; i < 10; i++)
                    for (int j = 0; j < 10; j++)
                        mapa[i][j] = 4;
                printf("starting new game\n");
            }

            if (recv_action.type == 7) {
                printf("client disconnected\n");
                exit(1);
            }
            
            printf("Posição atual: (%d, %d) = %d\n", posX, posY, matriz[posX][posY]);
            verificaEntorno(send_action.moves, linhas, colunas, matriz, posX, posY, 0);



    //-----------------------------------------------------------------------------------------

            send_action.type = 4;

            if (matriz[posX][posY] == 3) {
                send_action.type = 5;
                for (int i = 0; i < linhas; i++)
                    for (int j = 0; j < colunas; j++) 
                        send_action.board[i][j] = matriz[i][j];
            }


            // Envia a mensagem struct
            //---------------------------------------------------------------------------------
            count = send(csock, &send_action, sizeof(send_action), 0);

            if (count != sizeof(send_action)) {
                logexit("send");
            }
            //---------------------------------------------------------------------------------
            
            //close(csock);

            printf("----------------------------------------------------------\n");
        }
    }

    exit(EXIT_SUCCESS);
}


