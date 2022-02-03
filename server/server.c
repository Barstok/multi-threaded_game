#include "server.h"
#include "const.h"
#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
    server_data.map_empty_space=0;
    while(!feof(f)&&row<MAX_MAP_SIZE){
        fscanf(f,"%s",(char*)map[row]);
        for(int x=0;x<cols;x++){
            if(map[row][x]=='.'){
                map[row][x]=' ';
                server_data.map_empty_space++;
            }
        }
        row++;
    }
    
    fclose(f);

    if(row!=rows) return -3;

    return 0;
}

void drop_coins(enum treasure_type t){
    if(server_data.map_empty_space&&server_data.treasure_count<MAX_TREASURE_COUNT){
        while(1){
            int row=rand()%rows;
            int col=rand()%cols;
            if(map[row][col]==' '){
                if(t==T_COIN){
                    map[row][col]='c';
                }
                else if(t==SMALL_TREASURE) map[row][col]='t';
                else if(t==LARGE_TREASURE) map[row][col]='T';
                server_data.map_empty_space--;
                server_data.treasure_count++;
                break;
            }
        }
    }
}

void* key_handler(void* arg){
    while(1){
        char a = getchar();

        switch(a){
            case 'c':
                drop_coins(T_COIN);
                break;
            case 't':
                drop_coins(SMALL_TREASURE);
                break;
            case 'T':
                drop_coins(LARGE_TREASURE);
                break;
            case 'q':
            case 'Q':
            server_data.server_state=CLOSED;
                return NULL;
        }
    }
}

void server_data_init(){
    server_data.PID=getpid();
    server_data.server_state=OPEN;
    server_data.players_connected=0;
    server_data.round_number=0;
    server_data.treasure_count=0;

    server_data.player_data[0].connected=0;
    server_data.player_data[1].connected=0;
    server_data.player_data[2].connected=0;
    server_data.player_data[3].connected=0;

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


    for(int x=0;x<MAX_PLAYERS;x++){
        attron(COLOR_PAIR(PLAYER));
        if(server_data.player_data[x].connected){
            struct player_data_t* player=&server_data.player_data[x];

            mvprintw(player->position.x,player->position.y,"%c",x+1+'0');
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
            mvprintw(row,col+11*x,"%d",server_data.player_data->PID);
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

enum map_elements collision_check(int player_id,enum dir_t dir){
    struct player_data_t* player=&server_data.player_data[player_id];

    struct point_t desired_pos;
    desired_pos.x=player->position.x;
    desired_pos.y=player->position.y;

    switch(dir){
        case UP:
            desired_pos.x--;
            break;
        case DOWN:
            desired_pos.x++;
            break;
        case LEFT:
            desired_pos.y--;
            break;
        case RIGHT:
            desired_pos.y++;
            break;
    }

    switch(map[desired_pos.x][desired_pos.y]){
        case '|':
            return WALL;
        case 'c':
        case 't':
        case 'T':
            return COIN;
        case '*':
            return BEAST;
        case '#':
            return BUSH;
        case ' ': 
            return DEFAULT;
    }
}

int player_init(int pid){
    int player_num;
    for(player_num=0;player_num<MAX_PLAYERS;player_num++){
        if(!server_data.player_data[player_num].connected) break;
    }

    server_data.player_data[player_num].connected=1;
    server_data.player_data[player_num].PID=pid;
    server_data.player_data[player_num].t=HUMAN;
    
    server_data.player_data[player_num].deaths=0;
    server_data.player_data[player_num].slowed_down=0;
    server_data.player_data[player_num].coins_brought=0;
    server_data.player_data[player_num].coins_carried=0;

    while(1){
        int x=rand()%rows;
        int y=rand()%cols;
        
        if(map[x][y]==' '){
            server_data.player_data[player_num].position.x=x;
            server_data.player_data[player_num].position.y=y;
            break;
        }
    }
    return player_num;
}
void* player_connection(void* arg){
    int player_num = *(int*)arg;

    struct player_data_t* player = &server_data.player_data[player_num];

    char fifo_dir[22];
    sprintf(fifo_dir,"../temp/%d_c_fifo",server_data.player_data[player_num].PID);

    int fd = open(fifo_dir,O_RDONLY);

    while(1){

        int a;

        read(fd,&a,sizeof(int));

        switch(a){
            case KEY_LEFT:
                if(collision_check(player_num,LEFT)!=WALL) player->position.y--;
                break;
            case KEY_DOWN:
                if(collision_check(player_num,DOWN)!=WALL) player->position.x++;
                break;
            case KEY_UP:
                if(collision_check(player_num,UP)!=WALL) player->position.x--;
                break;
            case KEY_RIGHT:
                if(collision_check(player_num,RIGHT)!=WALL) player->position.y++;
                break;
            default:
            player->PID=0;
                close(fd);
                break;
        }

        usleep(1000*ROUND_TIME_MS);
    }
}

void* players_queue(void* arg){
    mkfifo("../temp/queue",0777);

    int qd = open("../temp/queue",O_RDONLY);

    server_data.players_connected++;
    int pid;
    read(qd,&pid,sizeof(int));

    int player_num = player_init(pid);
    pthread_create(&server_data.player_pt[player_num],NULL,player_connection,&player_num);

    close(qd);
    remove("../temp/queue");

    return NULL;
}

void* print_screen(void* arg){
    print_legend();
    while(1){
        if(server_data.server_state==CLOSED) return NULL;
        print_map();
        print_server_data();
        refresh();
        usleep(ROUND_TIME_MS*1000);
        server_data.round_number++;
    }
}

void run_server(){
    srand(time(NULL));

    screen_setup();
    load_map("map.txt");
    server_data_init();

    pthread_t pt_screen,pt_key_handler,pt_players_queue;

    pthread_create(&pt_players_queue,NULL,players_queue,NULL);
    pthread_create(&pt_screen,NULL,print_screen,NULL);
    pthread_create(&pt_key_handler,NULL,key_handler,NULL);

    pthread_join(pt_players_queue,NULL);
    pthread_join(pt_key_handler,NULL);
    pthread_join(pt_screen,NULL);

    close_screen();
}