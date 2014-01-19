//
//  main.c
//  LSP-EXAM
//
//  Created by Armands Baurovskis on 12/01/14.
//  Copyright (c) 2014 Armands Baurovskis. All rights reserved.
//


#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<pthread.h>

//Definētie lauki
struct User
{
    char nickname[100];
    int userId;
    int planetCount;
    void *userSocket;
    struct User *next;
    
} typedef User;

int userIDS = 1;
int REG_FLAG = 1;
int playerCount = 0;
int planetCount = 0;

int PLAYERS;
int START_TIME_DEFINE;
int SPEED;
int MIN_CAPACITY;
int MAX_CAPACITY;

int START_TIME;
pthread_t start_game_thread;

User *root = NULL;

//the thread function
void *connection_handler(void *);
//User message process
void processUserMessage(char usermessage[],void *socket_desc);
void sendDataAboutUsers(void *socket_desc);
void sendDataAboutMap(void *socket_desc);
void sendDataAboutAttacks(void *socket_desc);
void sendDataAboutSentAttacks(char usermessage[],void *socket_desc);
void registerUserForTheGame(char userData[],void *socket_desc);
void sendUsersGameInformation();
pthread_mutex_t lock;

int main(int argc , char *argv[])
{
    
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    
    fp = fopen("/Users/armandsbaurovskis/Documents/Developer/LSP-EXIS/LSP-EXAM/LSP-EXAM/config.txt", "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);
    
    while ((read = getline(&line, &len, fp)) != -1) {
        char * command = strtok(line, " ");
        char * value = strtok(NULL, " ");
        if (strcmp(command, "PLAYERS")==0) {
            PLAYERS = atoi(value);
        }else if (strcmp(command, "SPEED")==0) {
            SPEED = atoi(value);
        }else if (strcmp(command, "START_TIME")==0) {
            START_TIME_DEFINE = atoi(value);
        }else if (strcmp(command, "MIN_CAPACITY")==0) {
            MIN_CAPACITY = atoi(value);
        }else if (strcmp(command, "MAX_CAPACITY")==0) {
            MAX_CAPACITY = atoi(value);
        }
    }
    START_TIME = START_TIME_DEFINE;

    
    int socket_desc , client_sock , c , *new_sock;
    struct sockaddr_in server , client;
    
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
    
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );
    
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
    
    //Listen
    listen(socket_desc , 3);
    c = sizeof(struct sockaddr_in);
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    pthread_mutex_init(&lock, NULL);
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
        
        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = client_sock;
        
        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
        
        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( sniffer_thread , NULL);
        puts("Handler assigned");
    }
    
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
    
    return 0;
}

/*
 * This will handle connection for each client
 * */

void *countDownStartTime(void *arg)
{

    while(START_TIME!=0)
    {
        sleep(1);
        pthread_mutex_lock(&lock);
        START_TIME--;
        pthread_mutex_unlock(&lock);
    }
    
    REG_FLAG=0;
    return 0;
}

void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    long read_size;
    char client_message[1000];
    
    
    //Receive a message from client
    while( (read_size = recv(sock , client_message , 2000 , 0)) > 0 )
    {   pthread_mutex_lock(&lock);
        processUserMessage(client_message, socket_desc);
        pthread_mutex_unlock(&lock);
    }
    
    if(read_size == 0)
    {
        puts("Client disconnected");
        pthread_mutex_lock(&lock);
        if(REG_FLAG==1){
            if(playerCount>0)
                playerCount--;
            if(playerCount == 0){
                pthread_cancel(start_game_thread);
                START_TIME = START_TIME_DEFINE;
            }
        }
        
        if(REG_FLAG == 0 && playerCount == 1)
        {
            playerCount--;
            REG_FLAG=1;
            START_TIME = START_TIME_DEFINE;
        }
        pthread_mutex_unlock(&lock);
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }
    
    //Free the socket pointer
    free(socket_desc);
    
    return 0;
}

void processUserMessage(char usermessage[],void *socket_desc)
{
    char command = usermessage[0];
    switch (command) {
        case 'J':
            registerUserForTheGame(usermessage,socket_desc);
            break;
        case 'U':
            sendDataAboutUsers(socket_desc);
            break;
        case 'M':
            sendDataAboutMap(socket_desc);
            break;
        case 'A':
            sendDataAboutAttacks(socket_desc);
            break;
        case 'S':
            sendDataAboutSentAttacks(usermessage,socket_desc);
            break;
        default:
            break;
    }
}

void sendDataAboutUsers(void *socket_desc)
{
    int sock = *(int*)socket_desc;
    char message[1000];
    sprintf(message, "U %i",playerCount);
    User*p = root;
    while (p!=NULL) {
        char user[100];
        sprintf(user," %i_%s",p->userId,p->nickname);
        strcat(message,user);
        p=p->next;
    }
    send(sock , message , strlen(message) , 0);
}
void sendDataAboutMap(void *socket_desc)
{
    
    int sock = *(int*)socket_desc;
    char message[1000];
    planetCount = 3;
    sprintf(message, "M %i",planetCount);
    int i = 0;
    for(i = 0; i<planetCount; i++){
        char planet[100];
        int ships = rand() % 10 + 1;
        sprintf(planet," %i_%i_%i_%i_%i_%i",i,i*50,i*10,i,i*30+50,ships);
        strcat(message,planet);
    }
    send(sock , message , strlen(message) , 0);
}
void sendDataAboutAttacks(void *socket_desc)
{
    int sock = *(int*)socket_desc;
    int i =0;
    char message[1000];
    int count = rand() % 10 + 1;
    sprintf(message,"A %i",count);
    for (i = 0; i<count; i++) {
        int pid = rand() % 10 + 1;
        int amount = rand() % 100 + 1;
        int from = rand() % 10 + 1;
        int time = rand() % 100 + 1;
        char attack[100];
        sprintf(attack," %i_%i_%i_%i",pid,amount,from,time);
        strcat(message, attack);
    }
    send(sock , message , strlen(message) , 0);
}
void sendDataAboutSentAttacks(char usermessage[],void *socket_desc)
{
    int sock = *(int*)socket_desc;
    char msg[10];
    int randomSucces = rand() % 10;
    int randomTime =rand() % 10;
    if(randomSucces % 2 == 0){
        randomSucces = 0;
        randomTime = 0;
    }
    sprintf(msg,"S %i %i",randomSucces,randomTime);
    send(sock , msg , strlen(msg) , 0);
}
void registerUserForTheGame(char userData[],void *socket_desc)
{
    int sock = *(int*)socket_desc;
    char *userNickName;
    userNickName = strtok(userData, " ");
    userNickName = strtok(NULL, " ");
    User * user;
    user = (User *) malloc( sizeof(User) );
    strcpy(user->nickname, userNickName);
    user->userId = userIDS;
    user->planetCount = 1;
    user->next = NULL;
    user->userSocket = socket_desc;
    if(root == NULL)
        root=user;
    else{
        User*p = root;
        while (p->next!=NULL) {
            p = p->next;
        }
        p->next = user;
    }
    
    char message[1000];
    if(START_TIME <=0)
    {
        sprintf(message, "Game has already started");
        send(sock , message , strlen(message) , 0);
        
    }
    else if(playerCount > PLAYERS)
    {
        playerCount++;
        sprintf(message, "The room is full");
        send(sock , message , strlen(message) , 0);

    }else{
        playerCount++;
        sprintf(message, "J %i %i",userIDS,START_TIME);
        userIDS++;
        send(sock , message , strlen(message) , 0);
    }
    
    if(playerCount==1)
    {
        pthread_create(&start_game_thread, NULL, &countDownStartTime, NULL);
    }
    
}