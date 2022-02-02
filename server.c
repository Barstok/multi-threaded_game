#include "server.h"
#include "const.h"
#include <stdlib.h>
#include <ncurses.h>
#include <unistd.h>

char map[MAX_MAP_SIZE][MAX_MAP_SIZE];
int cols, rows;


int load_map(char* filename){
    if(!filename) return -1;

    FILE* f = fopen(filename,"r");
    if(!f) return -2;

    fscanf(f,"%d",&rows);
    fscanf(f,"%d",&cols);

    int row=0;
    while(!feof(f)&&row<MAX_MAP_SIZE){
        fscanf(f,"%s",(char*)map[row]);
        for(int x=0;x<cols;x++){
            if(map[row][x]=='.') map[row][x]=' ';
        }
        row++;
    }
    
    fclose(f);

    if(row!=rows) return -3;

    map[1][5]='A';

    return 0;
}

void screen_setup(){
    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr,TRUE);
    cbreak();
    init_colors();
    }

void close_screen(){
    endwin();
}

void init_colors(){
    start_color();


    init_pair(PLAYER,COLOR_GREEN,COLOR_BLACK);
    init_pair(WALL,COLOR_WHITE,COLOR_WHITE);
    init_pair(DEFAULT,COLOR_WHITE,COLOR_BLACK);
    init_pair(BUSH,COLOR_BRIGHT_GREEN,COLOR_BLACK);
    init_pair(COIN,COLOR_YELLOW,COLOR_BLACK);
    init_pair(CAMPSITE,COLOR_GOLDEN,COLOR_DARK_GREEN);
}

void print_map(){
    for(int x=0;x<rows;x++){
        for(int y=0;y<cols;y++){
            
            switch(map[x][y]){
                case '|':
                    attron(COLOR_PAIR(WALL));
                    break;
                case '#':
                    attron(COLOR_PAIR(BUSH));
                    break;
                case 'c':
                case 't':
                case 'T':
                    attron(COLOR_PAIR(COIN));
                    break;
                case 'A':
                    attron(COLOR_PAIR(CAMPSITE));
                    break;
                default:
                    attron(COLOR_PAIR(DEFAULT));
            }

            mvprintw(x,y,"%c",map[x][y]);
        }
    }
}

void* print_screen(void* arg){
    print_map();
    refresh();
}

void run_server(){
    screen_setup();
    load_map("map.txt");
    print_screen(NULL);
    getchar();

    close_screen();
}