#ifndef _SERVER_
#define _SERVER_
#include "const.h"

extern char map[MAX_MAP_SIZE][MAX_MAP_SIZE];
extern int rows;
extern int cols;

int load_map(char* filename);
void screen_setup();
void close_screen();
void init_colors();
void print_map();
void* print_screen(void* arg);
void run_server();

#endif