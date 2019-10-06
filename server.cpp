//
// Simple chat server for TSAM-409
//
// Command line: ./chat_server 4000
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

#include <iostream>
#include <sstream>
#include <thread>
#include <map>

#include <unistd.h>

// fix SOCK_NONBLOCK for OSX
// #ifndef SOCK_NONBLOCK
// #include <fcntl.h>
// #define SOCK_NONBLOCK O_NONBLOCK
// #endif

//Added includes
#include <ifaddrs.h>
#include <net/if.h>

#define BACKLOG 5 // Allowed length of queue of waiting connections
int maxfds;       // Passed to select() as max fd in set

// Simple class for handling connections from clients.
//
// Client(int socket) - socket to send/receive traffic from client.
class Client
{
public:
    int sock;         // socket of client connection
    std::string name; // Limit length of name of client's user

    Client(int socket) : sock(socket) {}

    ~Client() {} // Virtual destructor defined for base class
};

class Server
{
public:
    int sock;            // socket of client connection
    std::string groupID; // Group ID of the server
    std::string IP;      // IP address of client*/
    std::string port;    // Port of the server

    Server(int socket) : sock(socket) {}

    ~Server() {} // Virtual destructor defined for base class
};
std::map<int, Client *> clients; // Lookup table for per Client information
std::map<int, Server *> servers; // Lookup table for per Server information
// Open socket for specified port.
//
// Returns -1 if unable to create the socket for any reason.
int sendCommand(int clientSocket, std::string msg)
{
    int n = msg.length();

    char buffer[n + 2];
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, msg.c_str());
    memmove(buffer + 1, buffer, sizeof(buffer));
    buffer[0] = 0x01;
    buffer[n + 1] = 0x04;
    std::cout << "sending message\n";
    //if unable to send it will return -1
    return send(clientSocket, buffer, sizeof(buffer), 0);
}
//Added, based on main's client.cpp
void ConnectionToServers(std::string stringIpAddress, std::string stringPort, int clientSocket, fd_set *openSocekts)
{
    struct addrinfo hints, *svr;    // Network host entry for server
    struct sockaddr_in server_addr; // Socket address for server
    int serverSocket;               // Socket used for server
    int nwrite;                     // No. bytes written to server
    char buffer[1025];              // buffer for writing to server
    bool finished;
    int set = 1; // Toggle for setsockopt

    hints.ai_family = AF_INET; // IPv4 only addresses
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    memset(&hints, 0, sizeof(hints));

    const char *ipAddress = stringIpAddress.c_str();
    const char *port = stringPort.c_str();

    if (getaddrinfo(ipAddress, port, &hints, &svr) != 0)
    {
        perror("getaddrinfo failed: ");
        exit(0);
    }

    serverSocket = socket(svr->ai_family, svr->ai_socktype, svr->ai_protocol);

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after
    // program exit.

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        printf("Failed to set SO_REUSEADDR for port %s\n", port);
        perror("setsockopt failed: ");
    }

    if (connect(serverSocket, svr->ai_addr, svr->ai_addrlen) < 0)
    {
        printf("From the client within: Failed to open socket to server: %s\n", ipAddress);
        perror("Connect failed: ");
        printf("Please check that the server is running and try again");
    }

    Server *nServer = new Server(serverSocket);
    nServer->IP = ipAddress;
    nServer->port = port;
    servers.emplace(serverSocket, nServer);
    std::string msg;
    msg = "GROUP 20";
    FD_SET(serverSocket, openSocekts);
    // And update the maximum file descriptor
    maxfds = std::max(maxfds, serverSocket);
    nwrite = sendCommand(serverSocket, msg);
    if (nwrite == -1)
    {
        perror("send() to server failed: ");
        //finished = true;
    }
}
int open_socket(int portno)
{
    struct sockaddr_in sk_addr; // address settings for bind()
    int sock;                   // socket opened for this port
    int set = 1;                // for setsockopt

    // Create socket for connection. Set to be non-blocking, so recv will
    // return immediately if there isn't anything waiting to be read.

    if ((sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
    {
        perror("Failed to open socket");
        return (-1);
    }

    // Turn on SO_REUSEADDR to allow socket to be quickly reused after
    // program exit.

    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        perror("Failed to set SO_REUSEADDR:");
    }
    memset(&sk_addr, 0, sizeof(sk_addr));

    sk_addr.sin_family = AF_INET;
    sk_addr.sin_addr.s_addr = INADDR_ANY;
    sk_addr.sin_port = htons(portno);

    // Bind to socket to listen for connections from clients

    if (bind(sock, (sockaddr *)&sk_addr, sizeof(sk_addr)) < 0)
    {
        perror("Failed to bind to socket:");
        return (-1);
    }
    else
    {
        return (sock);
    }
}
// Close a client's connection, remove it from the client list, and
// tidy up select sockets afterwards.
void closeClient(int clientSocket, fd_set *openSockets, int *maxfds)
{
    // Remove client from the clients list
    clients.erase(clientSocket);
    // If this client's socket is maxfds then the next lowest
    // one has to be determined. Socket fd's can be reused by the Kernel,
    // so there aren't any nice ways to do this.
    if (*maxfds == clientSocket)
    {
        for (auto const &p : clients)
        {
            *maxfds = std::max(*maxfds, p.second->sock);
        }
    }
    // And remove from the list of open sockets.
    FD_CLR(clientSocket, openSockets);
}
///Hope this works. Please check if this makes sense
void closeServer(int serverSocket, fd_set *openSockets, int *maxfds)
{
    // Remove Server from the Server list
    clients.erase(serverSocket);
    // If this client's socket is maxfds then the next lowest
    // one has to be determined. Socket fd's can be reused by the Kernel,
    // so there aren't any nice ways to do this.
    if (*maxfds == serverSocket)
    {
        for (auto const &p : servers)
        {
            *maxfds = std::max(*maxfds, p.second->sock);
        }
    }
    // And remove from the list of open sockets.
    FD_CLR(serverSocket, openSockets);
}
void listClients()
{
    std::cout << "Listing servers: " << std::endl;
    for (auto const &x : clients)
    {
        std::cout << "Key: "
                  << x.first // string (key)
                  << ", Name: "
                  << x.second->name // string's value
                  << ", Socket: "
                  << x.second->sock // string's value
                  << std::endl;
    }
}
void listServers()
{
    std::cout << "Listing servers connected to this one: " << std::endl;
    for (auto const &x : servers)
    {
        std::cout << "Key: "
                  << x.first // string (key)
                  << ", groupID: "
                  << x.second->groupID // string's value
                  << ", IP: "
                  << x.second->IP // string's value
                  << ", Port: "
                  << x.second->port // string's value
                  << ", Socket: "
                  << x.second->sock // string's value
                  << std::endl;
    }
}
// Process command from client on the server
void clientCommand(int clientSocket, fd_set *openSockets, int *maxfds,
                   char *buffer)
{
    std::vector<std::string> tokens;
    std::string token;

    // Split command from client into tokens for parsing
    std::stringstream stream(buffer);

    while (stream >> token)
        tokens.push_back(token);

    if ((tokens[0].compare("CONNECT") == 0) && (tokens.size() == 2))
    {
        clients[clientSocket]->name = tokens[1];
    }
    else if (tokens[0].compare("LEAVE") == 0)
    {
        // Close the socket, and leave the socket handling
        // code to deal with tidying up clients etc. when
        // select() detects the OS has torn down the connection.

        closeClient(clientSocket, openSockets, maxfds);
    }
    else if (tokens[0].compare("WHO") == 0)
    {
        std::cout << "Who is logged on" << std::endl;
        std::string msg;

        for (auto const &names : clients)
        {
            msg += names.second->name + ",";
        }
        // Reducing the msg length by 1 loses the excess "," - which
        // granted is totally cheating.
        send(clientSocket, msg.c_str(), msg.length() - 1, 0);
    }
    // This is slightly fragile, since it's relying on the order
    // of evaluation of the if statement.
    else if ((tokens[0].compare("MSG") == 0) && (tokens[1].compare("ALL") == 0))
    {
        std::string msg;
        for (auto i = tokens.begin() + 2; i != tokens.end(); i++)
        {
            msg += *i + " ";
        }

        for (auto const &pair : clients)
        {
            send(pair.second->sock, msg.c_str(), msg.length(), 0);
        }
    }
    else if (tokens[0].compare("MSG") == 0)
    {
        for (auto const &pair : clients)
        {
            if (pair.second->name.compare(tokens[1]) == 0)
            {
                std::string msg;
                for (auto i = tokens.begin() + 2; i != tokens.end(); i++)
                {
                    msg += *i + " ";
                }
                send(pair.second->sock, msg.c_str(), msg.length(), 0);
            }
        }
    }
    else if (tokens[0].compare("SC") == 0)
    {
        std::string ipAddress = tokens[1];
        std::string port = tokens[2];
        ConnectionToServers(ipAddress, port, clientSocket, openSockets);
    }
    else if (tokens[0].compare("SM") == 0)
    {
        std::cout << "Comes to SM, the buffer is " << buffer << std::endl;

        //send(, buffer, sizeof(buffer), 0);
    }
    else if (tokens[0].compare("GROUP") == 0)
    {
        //Made so we dont get "Unknown command from client:" each time
        //we connect
    }
    else if (tokens[0].compare("LIST") == 0)
    {
        if (tokens[1].compare("SERVERS") == 0)
        {
            listServers();
        }
        else if (tokens[1].compare("CLIENTS") == 0)
        {
            listClients();
        }
    }
    else
    {
        std::cout << "Unknown command from client:" << buffer << std::endl;
    }
}

int main(int argc, char *argv[])
{
    bool finished;
    int listenCSock; // Socket for connections to server
    int listenSSock;
    int clientSock;       // Socket of connecting client
    int serverSock;       // Socket of connecting client
    fd_set openSockets;   // Current open sockets
    fd_set readSockets;   // Socket list for select()
    fd_set exceptSockets; // Exception socket list

    struct sockaddr_in client;
    socklen_t clientLen;

    struct sockaddr_in server;
    socklen_t serverLen;
    char buffer[1025]; // buffer for reading from clients

    if (argc != 2)
    {
        printf("Usage: chat_server <ip port>\n");
        exit(0);
    }
    // Setup socket for server to listen to
    listenCSock = open_socket(atoi(argv[1]));
    listenSSock = open_socket(atoi(argv[1]) + 1);
    printf("Listening for clients on port: %d\n", atoi(argv[1]));
    printf("Listening for servers on port: %d\n", (atoi(argv[1]) + 1));

    if (listen(listenCSock, BACKLOG) < 0)
    {
        printf("Listen failed on client port %s\n", (argv[1]));
        exit(0);
    }
    else if (listen(listenSSock, BACKLOG) < 0)
    {
        printf("Listen failed on server port %s\n", ((argv[1]) + 1));
        exit(0);
    }
    else
    // Add listen socket to socket set we are monitoring
    {
        FD_ZERO(&openSockets);
        FD_SET(listenCSock, &openSockets);
        FD_SET(listenSSock, &openSockets);
        maxfds = std::max(listenCSock, listenSSock);
    }
    finished = false;
    while (!finished)
    {
        // Get modifiable copy of readSockets
        readSockets = exceptSockets = openSockets;
        memset(buffer, 0, sizeof(buffer));
        // Look at sockets and see which ones have something to be read()
        int n = select(maxfds + 1, &readSockets, NULL, &exceptSockets, NULL);

        if (n < 0)
        {
            perror("select failed - closing down\n");
            finished = true;
        }
        else
        {
            // First, accept  any new connections to the server on the listening socket
            if (FD_ISSET(listenCSock, &readSockets))
            {
                clientSock = accept(listenCSock, (struct sockaddr *)&client,
                                    &clientLen);
                printf("accept on client socket***\n");
                // Add new client to the list of open sockets
                FD_SET(clientSock, &openSockets);

                // And update the maximum file descriptor
                maxfds = std::max(maxfds, clientSock);

                // create a new client to store information.
                clients[clientSock] = new Client(clientSock);

                // Decrement the number of sockets waiting to be dealt with
                n--;

                printf("Client connected on server: %d\n", clientSock);
            }
            if (FD_ISSET(listenSSock, &readSockets))
            {
                serverSock = accept(listenSSock, (struct sockaddr *)&server,
                                    &serverLen);
                printf("accept on server socket***\n");
                // Add new server to the list of open sockets
                FD_SET(serverSock, &openSockets);

                // And update the maximum file descriptor  //Pæla þarf ég að tékka á öll??
                maxfds = std::max(maxfds, serverSock);

                // create a new server to store information.
                servers[serverSock] = new Server(serverSock);

                // Decrement the number of sockets waiting to be dealt with
                n--;

                printf("server connected on server: %d\n", serverSock);
            }
            // Now check for commands from clients and server
            while (n-- > 0)
            {
                for (auto const &pair : clients)
                {
                    Client *client = pair.second;

                    if (FD_ISSET(client->sock, &readSockets))
                    {
                        // recv() == 0 means client has closed connection
                        if (recv(client->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                        {
                            printf("Client closed connection: %d", client->sock);
                            close(client->sock);

                            closeClient(client->sock, &openSockets, &maxfds);
                        }
                        else
                        {
                            std::cout << "Client buffer: " << buffer << std::endl;
                            clientCommand(client->sock, &openSockets, &maxfds,
                                          buffer);
                        }
                    }
                }
                for (auto const &pair : servers)
                {
                    Server *server = pair.second;

                    if (FD_ISSET(server->sock, &readSockets))
                    {
                        // recv() == 0 means client has closed connection
                        if (recv(server->sock, buffer, sizeof(buffer), MSG_DONTWAIT) == 0)
                        {
                            printf("Server closed connection: %d", server->sock);
                            close(server->sock);

                            closeServer(server->sock, &openSockets, &maxfds);
                        }
                        std::cout << "\nServer buffer: " << buffer << std::endl;
                        //clientCommand(server->sock, &openSockets, &maxfds, buffer);
                        // We don't check for -1 (nothing received) because select()
                        // only triggers if there is something on the socket for us.
                    }
                }
            }
        }
    }
}
