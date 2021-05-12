#include <iostream>
#include <unistd.h>
#include <string.h>
#include <chrono>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>9
#include <netdb.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>


using namespace std;

const int length = 4096;
int sock;

void *thread_command(void *ptr);
void *thread_result(void *ptr);
//void sigChildHandler(int signo);


int main(int argc, char *argv[]) {

    if(argc != 3) {
        char errMsg[] = "Incorrect number of arguments, provide localhost and port number ONLY\n";
        write(STDOUT_FILENO, errMsg, strlen(errMsg));
        exit(1);
    }

//    signal(SIGCHLD, sigChildHandler);

    struct sockaddr_in server;
    struct hostent *hp;

/* Create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("opening stream socket");
        exit(1);
    }
    /* Connect socket using name specified by command line. */
    server.sin_family = AF_INET;
    hp = gethostbyname(argv[1]);
    if (hp == 0) {
        fprintf(stderr, "%s: unknown host\n", argv[1]);
        exit(2);
    }
    bcopy(hp->h_addr, &server.sin_addr, hp->h_length);
    server.sin_port = htons(atoi(argv[2]));

    if (connect(sock,(struct sockaddr *) &server,sizeof(server)) < 0) {
        perror("connecting stream socket");
        exit(1);
    }

        pthread_t threadCommand, threadResult;

        int  iret1, iret2;

        /* Create independent threads each of which will execute function */
        iret1 = pthread_create( &threadCommand, NULL, thread_command, (void*) sock);
        if(iret1) {
            fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
            exit(EXIT_FAILURE);
        }

        iret2 = pthread_create( &threadResult, NULL, thread_result, (void*) sock);
        if(iret2) {
            fprintf(stderr,"Error - pthread_create() return code: %d\n",iret2);
            exit(EXIT_FAILURE);
        }

        pthread_join(threadCommand, NULL);
        pthread_join(threadResult, NULL);

        close(sock);


    return 0;

}


void *thread_command(void *ptr) {
//    int sock_local = (intptr_t) ptr;

    char prompt[] = "Enter command:\n\n";
        if(write(STDOUT_FILENO, prompt, strlen(prompt)) < 0)
            perror("writing on stdout ");


    int readB;
    char commandC[length];

    while(1) {

        if((readB = read(STDIN_FILENO, commandC, length)) < 0)
            perror("reading on stdout ");
        int wSockLen;
        if ((wSockLen = write(sock, commandC, readB)) < 0)
            perror("writing on socket ");
    }

}

void *thread_result(void *ptr) {
//    int sock_local = (intptr_t) ptr;

    int ansB;
    char ans[length];

    while(1) {
        ansB = read(sock, ans, length);
        if(ansB < 0)
            perror("reading on socket ");
        else if(ansB > 0){

        //    To prevent reading answer of previous command while doing strcmp
            ans[ansB] = '\0';

        //    Exit
            if(strcmp(ans, "exit") == 0) {
                exit(0);
            }
            else {
                ans[ansB] = '\n';
                ans[ansB+1] = '\0';
                if(write(STDOUT_FILENO, ans, strlen(ans)) < 0)
                    perror("Error writing on stdout ");
            }
        }
    }
}
