#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <arpa/inet.h>

#define TAM_MAX_BOARD 10

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

void logexit(const char *msg);

int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char *portstr, struct sockaddr_storage *storage);

Comando mapearComando(char* comando);

bool validaMovimento(int moves[4], int num);

void verificaAoRedor(int* resultado, int linhas, int colunas, int matriz[linhas][colunas], int posicaoX, int posicaoY);

void obterDimensoes(const char *arquivo, int *linhas, int *colunas);

void enviaERecebeMensagem(const char *arquivo, int *linhas, int *colunas);

void inicializarBoard(int board[TAM_MAX_BOARD][TAM_MAX_BOARD]);