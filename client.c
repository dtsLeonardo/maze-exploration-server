#include "common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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
    printf("usage: %s <server IP> <server port>\n", argv[0]);
    printf("example: %s 127.0.0.1 51511\n", argv[0]);
    exit(EXIT_FAILURE);
}

void print_board(int board[MAX_SIZE][MAX_SIZE]) {
    printf("\nLabirinto revelado:\n");
    for (int i = 0; i < MAX_SIZE; i++) {
        for (int j = 0; j < MAX_SIZE; j++) {
            if (board[i][j] == -1) {
                printf("   "); // Área não revelada
            } else {
                printf("%2d ", board[i][j]); // Elemento revelado
            }
        }
        printf("\n");
    }
}

void send_action(int socket, int type, int *moves) {
    struct action request = {0};
    request.type = type;
    if (moves) {
        memcpy(request.moves, moves, MAX_MOVES * sizeof(int));
    }
    send(socket, &request, sizeof(request), 0);
}

void handle_response(int socket) {
    struct action response = {0};
    recv(socket, &response, sizeof(response), 0);

    switch (response.type) {
        case UPDATE:
            print_board(response.board);
            printf("Aguardando próxima ação.\n");
            break;

        case WIN:
            printf("Parabéns! Você venceu o jogo!\n");
            close(socket);
            exit(0);

        default:
            printf("Resposta desconhecida recebida do servidor.\n");
            break;
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        usage(argc, argv);
    }

    struct sockaddr_storage storage;
	if (0 != addrparse(argv[1], argv[2], &storage)) {
		usage(argc, argv);
	}

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);

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

	printf("connected to %s\n", addrstr);

	char buf[BUFSZ];
	memset(buf, 0, BUFSZ);
	printf("mensagem> ");
	fgets(buf, BUFSZ-1, stdin);
	size_t count = send(s, buf, strlen(buf)+1, 0);
	if (count != strlen(buf)+1) {
		logexit("send");
	}

	memset(buf, 0, BUFSZ);
    unsigned total = 0;
    printf("Conectado ao servidor!\n");
    send_action(client_socket, START, NULL);
    handle_response(client_socket);

    while (1) {
        printf("\nEscolha uma ação:\n");
        printf("1. Mover\n");
        printf("2. Mostrar mapa\n");
        printf("3. Resetar jogo\n");
        printf("4. Sair\n");

        int choice, direction;
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("Escolha a direção do movimento:\n");
                printf("1. Cima\n");
                printf("2. Direita\n");
                printf("3. Baixo\n");
                printf("4. Esquerda\n");
                scanf("%d", &direction);
                send_action(client_socket, MOVE, &direction);
                break;

            case 2:
                send_action(client_socket, MAP, NULL);
                break;

            case 3:
                send_action(client_socket, RESET, NULL);
                break;

            case 4:
                send_action(client_socket, EXIT, NULL);
                close(client_socket);
                printf("Desconectado do servidor.\n");
                exit(0);

            default:
                printf("Opção inválida! Tente novamente.\n");
                continue;
        }

        handle_response(client_socket);
        count = recv(s, buf + total, BUFSZ - total, 0);
		if (count == 0) {
			// Connection terminated.
			break;
		}
		total += count;
    }
    close(s);

    printf("received %u bytes\n", total);
    puts(buf);

    exit(EXIT_SUCCESS);
}