#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define maxSizeOfLogin 31
#define maxSizeOfPassword 31
#define messBufMaxSize 7000
#define minSizeOfMess 2

struct Message
{
    char type;
    int length;
    int stringLength[messBufMaxSize];
    char* strings[messBufMaxSize];
    int k;
};

int sizeOfMes (char* mes)
{
    int i = 1;
    int res = 0;
    for (i; i < 5; i++) {
        res = res * 256 + (mes[i] + 256) % 256;
    }
    return res;
}

void Pars(char* mes, struct Message* parsMes)
{
    int c = 0;
    int a = 0;
    parsMes->type = mes[0];
    int i = 1;
    int j = 0;
    parsMes->length = sizeOfMes(mes);
    int k = 0;
    i = 5;
    while (i < parsMes->length + 5) {
        j = 0;
        parsMes->stringLength[k] = 0;
        for (j; j < 4; ++j) {
            parsMes->stringLength[k] = parsMes->stringLength[k] * 256 + (mes[i] + 256) % 256;
            i++;
        }
        j = 0;
        parsMes->strings[k] = malloc(parsMes->stringLength[k] + 1);
        parsMes->strings[k][0] = 0;
        for (j; j <  parsMes->stringLength[k]; j++) {
            parsMes->strings[k][j] = mes[i];
            i++;
        }
        parsMes->strings[k][j] = 0;
        k++;
    }
    parsMes->k = k;
    return;
}

void MakeMes(char* mes, struct Message* parsMes) {
    mes[0] = parsMes->type;
    int Length = parsMes->length;
    int i = 4;
    for (i; i >= 1; i--) {
        mes[i] = (char)(Length % 256);
        Length /= 256;
    }
    int j = 5;
    int k = 0;
    for (k; k < parsMes->k; k++) {
        Length = parsMes->stringLength[k];
        i = j + 3;
        for (i; i >= j; i--) {
            mes[i] = (char)(Length % 256);
            Length /= 256;
        }
        j += 4;
        int a = 0;
        for (a; a < parsMes->stringLength[k]; a++) {
            mes[j] = parsMes->strings[k][a];
            j++;
        }
    }
    return;
}

void PrintTime( char* timeMic )
{
    int i = 0;
    time_t timeInt = 0;
    for (i; i < 4; i++) {
        timeInt = timeInt * 256 + (timeMic[i] + 256) % 256;
    }
    struct tm* timeStruct = localtime(&timeInt);
    printf("[%d:%d:%d] ", timeStruct->tm_hour, (timeStruct->tm_min), timeStruct->tm_sec);
}

void* serverProcess(void * data) 
{
    int sockid = *(int*)data;
    
    while (1) {
        char messBuf[messBufMaxSize];
        int mess = recv(sockid, messBuf, 5, 0);
        if (mess <= 0) {
            printf("Server break work\n");
            close(sockid);
            exit(0);
            return NULL;
        }
        mess = recv(sockid, messBuf + 5, sizeOfMes(messBuf), 0);
        struct Message parsMes;
        Pars(messBuf, &parsMes);
        switch (parsMes.type) {
            case 'r':
                if (parsMes.k != 3) {
                    printf("Error in regular message\n");
                    continue;
                }
                PrintTime(parsMes.strings[0]);
                printf("%s: %s\n", parsMes.strings[1], parsMes.strings[2]);
                break;
            case 'm':
                if (parsMes.k != 2) {
                    printf("Error in server message\n");
                    continue;
                }
                PrintTime(parsMes.strings[0]);
                printf("--- %s ---\n", parsMes.strings[1]);
                break;
            case 'h':
                if (parsMes.k != 3) {
                    printf("Error in history message\n");
                    continue;
                }
                printf("History: ");
                PrintTime(parsMes.strings[0]);
                printf("%s: %s\n", parsMes.strings[1], parsMes.strings[2]);
                break;
                return NULL;
                break;
            case 'l':
                if (parsMes.k != 2) {
                    printf("Error in list message\n");
                    continue;
                }
                printf("--- %s %s is online ---\n", parsMes.strings[0], parsMes.strings[1]);
                break;
            case 'k':
                if (parsMes.k != 1) {
                    printf("Error in kick message\n");
                    continue;
                }
                printf("--- You were kicked because '%s' ---\n", parsMes.strings[0]);
                close(sockid);
                exit(0);
                break;
            case 's':
            {
                int res = 0, i = 0;
                for (i; i < 4; i++) {
                    res = res * 256 + (parsMes.strings[0][i] + 256) % 256;
                }
                switch (res) {
                    case 0:
                       printf("Server: OK\n");
                       break; 
                    case 1:
                        printf("Server: Unknown message\n");
                        break;
                    case 2:
                        printf("Server: Unauthorized user\n");
                        break;
                    case 3:
                        printf("Server: Authentication error\n");
                        break;
                    case 4:
                        printf("Server:  Registration error\n");
                        break;
                    case 5:
                        printf("Server: Access error\n");
                        break;
                    case 6:
                        printf("Server: Invalid message\n");
                        break;
                    default:
                        printf("Server: Unknown S-message\n");
                        break;
                }
                break;
            }
            default:
                printf("Unknown message\n");
                break;
        }
        int a = 0;
        for(a; a < parsMes.k; a++) {
            free(parsMes.strings[a]);
        }
    } 
}

void RMesSend(char* newMes, int sockid) 
{
    int r;
    size_t len = 0;
    char* text = NULL;
    if ((r = getline(&text, &len, stdin)) == -1) {
        printf("Error. Can not read\n");
        return;
    }
    if (r - 1 < minSizeOfMess) {
        printf("Too short message\n");
        return;
    }
    text[r - 1] = 0;
    struct Message notif;
    notif.type = 'r';
    notif.k = 1;
    notif.strings[0] = malloc(messBufMaxSize);
    notif.strings[0][0] = 0;
    strcpy(notif.strings[0], text);
    notif.stringLength[0] = strlen(notif.strings[0]);
    notif.length = notif.stringLength[0] + 4;
    MakeMes(newMes, &notif);
    free(notif.strings[0]);
    send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
}

void IMesSend(char* newMes, int sockid) 
{
    int r;
    size_t len = 0;
    char* text = NULL;
    if ((r = getline(&text, &len, stdin)) == -1) {
        printf("Error. Can not read\n");
        return;
    }
    if (r - 1 > maxSizeOfLogin) {
        printf("Too long login\n");
        return;
    }
    text[r - 1] = 0;
    struct Message notif;
    notif.type = 'i';
    notif.k = 2;
    char* s = malloc(messBufMaxSize);
    s[0] = 0;
    strcpy(s, text);
    notif.strings[0] = s;
    if ((r = getline(&text, &len, stdin)) == -1) {
        printf("Error. Can not read\n");
        return;
    }
    if (r - 1 > maxSizeOfLogin) {
        printf("Too long password\n");
        return;
    }
    text[r - 1] = 0;
    notif.strings[1] = text;
    notif.stringLength[0] = strlen(notif.strings[0]);
    notif.stringLength[1] = strlen(notif.strings[1]);
    notif.length = notif.stringLength[0] + notif.stringLength[1] + 8;
    MakeMes(newMes, &notif);
    free(s);
    send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
}

void OMesSend(char* newMes, int sockid) {
    struct Message notif;
    notif.type = 'o';
    notif.k = 0;
    notif.length = 0;
    MakeMes(newMes, &notif);
    send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
}

void HMesSend(char* newMes, int sockid) 
{
    int r;
    size_t len = 0;
    char* text = NULL;
    if ((r = getline(&text, &len, stdin)) == -1) {
        printf("Error. Can not read\n");
        return;
    }
    int i = 0;
    for (i; i < r - 1; i++) {
        if (text[i] < '0' || text[i] > '9') {
            printf("Incorrect size of history\n");
            return;
        }
    }
    text[r - 1] = 0;
    int num = atoi(text);
    struct Message notif;
    notif.type = 'h';
    notif.k = 1;
    int a = 3;
    for (a; a >= 0; a--) {
        notif.strings[0][a] = (char)(num % 256);
        num /= 256;
    }
    notif.stringLength[0] = 4;
    notif.length = notif.stringLength[0] + 4;
    MakeMes(newMes, &notif);
    send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
}

void LMesSend(char* newMes, int sockid) 
{
    struct Message notif;
    notif.type = 'l';
    notif.k = 0;
    notif.length = 0;
    MakeMes(newMes, &notif);
    send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
}

void KMesSend(char* newMes, int sockid) 
{
    int r;
    size_t len = 0;
    char* text = NULL;
    if ((r = getline(&text, &len, stdin)) == -1) {
        printf("Error. Can not read\n");
        return;
    }
    text[r - 1] = 0;
    struct Message notif;
    notif.type = 'k';
    notif.k = 2;
    char* s = malloc(messBufMaxSize);
    s[0] = 0;
    strcpy(s, text);
    notif.strings[0] = s;
    if ((r = getline(&text, &len, stdin)) == -1) {
        printf("Error. Can not read\n");
        return;
    }
    text[r - 1] = 0;
    notif.strings[1] = text;
    notif.stringLength[0] = strlen(notif.strings[0]);
    notif.stringLength[1] = strlen(notif.strings[1]);
    notif.length = notif.stringLength[0] + notif.stringLength[1] + 8;
    MakeMes(newMes, &notif);
    free(s);
    send(sockid, newMes, sizeOfMes(newMes) + 5, 0);
}

void clientProcess(int sockid)
{
    int r;
    size_t len = 0;
    char* text = NULL;
    while ((r = getline(&text, &len, stdin)) != -1) {
        text[r - 1] = 0;
        if (!strcmp(text, "--regular")) {
            char* newMes;
            newMes = malloc(messBufMaxSize);
            RMesSend(newMes, sockid);
            free(newMes);
            continue;
        } if (!strcmp(text, "--login")) {
            char* newMes;
            newMes = malloc(messBufMaxSize);
            IMesSend(newMes, sockid);
            free(newMes);
            continue;
        } if (!strcmp(text, "--logout")) {
            char* newMes;
            newMes = malloc(messBufMaxSize);
            OMesSend(newMes, sockid);
            free(newMes);
            continue;
        } if (!strcmp(text, "--history")) {
            char* newMes;
            newMes = malloc(messBufMaxSize);
            HMesSend(newMes, sockid);
            free(newMes);
            continue;
        } if (!strcmp(text, "--list")) {
            char* newMes;
            newMes = malloc(messBufMaxSize);
            LMesSend(newMes, sockid);
            free(newMes);
            continue;
        } if (!strcmp(text, "--kick")) {
            char* newMes;
            newMes = malloc(messBufMaxSize);
            KMesSend(newMes, sockid);
            free(newMes);
            continue;
        }
        printf("Incorrect message\n");
    }
    return;
}

int main(int argc, char* argv[])
{
    int portNum = 1337;
    char* ip = malloc(50);
    strcpy(ip, "127.0.0.1");
    int option;
    while ((option = getopt(argc, argv, "-p-i")) != -1) {
        if (option == 'p') {
            portNum = atoi(optarg);
        } else if (option == 'i') {
            strcpy(ip, optarg);
        }
    }
    int sockid;
    if ((sockid = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        fprintf(stderr, "Socket opening failed\n");
        return 1;
    }
    struct sockaddr_in port;
    port.sin_family = AF_INET;
    port.sin_port = htons((uint16_t)portNum);
    port.sin_addr.s_addr = inet_addr(ip);
    free(ip);
    if (connect(sockid, (struct sockaddr *) &port, sizeof(port)) == -1) {
        fprintf(stderr, "Server connecting failed\n");
        return 2;
    }
    pthread_t t;
    pthread_create(&t, NULL, serverProcess, (void *)&sockid);
    clientProcess(sockid);
    free(ip);
    return 0;
}