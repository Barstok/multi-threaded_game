#include "server.h"
#include "const.h"
#include <stdlib.h>
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>

char map[MAX_MAP_SIZE][MAX_MAP_SIZE];
struct server_data_t server_data;
int cols, rows;

pthread_mutex_t beast_mt,game_data_mt;


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

void dropped_treasure_insert(int x, int y, int value){
    struct dropped_treasure_t* temp=server_data.dropped_treasure.head;

    if(!temp){
       server_data.dropped_treasure.head=malloc(sizeof(struct dropped_treasure_t));
       server_data.dropped_treasure.head->point.x=x;
       server_data.dropped_treasure.head->point.y=y;
       server_data.dropped_treasure.head->value=value;
       server_data.dropped_treasure.head->next=NULL;
       return;
    }

    while(temp->next){
        temp=temp->next;
    }

    temp->next=malloc(sizeof(struct dropped_treasure_t));
    temp->next->point.x=x;
    temp->next->point.y=y;
    temp->next->value=value;
    temp->next->next=NULL;
}
void dropped_treasure_list_clear(){
    struct dropped_treasure_t* temp=server_data.dropped_treasure.head;

    while(temp){
        temp=temp->next;
        free(server_data.dropped_treasure.head);
        server_data.dropped_treasure.head=NULL;
        server_data.dropped_treasure.head=temp;
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
            case 'b':
            case 'B':
                beast_init();
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
    
    server_data.dropped_treasure.head=NULL;

    server_data.player_data[0].connected=0;
    server_data.player_data[1].connected=0;
    server_data.player_data[2].connected=0;
    server_data.player_data[3].connected=0;

    server_data.player_data[2].t=1;

    server_data.campsite.x=11;
    server_data.campsite.y=23;
    map[11][23] = 'A';

    server_data.beasts_count=0;
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

    pthread_mutex_lock(&beast_mt);
    for(int x=0;x<server_data.beasts_count;x++){
        attron(COLOR_PAIR(BEAST));
        mvprintw(server_data.beasts[x].pos.x,server_data.beasts[x].pos.y,"*");
    }
    pthread_mutex_unlock(&beast_mt);

    struct dropped_treasure_t* temp = server_data.dropped_treasure.head;
    attron(COLOR_PAIR(COIN));
    while(temp){
        mvprintw(temp->point.x,temp->point.y,"D");
        temp=temp->next;
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
            mvprintw(row,col+11*x,"%d",server_data.player_data[x].PID);
            if(server_data.player_data[x].t==HUMAN){
                mvprintw(row+1,col+11*x,"HUMAN");
            }else{
                mvprintw(row+1,col+11*x,"CPU");
            }
            mvprintw(row+2,col+11*x,"(%d,%d)     ",server_data.player_data[x].position.x,server_data.player_data[x].position.y);
            mvprintw(row+3,col+11*x,"%d    ",server_data.player_data[x].deaths);

            mvprintw(row+6,col+11*x,"%d    ",server_data.player_data[x].coins_carried);
            mvprintw(row+7,col+11*x,"%d    ",server_data.player_data[x].coins_brought);
        }
        else{
            mvprintw(row,col+11*x,"    ");
            mvprintw(row+1,col+11*x,"       ");
            mvprintw(row+2,col+11*x,"       ");
            mvprintw(row+3,col+11*x,"     ");
            mvprintw(row+6,col+11*x,"     ");
            mvprintw(row+7,col+11*x,"     ");
        }
    }
}

enum map_elements collision_check(struct point_t* point){ 
     switch(map[point->x][point->y]){
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
        case 'A':
            return CAMPSITE;
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
    server_data.player_data[player_num].moved=1;

    while(1){
        int x=rand()%rows;
        int y=rand()%cols;
        
        if(map[x][y]==' '){
            server_data.player_data[player_num].position.x=x;
            server_data.player_data[player_num].position.y=y;
            server_data.player_data[player_num].spawn_point.x=x;
            server_data.player_data[player_num].spawn_point.y=y;
            break;
        }
    }
    return player_num;
}

void player_quit(int player_num){
    server_data.player_data[player_num].connected=0;
    server_data.players_connected--;
}
void* player_connection_reader(void* arg){
    int player_num = *(int*)arg;

    struct player_data_t* player = &server_data.player_data[player_num];

    char fifo_dir[22];
    sprintf(fifo_dir,"../temp/%d_c_fifo",server_data.player_data[player_num].PID);

    mkfifo(fifo_dir,0777);

    int fd = open(fifo_dir,O_RDONLY);

    while(1){
        if(server_data.server_state==CLOSED){
            close(fd);
            remove(fifo_dir);
            return NULL;
        }
        int a;

        if(read(fd,&a,sizeof(int))==-1){
                player_quit(player_num);
                close(fd);
                remove(fifo_dir);
                return NULL;
        }

        switch(a){
            case 'q':
                player_quit(player_num);
                close(fd);
                remove(fifo_dir);
                return NULL;
            default:
                pthread_mutex_lock(&game_data_mt);
                if(server_data.player_data[player_num].moved){
                    pthread_mutex_unlock(&game_data_mt);
                    usleep(1000*ROUND_TIME_MS);
                    continue;
                }
                player_move(a,player_num);
                server_data.player_data[player_num].moved=1;
                pthread_mutex_unlock(&game_data_mt);
        }

        usleep(1000*ROUND_TIME_MS);
    }
}

void* player_connection_sender(void* arg){
    int player_num = *(int*)arg;

    struct player_data_t* player = &server_data.player_data[player_num];

    char fifo_dir[22];

    sprintf(fifo_dir,"../temp/%d_s_fifo",server_data.player_data[player_num].PID);

    mkfifo(fifo_dir,0777);

    int fd = open(fifo_dir,O_WRONLY);

    write(fd,&server_data.PID,sizeof(int));
    int send_p_num=player_num+1;
    write(fd,&send_p_num,sizeof(int));

    while(1){
        if(server_data.server_state==CLOSED||!server_data.player_data[player_num].connected){
            close(fd);
            remove(fifo_dir);
            return NULL;
        }

        struct player_data_chunk_t p_data;

        p_data.pos=player->position;
        p_data.deaths=player->deaths;
        p_data.coins_carried=player->coins_carried;
        p_data.coins_brought=player->coins_brought;

        p_data.round_number=server_data.round_number;

        p_data.beginning=p_data.pos;
        p_data.beginning.x-=2;
        p_data.beginning.y-=2;

        for(int x=0;x<SIGHT;x++){
            for(int y=0;y<SIGHT;y++){
                p_data.map_chunk[x][y]=map[p_data.beginning.x+x][p_data.beginning.y+y];
            }
        }
        //todo player and beast printing
        for(int x=0;x<MAX_PLAYERS;x++){
            if(server_data.player_data[x].connected&&server_data.player_data[x].position.x>=p_data.beginning.x&&server_data.player_data[x].position.x<p_data.beginning.x+SIGHT&&
            server_data.player_data[x].position.y>=p_data.beginning.y&&server_data.player_data[x].position.y<p_data.beginning.y+SIGHT){
                p_data.map_chunk[server_data.player_data[x].position.x-p_data.beginning.x][server_data.player_data[x].position.y-p_data.beginning.y]=x+'0'+1;
            }
        }

        for(int x=0;x<MAX_BEASTS;x++){
            if(server_data.beasts[x].pos.x>=p_data.beginning.x&&server_data.beasts[x].pos.x<p_data.beginning.x+SIGHT&&
            server_data.beasts[x].pos.y>=p_data.beginning.y&&server_data.beasts[x].pos.y<p_data.beginning.y+SIGHT){
                p_data.map_chunk[server_data.beasts[x].pos.x-p_data.beginning.x][server_data.beasts[x].pos.y-p_data.beginning.y]='*';
            }
        }

        write(fd,&p_data,sizeof(struct player_data_chunk_t));

        usleep(1000*ROUND_TIME_MS);
    }
}

void beast_collision_handle(int beast_num){
    for(int x=0;x<MAX_PLAYERS;x++){
        struct player_data_t* player_comp=&server_data.player_data[x];
        if(player_comp->connected){
                if(player_comp->position.x==server_data.beasts[beast_num].pos.x&&player_comp->position.y==server_data.beasts[beast_num].pos.y){
                    if(player_comp->coins_carried)   dropped_treasure_insert(player_comp->position.x,player_comp->position.y,player_comp->coins_carried);

                    player_comp->position.x=player_comp->spawn_point.x;
                    player_comp->position.y=player_comp->spawn_point.y;
                    player_comp->coins_carried=0;
                    player_comp->deaths++;
                    return;
                }
        }
    }
}

void player_collision_handle(int player_num){
    struct player_data_t* player=&server_data.player_data[player_num];
    for(int x=0;x<MAX_PLAYERS;x++){
        struct player_data_t* player_comp=&server_data.player_data[x];
        if(player->connected){
            for(int x=0;x<MAX_BEASTS;x++){
                if(player->position.x==server_data.beasts[x].pos.x&&player->position.y==server_data.beasts[x].pos.y){
                    if(player->coins_carried)   dropped_treasure_insert(player->position.x,player->position.y,player->coins_carried);

                    player->position.x=player->spawn_point.x;
                    player->position.y=player->spawn_point.y;
                    player->coins_carried=0;
                    player->deaths++;
                    return;
                }
            }
        }
        
        if(x!=player_num&&player->connected){
            if(player->position.x==player_comp->position.x&&
            player->position.y==player_comp->position.y){
                if(player->coins_carried+player_comp->coins_carried)
                    dropped_treasure_insert(player->position.x,player->position.y,player->coins_carried+player_comp->coins_carried);
                player->position.x=player->spawn_point.x;
                player->position.y=player->spawn_point.y;
                player->coins_carried=0;
                player->deaths++;

                player_comp->position.x=player_comp->spawn_point.x;
                player_comp->position.y=player_comp->spawn_point.y;
                player_comp->coins_carried=0;
                player_comp->deaths++;
                return;
            }
        }
    }
}

void player_move(int a,int player_num){
    struct player_data_t* player=&server_data.player_data[player_num];
    struct point_t desired_pos={.x=player->position.x,
                                .y=player->position.y};

    switch(a){
        case KEY_LEFT:
            desired_pos.y--;
            break;
        case KEY_DOWN:
            desired_pos.x++;
            break;
        case KEY_UP:
            desired_pos.x--;
            break;
        case KEY_RIGHT:
            desired_pos.y++;
            break;
    }

    switch(collision_check(&desired_pos)){
        case WALL:
            return;
        case BUSH:
            if(player->slowed_down){
                player->slowed_down=0;
                return;
            }
            else player->slowed_down=1;
            break;
        case DEFAULT:
            player->slowed_down=0;
            break;
        case COIN:
            player->slowed_down=0;
            switch(map[desired_pos.x][desired_pos.y]){
                case 'c':
                    player->coins_carried+=1;
                    break;
                case 't':
                    player->coins_carried+=10;
                    break;
                case 'T':
                    player->coins_carried+=50;
                    break;
            }
            map[desired_pos.x][desired_pos.y]=' ';
            break;
        case CAMPSITE:
            player->slowed_down=0;
            player->coins_brought+=player->coins_carried;
            player->coins_carried=0;
    }

    player->position.x=desired_pos.x;
    player->position.y=desired_pos.y;

    struct dropped_treasure_t* temp=server_data.dropped_treasure.head;
    struct dropped_treasure_t* prev=NULL;

    while(temp){
        if(temp->point.x==player->position.x&&temp->point.y==player->position.y){
            player->coins_carried+=temp->value;
            if(prev) prev->next=temp->next;
            else{
                server_data.dropped_treasure.head=server_data.dropped_treasure.head->next;
            }
            free(temp);
            return;
        }
        prev=temp;
        temp=temp->next;
    }

    player_collision_handle(player_num);
}

void* players_queue(void* arg){
    mkfifo("../temp/queue",0777);

    int qd = open("../temp/queue",O_RDONLY);

    while(1){
        if(server_data.server_state==CLOSED){
            for(int x=0;x<MAX_PLAYERS;x++){
                pthread_join(server_data.player_pt_r[x],NULL);
                pthread_join(server_data.player_pt_s[x],NULL);
            }
            close(qd);
            remove("../temp/queue");
            return NULL;
        }
        int pid;
        if(read(qd,&pid,sizeof(int))==sizeof(int)&&server_data.players_connected<MAX_PLAYERS){
            server_data.players_connected++;
            int player_num = player_init(pid);
            pthread_create(&server_data.player_pt_s[player_num],NULL,player_connection_sender,&player_num);
            pthread_create(&server_data.player_pt_r[player_num],NULL,player_connection_reader,&player_num);
        }
        usleep(1000*ROUND_TIME_MS);
    }

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
    }
}

void beast_init(){
    pthread_mutex_lock(&beast_mt);
    if(server_data.beasts_count>=MAX_BEASTS){
        pthread_mutex_unlock(&beast_mt);
        return;
    }
    struct beast_data_t* beast=&server_data.beasts[server_data.beasts_count];
    beast->moved=1;
    while(1){
        int x=rand()%rows;
        int y=rand()%cols;
        if(map[x][y]!='|'){
            beast->pos.x=x;
            beast->pos.y=y;
            break;
        }
    }
    pthread_create(&server_data.beasts_pt[server_data.beasts_count],NULL,run_beast,&server_data.beasts_count);
    server_data.beasts_count++;
    pthread_mutex_unlock(&beast_mt);
}

void* run_beast(void* arg){
    int index = *(int*)arg-1;

    struct beast_data_t* beast=&server_data.beasts[index];
    
    struct point_t desired_pos;

    while(1){
        if(server_data.server_state==CLOSED) return NULL;
        pthread_mutex_lock(&beast_mt);
        if(beast->moved) {
            pthread_mutex_unlock(&beast_mt);
            usleep(1000*ROUND_TIME_MS);
            continue;
        }
        pthread_mutex_unlock(&beast_mt);
        desired_pos=beast->pos;
        int dir = rand()%5;

        switch(dir){
            case 0:
                break;
            case 1:
                desired_pos.x++;
                if(collision_check(&desired_pos)!=WALL){
                    beast->pos=desired_pos;
                }
                break;
            case 2:
                desired_pos.x--;
                if(collision_check(&desired_pos)!=WALL){
                    beast->pos=desired_pos;
                }
                break;
            case 3:
                desired_pos.y++;
                if(collision_check(&desired_pos)!=WALL){
                    beast->pos=desired_pos;
                }
                break;
            case 4:
                desired_pos.y--;
                if(collision_check(&desired_pos)!=WALL){
                    beast->pos=desired_pos;
                }
                break;
        }
        beast_collision_handle(index);
        beast->moved=1;
        usleep(1000*ROUND_TIME_MS);
    }
}

void* run_clock(void* arg){
    while(1){
        if(server_data.server_state==CLOSED) return NULL;
        pthread_mutex_lock(&beast_mt);
        for(int x=0;x<server_data.beasts_count;x++){
            server_data.beasts[x].moved=0;
        }
        pthread_mutex_unlock(&beast_mt);
        for(int x=0;x<MAX_PLAYERS;x++){
            pthread_mutex_lock(&game_data_mt);
            if(server_data.player_data[x].connected){
                server_data.player_data[x].moved=0;
            }
            pthread_mutex_unlock(&game_data_mt);
        }
        server_data.round_number++;
        usleep(2500*ROUND_TIME_MS);
    }
}

void run_server(){
    srand(time(NULL));

    screen_setup();
    load_map("map.txt");
    server_data_init();

    pthread_mutex_init(&game_data_mt,NULL);
    pthread_mutex_init(&beast_mt,NULL);

    pthread_t pt_clock,pt_screen,pt_key_handler,pt_players_queue;

    pthread_create(&pt_clock,NULL,run_clock,NULL);
    pthread_create(&pt_players_queue,NULL,players_queue,NULL);
    pthread_create(&pt_screen,NULL,print_screen,NULL);
    pthread_create(&pt_key_handler,NULL,key_handler,NULL);

    pthread_join(pt_clock,NULL);
    pthread_join(pt_players_queue,NULL);
    pthread_join(pt_key_handler,NULL);
    pthread_join(pt_screen,NULL);

    for(int x=0;x<server_data.beasts_count;x++){
        pthread_join(server_data.beasts_pt[x],NULL);
    }

    pthread_mutex_destroy(&beast_mt);
    pthread_mutex_destroy(&game_data_mt);

    close_screen();
}