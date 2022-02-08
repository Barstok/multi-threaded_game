#include "client.h"
#include "const.h"
#include "stdlib.h"
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>


struct player_data_t player_data;
int rows=25;
int cols=51;

pthread_mutex_t game_mt;

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

void print_map(struct player_data_chunk_t p_data){
    for(int x=0;x<rows;x++){
        for(int y=0;y<cols;y++){
            attron(COLOR_PAIR(DEFAULT));
            mvprintw(x,y," ");
        }
    }

    for(int x=0;x<SIGHT;x++){
        for(int y=0;y<SIGHT;y++){
            switch(p_data.map_chunk[x][y]){
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
                case '1':
                case '2':
                case '3':
                case '4':
                    attron(COLOR_PAIR(PLAYER));
                    break;
                case '*':
                    attron(COLOR_PAIR(BEAST));
                    break;
                case 'A':
                    attron(COLOR_PAIR(CAMPSITE));
                    break;
                default:
                    attron(COLOR_PAIR(DEFAULT));
            }

            mvprintw(p_data.beginning.x+x,p_data.beginning.y+y,"%c",p_data.map_chunk[x][y]);
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
    mvprintw(row,col,"Server's PID: %d",player_data.server_PID);
    attroff(A_BOLD);
    mvprintw(++row,++col,"Campsite cords: (%d,%d)",player_data.campsite.y);
    mvprintw(++row,col,"Round number: %d",player_data.round_number);

    row+=4;
    mvprintw(row,--col,"Player");
    mvprintw(++row,++col,"Number");
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
    if(player_data.connected){
        mvprintw(row,col+11,"%d",player_data.player_number);
        if(player_data.t==HUMAN){
            mvprintw(row+1,col+11,"HUMAN");
        }else{
               mvprintw(row+1,col+11,"CPU");
        }
        mvprintw(row+2,col+11,"(%d,%d)     ",player_data.position.x,player_data.position.y);
        mvprintw(row+3,col+11,"%d    ",player_data.deaths);

        mvprintw(row+6,col+11,"%d    ",player_data.coins_carried);
        mvprintw(row+7,col+11,"%d    ",player_data.coins_brought);
    }
    else{
        mvprintw(row,col+11,"    ");
        mvprintw(row+1,col+11,"       ");
        mvprintw(row+2,col+11,"       ");
        mvprintw(row+3,col+11,"     ");
        mvprintw(row+6,col+11,"     ");
        mvprintw(row+7,col+11,"     ");
    }
}

void* print_screen(void* arg){
    char fifo_dir[24];

    sprintf(fifo_dir,"../temp/%d_s_fifo",getpid());

    int fd = open(fifo_dir,O_RDONLY);
    int attempt_counter=0;
    while(fd==-1&&attempt_counter<5){
        fd = open(fifo_dir,O_RDONLY);
        attempt_counter++;
        usleep(10000);
    }

    if(fd==-1){
        mvprintw(10,10,"Server is either full or closed");
        refresh();
        return NULL;
    }

    struct player_data_chunk_t p_data;

    if(read(fd,&player_data.server_PID,sizeof(int))!=sizeof(int)){
        mvprintw(10,10,"errno pid %d",errno);
        close(fd);
        refresh();
        return NULL;
    }
    
    if(read(fd,&player_data.player_number,sizeof(int))!=sizeof(int)){
        mvprintw(10,10,"errno num %d",errno);
        close(fd);
        refresh();
        return NULL;
    }
    pthread_mutex_lock(&game_mt);
    player_data.connected=1;
    pthread_mutex_unlock(&game_mt);
    print_legend();
    while(1){
        if(!player_data.connected){
            close(fd);
            return NULL;
            }

        if(read(fd,&p_data,sizeof(struct player_data_chunk_t))!=sizeof(struct player_data_chunk_t)){
            close(fd);
            return NULL;
        }
        player_data.round_number=p_data.round_number;
        player_data.position=p_data.pos;
        player_data.deaths=p_data.deaths;
        player_data.coins_carried=p_data.coins_carried;
        player_data.coins_brought=p_data.coins_brought;

        print_map(p_data);
        print_server_data();
        refresh();
        usleep(ROUND_TIME_MS*500);
    }
}

void* key_handler(void* arg){
    
    char client_fifo_dir[22];

    sprintf(client_fifo_dir,"../temp/%d_c_fifo",getpid());

    int fd = open(client_fifo_dir,O_WRONLY);

    int attempt_counter=0;
    while(fd==-1&&attempt_counter<5){
        fd = open(client_fifo_dir,O_WRONLY);
        attempt_counter++;
        usleep(10000);
    }
    if(fd==-1){
        mvprintw(10,10,"Server is either full or closed");
        refresh();
        getchar();
        return NULL;
    }

    while(1){

        int a = getch();

        if(write(fd,&a,sizeof(int))==-1){
            close(fd);
            return NULL;
        }

        if(a=='q'){
            player_data.connected=0;
            close(fd);
            return NULL;
        }

        usleep(1000*ROUND_TIME_MS);
    }
}

int connect_to_server(){
    int qd = open("../temp/queue",O_WRONLY);
    if(qd==-1){
        return 0;
    }

    int pid = getpid();
    int res=write(qd,&pid,sizeof(int));

    close(qd);
    return 1;
}

void run_client(){
    screen_setup();
    if(!connect_to_server()){
        mvprintw(10,10,"Connection to server failed");
        refresh();
        getchar();
        close_screen();
        return;
    }

    pthread_mutex_init(&game_mt,NULL);

    pthread_t pt_screen,pt_key_handler;

    pthread_create(&pt_key_handler,NULL,key_handler,NULL);
    pthread_create(&pt_screen,NULL,print_screen,NULL);

    pthread_join(pt_screen,NULL);
    pthread_join(pt_key_handler,NULL);

    pthread_mutex_destroy(&game_mt);

    close_screen();
}