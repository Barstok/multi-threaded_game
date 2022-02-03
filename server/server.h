#ifndef _SERVER_
#define _SERVER_
#include "const.h"
#include <pthread.h>

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
    enum server_state_t server_state;
    int round_number;
    int map_empty_space;
    int treasure_count;

    int players_connected;
    struct player_data_t player_data[MAX_PLAYERS];
    pthread_t player_pt[MAX_PLAYERS];
};

int load_map(char* filename);
void drop_coins(enum treasure_type t);
void* key_handler(void* arg);
void screen_setup();
void close_screen();
void init_colors();
int player_init();
void* player_connection(void* arg);
void* players_queue(void* arg);
void print_map();
void print_legend();
void print_server_data();
void* print_screen(void* arg);
void run_server();

#endif