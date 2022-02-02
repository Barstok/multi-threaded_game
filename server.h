#ifndef _SERVER_
#define _SERVER_
#include "const.h"

extern char map[MAX_MAP_SIZE][MAX_MAP_SIZE];
extern int rows;
extern int cols;

struct point_t{
    int x;
    int y;
};

struct player_data_t{
    int connected;
    int PID;
    int server_PID;

    enum type t;

    struct point_t position;

    int deaths;
    int slowed_down;

    int coins_carried;
    int coins_brought;

};

struct server_data_t{
    int PID;

    struct point_t campsite;
    int round_number;

    int players_connected;
    struct player_data_t player_data[MAX_PLAYERS];
};

int load_map(char* filename);
void screen_setup();
void close_screen();
void init_colors();
void print_map();
void print_legend();
void print_server_data();
void* print_screen(void* arg);
void run_server();

#endif