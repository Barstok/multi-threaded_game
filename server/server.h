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

struct dropped_treasure_t{
    struct point_t point;
    int value;
    struct dropped_treasure_t* next;
};

struct dropped_treasure_list_t{
    struct dropped_treasure_t* head;
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
    int PID;

    enum type t;

    struct point_t spawn_point;
    struct point_t position;

    int moved;

    int deaths;
    int slowed_down;

    int coins_carried;
    int coins_brought;

};

struct beast_data_t{
    struct point_t pos;
    int moved;
};

struct server_data_t{
    int PID;

    struct point_t campsite;
    enum server_state_t server_state;
    int round_number;
    int map_empty_space;
    int treasure_count;

    struct dropped_treasure_list_t dropped_treasure;

    int players_connected;
    struct player_data_t player_data[MAX_PLAYERS];
    pthread_t player_pt_r[MAX_PLAYERS];
    pthread_t player_pt_s[MAX_PLAYERS];
    pthread_t beasts_pt[MAX_BEASTS];
    struct beast_data_t beasts[MAX_BEASTS];
    int beasts_count;
};

void dropped_treasure_insert(int x, int y, int value);
void dropped_treasure_list_clear();

int load_map(char* filename);
void drop_coins(enum treasure_type t);
enum map_elements collision_check(struct point_t* point);
void* key_handler(void* arg);
void screen_setup();
void close_screen();
void init_colors();
int player_init();
void player_quit(int player_num);
void player_collision_handle(int player_num);
void player_move(int a,int player_num);
void* player_connection_reader(void* arg);
void* player_connection_sender(void* arg);
void* players_queue(void* arg);
void beast_init();
void* run_beast(void* arg);
void print_map();
void print_legend();
void print_server_data();
void* print_screen(void* arg);
void* run_clock(void* arg);
void run_server();

#endif