#pragma once

#include <stdlib.h>
#include <arpa/inet.h>

typedef struct {
    double latitude;
    double longitude;
} Coordinate;

void logexit(const char *msg);

int client_sockaddr_init(const char *proto, const char *addrstr, const char *portstr,
                struct sockaddr_storage *storage);

void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);

int server_sockaddr_init(const char *proto, const char* portstr,
                            struct sockaddr_storage *storage);

int calculateDistanceCoordinates(double lat1, double lon1,
                        double lat2, double lon2);
                        
int count_characters(const char *str);

void print_menu(const char *title, int num_mesages, const char **mesages) ;