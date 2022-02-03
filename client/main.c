#include "client.h"
#include <stdio.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

#define ROUND_TIME_MS 50

int main(){
    printf("STARTED\n");



    int qd = open("../temp/queue",O_WRONLY);
    if(qd==-1){
        printf("Server is off\n");
        return -1;
    }


    int pid = getpid();
    int res=write(qd,&pid,sizeof(int));

    char client_fifo_dir[22];

    sprintf(client_fifo_dir,"../temp/%d_c_fifo",pid);

    mkfifo(client_fifo_dir,0777);

    int fd = open(client_fifo_dir,O_WRONLY);

    initscr();
    noecho();
    cbreak();
    curs_set(FALSE);
    keypad(stdscr,TRUE);

    while(1){

        int a = getch();

        write(fd,&a,sizeof(int));

        usleep(1000*ROUND_TIME_MS);
    }

    close(qd);
    endwin();
    
    return 0;
}