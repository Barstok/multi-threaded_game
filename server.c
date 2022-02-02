#include "server.h"
#include "const.h"
#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>

char map[MAX_MAP_SIZE][MAX_MAP_SIZE];
struct server_data_t server_data;
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

    return 0;
}

void server_data_init(){
    server_data.PID=getpid();
    server_data.players_connected=0;
    server_data.round_number=0;

    server_data.player_data[0].connected=1;
    server_data.player_data[1].connected=1;
    server_data.player_data[2].connected=1;
    server_data.player_data[3].connected=1;

    server_data.player_data[2].t=1;

    server_data.campsite.x=11;
    server_data.campsite.y=23;
    map[11][23] = 'A';

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


    init_pair(PLAYER,COLOR_WHITE,COLOR_MAGENTA);
    init_pair(WALL,COLOR_WHITE,COLOR_WHITE);
    init_pair(DEFAULT,COLOR_WHITE,COLOR_BLACK);
    init_pair(BEAST,COLOR_ORANGE,COLOR_BLACK);
    init_pair(BUSH,COLOR_BRIGHT_GREEN,COLOR_BLACK);
    init_pair(COIN,COLOR_BLACK,COLOR_YELLOW);
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

void print_legend(){
    int row=18;
    int col=cols+2;

    attron(A_BOLD);
    mvprintw(row,col,"Legend");
    attroff(A_BOLD);

    col++;
    row++;
    attron(COLOR_PAIR(PLAYER));
    mvprintw(row,col,"1234");
    attroff(COLOR_PAIR(PLAYER));
    mvprintw(row,col+5,"- players");

    attron(COLOR_PAIR(WALL));
    mvprintw(++row,col,"|");
    attroff(COLOR_PAIR(WALL));
    mvprintw(row,col+5,"- wall");

    attron(COLOR_PAIR(BUSH));
    mvprintw(++row,col,"#");
    attroff(COLOR_PAIR(BUSH));
    mvprintw(row,col+5,"- bush");
    
    attron(COLOR_PAIR(BEAST));
    mvprintw(++row,col,"*");
    attroff(COLOR_PAIR(BEAST));
    mvprintw(row,col+5,"- beast");

    attron(COLOR_PAIR(CAMPSITE));
    mvprintw(++row,col,"A");
    attroff(COLOR_PAIR(CAMPSITE));
    mvprintw(row,col+5,"- campsite");

    row-=4;
    col+=20;

    attron(COLOR_PAIR(COIN));
    mvprintw(row,col,"c");
    mvprintw(++row,col,"t");
    mvprintw(++row,col,"T");
    mvprintw(++row,col,"D");
    attroff(COLOR_PAIR(COIN));

    row-=3;
    col+=3;
    mvprintw(row,col,"- one coin");
    mvprintw(++row,col,"- treasure (10 coins)");
    mvprintw(++row,col,"- large treasure (50 coins)");
    mvprintw(++row,col,"- dropped treasure");
}

void print_server_data(){
    int row=1;
    int col=cols+2;
    attron(COLOR_PAIR(DEFAULT));
    attron(A_BOLD);
    mvprintw(row,col,"Server's PID: %d",server_data.PID);
    attroff(A_BOLD);
    mvprintw(++row,++col,"Campsite cords: (%d,%d)",server_data.campsite.x,server_data.campsite.y);
    mvprintw(++row,col,"Round number: %d",server_data.round_number);

    row+=4;
    mvprintw(row,--col,"Parameters    Player1    Player2    Player3    Player4");
    mvprintw(++row,++col,"PID");
    mvprintw(++row,col,"Type");
    mvprintw(++row,col,"Cords");
    mvprintw(++row,col,"Deaths");

    row+=2;
    attron(A_BOLD);
    mvprintw(row,--col,"Coins");
    attroff(A_BOLD);
    mvprintw(++row,++col,"carried");
    mvprintw(++row,col,"brought");

    row-=7;
    col+=13;
    for(int x=0;x<MAX_PLAYERS;x++){
        if(server_data.player_data[x].connected){
            mvprintw(row,col+11*x,"%d",420);
            if(server_data.player_data[x].t==HUMAN){
                mvprintw(row+1,col+11*x,"HUMAN");
            }else{
                mvprintw(row+1,col+11*x,"CPU");
            }
            mvprintw(row+2,col+11*x,"(%d,%d)",server_data.player_data[x].position.x,server_data.player_data[x].position.y);
            mvprintw(row+3,col+11*x,"%d",server_data.player_data[x].deaths);

            mvprintw(row+6,col+11*x,"%d",server_data.player_data[x].coins_carried);
            mvprintw(row+7,col+11*x,"%d",server_data.player_data[x].coins_brought);
        }
    }
}

void* print_screen(void* arg){
    print_legend();
    while(1){
        print_map();
        print_server_data();
        refresh();
        usleep(ROUND_TIME_MS*1000);
        server_data.round_number++;
    }
}

void run_server(){
    screen_setup();
    load_map("map.txt");
    server_data_init();

    pthread_t pt_screen;

    pthread_create(&pt_screen,NULL,print_screen,NULL);

    pthread_join(pt_screen,NULL);

    close_screen();
}