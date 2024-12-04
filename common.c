#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <arpa/inet.h>

#define TAM_MAX_BOARD 10

struct action {
    int type;
    int moves[100];
    int board[TAM_MAX_BOARD][TAM_MAX_BOARD];
};

/*Lógica de comunicação*/

void logexit(const char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}

int addrparse(const char *addrstr, const char *portstr,
              struct sockaddr_storage *storage) {
    if (addrstr == NULL || portstr == NULL) {
        return -1;
    }

    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    if (port == 0) {
        return -1;
    }
    port = htons(port); // host to network short

    struct in_addr inaddr4; // 32-bit IP address
    if (inet_pton(AF_INET, addrstr, &inaddr4)) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_port = port;
        addr4->sin_addr = inaddr4;
        return 0;
    }

    struct in6_addr inaddr6; // 128-bit IPv6 address
    if (inet_pton(AF_INET6, addrstr, &inaddr6)) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_port = port;
        // addr6->sin6_addr = inaddr6
        memcpy(&(addr6->sin6_addr), &inaddr6, sizeof(inaddr6));
        return 0;
    }

    return -1;
}

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize) {
    int version;
    char addrstr[INET6_ADDRSTRLEN + 1] = "";
    uint16_t port;

    if (addr->sa_family == AF_INET) {
        version = 4;
        struct sockaddr_in *addr4 = (struct sockaddr_in *)addr;
        if (!inet_ntop(AF_INET, &(addr4->sin_addr), addrstr,
                       INET6_ADDRSTRLEN + 1)) {
            logexit("ntop");
        }
        port = ntohs(addr4->sin_port); // network to host short
    } else if (addr->sa_family == AF_INET6) {
        version = 6;
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)addr;
        if (!inet_ntop(AF_INET6, &(addr6->sin6_addr), addrstr,
                       INET6_ADDRSTRLEN + 1)) {
            logexit("ntop");
        }
        port = ntohs(addr6->sin6_port); // network to host short
    } else {
        logexit("unknown protocol family.");
    }
    if (str) {
        snprintf(str, strsize, "IPv%d %s %hu", version, addrstr, port);
    }
}

int server_sockaddr_init(const char *proto, const char *portstr,
                         struct sockaddr_storage *storage) {
    uint16_t port = (uint16_t)atoi(portstr); // unsigned short
    if (port == 0) {
        return -1;
    }
    port = htons(port); // host to network short

    memset(storage, 0, sizeof(*storage));
    if (0 == strcmp(proto, "v4")) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = port;
        return 0;
    } else if (0 == strcmp(proto, "v6")) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = port;
        return 0;
    } else {
        return -1;
    }
}

/*Lógica para o cliente*/

typedef enum {
    UP,
    RIGHT,
    DOWN,
    LEFT,
    MAP,
    RESET,
    EXIT,
    INVALID
} Comando;

// Função para mapear o comando recebido para um valor do enum
Comando mapearComando(char* comando) {
    if (strcmp(comando, "up") == 0) return UP;
    if (strcmp(comando, "right") == 0) return RIGHT;
    if (strcmp(comando, "down") == 0) return DOWN;
    if (strcmp(comando, "left") == 0) return LEFT;
    if (strcmp(comando, "map") == 0) return MAP;
    if (strcmp(comando, "reset") == 0) return RESET;
    if (strcmp(comando, "exit") == 0) return EXIT;
    return INVALID; // Retorna INVALID se o comando não for reconhecido
}

bool validaMovimento(int moves[4], int num) {
    bool found = false;
    for (int i = 0; i < 4; i++) {
        if (moves[i] == num) {
            found = true;
            break;
        }
    }
    return found;
}

/*Lógica para o servidor*/

void verificaAoRedor(int* resultado, int linhas, int colunas, int matriz[linhas][colunas], int posicaoX, int posicaoY) {
    memset(resultado, 0, 100 * sizeof(int));

    int count = 0;
    int direcoes[4][2] = {
        {-1, 0},  // Cima
        {0, 1},   // Direita
        {1, 0},   // Baixo
        {0, -1}   // Esquerda
    };

    for (int i = 0; i < 4; i++) {
        int novoX = posicaoX + direcoes[i][0];
        int novoY = posicaoY + direcoes[i][1];

        if (novoX >= 0 && novoX < linhas && novoY >= 0 && novoY < colunas && matriz[novoX][novoY] != 0) {
            resultado[count] = i + 1;
            count++;
        }
    }
}

void obterDimensoes(const char *nomeArquivo, int *numLinhas, int *numColunas) {
    FILE *arquivo = fopen(nomeArquivo, "r");
    if (!arquivo) {
        perror("Erro ao abrir arquivo");
        exit(EXIT_FAILURE);
    }

    *numLinhas = 0;
    *numColunas = 0;
    char linha[256];

    // Contar o número de linhas.
    while (fgets(linha, sizeof(linha), arquivo)) {
        (*numLinhas)++;
    }

    rewind(arquivo);

    if (fgets(linha, sizeof(linha), arquivo)) {
        const char *delimitadores = " \t\n";
        char *palavra = strtok(linha, delimitadores);
        while (palavra) {
            (*numColunas)++;
            palavra = strtok(NULL, delimitadores);
        }
    }

    fclose(arquivo);
}

void inicializarBoard(int board[TAM_MAX_BOARD][TAM_MAX_BOARD]) {
    for (int i = 0; i < TAM_MAX_BOARD; i++) {
        for (int j = 0; j < TAM_MAX_BOARD; j++) {
            board[i][j] = -1;
        }
    }
}



