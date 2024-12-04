#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/socket.h>
#include <sys/types.h>

#define BUFSZ 1024
#define TAM_MAX_BOARD 10

// Estrutura de dados 'action'
struct action {
    int type;
    int moves[100];
    int board[TAM_MAX_BOARD][TAM_MAX_BOARD];
};

void usage(int argc, char **argv) {
    printf("usage: %s <v4|v6> <server port> -i <input_file>\n", argv[0]);
    printf("example: %s v4 51511 -i input/in.txt\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv) {
    if (argc < 5) {
        usage(argc, argv);
    }

    if (strcmp(argv[3], "-i") != 0) {
        usage(argc, argv);
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
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0) {
        logexit("setsockopt");
    }

    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if (bind(s, addr, sizeof(storage)) != 0) {
        logexit("bind");
    }

    if (listen(s, 10) != 0) {
        logexit("listen");
    }

    char addrstr[BUFSZ];
    addrtostr(addr, addrstr, BUFSZ);
    printf("bound to %s, waiting connections\n", addrstr);

    // Abre o arquivo
    FILE *file = fopen(argv[4], "r");
    if (!file) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    int linhas = 0;
    int colunas = 0;
    obterDimensoes(argv[4], &linhas, &colunas);

    int posicaoX = 0;
    int posicaoY = 0;

    int matriz[linhas][colunas];
    int mapa[TAM_MAX_BOARD][TAM_MAX_BOARD];

    //Define posição inicial do jogador
    for (int i = 0; i < linhas; i++) {
        for (int j = 0; j < colunas; j++) {
            fscanf(file, "%d", &matriz[i][j]);
            if(matriz[i][j] == 2) {
                posicaoX = i;
                posicaoY = j;
            }
        }
    }

    //Define mapa inicial
    for (int i = 0; i < TAM_MAX_BOARD; i++)
        for (int j = 0; j < TAM_MAX_BOARD; j++)
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
            struct action receberMensagem;
            memset(&receberMensagem, 0, sizeof(receberMensagem));
            recv(csock, &receberMensagem, sizeof(receberMensagem), 0);

            struct action enviarMensagem;
            memset(&enviarMensagem, 0, sizeof(enviarMensagem));

            inicializarBoard(enviarMensagem.board);

            mapa[posicaoX][posicaoY] = matriz[posicaoX][posicaoY];
            if (posicaoX > 0)
                mapa[posicaoX - 1][posicaoY] = matriz[posicaoX - 1][posicaoY];
            if (posicaoY > 0)
                mapa[posicaoX][posicaoY - 1] = matriz[posicaoX][posicaoY - 1];
            if (posicaoY < (colunas-1))
                mapa[posicaoX][posicaoY + 1] = matriz[posicaoX][posicaoY + 1];
            if (posicaoX < (linhas-1))
                mapa[posicaoX + 1][posicaoY] = matriz[posicaoX + 1][posicaoY];

            switch (receberMensagem.type) {
                case 1: // Move o jogador
                    int direcoes[4][2] = {
                        {-1, 0}, // Move para cima
                        {0, 1},  // Move para direita
                        {1, 0},  // Move para baixo
                        {0, -1}  // Move para esquerda
                    };

                    int direcao = receberMensagem.moves[0] - 1; // Ajusta para índice do array
                    int novoX = posicaoX + direcoes[direcao][0];
                    int novoY = posicaoY + direcoes[direcao][1];

                    if (novoX >= 0 && novoX < linhas && novoY >= 0 && novoY < colunas && matriz[novoX][novoY] != 0) {
                        posicaoX = novoX;
                        posicaoY = novoY;
                    }
                    break;

                case 2: // Atualiza o board
                    for (int i = 0; i < linhas; i++) {
                        memcpy(enviarMensagem.board[i], mapa[i], colunas * sizeof(int));
                    }
                    enviarMensagem.board[posicaoX][posicaoY] = 5;
                    break;

                case 6: // Reinicia o jogo
                    posicaoX = 0;
                    posicaoY = 0;
                    matriz[posicaoX][posicaoY] = 2;
                    for (int i = 0; i < TAM_MAX_BOARD; i++) {
                        for (int j = 0; j < TAM_MAX_BOARD; j++) {
                            mapa[i][j] = 4;
                        }
                    }
                    printf("starting new game\n");
                    break;

                case 7: // Cliente desconectado
                    printf("client disconnected\n");
                    exit(1);
                    break;

                default: // Tipo de mensagem desconhecida
                    break;
            }
            
            printf("Posição atual: (%d, %d) = %d\n", posicaoX, posicaoY, matriz[posicaoX][posicaoY]);
            verificaEntorno(enviarMensagem.moves, linhas, colunas, matriz, posicaoX, posicaoY);

            enviarMensagem.type = 4;

            // Verifica se o jogador escapou
            if (matriz[posicaoX][posicaoY] == 3) {
                enviarMensagem.type = 5;
                for (int i = 0; i < linhas; i++)
                    for (int j = 0; j < colunas; j++) 
                        enviarMensagem.board[i][j] = matriz[i][j];
            }

            send(csock, &enviarMensagem, sizeof(enviarMensagem), 0);

            printf("----------------------------------------------------------\n");
        }
    }

    exit(EXIT_SUCCESS);
}


