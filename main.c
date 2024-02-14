#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <pthread.h>
#include "list.h"
#include "helper.h"

#define MAX_BUF_SIZE 1024

int sockfd; // initialize socket
List* sendList; // initializing lists
List* displayList;
static pthread_mutex_t sendMutex, displayMutex; // initializing mutexs, conds, and threads
static pthread_cond_t sendCond, displayCond;
pthread_t keyboardInputThread, sendUDPThread, receiveUDPThread, screenDisplayThread;
char buffer[MAX_BUF_SIZE]; // max chars allowed is 1024
struct sockaddr_in your_addr; // your_addr referes to the person ur trying to message

int main(int argc, char *argv[]) {
    if (argc != 4) { // making sure the command is correct
        fprintf(stderr, "Usage: %s <my port number> <target machine name> <target port number>\n", argv[0]);
        exit(1);
    }
    printf("Starting s-talk...\nEnter a message to send:\n");

    int my_port = atoi(argv[1]);
    const char *target_machine = argv[2];
    int target_port = atoi(argv[3]);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0); // creating the socket
    if (sockfd == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in my_addr; // my address info;
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(my_port);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    // binding the socket to my address
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1) { 
        perror("bind");
        close(sockfd);
        exit(1);
    }

    struct addrinfo hints, *res; // getting the target address address info
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(target_machine, NULL, &hints, &res) != 0) {
        fprintf(stderr, "Error resolving target address\n");
        close(sockfd);
        exit(1);
    }
    
    your_addr = *((struct sockaddr_in *)(res->ai_addr));
    your_addr.sin_port = htons(target_port);

    // initialize mutexs
    pthread_mutex_init(&sendMutex, NULL);
    pthread_mutex_init(&displayMutex, NULL);
    pthread_cond_init(&sendCond, NULL);
    pthread_cond_init(&displayCond, NULL);

    // creating the lists
    sendList = List_create();
    displayList = List_create();

    


    // creating the pthreads

    // uses sendlist, send mutex, send cond
    if (pthread_create(&keyboardInputThread, NULL, keyboardInput, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
    // takes info from displayList
    if (pthread_create(&screenDisplayThread, NULL, screenDisplay, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
    // gets info from sendlist
    if (pthread_create(&sendUDPThread, NULL, sendUDP, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
    // puts message in displayList
    if (pthread_create(&receiveUDPThread, NULL, receiveUDP, NULL) != 0) {
        perror("pthread_create");
        exit(1);
    }
 
    //  join the pthreads
    pthread_join(keyboardInputThread, NULL);
    pthread_join(screenDisplayThread, NULL);
    pthread_join(sendUDPThread, NULL);
    pthread_join(receiveUDPThread, NULL);

    // reaches this point when conneciton terminates
    close(sockfd);
    freeaddrinfo(res);

    return 0;
}

// step 1: keyboardInput gets input from the keyboard, adds it to sendList.
//         once it does this, it signals to sendUDP that there's list stuff avail
// step 2: sendUDP takes the stuff out of sendList, sends it over to the target socket displayList
// step 3: receiveUDP receives the stuff, and sticks it into displayList
// step 4: screenDisplay takes each message off this list and prints it onto the screen.

// gets message from terminal, and puts it into sendList
void* keyboardInput(void* args) {
    while (1) {
        fgets(buffer, MAX_BUF_SIZE, stdin); // get the message from the user
        buffer[strcspn(buffer, "\n")] = '\0'; 
        if (strlen(buffer) > 0) { // if the message received is more than 0 chars
            pthread_mutex_lock(&sendMutex); // stop other threads from accessing list
            List_append(sendList, strdup(buffer)); // make a copy to store in the list
            pthread_mutex_unlock(&sendMutex); // let someone else access
            pthread_cond_signal(&sendCond); // Notify the send cond thing
        }
    }
    return NULL;
}

// function to get message from the sendList, and send it to remote user
void* sendUDP(void* args) {
    while (1) {
        pthread_mutex_lock(&sendMutex); // lock mutex instantly
        while (sendList == NULL || List_count(sendList) <= 0) { // Wait until message avail from input
            pthread_cond_wait(&sendCond, &sendMutex); // gets signal from keyboardinput and miuut
            
        }
        // Once a message is available, get it
        char* buffer = (char *)List_trim(sendList);
        if(strcmp(buffer , "!") == 0){ // check if someone terminated s-talk
            // if yes, send the message anyways, so that we can terminate on the other end too.
            ssize_t bytes_sent = sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&your_addr, sizeof(your_addr));
            if (bytes_sent == -1) {
                perror("sendto");
            }
            // alert that the connection is terminated
            printf("Connection is terminated\n");
            terminate_everything(); // this function kills/closes/ends everything
        }
        
        //otherwise, the message isn't '!' and we can send normally
        // Send the message as a UDP datagram to the remote client
        if (buffer != NULL) {
            ssize_t bytes_sent = sendto(sockfd,buffer, strlen(buffer), 0, (struct sockaddr *)&your_addr, sizeof(your_addr));
            if (bytes_sent == -1) {
                perror("sendto");
            } else {
                printf("Message sent: %s\n", buffer);
            }
            free(buffer);
        }
        pthread_mutex_unlock(&sendMutex); // allow others to access the sendList
    }
    return NULL;
}

//function to get incoming messages and add them into displayList
void* receiveUDP(void* args) {
    while (1) {
        char buffer[MAX_BUF_SIZE];
        struct sockaddr_in remote_addr;
        socklen_t remote_addr_len = sizeof(remote_addr);
        // Receive a UDP datagram from the remote client
        ssize_t bytes_received = recvfrom(sockfd, buffer, MAX_BUF_SIZE, 0, (struct sockaddr *)&remote_addr, &remote_addr_len);
        if (bytes_received < 0) {
            perror("recvfrom");
            continue; // Handle the error and continue receiving
        }

        buffer[bytes_received] = '\0';

        // Add the received message to the display message list
        pthread_mutex_lock(&displayMutex);
        if (displayList == NULL) {
            displayList = List_create();
        }

        List_prepend(displayList, strdup(buffer)); // Make a copy to store in the list
        pthread_mutex_unlock(&displayMutex);
        pthread_cond_signal(&displayCond); // Notify the printing thread
    }
    return NULL;
}

//function to display received messages in terminal
void* screenDisplay(void* args) {
    while (1) {

        pthread_mutex_lock(&displayMutex);
        while (List_count(displayList) <= 0) { // keep waiting until message available
            pthread_cond_wait(&displayCond, &displayMutex);
        }

        // once a message is available, get it from displayList
        int count = List_count(displayList);
        while (count != 0) {
            char *buffer = List_trim(displayList);
            //condition if ! is put into terminal
            if(strcmp(buffer, "!") == 0){
                printf("Connection is terminated\n");
                terminate_everything();
                return NULL;
            }
            //display received message
            if (buffer != NULL) {
                printf("Message received: %s\n", buffer);
            }
            //cycle through list and remove
            count--;
            free(buffer);
        }

        pthread_mutex_unlock(&displayMutex);
    }
    return NULL;
}

//function to free items in list
void free_fn(void* item){
    free(item);

}

//create function to terminate all threads, destroy mutexes and free memory
void* terminate_everything(){
    pthread_cancel(keyboardInputThread);
    pthread_cancel(sendUDPThread);
    pthread_cancel(receiveUDPThread);
    pthread_cancel(screenDisplayThread);
    pthread_cond_destroy(&sendCond);
    pthread_cond_destroy(&displayCond);
    pthread_mutex_destroy(&sendMutex);
    pthread_mutex_destroy(&displayMutex);
    List_free(sendList, free_fn);
    List_free(displayList, free_fn);
    return NULL;
}
