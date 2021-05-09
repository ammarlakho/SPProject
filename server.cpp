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
        int id;
        char ip[100];
        int port;
        int fd;
    Client(int t_id, char t_ip[], int t_port, int t_fd) {
        id = t_id;
        strcpy(ip, t_ip);
        port = t_port;
        fd = t_fd;
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

// global constants
const int length = 4096;
const int MAX_ACTIVE = 15;
const int MAX_ALL = 30;
// global variables
MyProcess allList[MAX_ALL];
MyProcess activeList[MAX_ACTIVE];
vector<Client> clients;
unordered_map<int, int> handlerToClient;



int add(char *numbers);
int sub(char *numbers);
int mult(char *numbers);
double div(char *numbers);
double solve(char *numbers, int type);
int opType(char *s, int blank);
int validInput(char *numbers, int type);
int first_vacant(MyProcess processes[], int len);
void sigChildHandler(int signo);
int removebyID(int id, int wannaKill);
int removebyName(char *name, int wannaKill);
void *thread_accept(void *ptr);
void *thread_command(void *ptr);
void removeClient(int id);




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
                resL += sprintf(row, "ID: %d, IP: %s, Port: %d, fd: %d\n", clients[i].id, clients[i].ip, clients[i].port, clients[i].fd);
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

            second[strlen(second)] = '\n';
            if (third == NULL) {
                for(int i=0; i<clients.size(); i++) {
                    write(clients[i].fd, second, strlen(second));
                }
            }
            else {
                int c_id = atoi(third);
                cout << third << endl;
                for(int i=0; i<clients.size(); i++) {
                    if(c_id == clients[i].fd) {

                        write(clients[i].fd, second, strlen(second));
                        break;
                    }
                }
            }
        }

        else if(strcmp(first, "processList") == 0)  {
            char *second = strtok(NULL, " ");
            char *third = strtok(NULL, " ");

            write(STDOUT_FILENO, "PROCESS\n\n", 9);
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
        if (msgsock == -1)
            perror("accept");
        else {
            iter++;
            char cMsg[100];
//            int cLen = sprintf(cMsg,"server: got connection from %s port %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
//            write(STDOUT_FILENO, cMsg, cLen);
            Client client = Client(iter, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), msgsock);
            clients.push_back(client);

            int fid = fork();
//            handlerToClient[fid] = client.id;
            if (fid > 0) {
                handlerToClient[fid] = client.id;
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

                    if ((rval = read(msgsock, commandS, length)) < 0)
                        perror("reading on socket");

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
                        bzero(activeList, sizeof(activeList));
                        bzero(allList, sizeof(allList));
                        write(STDOUT_FILENO, msg, strlen(msg));
                        break;
                    }
                    else {
                        char commandSCopy[strlen(commandS)];
                        for(int i=0; i<strlen(commandS); i++) {
                            commandSCopy[i] = commandS[i];
                        }

    //

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
                                char *args[100];
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
                                            int v1 = first_vacant(activeList, MAX_ACTIVE);
                                            int v2 = first_vacant(allList, MAX_ALL);
                                            if(v1 == MAX_ACTIVE) {
                                                ansL = sprintf(ansS, "Error: Max limit reached for active processes. Retry after killing a process.\n");
                                            }
                                            else if(v2 == MAX_ALL) {
                                                ansL = sprintf(ansS, "Error: Max limit reached for total processes. No more processes for you :(\n");
                                            }
                                            else {
                                                activeList[v1] = p;
                                                allList[v2] = p;
                                                ansL = sprintf(ansS, "Successful execution of %s\n", args[0]);
                                            }
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

                                    if (arg[strlen(arg)-1] == '\n')
                                        arg[strlen(arg)-1] = '\0';

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
                                    auto current_clock = high_resolution_clock::now();
                                    time_t current_time = chrono::system_clock::to_time_t(current_clock);
                                    ansL = 0;
                                    for(int i=0; i<MAX_ACTIVE; i++) {
                                        if(activeList[i].pid != 0 && strcmp(activeList[i].pname, "") != 0) {
                                //            Time since it was run
                                            int elapsed_time = difftime(current_time, activeList[i].start_time);
                                            char row[128];
                                            ansL += sprintf(row, "ID: %d, Name: %s, Start: %s, T(Elapsed): %ds\n", activeList[i].pid, activeList[i].pname, ctime(&activeList[i].start_time), elapsed_time);
                                            strcat(ansS, row);
                                        }
                                    }
                                    strcat(ansS, "\n");
                                    if(ansL > 0)
                                        ansL = strlen(ansS);
                                    else
                                        ansL = sprintf(ansS, "No processes active\n");
                            }
                            else if(type == 8) {
                                auto current_clock = high_resolution_clock::now();
                                time_t current_time = chrono::system_clock::to_time_t(current_clock);
                                ansL = 0;
                                for(int i=0; i<MAX_ALL; i++) {
                                    if(allList[i].pid != 0 && strcmp(allList[i].pname, "") != 0) {
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
                                            ansL += sprintf(rowP1, "ID: %d, Name: %s, Start: %s, End: N/A, T(Elapsed): %ds, T(Execution): N/A, Active:%s\n", allList[i].pid, allList[i].pname, ctime(&allList[i].start_time), elapsed_time, active);
                                            strcat(ansS, rowP1);
                                        }
                                        else {
                                            time_t end_time = allList[i].end_time;
                        //                    total time it ran for
                                            int execution_time = difftime(allList[i].end_time, allList[i].start_time);
                                            ansL += sprintf(rowP1, "ID: %d, Name: %s, Start: %s, ", allList[i].pid, allList[i].pname, ctime(&allList[i].start_time));
                                            ansL += sprintf(rowP2, "End: %s, T(Elapsed): %ds, T(Execution): %ds, Active:%s\n", ctime(&end_time), elapsed_time, execution_time, active);
                                            strcat(ansS, rowP1);
                                            strcat(ansS, rowP2);
                                        }
                                    }
                                }
                                strcat(ansS, "\n");
                                if(ansL > 0)
                                    ansL = strlen(ansS);
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
                            for(int i=0; i<MAX_ACTIVE; i++) {
                                if(activeList[i].pid != 0 && strcmp(activeList[i].pname, "") != 0) {
                                    if(kill(activeList[i].pid, SIGTERM) < 0)
                                        perror("kill ");

                                }
                            }

//                            Send a message back to conn to delete this particular client
                            for(int i=0; i<clients.size(); i++) {
                               if(clients[i].id == client.id) {
                                   clients.erase(clients.begin() + i);
                                    break;
                                }
                            }

                            exit(0);
                        }
                    }
                }
            }
        }
//        close(msgsock);
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

void removeClient(int id) {
    // itr works as a pointer to pair<string, double>
    // type itr->first stores the key part  and
    // itr->second stroes the value part
    //cout << itr->first << "  " << itr->second << endl;
    int deleteID = -1;
    unordered_map<int, int>:: iterator itr;
    for (itr = handlerToClient.begin(); itr != handlerToClient.end(); itr++) {
        if(itr->first != 0) {
            if(itr->first == id) {
                deleteID = itr->second;
                break;
            }
        }
     }
     if(deleteID != -1) {
        for(int i=0; i<clients.size(); i++) {
            if(clients[i].id == deleteID) {
                clients.erase(clients.begin() + i);
                break;
            }
        }
     }
}

int removebyID(int id, int wannaKill) {
    MyProcess emptyP;
    bool foundID = false;
    for(int i=0; i<MAX_ACTIVE; i++) {
        if(activeList[i].pid == id && activeList[i].pid != 0) {
            foundID = true;
            if(wannaKill == 1)
                if(kill(activeList[i].pid, SIGTERM) < 0)
                    perror("kill error: ");
            activeList[i] = emptyP;
            auto current_clock = high_resolution_clock::now();
            time_t current_time = chrono::system_clock::to_time_t(current_clock);
            allList[i].end_time = current_time;
            allList[i].active = false;

            break;
        }
    }
    if(!foundID)
        return -1;
    else
        return 0;

}
int removebyName(char *name, int wannaKill) {
    MyProcess emptyP;
    bool foundName = false;
    for(int i=0; i<MAX_ACTIVE; i++) {
        if(strcmp(activeList[i].pname, name) == 0) {
            foundName = true;
            int num = atoi(name);
            if(wannaKill == 1)
                if(kill(activeList[i].pid, SIGTERM) < 0)
                    perror("kill error: ");
            activeList[i] = emptyP;
            auto current_clock = high_resolution_clock::now();
            time_t current_time = chrono::system_clock::to_time_t(current_clock);
            allList[i].end_time = current_time;
            allList[i].active = false;

            break;
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

int first_vacant(MyProcess processes[], int len) {
    int last = len;
        for(int i=0; i<last; i++) {
            if(processes[i].pid == 0 && strcmp(processes[i].pname, "") == 0) {
                last = i;
                break;
            }
        }
    return last;
}




