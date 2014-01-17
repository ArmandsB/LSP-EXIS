//
//  client.c
//  LSP-EXAM
//
//  Created by Armands Baurovskis on 12/01/14.
//  Copyright (c) 2014 Armands Baurovskis. All rights reserved.
//

#include<stdio.h>
#include<string.h>
#include<sys/socket.h>
#include<arpa/inet.h> 
#include <unistd.h>
#include <ncurses.h>
#include <stdlib.h>
typedef struct
{
    char nickname[100];
    int userId;
    
} UserInformation;
UserInformation user;
char * mesg;	/* message to be appeared on the screen */
int row,col;
int sock;

void createConnection(char server_ip[], char userNickName[])
{
    struct sockaddr_in server;
    char message[1000] , server_reply[1000];
    
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
    }
    
    server.sin_addr.s_addr = inet_addr(server_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons( 8888 );
    
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        clear();
        mesg = "Did not connect! Check Ip addres or try later again!";
        mvprintw(row/2,(col-strlen(mesg))/2.0,"%s",mesg);
        refresh();
        sleep(2);
        clear();
        endwin();
        exit(EXIT_FAILURE);
    }
    
    clear();
    mesg = "Connected!";
    mvprintw(row/2,(col-strlen(mesg))/2.0,"%s",mesg);
    refresh();
    
    sleep(2);
    clear();
    mesg = "Waiting replay from server...";
    mvprintw(row/2,(col-strlen(mesg))/2.0,"%s",mesg);
    refresh();
    //Send new request;
    sprintf(message, "J %s", user.nickname);
    send(sock , message , strlen(message) , 0);
    
    if( recv(sock , server_reply , 1000 , 0) > 0)
    {
        char *userID;
        char *start_time;
        int start_time_int;
        char *command;
        char msgToScreen[200];
        char *serv_message = malloc(sizeof(server_reply));
        sprintf(serv_message, "%s",server_reply);
        
        command = strtok(server_reply, " ");
        if(strcmp(command, "J") != 0){
            clear();
            mvprintw(row/2,(col-strlen(serv_message))/2.0,"%s",serv_message);
            refresh();
            sleep(2);
            clear();
            endwin();
            exit(EXIT_FAILURE);
        }
        userID = strtok(NULL, " ");
        start_time = strtok(NULL, " ");
        start_time_int = atoi(start_time);
        user.userId = atoi(userID);
        while (start_time_int!=0) {
            clear();
            sprintf(msgToScreen, "The game will start in %i seconds",start_time_int);
            mvprintw(row/2,(col-strlen(mesg))/2.0,"%s",msgToScreen);
            refresh();
            sleep(1);
            start_time_int--;
        }
        
        clear();
        mesg = "Downloading map....";
        mvprintw(row/2,(col-strlen(mesg))/2.0,"%s",mesg);
        
        refresh();
        while(1){ refresh(); };
    }
}
int didUsernameHasOnlyAlhabet(char *nick)
{
    int flag = 0;
    int i = 0;
    for (i = 0 ; nick[i]!=0; i++) {
        if(nick[i]<'a' || nick[i]>'z')
            flag = 1;
    }
    return flag;
    
}
int main(int argc , char *argv[])
{
    mesg ="Enter a server IP address: ";		/* message to be appeared on the screen */
    char serverIP[100];
    initscr();
    cbreak();
    keypad(stdscr, TRUE);/* start the curses mode */
    getmaxyx(stdscr,row,col);		/* get the number of rows and columns */
    mvprintw(row/2,(col-strlen(mesg))/2.0,"%s",mesg);
    /* print the message at the center of the screen */
    getstr(serverIP);
    clear();
    mesg = "Enter your nickname: ";
    mvprintw(row/2,(col-strlen(mesg))/2.0,"%s",mesg);
    getstr(user.nickname);
    while (didUsernameHasOnlyAlhabet(user.nickname)==1) {
        clear();
        mesg = "Wrong nickname! Enter your nickname: ";
        mvprintw(row/2,(col-strlen(mesg))/2.0,"%s",mesg);
        getstr(user.nickname);
    }
    //sprintf(serverIP, "127.0.0.1");
    //sprintf(user.nickname,"ArmandsB");
    createConnection(serverIP, user.nickname);
    
    return 0;
}