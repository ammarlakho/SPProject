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
using namespace std::chrono;


int main(int argc, char *argv[]) {

    int length = 4096;
    int readB;
    int ansB;
    char commandC[length];
    char ans[length];

    int sock;
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

        while(1) {
//            Prompt for command
            char prompt[] = "Enter command:\n";
            if(write(STDOUT_FILENO, prompt, strlen(prompt)) < 0)
                perror("writing on stdout ");

//            Send command to server
            if((readB = read(STDIN_FILENO, commandC, length)) < 0)
                perror("reading on stdout ");
            int wSockLen;
            if ((wSockLen = write(sock, commandC, readB)) < 0)
                perror("writing on socket ");

//            Get result from server
            if((ansB = read(sock, ans, length)) < 0)
                perror("reading on socket ");

//            To prevent reading answer of previous command while doing strcmp
            ans[ansB] = '\0';

//            Exit
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

        close(sock);

    return 0;

}
