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
#include <pthread.h>
#include <menu.h>

#define WORLD_WIDTH 90
#define WORLD_HEIGHT 43

typedef struct
{
    char nickname[100];
    int userId;
    
} UserInformation;
UserInformation user;
char * mesg;	/* message to be appeared on the screen */
int row,col;
int sock;

WINDOW * galcon_world;
WINDOW * users_win;
WINDOW * commands_win;
WINDOW * messages_win;
MENU *commands;
void processServerCalls(char *server_reply);
void didRegisterUserForTheGame(char *server_reply);
void didReceiveInfoAboutOtherUsers(char *server_reply);
void *communicationWithServer(void *arg);
void createGameWindow();
void reloadData(int sig);

void createConnection(char server_ip[], char userNickName[])
{
    struct sockaddr_in server;
    char message[1000];
    char server_reply[1000];
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
    
    while( recv(sock , server_reply , 1000 , 0) > 0 )
    {
        processServerCalls(server_reply);
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
void didReceiveInfoAboutOtherUsers(char *server_reply)
{
    wclear(users_win);
    box(users_win, 0 , 0);
    int row_user_win,col_user_win;
    int x,y;
    getmaxyx(users_win,row_user_win,col_user_win);
    getbegyx(users_win, y, x);
    wattron(users_win, A_BOLD);
    mvwaddstr(users_win, y+1, (col_user_win-strlen("Players"))/2.0, "Players");
    wattroff(users_win, A_BOLD);
    y+=1;
    char players2[1000];
    sprintf(players2,"%s",server_reply);
    
    strtok(players2, " ");
    char * user_count = strtok(NULL, " ");
    int i = 0;
    char * playersArray[atoi(user_count)];
    
    for (i = 0; i<atoi(user_count); i++) {
        char * user = strtok(NULL, " ");
        playersArray[i] = malloc(sizeof(user));
        strcpy(playersArray[i], user);
    }
    for (i = 0; i<atoi(user_count); i++) {
        char * user = playersArray[i];
        char *userID = strtok(user, "_");
        char *userName = strtok(NULL, "_");
        char *fullName = malloc(sizeof(userID)+4+sizeof(userName)+4);
        sprintf(fullName, "%s(%i)",userName,atoi(userID));
        if(y!=row_user_win-2){
            mvwaddstr(users_win, y+1, x+1, fullName);
            y+=1;
        }
    }
    wrefresh(users_win);
    
}
void didRegisterUserForTheGame(char *server_reply)
{
    char *userID;
    char *start_time;
    int start_time_int;

    char msgToScreen[200];
    strtok(server_reply, " ");
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
    refresh();
    
    signal(SIGALRM, reloadData);
    createGameWindow();
    alarm(1);

}
void processServerCalls(char *server_reply)
{
    char command = server_reply[0];
    switch (command) {
        case 'J':
            didRegisterUserForTheGame(server_reply);
            break;
        case 'U':
            didReceiveInfoAboutOtherUsers(server_reply);
            break;
        default:
            clear();
            mvprintw(row/2,(col-strlen(server_reply))/2.0,"%s",server_reply);
            refresh();
            sleep(2);
            clear();
            endwin();
            exit(EXIT_FAILURE);
            break;
    }
}
void createGameWindow()
{
    int offsetx_world, offsety_world;
    offsetx_world = 42;
    offsety_world = 0;
    
    galcon_world = newwin(WORLD_HEIGHT,
                          WORLD_WIDTH,
                          offsety_world,
                          offsetx_world);
    
    box(galcon_world, 0 , 0);
    wrefresh(galcon_world);
    
    int offsetx_additions, offsety_additions;
    offsetx_additions = 0;
    offsety_additions = 0;
    users_win = newwin(WORLD_HEIGHT/3,
                       40,
                       offsety_additions,
                       offsetx_additions);
    
    box(users_win, 0 , 0);
    scrollok(users_win, FALSE);
    clearok(users_win, TRUE);
    wrefresh(users_win);
    //Commands
    
    offsety_additions+=WORLD_HEIGHT/3;
    commands_win = newwin(WORLD_HEIGHT/3,
                          40,
                          offsety_additions,
                          offsetx_additions);
    
    box(commands_win, 0 , 0);
    wattron(commands_win, A_BOLD);
    mvwaddstr(commands_win, 1, (40-strlen("Commands"))/2.0, "Commands");
    wattroff(commands_win, A_BOLD);
    wrefresh(commands_win);

    
    ITEM **my_items;
    my_items = (ITEM **)calloc(3, sizeof(ITEM *));
    my_items[0] = new_item("Attack","");
    my_items[1] = new_item("Quit","");
    
    commands = new_menu((ITEM **)my_items);
    keypad(commands_win, TRUE);
    
    
    set_menu_win(commands, commands_win);
    set_menu_sub(commands, derwin(commands_win, 6, 38, 3, 1));
    set_menu_mark(commands, " * ");
    post_menu(commands);
    wrefresh(commands_win);
    
    offsety_additions+=WORLD_HEIGHT/3;
    messages_win = newwin(WORLD_HEIGHT/3+1,
                          40,
                          offsety_additions,
                          offsetx_additions);
    
    box(messages_win, 0 , 0);
    wattron(messages_win, A_BOLD);
    mvwaddstr(messages_win, 1, (40-strlen("Messages"))/2.0, "Messages");
    wattroff(messages_win, A_BOLD);
    wrefresh(messages_win);
}
void reloadData(int sig) {
    
    send(sock , "U" , strlen("U") , 0);
    send(sock , "M" , strlen("M") , 0);
    send(sock , "A" , strlen("A") , 0);
    alarm(5);
}
int main(int argc , char *argv[])
{
    mesg ="Enter a server IP address: ";
    char serverIP[100];
    //initscr();
    cbreak();
    curs_set(0);
    init_pair(1, COLOR_RED, COLOR_BLACK);
    
    getmaxyx(stdscr,row,col);
    mvprintw(row/2,(col-strlen(mesg))/2.0,"%s",mesg);
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
    
    sprintf(serverIP, "127.0.0.1");
    sprintf(user.nickname,"ArmandsB");
    createConnection(serverIP, user.nickname);
     
    return 0;
}