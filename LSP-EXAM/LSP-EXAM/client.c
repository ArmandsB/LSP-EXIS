//
//  LSP - client.c
//  LSP - Eksāmens
//
//  Created by Armands Baurovskis on 12/01/14.
//  Copyright (c) 2014 Armands Baurovskis. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ncurses.h>
#include <stdlib.h>
#include <pthread.h>
#include <menu.h>

//User:self
typedef struct
{
    char nickname[100];
    int userId;
    
} UserInformation;

//Planets - window struct
struct Planets
{
    int planetID;
    WINDOW *planet;
    struct Planets*next;
    
} typedef Planets;

//Player border struct
struct UserBorders
{
    char border;
    int userId;
    struct UserBorders*next;
    
} typedef UserBorders;

UserInformation user_self;
int sock;
//Drawing global values
char * mesg;
int row,col;
int world_x,world_y,max_world_x,max_world_y;
int WORLD_HEIGHT =0;
int WORLD_WIDTH =0;
WINDOW * galcon_world;
WINDOW * users_win;
WINDOW * commands_win;
WINDOW * messages_win;
Planets * root_planets = NULL;
UserBorders * root_border = NULL;
MENU *commands;

//Process server data
void processServerCalls(char *server_reply);
void didRegisterUserForTheGame(char *server_reply);
void didReceiveInfoAboutOtherUsers(char *server_reply);
void didReceiveInfoAboutMap(char *server_reply);
void didReceiveInfoAboutAttacks(char *server_reply);
void didReceiveInfoAboutSendindAttacks(char *server_reply);
void didReceiveInfoAboutStatus(char *server_reply);

//Reload data methods
void *reloadMapInformation(void *arg);
void *reloadAttackInformation(void *arg);
void *reloadUserInformation(void *arg);
void *get_commands(void *arg);
void userDidPressAttackCommand();

void createGameWindow();
char findBorder(int user_id);
void gameDidEnd(int status);

pthread_mutex_t lock;

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
    
    sprintf(message, "J %s", user_self.nickname);
    send(sock , message , strlen(message) , 0);
    
    if( recv(sock , server_reply , 1000 , 0) > 0 )
    {
        processServerCalls(server_reply);
    }
    
}
#pragma API receive methods
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
void didReceiveInfoAboutMap(char *server_reply)
{
    char planets[1000];
    sprintf(planets,"%s",server_reply);
    
    strtok(planets, " ");
    char * planet_count = strtok(NULL, " ");
    
    //Create map for the first time
    if(root_planets == NULL)
    {
        getmaxyx(galcon_world, max_world_y, max_world_x);
        getbegyx(galcon_world, world_y, world_x);
        max_world_y--;
        max_world_x--;
        world_x++;
        world_y++;
        
        int i = 0;
        char planetArray[atoi(planet_count)][100];
        
        for (i = 0; i<atoi(planet_count); i++) {
            char * planet = strtok(NULL, " ");
            strcpy(planetArray[i], planet);
        }
        for (i = 0; i<atoi(planet_count); i++) {
            char * planet = planetArray[i];
            char *PID = strtok(planet, "_");
            char *x_coordinate = strtok(NULL, "_");
            char *y_coordinate = strtok(NULL, "_");
            char *UID = strtok(NULL, "_");
            char *capacity = strtok(NULL, "_");
            int planet_size;
            //Check for planet size
            if(atoi(capacity)<=50)
            {
                planet_size=4;
            }
            else if(atoi(capacity)<=100)
            {
                planet_size=5;
            }
            else if(atoi(capacity)>100)
            {
                planet_size=6;
            }else
            {
                planet_size=4;
            }
            char *ships = strtok(NULL, "_");
            Planets *planet_win;
            if (root_planets==NULL) {
                root_planets = malloc(sizeof(Planets));
                root_planets->planetID=atoi(PID);
                root_planets->planet = newwin(planet_size, planet_size*2, world_y+atoi(y_coordinate), world_x+atoi(x_coordinate));
                root_planets->next = NULL;
                planet_win = root_planets;
            }else{
                Planets *p = root_planets;
                while (p->next!=NULL) {
                    p=p->next;
                }
                Planets *u = malloc(sizeof(Planets));
                u->planetID=atoi(PID);
                u->planet = newwin(planet_size, planet_size*2, world_y+atoi(y_coordinate), world_x+atoi(x_coordinate));
                p->next = u;
                planet_win = u;
            }
            //Create planet border for specific player
            if (UID!=NULL) {
                char c = findBorder(atoi(UID));
                if(c != '|'){
                    wborder(planet_win->planet, '|', '|', c, c, c, c, c, c);
                }else{
                    box(planet_win->planet, 0, 0);
                }
            }
            else{
                box(planet_win->planet, 0, 0);
            }
            int row_planet,col_planet;
            getmaxyx(planet_win->planet, row_planet, col_planet);
            char message[10];
            sprintf(message, "%i/%i",atoi(PID),atoi(ships));
            mvwaddstr(planet_win->planet, row_planet/2-((planet_size-5)*(planet_size-5)), (col_planet-strlen(message))/2.0, message);
            wrefresh(planet_win->planet);
        }
    }else{
        int i = 0;
        char planetArray[atoi(planet_count)][100];
        
        for (i = 0; i<atoi(planet_count); i++) {
            char * planet = strtok(NULL, " ");;
            strcpy(planetArray[i], planet);
        }
        for (i = 0; i<atoi(planet_count); i++) {
            char * planet = planetArray[i];
            char *PID = strtok(planet, "_");
            strtok(NULL, "_");
            strtok(NULL, "_");
            char *UID = strtok(NULL, "_");
            char *capacity =strtok(NULL, "_");
            char *ships = strtok(NULL, "_");
            Planets *planet_win = root_planets;
            while (planet_win!=NULL) {
                if(planet_win->planetID == atoi(PID))
                    break;
                planet_win=planet_win->next;
            }
            wclear(planet_win->planet);
            if (UID!=NULL) {
                char c = findBorder(atoi(UID));
                if(c != '|'){
                    wborder(planet_win->planet, '|', '|', c, c, c, c, c, c);
                }else{
                    box(planet_win->planet, 0, 0);
                }
            }
            else{
                box(planet_win->planet, 0, 0);
            }
            int planet_size;
            if(atoi(capacity)<=50)
            {
                planet_size=4;
            }
            else if(atoi(capacity)<=100)
            {
                planet_size=5;
            }
            else if(atoi(capacity)>100)
            {
                planet_size=6;
            }else
            {
                planet_size=4;
            }
            int row_planet,col_planet;
            getmaxyx(planet_win->planet, row_planet, col_planet);
            char message[10];
            sprintf(message, "%i/%i",atoi(PID),atoi(ships));
            mvwaddstr(planet_win->planet, row_planet/2-((planet_size-5)*(planet_size-5)), (col_planet-strlen(message))/2.0, message);
            wrefresh(planet_win->planet);
        }
        
    }
}
void didReceiveInfoAboutAttacks(char *server_reply)
{
    wclear(messages_win);
    box(messages_win, 0 , 0);
    wattron(messages_win, A_BOLD);
    int row_messages_win,col_messages_win;
    int x,y;
    getmaxyx(messages_win,row_messages_win,col_messages_win);
    mvwaddstr(messages_win, 1, (40-strlen("Messages"))/2.0, "Messages");
    wattroff(messages_win, A_BOLD);
    y= 2;
    x= 0;
    char attacks[1000];
    sprintf(attacks,"%s",server_reply);
    
    strtok(attacks, " ");
    char * attack_count = strtok(NULL, " ");
    int i = 0;
    char attackArray[atoi(attack_count)][100];
    
    for (i = 0; i<atoi(attack_count); i++) {
        char * attack = strtok(NULL, " ");
        strcpy(attackArray[i], attack);
    }
    for (i = 0; i<atoi(attack_count); i++) {
        char * attack = attackArray[i];
        char *attackOnPlanet = strtok(attack, "_");
        char *shipAmount = strtok(NULL, "_");
        char *attackFromPlanet = strtok(NULL, "_");
        char *timeAttack = strtok(NULL, "_");
        char attackDesc[100];
        sprintf(attackDesc, "To: %i From: %i Amount: %i After: %i",atoi(attackOnPlanet),atoi(attackFromPlanet),atoi(shipAmount),atoi(timeAttack));;
        if(y!=col_messages_win-2){
            mvwaddstr(messages_win, y+1, x+1, attackDesc);
            y+=1;
        }
    }
    wrefresh(messages_win);
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
    char playersArray[atoi(user_count)][100];
    
    for (i = 0; i<atoi(user_count); i++) {
        char * user = strtok(NULL, " ");
        strcpy(playersArray[i], user);
    }
    if(root_border==NULL){
        for (i = 0; i<atoi(user_count); i++) {
            char * user = playersArray[i];
            char *userID = strtok(user, "_");
            char *userName = strtok(NULL, "_");
            char fullName[100];
            //Register borders for player MAX 5;
            if(i<5)
            {
                if(root_border==NULL)
                {
                    root_border = malloc(sizeof(UserBorders));
                    root_border->userId = user_self.userId;
                    root_border->border = '*';
                    root_border->next =NULL;
                    
                }else{
                    UserBorders *p = root_border;
                    while (p->next!=NULL) {
                        p=p->next;
                    }
                    UserBorders *u;
                    u = malloc(sizeof(UserBorders));
                    u->userId=atoi(userID);
                    switch (i) {
                        case 1:
                            u->border = '@';
                            break;
                        case 2:
                            u->border = '#';
                            break;
                        case 3:
                            u->border = '=';
                            break;
                        case 4:
                            u->border = '+';
                            break;
                            
                        default:
                            break;
                    }
                    u->next=NULL;
                    p->next = u;
                }
            }
            sprintf(fullName, "%s(%c)",userName,findBorder(atoi(userID)));
            if(y!=row_user_win-2){
                if(atoi(userID) == user_self.userId)
                    wattron(users_win, A_BOLD);
                mvwaddstr(users_win, y+1, x+1, fullName);
                wattroff(users_win, A_BOLD);
                y+=1;
            }
        }
    }else{
        for (i = 0; i<atoi(user_count); i++) {
            char * user = playersArray[i];
            char *userID = strtok(user, "_");
            char *userName = strtok(NULL, "_");
            char fullName[100];
            sprintf(fullName, "%s(%c)",userName,findBorder(atoi(userID)));
            if(y!=row_user_win-2){
                if(atoi(userID) == user_self.userId)
                    wattron(users_win, A_BOLD);
                mvwaddstr(users_win, y+1, x+1, fullName);
                wattroff(users_win, A_BOLD);
                y+=1;
            }
        }
    }
    wrefresh(users_win);
    
}
void didReceiveInfoAboutSendindAttacks(char *server_reply)
{
    char sent_amount[4];
    char arrival_time[4];
    strtok(server_reply, " ");
    sprintf(sent_amount, "%s",strtok(NULL, " "));
    sprintf(arrival_time, "%s",strtok(NULL, " "));
    
    wclear(commands_win);
    box(commands_win, 0 , 0);
    wattron(commands_win, A_BOLD);
    mvwaddstr(commands_win, 1, (40-strlen("Commands"))/2.0, "Commands");
    wattroff(commands_win, A_BOLD);
    set_menu_win(commands, commands_win);
    set_menu_sub(commands, derwin(commands_win, 6, 38, 3, 1));
    set_menu_mark(commands, " * ");
    post_menu(commands);
    int row_commands,col_commands;
    getmaxyx(commands_win,row_commands,col_commands);
    
    if (atoi(sent_amount)== 0 && atoi(arrival_time) == 0) {
        wattron(commands_win, A_BOLD);
        mvwaddstr(commands_win, row_commands/2, (40-strlen("Failure!"))/2.0, "Failure!");
        wattroff(commands_win, A_BOLD);
        wrefresh(commands_win);
        sleep(2);
        wclear(commands_win);
        box(commands_win, 0 , 0);
        wattron(commands_win, A_BOLD);
        mvwaddstr(commands_win, 1, (40-strlen("Commands"))/2.0, "Commands");
        wattroff(commands_win, A_BOLD);
        set_menu_win(commands, commands_win);
        set_menu_sub(commands, derwin(commands_win, 6, 38, 3, 1));
        set_menu_mark(commands, " * ");
        post_menu(commands);
        wrefresh(commands_win);
        
    }else{
        wattron(commands_win, A_BOLD);
        char message[50];
        sprintf(message,"Ships will arrive after %i seconds",atoi(arrival_time));
        mvwaddstr(commands_win, row_commands/2, (40-strlen("Success!"))/2.0, "Success!");
        mvwaddstr(commands_win, row_commands/2+2, (40-strlen(message))/2.0, message);
        wattroff(commands_win, A_BOLD);
        wrefresh(commands_win);
        sleep(2);
        
        wclear(commands_win);
        box(commands_win, 0 , 0);
        wattron(commands_win, A_BOLD);
        mvwaddstr(commands_win, 1, (40-strlen("Commands"))/2.0, "Commands");
        wattroff(commands_win, A_BOLD);
        set_menu_win(commands, commands_win);
        set_menu_sub(commands, derwin(commands_win, 6, 38, 3, 1));
        set_menu_mark(commands, " * ");
        post_menu(commands);
        wrefresh(commands_win);
    }
}
void didReceiveInfoAboutStatus(char *server_reply)
{
    char * status;
    strtok(server_reply, " ");
    status = strtok(NULL, " ");
    gameDidEnd(atoi(status));
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
    user_self.userId = atoi(userID);
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
    createGameWindow();
    
    send(sock , "U" , strlen("U") , 0);
    if( recv(sock , server_reply , 1000 , 0) > 0 )
    {
        processServerCalls(server_reply);
    }
    
    //Create thread for each API request
    pthread_mutex_init(&lock, NULL);
    pthread_t reload_users;
    pthread_create( &reload_users , NULL ,  reloadUserInformation , NULL);
    pthread_t reload_map;
    pthread_create( &reload_map , NULL ,  reloadMapInformation , NULL);
    pthread_t reload_attack;
    pthread_create( &reload_attack , NULL ,  reloadAttackInformation , NULL);
    pthread_t commands_thread;
    pthread_create( &commands_thread , NULL ,  get_commands , NULL);
    
    //Loop till die
    while (1) {}
    
}
#pragma API - reload data methods
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
        case 'A':
            didReceiveInfoAboutAttacks(server_reply);
            break;
        case 'M':
            didReceiveInfoAboutMap(server_reply);
            break;
        case 'S':
            didReceiveInfoAboutSendindAttacks(server_reply);
            break;
        case 'W':
            didReceiveInfoAboutStatus(server_reply);
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
void *reloadMapInformation(void *arg)
{
    
    while(1)
    {
        char server_reply[1000];
        pthread_mutex_lock(&lock);
        send(sock , "M" , strlen("M") , 0);
        if( recv(sock , server_reply , 1000 , 0) > 0 )
        {
            processServerCalls(server_reply);
        }
        pthread_mutex_unlock(&lock);
        sleep(2);
    }
    return 0;
}
void *reloadAttackInformation(void *arg)
{
    
    while(1)
    {
        pthread_mutex_lock(&lock);
        char server_reply[1000];
        send(sock , "A" , strlen("A") , 0);
        if( recv(sock , server_reply , 1000 , 0) > 0 )
        {
            processServerCalls(server_reply);
        }
        pthread_mutex_unlock(&lock);
        sleep(3);
    }
    return 0;
}
void *reloadUserInformation(void *arg)
{
    
    while(1)
    {
        pthread_mutex_lock(&lock);
        char server_reply[1000];
        send(sock , "U" , strlen("U") , 0);
        if( recv(sock , server_reply , 1000 , 0) > 0 )
        {
            processServerCalls(server_reply);
        }
        pthread_mutex_unlock(&lock);
        sleep(2);
    }
    return 0;
}
#pragma Commands menu
void *get_commands(void *arg)
{
    int c;
    keypad(commands_win, TRUE);
    while((c = getch()))
	{
        pthread_mutex_lock(&lock);
        switch(c)
        {	case KEY_DOWN:
				menu_driver(commands, REQ_DOWN_ITEM);
				break;
			case KEY_UP:
				menu_driver(commands, REQ_UP_ITEM);
				break;
			case 10: /* Enter */
			{
                keypad(stdscr, false);
                ITEM *cur;
                cur = current_item(commands);
				if(cur->index == 0)
                    userDidPressAttackCommand();
                else
                    gameDidEnd(0);
                keypad(stdscr, true);
				break;
			}
                break;
		}
        wrefresh(commands_win);
        pthread_mutex_unlock(&lock);
	}
    
    return 0;
}
void userDidPressAttackCommand()
{
    
    char from[4];
    char to[4];
    char amount[4];
    char server_reply[10];
    
    int row_commands,col_commands;
    getmaxyx(commands_win,row_commands,col_commands);
    //From
    wattron(commands_win, A_BOLD);
    mvwaddstr(commands_win, row_commands/2, 1, "From: ");
    wattroff(commands_win, A_BOLD);
    wgetstr(commands_win, from);
    wrefresh(commands_win);
    //To
    wattron(commands_win, A_BOLD);
    mvwaddstr(commands_win, row_commands/2+1, 1, "To: ");
    wattroff(commands_win, A_BOLD);
    wgetstr(commands_win, to);
    wrefresh(commands_win);
    //Amount
    wattron(commands_win, A_BOLD);
    mvwaddstr(commands_win, row_commands/2+2, 1, "Amount: ");
    wattroff(commands_win, A_BOLD);
    wgetstr(commands_win, amount);
    wrefresh(commands_win);
    char message[32];
    sprintf(message,"S %i %i %i",atoi(from),atoi(to),atoi(amount));
    
    send(sock , message , strlen(message) , 0);
    if( recv(sock , server_reply , 1000 , 0) > 0 )
    {
        processServerCalls(server_reply);
    }
    
    
}
#pragma - Additional functions
void gameDidEnd(int status)
{
    clear();
    attron(A_BLINK | A_BOLD);
    if(status==0)
        mvprintw(row/2,(col-strlen("YOU LOSE THE GAME. THE GAME IS OVER"))/2.0,"YOU LOSE THE GAME. THE GAME IS OVER");
    else
        mvprintw(row/2,(col-strlen("WINNER!!!!! YOU DID WIN THE GAME!!!!"))/2.0,"WINNER!!!!! YOU DID WIN THE GAME!!!!");
    
    attroff(A_BLINK | A_BOLD);
    refresh();
    sleep(5);
    clear();
    endwin();
    exit(EXIT_FAILURE);
}
char findBorder(int user_id)
{
    if(user_id == 0)
        return '|';
    UserBorders *p = root_border;
    while (p!=NULL) {
        if(p->userId == user_id){
            return p->border;
        }
        p=p->next;
    }
    return '|';
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
    wattron(users_win, A_BOLD);
    mvwaddstr(users_win, 1, (40-strlen("Players"))/2.0, "Players");
    wattroff(users_win, A_BOLD);
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
int main(int argc , char *argv[])
{
    mesg ="Enter a server IP address: ";
    char serverIP[100];
    initscr();
    cbreak();
    curs_set(0);
    keypad(stdscr, TRUE);
    
    getmaxyx(stdscr,row,col);
    WORLD_WIDTH = col-42;
    WORLD_HEIGHT = row;
    mvprintw(row/2,(col-strlen(mesg))/2.0,"%s",mesg);
    getstr(serverIP);
    clear();
    mesg = "Enter your nickname: ";
    mvprintw(row/2,(col-strlen(mesg))/2.0,"%s",mesg);
    getstr(user_self.nickname);
    while (didUsernameHasOnlyAlhabet(user_self.nickname)==1) {
        clear();
        mesg = "Wrong nickname! Enter your nickname: ";
        mvprintw(row/2,(col-strlen(mesg))/2.0,"%s",mesg);
        getstr(user_self.nickname);
    }
    if(strlen(serverIP)==0)
        sprintf(serverIP, "127.0.0.1");
    createConnection(serverIP, user_self.nickname);
    endwin();
    return 0;
}