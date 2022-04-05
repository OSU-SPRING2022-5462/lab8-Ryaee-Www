/*
 * This program is a data-gram socket client-server file. It is based on the simple DGRAM socket server illustrated by Professor Dave Ogle, and further implemented and modifed by YuChen Gu on 3/2/2022.
 * This program will receive the incomming message, and send msg to all server listed in config file if detected user input in console.
 */

#include <string.h> 
#include <sys/select.h>
#include <stdio.h> 
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <arpa/inet.h>
#include <unistd.h> 
#include <stdlib.h> 
#include <time.h>
#include <math.h>
#define STDIN 0
const char* configFileName = "config.txt";
const char currentVersion = '4';
const char HOPCOUNT = '4';
char currentLoc[29];/*local host loc str*/
int matrixRow = 0;
int matrixColumn = 0;

int sendMsg(char *buffer, int newSocket, struct sockaddr_in serverAddress);

void processRecvdV2(char* buffer, char* version, int *loc, char* dest, char* source, int *hopCount, char* msg);

void processRecvdV3(char* buffer, char* version, int *loc, char* dest, char* source, int *hopCount, char* msgType, int* msgID, char* remoteRoute, char* msg);

void getRemote(char *line, char *remoteIP, char* remotePort, char* location);

int checkOutOfRange(int localLoc, int remoteLoc);

int sendMsgV2(char *buffer, int newSocket, struct sockaddr_in serverAddress, char* recvdPort);

int findRemotePortID(int portNum);

void updateSeqID (int portNum, int newID);

struct remoteHost{/*linked list of valid remote host in conifg.txt*/
    struct sockaddr_in serverAddr;
    struct remoteHost *nextHost;
    int loc;
    int remotePort;
    int msgSeq;
};
struct remoteHost *head = NULL;
struct remoteHost *currentHost = NULL;

int main(int argc, char *argv[]) {
    const int detectionRange = 2;
    int newSocket;/*socket describer*/
    int returnCode;/*return code*/
    struct sockaddr_in serverAddress;/*server address*/
    struct sockaddr_in clientAddress;/*client address*/
    char buffer[227];/*buffer*/
    int flag = 0;/*socket call flags*/
    int portNum;/*remote port*/
    char portNumStr[29];
    char serverIP[29];/*server IP*/
    socklen_t clientLength = sizeof(struct sockaddr);/*client socket length*/
    int DEBUG = 0;
    char msg[131];
    fd_set socketFDS;
    int maxSD;
    char line[256];
    char locStr[4];
    int configLineCount=0;/*the # line in the config file*/
    int i = 0;/*loop variant*/
    int totalHost = 0;/*total remote host recorded*/
    int invalidLine;/*boolean indication, 1 means the current config line has ip and/or port number invalid*/
    int flawedProcedure=0;/*boolean indication, 1 means at least one of the item in config.txt is not valid*/
    int foundHostInfo = 0;/*boolean indication, 1 means host port was located in the config file, 0 otherwise*/
    char userDecision;/*y or n or else, user input that decide what happen next if the local host was not found*/
    char matrixRowStr[256];
    char matrixColumnStr[256];
    int currentLocDigit = 0;/*local host loc digit*/
    

    /*version 3 new variables*/
    int outletMsgSeq;
    char remoteMSGType [4];
    int remoteMsgID;
    char remoteRoute [25];

    FILE *configFile/*config file*/;
    char recversion[2];
    char recvLoc[4];
    int recvLocDigit;
    int recvHopCount;
    char recvSourcePort[29];
    char recvMsg[101];
    
    if (argc < 2){/*error: check cmd line syntax*/
        printf("usage is: %s <local port number>\n",argv[0]);
        exit(1);
    }

    /*fetch remote IP info from cmd line*/
    for (i = 0; i < strlen(argv[1]); i++){
        if (!isdigit(argv[1][i])){
            printf("The provided port number is invalid, it must be only digits\n");
            exit(1);
        }
    }



    /* row and column as prompt
    printf("Enter the number of column in the matrix: ");
    scanf("%s", matrixColumnStr);
    matrixColumn = atoi(matrixColumnStr);
    while (matrixColumn ==0){
        printf("Invalid input, matrix column number must be non-zero numerical value.\n");
        printf("Enter the number of column in the matrix: ");
        scanf("%s", matrixColumnStr);
        matrixColumn = atoi(matrixColumnStr);
    }

    printf("Enter the number of row in the matrix: ");
    scanf("%s", matrixRowStr);
    
    matrixRow = atoi(matrixRowStr);
    while (matrixRow ==0){
        printf("Invalid input, matrix row number must be non-zero numerical value.\n");
        printf("Enter the number of row in the matrix: ");
        scanf("%s", matrixRowStr);
        matrixRow = atoi(matrixRowStr);
    }
    */


    /*open and read config file*/
    configFile = fopen(configFileName,"r");

    if (!configFile){
       printf("Error when opening config file");
       fclose(configFile);
       exit(1);
    }
    printf("\nAttempting fetching remote Host info from \"config.txt\"...\n\n");

    /*socket creation*/
    newSocket = socket(AF_INET, SOCK_DGRAM, 0);

    /*Extract all host from config*/
    memset(line,'\0',256);
    while (fgets(line, sizeof(line), configFile)) {
        invalidLine = 0;
        configLineCount++;
        
        memset(serverIP,'\0',29);
        memset(portNumStr,'\0',29);
        memset(locStr,'\0',4);
        getRemote(line, serverIP, portNumStr,locStr);

        /*check address validity*/
        if(configLineCount == 1){/*First Line row, column*/
            matrixRow = atoi(serverIP);/*reuse code block, serverIP should contain row*/
            if (matrixRow ==0){
                printf("Invalid input, matrix row number must be non-zero numerical value.\nCheck you config file. Program will terminate...");
                exit(1);
            }
            matrixColumn = atoi(portNumStr);/*reuse code block, serverIP should contain column*/
            if (matrixColumn ==0){
                printf("Invalid input, matrix column number must be non-zero numerical value.\nCheck you config file. Program will terminate...");
                exit(1);
            }

        }else{/*rest of the file is host address and port, loc*/
            for (i = 0; i < strlen(serverIP); i ++){
                if(!isdigit(serverIP[i]) && serverIP[i]!='.'){
                    flawedProcedure = 1;
                    invalidLine = 1;
                    printf("\nWARNING:: The IP address contained in Line #%d (%s) is not valid.\n", configLineCount,serverIP);
                    break;
                }
            }

            /*Check port validity*/
            for (i = 0; i < strlen(portNumStr); i++){
                if (!isdigit(portNumStr[i])){
                    flawedProcedure = 1;
                    invalidLine = 1;
                    printf("\nWARNING:: The port number contained in Line #%d (%s) is not valid.\n", configLineCount, portNumStr);
                    break;
                }
            }

            for (i = 0; i < strlen(locStr); i++){
                if (!isdigit(locStr[i])){
                    flawedProcedure = 1;
                    invalidLine = 1;
                    printf("\nWARNING:: The loc number contained in Line #%d (%s) is not valid.\n", configLineCount, locStr);
                    break;
                }
            }
            
            if (strcmp(portNumStr, argv[1]) == 0){/*check if is local host*/
                strcpy(currentLoc, locStr);
                foundHostInfo = 1;
            }
            /*setup server address*/
            else if(!invalidLine){/*if the line is valid*/
                portNum = strtol(portNumStr,NULL,10);
                serverAddress.sin_family = AF_INET;
                serverAddress.sin_port = htons(portNum);
                serverAddress.sin_addr.s_addr = inet_addr(serverIP);

                /*Store in linked list*/
                currentHost = (struct remoteHost*) malloc(sizeof(struct remoteHost));
        
                currentHost->serverAddr = serverAddress;

                currentHost->loc = atoi(locStr);

                currentHost->remotePort = portNum;

                currentHost->msgSeq = 0;
                    
                currentHost->nextHost = head;

                head = currentHost;
                printf("Loaded remote host: %s:%s at location %d\n", serverIP, portNumStr,currentHost->loc);
                totalHost++;
            }
            
            memset(line,'\0',256);
        }
        
    }
    
    /*all data collected, close config file*/
    fclose(configFile);
    printf("Loaded %d remote host in total\n\n",totalHost);

    if(!foundHostInfo){/*if provided port was not included in config file*/
        while ( getchar() != '\n');/*remove excess input remain in buffer*/
        printf("\nWARNING:: The host info was not found in config file. Do you wish to continue?\n");
        printf("\nNOTICE:: If you choose to continue, this drone will be given a location at 0,\nhence it will not be able to send any information, nor will it receive any.\n\n");
        printf("Please enter 'y' to continue, and 'n' to terminate the program(y/n): ");

        userDecision = getchar();
        if(userDecision != '\n'){
            while ( getchar() != '\n');
        }
        while(userDecision != 'y' && userDecision != 'n'){/*rule out invalid input*/
            printf("Invalid input\n");
            printf("please enter 'y' to continue or 'n' to terminate: ");
            userDecision = getchar();
            if(userDecision != '\n'){
                while ( getchar() != '\n');
            }
        }

        if (userDecision == 'n'){/*If the user choose to exit, the program will terminate.*/
            printf("The program will terminate...\n");
            return(0);
        }else{/*If user choose to proceed, loc 0 will be assigned to the host*/
            printf("The program will proceed.\n\n");
            currentLoc[0] = '0';
            currentLoc[1] = '\0';
        }
    }
    if (atoi(currentLoc) > matrixColumn * matrixRow || atoi(currentLoc) <= 0){
        printf("WARNING:: The host loc is out of bound of the matrix. \n(IE: it will not be able to receive or send from other drones.)\n\n");
    }

    currentLocDigit = atoi(currentLoc);

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(strtol(argv[1],NULL,10));
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    /*bind socket with address*/
    returnCode = bind(newSocket, (struct sockaddr *) & serverAddress,sizeof(serverAddress));

    printf("Listening port at %s, current location at %s(x = %d, y = %d) on a %d by %d matrix.\n", argv[1], currentLoc, (currentLocDigit-1)%matrixColumn +1,(currentLocDigit-1)/matrixColumn+1 ,matrixColumn, matrixRow);
    

    if (returnCode<0){/*check return code for binding process*/
        perror ("bind");
        exit(1);    
    }
    while ( getchar() != '\n');
    memset(msg,'\0',131);
    for(;;){
        fflush(stdout);
        memset(buffer,'\0',227);/*reset buffer*/
        FD_ZERO(&socketFDS);
        FD_SET(newSocket,&socketFDS);
        FD_SET(STDIN,&socketFDS);
        if (STDIN > newSocket)
            maxSD = STDIN;
        else
            maxSD = newSocket;

        returnCode = select (maxSD+1, &socketFDS, NULL,NULL,NULL);
        printf("\nselect popped\n");
        if(FD_ISSET(STDIN,&socketFDS)){
            scanf("%[^\n]%*c", msg);

            if (strlen(msg)>130){/*check input size*/
                printf("Your input exceeded buffer size, try again.\n");
                /*while ( getchar() != '\n');*/
            }else if(strlen(msg) == 0){
                printf("Your input is empty, nothing is sent.\n");
                while ( getchar() != '\n');
            }else{
                if(strcmp(msg, "STOP")){/*if no STOP signal, send*/
                    sscanf(msg, "%s", portNumStr);
                    if(portNumStr[0] != '\n'){
                        portNum = atoi(portNumStr);
                        if(portNum){
                            sscanf(msg,"%s %[^\n]%*c", portNumStr, msg);
                            outletMsgSeq = findRemotePortID(portNum);
                            if(outletMsgSeq != -1){
                                sprintf(buffer, "%c %s %d %s %c MSG %d %s %s", currentVersion,currentLoc, portNum, argv[1],HOPCOUNT, outletMsgSeq +1 ,argv[1],msg);/*msg into buffer*/
                                /*Current ouput <v> <loc> <to> <from> <hopcount> <msg>*/
                                /*Protocol change to <v> <loc> <to> <from> <hopcount> <msg_type> <msg_id> <route> <msg>*/

                                /*call helper to send message*/
                                sendMsg(buffer, newSocket, serverAddress);
                            }
                        }else{
                            printf("WARNING: port number must be integer\n");
                        }
                        /*while ( getchar() != '\n');*/
                    }else{
                        while ( getchar() != '\n');
                    }

                    
                }else{/*if STOP signal, shutdown*/
                    printf("\nShutting down client...\n");
                    close (newSocket);
                    return (0);
                }
                memset(msg,'\0',131);
            }
            
        }
        if (FD_ISSET (newSocket, &socketFDS)){
            memset(msg,'\0',131);
            returnCode = recvfrom(newSocket,buffer,sizeof(buffer),flag,(struct sockaddr *) &clientAddress, &clientLength);
            getRemote(buffer, recversion,recvLoc,recvMsg);
            recvLocDigit = atoi(recvLoc);
            if(buffer[0] == '1'){/*If old version, do as that version*/
                if(!(recvLocDigit > matrixColumn*matrixRow || recvLocDigit <=0 || checkOutOfRange(currentLocDigit,recvLocDigit))){
                    printf ("IN RANGE Receved %d bytes from %s, version: %s, at loc %s: '%s'\n", returnCode,inet_ntoa(clientAddress.sin_addr), recversion, recvLoc, recvMsg);
                }
            }else if (buffer[0] == '2'){
                processRecvdV2(buffer, recversion, &recvLocDigit, portNumStr, recvSourcePort, &recvHopCount, recvMsg);
                if(!checkOutOfRange(recvLocDigit, currentLocDigit)){/*if within range*/
                    if(strcmp(portNumStr,argv[1])){/*if message not to local host*/
                        if(recvHopCount > 0){/*if life step not met*/
                            
                            recvHopCount --;
                            memset(buffer,'\0',227);
                            sprintf(buffer,"%c %s %s %s %d %s",currentVersion, currentLoc, portNumStr, recvSourcePort, recvHopCount, recvMsg);
                            sendMsgV2(buffer, newSocket, serverAddress,recvSourcePort);
                            printf("forwarded a V2 Message\n");
                        }
                    }else{
                        printf("IN RANGE Received %d bytes from %s, version %s, at loc %d: '%s'\n",returnCode, inet_ntoa(clientAddress.sin_addr), recversion, recvLocDigit, recvMsg);
                    }
                }else{
                    printf("Out of range msg Ignored\n");
                }
            }else if(buffer[0] == '3'|| buffer[0] =='4'){
                processRecvdV3(buffer, recversion, &recvLocDigit, portNumStr, recvSourcePort, &recvHopCount, remoteMSGType, &remoteMsgID, remoteRoute,recvMsg);
                if(!strcmp(remoteMSGType, "MOV")){/*unless MOV, otherwise a distance should be checked*/
                    if(!strcmp(portNumStr,argv[1])){/*if message is to local host*/
                        strcpy(currentLoc, recvMsg);
                        currentLocDigit = atoi(recvMsg);
                        printf("Instructed to mov to location at %d\n", currentLocDigit);
                    }else{
                        /*Should update this mov in record*/
                        printf("Overheard instruction to %s to move to loc at %s", portNumStr, recvMsg);
                    }
                }else if(!checkOutOfRange(recvLocDigit, currentLocDigit)){/*if within range*/
                    if(strcmp(portNumStr,argv[1])){/*if message not to local host, proceed forward*/
                        if(recvHopCount > 0){/*if life step not met*/
                            recvHopCount --;
                            memset(buffer,'\0',227);
                            sprintf(buffer, "%s %d %s %s %d %s %d %s,%s %s", recversion,currentLocDigit, portNumStr, recvSourcePort,recvHopCount, remoteMSGType, remoteMsgID, argv[1],remoteRoute,recvMsg);/*msg into buffer*/
                            /*<v> <loc> <to> <from> <hopcount> <msg_type> <msg_id> <route> <msg>*/
                            sendMsgV2(buffer, newSocket, serverAddress,recvSourcePort);
                            printf("forwarded a V3 Message\n");
                        }
                    }else{
                        if(remoteMsgID > findRemotePortID(atoi(recvSourcePort))){/*1st time msg or 1st recvd ack*/
                            updateSeqID(atoi(recvSourcePort), remoteMsgID);
                            
                            if(!strcmp(remoteMSGType, "MSG")){/*IF message is MSG type, do ACK*/
                            
                                sprintf(buffer, "%c %d %s %s %d %s %d %s %s", currentVersion, currentLocDigit, recvSourcePort, argv[1],HOPCOUNT, "ACK", findRemotePortID(atoi(recvSourcePort)), argv[1] ,"==ACK MSG==");/*msg into buffer*/
                                printf("IN RANGE Received %d bytes from %s:%s, version %s, upsteam loc %d: '%s'\n",returnCode, inet_ntoa(clientAddress.sin_addr), portNumStr,recversion, recvLocDigit, recvMsg);
                                printf("Confirmation ACK sent to %s.\n", recvSourcePort);
                                sendMsgNoPrint(buffer, newSocket, serverAddress);
                            }else if (!strcmp(remoteMSGType, "ACK")){/*if ACK, hide and confirm received*/
                                printf("Msg ID '%d' to Port %s was confirmed received.\n", remoteMsgID, portNumStr);
                            }
                        }else{
                            printf("Ignored duplicated msg from Port %s via loc %d, seq ID %d\n", recvSourcePort, recvLocDigit, remoteMsgID);
                        }

                        
                    }
                }
            }else{
                printf("Unknown version number %c\n", buffer[0]);
            }
        }
    }
    return 0;
}
/*This function return 1 if the remote loc is out of range, 0 otherwise*/
int checkOutOfRange(int localLoc, int remoteLoc){
    double x1 = (localLoc-1)%matrixColumn +1;
    double y1 = (localLoc-1)/matrixColumn +1;
    double x2 = (remoteLoc-1)%matrixColumn +1;
    double y2 = (remoteLoc-1)/matrixColumn +1;
    return (int)(sqrt(pow(x1-x2,2) + pow(y1-y2,2))) > 2;
}

void processRecvdV2(char* buffer, char* version, int *loc, char* dest, char* source, int *hopCount, char* msg){
    sscanf(buffer, "%s %d %s %s %d %[^\n]%*c", version, loc, dest, source, hopCount, msg);
}

void processRecvdV3(char* buffer, char* version, int *loc, char* dest, char* source, int *hopCount, char* msgType, int* msgID, char* remoteRoute, char* msg){
    sscanf(buffer, "%s %d %s %s %d %s %d %s %[^\n]%*c", version, loc, dest, source, hopCount, msgType, msgID, remoteRoute, msg);
}

/*This function extract a string of IP and portprocessRecvdV2 into two separate strings*/
void getRemote(char *line, char *remoteIP, char* remotePort, char* location){
    sscanf(line, "%s %s %s", remoteIP, remotePort, location);

}

/*this function handles the actual send process*/
int sendMsg(char *buffer, int newSocket, struct sockaddr_in serverAddress){
    currentHost = head;
    while (currentHost != NULL){/*loop until a NULL node is found*/
        /*setup return code*/
        int returnCode = 0;
        /*send msg*/
        returnCode = sendto(newSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&(currentHost->serverAddr), sizeof(serverAddress));
        
        printf("sent %d bytes (%s) to %s:%d at location '%d'\n", returnCode, buffer, inet_ntoa(currentHost->serverAddr.sin_addr), ntohs(currentHost->serverAddr.sin_port),currentHost->loc);
        currentHost = currentHost->nextHost;/*update pointer*/
    }
    return (0);
}

/*this function handles the actual send process but do not print details*/
int sendMsgNoPrint(char *buffer, int newSocket, struct sockaddr_in serverAddress){
    currentHost = head;
    while (currentHost != NULL){/*loop until a NULL node is found*/
        /*setup return code*/
        int returnCode = 0;
        /*send msg*/
        returnCode = sendto(newSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&(currentHost->serverAddr), sizeof(serverAddress));

        currentHost = currentHost->nextHost;/*update pointer*/
    }
    return (0);
}

int findRemotePortID(int portNum){
    currentHost = head;
    while(currentHost!= NULL){
        if(currentHost->remotePort != portNum){
            currentHost = currentHost->nextHost;
        }else{
            return currentHost->msgSeq;
        }
    }
    return -1;
}

void updateSeqID (int portNum, int newID){
    currentHost = head;
    while(currentHost!= NULL){
        if(currentHost->remotePort != portNum){
            currentHost = currentHost->nextHost;
        }else{
            currentHost->msgSeq = newID;
            break;
        }
    }
}


/*this function handles the actual send process, optimized to not send to the upstream sender*/
int sendMsgV2(char *buffer, int newSocket, struct sockaddr_in serverAddress, char* recvdPort){
    currentHost = head;
    while (currentHost != NULL){/*loop until a NULL node is found*/
        /*setup return code*/
        int returnCode = 0;
        if(ntohs(currentHost->serverAddr.sin_port) != atoi(recvdPort) ){
            /*send msg*/
            returnCode = sendto(newSocket, buffer, strlen(buffer), 0, (struct sockaddr *)&(currentHost->serverAddr), sizeof(serverAddress));
            
            printf("sent %d bytes (%s) to %s:%d at location '%d'\n", returnCode, buffer, inet_ntoa(currentHost->serverAddr.sin_addr), ntohs(currentHost->serverAddr.sin_port),currentHost->loc);
            
        }
        currentHost = currentHost->nextHost;/*update pointer*/
    }
    return (0);
}