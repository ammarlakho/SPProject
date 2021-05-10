#include <iostream>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <chrono>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <vector>
#include <poll.h>
#include <unordered_map>

using namespace std;
using namespace std::chrono;


class Client {
    public:
        int fd;
        char ip[100];
        int port;
    Client(int t_fd, char t_ip[], int t_port) {
        fd = t_fd;
        strcpy(ip, t_ip);
        port = t_port;

    }
    Client() {
    }
};

class MyProcess {
  public:
    int pid;
    char pname[50];
    time_t start_time;
    time_t end_time;
    time_t elapsed_time;
    time_t execution_time;
    bool active;
    MyProcess(int x, char y[], time_t t) {
        pid = x;
        strcpy(pname, y);
        start_time = t;
        end_time = 0;
        elapsed_time = 0;
        execution_time = 0;
        active = true;
    }
    MyProcess() {
        pid = 0;
        strcpy(pname, "");
    }
};


// global variables
const int length = 4096;
vector<MyProcess> allList;
vector<MyProcess> activeList;
vector<Client> clients;
unordered_map<int, int> handlerToClient;



int add(char *numbers);
int sub(char *numbers);
int mult(char *numbers);
double div(char *numbers);
double solve(char *numbers, int type);
int opType(char *s, int blank);
int validInput(char *numbers, int type);
void sigChildHandler(int signo);
int removebyID(int id, int wannaKill);
int removebyName(char *name, int wannaKill);
void *thread_accept(void *ptr);
void *thread_command(void *ptr);
void removeClient(int id);
void computeList(char* plist, int type);
void writeFileContents(int fname);





int main() {

    signal(SIGCHLD, sigChildHandler);

    int sock, serverLength;
    struct sockaddr_in server;

    /* Create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("opening stream socket");
        exit(1);
    }

//    int flags;
//    flags = fcntl(sock, F_GETFL);
//    if(flags < 0)
//        perror("could not get flags on TCP listening socket");
//    if(fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
//        perror("could not set TCP listening socket to be non-blocking");


    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = ntohs(2223);
    if (bind(sock, (struct sockaddr *) &server, sizeof(server))) {
        perror("binding stream socket");
        exit(1);
    }

    /* Find out assigned port number and print it out */
    serverLength = sizeof(server);
    if (getsockname(sock, (struct sockaddr *) &server, (socklen_t*) &serverLength)) {
        perror("getting socket name");
        exit(1);
    }
    char pNum[100];
    int pLen = sprintf(pNum, "Socket has port #%d\n", ntohs(server.sin_port));
    write(STDOUT_FILENO, pNum, pLen);

    /* Start accepting connections */
    listen(sock, 5);


    pthread_t threadAccept, threadCommand;

    int  iret1, iret2;

    /* Create independent threads each of which will execute function */
    iret1 = pthread_create(&threadAccept, NULL, thread_accept, (void*) sock);
    if(iret1) {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret1);
        exit(EXIT_FAILURE);
    }

    iret2 = pthread_create(&threadCommand, NULL, thread_command, (void*) sock);
    if(iret2) {
        fprintf(stderr,"Error - pthread_create() return code: %d\n",iret2);
        exit(EXIT_FAILURE);
    }

    pthread_join(threadAccept, NULL);
    pthread_join(threadCommand, NULL);

}


void *thread_command(void *ptr) {

    char commandC[length];
    char result[length];
    int resL = 0;
    int readB = 0;
    char prompt[] = "Enter command:\n\n";
    if(write(STDOUT_FILENO, prompt, strlen(prompt)) < 0)
            perror("writing on stdout ");

    while(1) {
        bzero(result, sizeof(result));
        bzero(commandC, sizeof(commandC));

        if((readB = read(STDIN_FILENO, commandC, length)) < 0)
            perror("reading on stdout ");


//      Removing new line character and any spaces at the end
        commandC[readB-1] = '\0';
        for (int i=readB-2; i>=0; i--) {
            if(commandC[i] == ' ')
                commandC[i] = '\0';
            else
                break;
        }

        char *first = strtok(commandC, " ");

        if(strcmp(first, "connList")== 0) {
            resL = 0;
            for(int i=0; i<clients.size(); i++) {
                char row[256];
                resL += sprintf(row, "FD: %d, IP: %s, Port: %d\n", clients[i].fd, clients[i].ip, clients[i].port);
                strcat(result, row);
            }
            strcat(result, "\n");
            if(resL > 0)
                resL = strlen(result);
            else
                resL = sprintf(result, "No Clients\n\n");
            write(STDOUT_FILENO, result, resL);
        }

        else if(strcmp(first, "print")== 0) {

            char *second = strtok(NULL, " ");
            char *third = strtok(NULL, " ");

            char printMsg[100];
            strcpy(printMsg, second);
            int lastByte = strlen(second);
            printMsg[lastByte] = '\n';
            printMsg[lastByte+1] = '\0';

            if (third == NULL) {
                for(int i=0; i<clients.size(); i++) {
                    write(clients[i].fd, printMsg, strlen(printMsg));
                }
            }
            else {
                int c_id = atoi(third);
                for(int i=0; i<clients.size(); i++) {
                    if(c_id == clients[i].fd) {
                        write(clients[i].fd, printMsg, strlen(printMsg));
                        break;
                    }
                }
            }
            write(STDOUT_FILENO, "\n", 1);
        }

        else if(strcmp(first, "processList") == 0)  {
            char *second = strtok(NULL, " ");

            if(second != NULL) {
                bool exist = false;
                int second_int = atoi(second);
                for(int i=0; i<clients.size(); i++) {
                    if (second_int == clients[i].fd) {
                        exist = true;
                        break;
                    }
                }
                if(exist) {
                    writeFileContents(second_int);

                }
                else {
                    char errMsg[100];
                    int errLen = sprintf(errMsg, "Error: Client with fd=%d doesn't exist\n\n", second_int);
                    write(STDOUT_FILENO, errMsg, errLen);

                }
            }
            else {
                if(clients.size() > 0) {
                    for(int i=0; i<clients.size(); i++) {
                        writeFileContents(clients[i].fd);
                    }
                }
                else {
                    char errMsg[100];
                    int errLen = sprintf(errMsg, "No Clients connected currently.\n\n");
                    write(STDOUT_FILENO, errMsg, errLen);
                }
            }
        }

        else {
            char errMsg[] = "Invalid Command\n\n";
            write(STDOUT_FILENO, errMsg, strlen(errMsg));
        }
    }

}


void *thread_accept(void *ptr) {
    int sock_local = (intptr_t) ptr;
    struct sockaddr_in client_addr;
    socklen_t clilen = sizeof(client_addr);
    int rval;
    int iter = 0;

    while(1) {
        int msgsock = accept(sock_local, (struct sockaddr *) &client_addr, &clilen);
        int acceptError = errno;
        if (msgsock == -1 && acceptError != EWOULDBLOCK)
            perror("accept");
        else {
            Client client = Client(msgsock, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
            clients.push_back(client);

            int fid = fork();

            if (fid > 0) {
                handlerToClient[fid] = client.fd;

            }
            if (fid == 0) {
                while(1) {
                    char commandS[length];
                    char *commandTokenized;
                    char *commandTokenizedCopy;
                    char ansS[length];
                    int ansL = 0;
                    bzero(ansS, sizeof(ansS));
                    bzero(commandS, sizeof(commandS));

                    char fileName[100];
                    sprintf(fileName, "processInfo/%d", client.fd);
                    int fd_wr = open(fileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
                    if(fd_wr < 0)
                        perror("IDHAR ");

                    else {
                        char listForServer[length];
                        bzero(listForServer, sizeof(listForServer));

                        if(activeList.size() > 0)
                            computeList(listForServer, 0);

                        if(write(fd_wr, listForServer, strlen(listForServer)) < 0)
                            perror("writing on file ");
                    }
//                    if(acceptError != EWOULDBLOCK) {
                        if ((rval = read(msgsock, commandS, length)) < 0)
                        perror("reading on socket");
//                    }

    //                Removing new line character and any spaces at the end
                    commandS[rval-1] = '\0';
                    for (int i=rval-2; i>=0; i--) {
                        if(commandS[i] == ' ')
                            commandS[i] = '\0';
                        else
                            break;
                    }


                    if (rval == 0) {
                        char msg[] = "Ending Connection\n";
//                        write(STDOUT_FILENO, msg, strlen(msg));
//                        break;
                    }
                    else {
                        char commandSCopy[length];
                        strcpy(commandSCopy, commandS);

            //            Do the command
                        commandTokenized = strtok (commandS, " ");
                        int type;
                        if(strlen(commandS) == 0)
                            type = opType(commandTokenized, 1);
                        else
                            type = opType(commandTokenized, 0);


                        if(type != -1 && type != -99) {
                            if(type == 0) {
                                ansL = sprintf(ansS, "exit");
                            }
            //                Math operations
                            else if(type >= 1 && type <= 4) {
                                int valid = validInput(commandTokenized, type);
                                if(valid == 0) {
                                    commandTokenizedCopy = strtok(commandSCopy, " ");
                                    double ans = solve(commandTokenizedCopy, type);
                                    if(type!=4)
                                        ansL = sprintf(ansS, "Ans=%.0f\n", ans);
                                    else
                                        ansL = sprintf(ansS, "Ans=%.2f\n", ans);
                                }
                                else if(valid == -1){
                                    ansL = sprintf(ansS, "Error: Enter numbers only!\n");
                                }
                                else if(valid == -2){
                                    ansL = sprintf(ansS, "Error: Can't divide by 0!\n");
                                }
                            }

            //                exec
                            else if(type==5) {
                                commandTokenized = strtok(NULL, " ");
                                char *args[10];
                                int j = 0;
                                while (commandTokenized != NULL) {
                                    args[j] = commandTokenized;
                                    commandTokenized = strtok(NULL, " ");
                                    j++;
                                }
                                args[j] = NULL;

                                if (j>=1) {
                                    int pipeProcess[2];
                                    pipe(pipeProcess);
                                    int pipeFailure[2];
                                    pipe2(pipeFailure, O_CLOEXEC);

                                    int pidRun = fork();
                                    if(pidRun == 0) {
                                        close(pipeProcess[0]);
                                        close(pipeFailure[0]);
                                        auto clock = high_resolution_clock::now();
                                        time_t start_time = chrono::system_clock::to_time_t(clock);

                                        MyProcess newP = MyProcess(getpid(), args[0], start_time);

                                        if(write(pipeProcess[1], &newP, sizeof(MyProcess)) < 0)
                                            perror("write on pipe");
                                        int a = execvp(args[0], args);
                                        if(a == -1) {
                                            perror("ERROR ");
                                            write(pipeFailure[1], "failed", 6);
                                            exit(0);
                                        }
                                    }
                                    else {
                                        close(pipeProcess[1]);
                                        close(pipeFailure[1]);

                                        MyProcess p;
                                        read(pipeProcess[0], &p, sizeof(MyProcess));

                                        char *failureMsg;
                                        int f = read(pipeFailure[0], &failureMsg, 6);
                                        if(f == 0) {
                                            activeList.push_back(p);
                                            allList.push_back(p);
                                            ansL = sprintf(ansS, "Successful execution of %s\n", args[0]);
                                        }
                                        else {
                                            ansL = sprintf(ansS, "Exec failed. Try again :(\n");
                                        }
                                    }
                                }
                                else {
                                    ansL = sprintf(ansS, "Insufficient arguments to run\n");
                                }

                            }
                            else if(type == 6) {
                                MyProcess emptyP;
                                char *arg = strtok(NULL, " ");
                                if(arg == NULL) {
                                    ansL = sprintf(ansS, "Insufficient arguments to kill\n");
                                }
                                else {
                                    bool idOrNot = true;
                                    int foundID = 0;
                                    int foundName = 0;

                                    for(int i=0; i<strlen(arg); i++) {
                                        if(!isdigit(arg[i])) idOrNot = false;
                                    }
                                    if(idOrNot) {
                                        int num = atoi(arg);
                                        foundID = removebyID(num, 1);
                                    }
                                    else {
                                        foundName = removebyName(arg, 1);
                                    }

                                    if (foundName == -1)
                                        ansL = sprintf(ansS, "Can not find process with the name '%s'\n", arg);
                                    else if (foundID == -1)
                                        ansL = sprintf(ansS, "Can not find process with the id '%s'\n", arg);
                                    else
                                        ansL = sprintf(ansS, "Killed!\n");
                                }
                            }
                            else if(type == 7) {
                                if(activeList.size() > 0) {
                                    computeList(ansS, 0);
                                    ansL = strlen(ansS);
                                }
                                else
                                    ansL = sprintf(ansS, "No processes active\n");

                            }
                            else if(type == 8) {
                                if(allList.size() > 0) {
                                    computeList(ansS, 1);
                                    ansL = strlen(ansS);
                                }
                                else
                                    ansL = sprintf(ansS, "No processes\n");
                            }
                        }
                        else {
                            if(type == -1) {
                                if (commandTokenized[strlen(commandTokenized)-1] == '\n')
                                    commandTokenized[strlen(commandTokenized)-1] = '\0';
                            }

                            ansL = sprintf(ansS, "Error: Cannot recognize the command: %s\n", commandTokenized);

                        }

                //        Send result to client
                        int wSockLen;
                        if ((wSockLen = write(msgsock, ansS, ansL)) < 0)
                            perror("writing on socket ");

                        if(type == 0) {
//                            Kill all processes executed by client
                            for(int i=0; i<activeList.size(); i++) {
                                if(activeList[i].pid != 0 && strcmp(activeList[i].pname, "") != 0) {
                                    if(kill(activeList[i].pid, SIGTERM) < 0)
                                        perror("kill ");

                                }
                            }
//                            Delete the file you are storing your process list in
                            remove(fileName);

                            exit(0);
                        }
                    }
                }
            }
        }
//        close(msgsock);
    }
}


void writeFileContents(int fname) {
    char fileName[100];
    sprintf(fileName, "processInfo/%d", fname);
//    cout << fileName << endl;
    int fd_re = open(fileName, O_RDONLY, S_IRWXU);
    if(fd_re < 0)
        perror("opening file ");
    else {
        char listToPrint[length];
        char content[length-100];
        char heading[100];
        bzero(listToPrint, sizeof(listToPrint));
        bzero(content, sizeof(content));
        bzero(heading, sizeof(heading));

        sprintf(heading, "Processes for fd=%d\n", fname);
        strcat(listToPrint, heading);

        int listLen = read(fd_re, content, length);
        strcat(listToPrint, content);
        strcat(listToPrint, "\n\0");

        write(STDOUT_FILENO, listToPrint, strlen(listToPrint));
    }
}


void sigChildHandler(int signo) {
    int status;
    int eID = waitpid(-1, &status, 0);

    if(eID != -1) {
        removebyID(eID, 0);
        removeClient(eID);
    }
    else {
        perror("Wait Error: ");
    }
}

void removeClient(int pid) {
    int deleteID = -1;
    unordered_map<int, int>:: iterator deleteObj = handlerToClient.find(pid);

    if(deleteObj != handlerToClient.end()) {
        deleteID = deleteObj->second;
        for(int i=0; i<clients.size(); i++) {
            if(clients[i].fd == deleteID) {
                clients.erase(clients.begin() + i);
                break;
            }
        }
    }
}

int removebyID(int id, int wannaKill) {
    bool foundID = false;
    for(int i=0; i<activeList.size(); i++) {
        if(activeList[i].pid == id && activeList[i].pid != 0) {
            foundID = true;
            if(wannaKill == 1)
                if(kill(activeList[i].pid, SIGTERM) < 0)
                    perror("kill error: ");
            activeList.erase(activeList.begin()+i);
            break;
        }
    }

    auto current_clock = high_resolution_clock::now();
    time_t current_time = chrono::system_clock::to_time_t(current_clock);
    for(int j=0; j<allList.size(); j++) {
        if(allList[j].pid == id) {
            allList[j].end_time = current_time;
            allList[j].active = false;
            break;
        }
    }


    if(!foundID)
        return -1;
    else
        return 0;

}
int removebyName(char *name, int wannaKill) {
    bool foundName = false;
    int id = -1;
    for(int i=0; i<activeList.size(); i++) {
        if(strcmp(activeList[i].pname, name) == 0) {
            foundName = true;
            int num = atoi(name);
            if(wannaKill == 1)
                if(kill(activeList[i].pid, SIGTERM) < 0)
                    perror("kill error: ");
            activeList.erase(activeList.begin()+i);
            id = activeList[i].pid;
            break;
        }
    }

    if(foundName) {
        auto current_clock = high_resolution_clock::now();
        time_t current_time = chrono::system_clock::to_time_t(current_clock);
        for(int j=0; j<allList.size(); j++) {
            if(allList[j].pid == id) {
                allList[j].end_time = current_time;
                allList[j].active = false;
                break;
            }
        }
    }

    if(!foundName)
        return -1;
    else
        return 0;

}


double solve(char *numbers, int type) {
    if(type == 1) {
        return add(numbers);
    }
    else if(type == 2) {
        return sub(numbers);
    }
    else if(type == 3) {
        return mult(numbers);
    }
    else if(type == 4) {
        return div(numbers);
    }
}


int add(char *numbers) {
    int sum = 0;
    numbers = strtok (NULL, " ");
    while (numbers != NULL) {
        int num = atoi(numbers);
        sum += num;
        numbers = strtok (NULL, " ");
    }
    return sum;
}

int sub(char *numbers) {
    int ans;
    numbers = strtok (NULL, " ");
    int i=0;
    while (numbers != NULL) {
        int num = atoi(numbers);
        if (i == 0)
            ans = num;
        else
            ans -= num;

        numbers = strtok (NULL, " ");
        i++;
    }
    return ans;
}

int mult(char *numbers) {
    int ans = 1;
    numbers = strtok (NULL, " ");
    while (numbers != NULL) {
        int num = atoi(numbers);
        ans *= num;
        numbers = strtok (NULL, " ");
    }
    return ans;
}

double div(char *numbers) {
    double ans;
    numbers = strtok (NULL, " ");
    int i = 0;
    while (numbers != NULL) {
        int num = atoi(numbers);
        if (i == 0)
            ans = num;
        else {
            ans /= num;
        }
        numbers = strtok (NULL, " ");
        i++;
    }
    return ans;
}


int validInput(char *numbers, int type) {
    numbers = strtok (NULL, " ");
    int j = 0;
    while (numbers != NULL) {
        for(int i=0; i<strlen(numbers); i++) {
            if(!isdigit(numbers[i]) && numbers[i] != '\n') {
                return -1;
            }
        }
        if(j!=0 && type==4) {
            if(atoi(numbers) == 0) {
                return -2;
            }
        }
        numbers = strtok (NULL, " ");
        j++;
    }
    return 0;
}


int opType(char *s, int blank) {

    if(blank == 1)
        return -99;

    if (s[strlen(s)-1] == '\n')
        s[strlen(s)-1] = '\0';

    if(strcmp(s, "add")==0) {
        return 1;
    }
    else if(strcmp(s, "sub") == 0) {
        return 2;
    }
    else if(strcmp(s, "mult") == 0) {
        return 3;
    }
    else if(strcmp(s, "div") == 0) {
        return 4;
    }
    else if(strcmp(s, "run") == 0) {
        return 5;
    }
    else if(strcmp(s, "kill") == 0) {
        return 6;
    }
    else if(strcmp(s, "list") == 0) {
        return 7;
    }
    else if(strcmp(s, "listAll") == 0) {
        return 8;
    }

    else if(strcmp(s, "exit") ==0 ) {
        return 0;
    }
    return -1;
}


void computeList(char* plist, int type) {
    if(type == 0) {
        auto current_clock = high_resolution_clock::now();
        time_t current_time = chrono::system_clock::to_time_t(current_clock);
        for(int i=0; i<activeList.size(); i++) {
            //Time since it was run
            int elapsed_time = difftime(current_time, activeList[i].start_time);
            char row[128];
            sprintf(row, "ID: %d, Name: %s, Start: %s, T(Elapsed): %ds\n", activeList[i].pid, activeList[i].pname, ctime(&activeList[i].start_time), elapsed_time);
            strcat(plist, row);
        }
        strcat(plist, "\n");
        strcat(plist, "\0");
    }
    else {
        auto current_clock = high_resolution_clock::now();
        time_t current_time = chrono::system_clock::to_time_t(current_clock);
        for(int i=0; i<allList.size(); i++) {
            char rowP1[128];
            char rowP2[128];
//            Time since it was run
            int elapsed_time = difftime(current_time, allList[i].start_time);
            char *active;
            if(allList[i].active)
                active = "yes";
            else
                active = "no";
            if(allList[i].end_time == 0) {
                sprintf(rowP1, "ID: %d, Name: %s, Start: %s, End: N/A, T(Elapsed): %ds, T(Execution): N/A, Active:%s\n", allList[i].pid, allList[i].pname, ctime(&allList[i].start_time), elapsed_time, active);
                strcat(plist, rowP1);
            }
            else {
                time_t end_time = allList[i].end_time;
//                    total time it ran for
                int execution_time = difftime(allList[i].end_time, allList[i].start_time);
                sprintf(rowP1, "ID: %d, Name: %s, Start: %s, ", allList[i].pid, allList[i].pname, ctime(&allList[i].start_time));
                sprintf(rowP2, "End: %s, T(Elapsed): %ds, T(Execution): %ds, Active:%s\n", ctime(&end_time), elapsed_time, execution_time, active);
                strcat(plist, rowP1);
                strcat(plist, rowP2);
            }

        }
        strcat(plist, "\n");
        strcat(plist, "\0");
    }

}
