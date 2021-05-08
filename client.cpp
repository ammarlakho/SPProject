#include <iostream>
#include <unistd.h>
#include <string.h>
#include <chrono>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>9
#include <netdb.h>
#include <stdio.h>


using namespace std;

int length = 4096;
int readB;
int ansB;
char commandC[4096];
char ans[4096];

int sock;
struct sockaddr_in server;
struct hostent *hp;



void *thread_command(void *ptr);
void *thread_result(void *ptr);


int main(int argc, char *argv[]) {

//    int sock;
//    struct sockaddr_in server;
//    struct hostent *hp;

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

//    int length = 4096;
//    int readB;
//    int ansB;
//    char commandC[length];
//    char ans[length];

    while(1) {
        pthread_t threadCommand, threadResult;
        const char *message1 = "Thread Command";
        const char *message2 = "Thread Result";
        int  iret1, iret2;

        /* Create independent threads each of which will execute function */

        iret1 = pthread_create( &threadCommand, NULL, thread_command, (void*) message1);
        if(iret1)
        {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
        exit(EXIT_FAILURE);
        }

        iret2 = pthread_create( &threadResult, NULL, thread_result, (void*) message2);
        if(iret2)
        {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret2);
        exit(EXIT_FAILURE);
        }

//        char tempC[100];
//        char tempR[100];
//        int c = sprintf(tempC, "pthread_create() for thread C returns: %d\n", iret1);
//        write(STDOUT_FILENO, tempC, c);
//        c = sprintf(tempR, "pthread_create() for thread R returns: %d\n",iret2);
//        write(STDOUT_FILENO, tempR, c);
        /* Wait till threads are complete before main continues. Unless we  */
        /* wait we run the risk of executing an exit which will terminate   */
        /* the process and all threads before the threads have completed.   */

        pthread_join( threadCommand, NULL);
        pthread_join( threadResult, NULL);
    }

        close(sock);

    return 0;

}


void *thread_command(void *ptr)
{
//    char *message;
//    message = (char *) ptr;
//    write(STDOUT_FILENO, message, strlen(message));
//    Prompt for command
    char prompt[] = "Enter command:\n";
    if(write(STDOUT_FILENO, prompt, strlen(prompt)) < 0)
        perror("writing on stdout ");

//    Send command to server
    if((readB = read(STDIN_FILENO, commandC, length)) < 0)
        perror("reading on stdout ");
    int wSockLen;
    if ((wSockLen = write(sock, commandC, readB)) < 0)
        perror("writing on socket ");
}

void *thread_result(void *ptr)
{
//    char *message;
//    message = (char *) ptr;
//    printf("%s \n", message);s

//    Get result from server
    if((ansB = read(sock, ans, length)) < 0)
        perror("reading on socket ");

//    To prevent reading answer of previous command while doing strcmp
    ans[ansB] = '\0';

//    Exit
    if(strcmp(ans, "exit") == 0) {
        exit(0);
    }
    else {
        if(write(STDOUT_FILENO, ans, ansB) < 0)
            perror("Error writing on stdout ");
        if(write(STDOUT_FILENO, "\n", 1) < 0)
        perror("Error writing on stdout ");
    }
}



////            Prompt for command
//            char prompt[] = "Enter command:\n";
//            if(write(STDOUT_FILENO, prompt, strlen(prompt)) < 0)
//                perror("writing on stdout ");
//
////            Send command to server
//            if((readB = read(STDIN_FILENO, commandC, length)) < 0)
//                perror("reading on stdout ");
//            int wSockLen;
//            if ((wSockLen = write(sock, commandC, readB)) < 0)
//                perror("writing on socket ");



////            Get result from server
//            if((ansB = read(sock, ans, length)) < 0)
//                perror("reading on socket ");
//
////            To prevent reading answer of previous command while doing strcmp
//            ans[ansB] = '\0';
//
////            Exit
//            if(strcmp(ans, "exit") == 0) {
//                exit(0);
//            }
//            else {
//                if(write(STDOUT_FILENO, ans, ansB) < 0)
//                    perror("Error writing on stdout ");
//                if(write(STDOUT_FILENO, "\n", 1) < 0)
//                    perror("Error writing on stdout ");
//            }
