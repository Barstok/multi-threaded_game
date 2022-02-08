#ifndef _CLIENT_
#define _CLIENT_
#include "const.h"

extern int rows;
extern int cols;

struct point_t{
    int x;
    int y;
};

struct player_data_chunk_t{
    char map_chunk[SIGHT][SIGHT];
    struct point_t beginning;
    struct point_t pos;
    int deaths;
    int coins_carried;
    int coins_brought;

    int round_number;
};

struct player_data_t{
    int connected;
    int server_PID;
    int player_number;
    int round_number;

    enum type t;
    
    struct point_t campsite;
    struct point_t position;

    int deaths;

    int coins_carried;
    int coins_brought;
};

void screen_setup();
void close_screen();
void init_colors();

void print_map(struct player_data_chunk_t p_data);
void print_legend();
void print_server_data();
void* print_screen(void* arg);

void* key_handler(void* arg);
int connect_to_server();

void run_client();

#endif