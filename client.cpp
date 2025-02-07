//
// Simple chat client for TSAM-409
//
// Command line: ./chat_client 4000
//
// Author: Jacky Mallett (jacky@ru.is)
//
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <vector>
#include <thread>

#include <iostream>
#include <sstream>
#include <thread>
#include <map>

// Threaded function for handling responss from server

void listenServer(int serverSocket)
{
    int nread;         // Bytes read from socket
    char buffer[1025]; // Buffer for reading input

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        nread = read(serverSocket, buffer, sizeof(buffer));

        if (nread == 0) // Server has dropped us
        {
            printf("Over and Out\n");
            exit(0);
        }
        else if (nread > 0)
        {
            printf("%s\n", buffer);
        }
    }
}

int main(int argc, char *argv[])
{
    struct addrinfo hints, *svr;  // Network host entry for server
    struct sockaddr_in serv_addr; // Socket address for server
    int serverSocket;             // Socket used for server
    int nwrite;                   // No. bytes written to server
    char buffer[1025];            // buffer for writing to server
    bool finished;
    int set = 1; // Toggle for setsockopt

    if (argc != 3)
    {
        printf("Usage: chat_client <ip  port>\n");
        printf("Ctrl-C to terminate\n");
        exit(0);
    }

    hints.ai_family = AF_INET; // IPv4 only addresses
    hints.ai_socktype = SOCK_STREAM;

    memset(&hints, 0, sizeof(hints));

    if (getaddrinfo(argv[1], argv[2], &hints, &svr) != 0)
    {
        perror("getaddrinfo failed: ");
        exit(0);
    }

    struct hostent *server;
    server = gethostbyname(argv[1]);

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(atoi(argv[2]));

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after
    // program exit.

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        printf("Failed to set SO_REUSEADDR for port %s\n", argv[2]);
        perror("setsockopt failed: ");
    }

    if (connect(serverSocket, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("Failed to open socket to server: %s\n", argv[1]);
        perror("Connect failed: ");
        exit(0);
    }

    // Listen and print replies from server
    std::thread serverThread(listenServer, serverSocket);

    finished = false;
    // printf("CONNECT <name>        : Connect to server as <name>\n");
    // printf("LEAVE                 : Close connection to client\n");
    // printf("WHO                   : Show connections to server\n");
    // printf("MSG <name> <message>  : Send message to name\n");
    // //printf("MSG ALL <message>     : Send message to all connected\n");
    // printf("SC <IPaddress> <port> : Connect to server through the server\n");
    // printf("LIST <CLIENTS>/<SERVERS>: Lists connected clients/servers\n");
    printf("====================CLIENT COMMAND=======================\n");
    printf("QC <int, int>             Connect to 127.0.0.0 port 100??\n");
    printf("SC <IPaddress> <port>: Connect to serv through the server\n");
    printf("SENDMSG <GROUPID>     Send a message to a specific server\n");
    printf("GETMSG <GROUPID>        Get all msg from a specfic server\n");
    printf("LISTSERVERS                   Lists all connected servers\n");
    printf("====================SERVER COMMAND=======================\n");
    // printf("CONNECT <GROUPID,PORT,IP>        TODO, take a closer look\n");
    printf("LISTSERVERS <GROUPID> Servers conneced to some other serv\n");
    printf("=========================================================\n");
    while (!finished)
    {
        bzero(buffer, sizeof(buffer));

        fgets(buffer, sizeof(buffer), stdin);

        nwrite = send(serverSocket, buffer, strlen(buffer), 0);

        if (nwrite == -1)
        {
            perror("send() to server failed: ");
            finished = true;
        }
    }
}
