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
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <dirent.h>

#define BACKLOG 5 // Allowed length of queue of waiting connections
int maxfds;       // Passed to select() as max fd in set
std::string serverName = "V_GROUP_20";
std::string serverPort;

// Help functions, are below main()
std::string viewFiles();
std::string constructCommand(std::string str);
std::string getTimeStamp();
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
    std::string temp(buffer, sizeof(buffer));
    // for (size_t i{1}; i <= temp.size(); ++i)
    // {
    //     std::cout << std::hex << (size_t)temp.at(i - 1) << ((i % 16 == 0) ? "\n" : " ");
    // }
    std::cout << "\nPadding finished, sending message\n";
    return send(clientSocket, buffer, sizeof(buffer), 0);
}

std::string removePadding(std::string msg)
{
    // std::cout << "Size of msg: " << msg.length() << std::endl;
    // std::cout << "The message in removePadding '" << msg << "' This is the HEX" << std::endl;
    // for (size_t i{1}; i <= msg.size(); ++i)
    // {
    //     std::cout << std::hex << (size_t)msg.at(i - 1) << ((i % 16 == 0) ? "\n" : " ");
    // }
    //int i;
    std::string removePadding;
    msg.erase((msg.length() - 1), 1);
    msg.erase(0, 1);
    removePadding = msg;
   
    //std::cout << "\nAfter removing the padding the HEX is \n";
    // for (size_t i{1}; i <= removePadding.size(); ++i)
    // {
    //     std::cout << std::hex << (size_t)removePadding.at(i - 1) << ((i % 16 == 0) ? "\n" : " ");
    // }
   // std::cout << "\nend of removePadding, returning message" << std::endl;
    //std::cout << removePadding << " HERE IS THE REMOVE PADDING ";
    //std::cout << "The size at the end is: " << removePadding.length() << std::endl;
    return msg;
}
std::string checkMessage(char *buffer)
{

    if (buffer[0] == 0x01)
    {
        std::string msg = buffer;
        std::cout << "\n now in checkMessage 'if (buffer[0] == 0x01)'" << std::endl;
        //std::cout << "\n now printing HEX in checkMessage " << std::endl;
        // for (size_t i{1}; i <= msg.size(); ++i)
        // {
        //     std::cout << std::hex << (size_t)msg.at(i - 1) << ((i % 16 == 0) ? "\n" : " ");
        // }
        // std::cout << std::endl;
        // bool finished = false;
        // while (!finished)
        // {
        //     if (buffer[sizeof(buffer) + 1] == 0x04)
        //     {
        //         finished = true;
        //     }
        // }
        if (msg[0] == 0x01 && msg[msg.length() - 1] == 0x04)
        {
            std::string msg = buffer;
            std::cout << "passed SOH and EOT check" << std::endl;
            std::string outcome;
            outcome = removePadding(msg);
            //char buffermsg[1025];
            //strcpy(buffermsg, msg.c_str());
            return outcome;
        }
        else
        {
            std::cout << "No EOT on the message, lets wait a bit" << std::endl;
            usleep(10000);
            if (msg[0] == 0x01 && msg[msg.length() - 1] == 0x04)
            {
                std::string msg(buffer, sizeof(buffer));
                std::cout << "passed SOH and EOT check" << std::endl;
                msg = removePadding(msg);
                char buffermsg[1025];
                strcpy(buffermsg, msg.c_str());
                return msg;
            }
            std::cout << "No EOT on the message" << std::endl;
            return buffer;
            //What to do?
        }
        std::cout << "no padding on this message" << std::endl;
    }
    else
    {
        std::cout << "No padding on this message" << std::endl;
        return buffer;
    }
}
int writeToFile(char *buffer)
{
    std::ofstream file;
    //Creates a new text file, in the future with each group name
    file.open("./data/GROUPNAME.txt", std::ios::in | std::ios::app | std::ios::out);
    file << getTimeStamp();
    file << buffer;
    file.close();
}
void readFromFile()
{
    std::string line;
    std::ifstream myfile("./data/TEST.txt");
    if (myfile.is_open())
    {
        while (getline(myfile, line))
        {
            std::cout << line << '\n';
        }
        myfile.close();
    }
    else
        std::cout << "Unable to open file";
}
std::string getnameinformation()
{
    struct ifaddrs *myaddrs, *ifa;
    void *in_addr;
    char buf[64];
    std::string ipInfo;
    std::ostringstream oss;
    if (getifaddrs(&myaddrs) != 0)
    {
        perror("getifaddrs");
        exit(1);
    }
    for (ifa = myaddrs; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;
        if (!(ifa->ifa_flags & IFF_UP))
            continue;

        switch (ifa->ifa_addr->sa_family)
        {
        case AF_INET:
        {
            struct sockaddr_in *s4 = (struct sockaddr_in *)ifa->ifa_addr;
            in_addr = &s4->sin_addr;
            break;
        }

        case AF_INET6:
        {
            struct sockaddr_in6 *s6 = (struct sockaddr_in6 *)ifa->ifa_addr;
            in_addr = &s6->sin6_addr;
            break;
        }

        default:
            continue;
        }

        if (!inet_ntop(ifa->ifa_addr->sa_family, in_addr, buf, sizeof(buf)))
        {
            printf("%s: inet_ntop failed!\n", ifa->ifa_name);
        }
        else
        {
            //printf("%s: %s\n", ifa->ifa_name, buf);
            oss << buf << " ";
        }
    }
    freeifaddrs(myaddrs);
    ipInfo = oss.str();
    return ipInfo;
}

std::string getIp()
{
    std::string str = getnameinformation();
    std::vector<std::string> ipTokens;
    std::string ipToken;
    std::stringstream stream(str);
    while (stream >> ipToken)
        ipTokens.push_back(ipToken);
    return ipTokens[1];
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
    nServer->groupID = "V_GROUP_0";
    servers.emplace(serverSocket, nServer);
    FD_SET(serverSocket, openSocekts);
    // And update the maximum file descriptor
    maxfds = std::max(maxfds, serverSocket);
    std::string hellomessage = "CONNECT," + serverName + "," + port + "," + ipAddress;
    nwrite = sendCommand(serverSocket, hellomessage);
    if (nwrite == -1)
    {
        perror("send() to server failed: ");
        //finished = true;
    }
}
int open_socket(int portno);
void closeClient(int clientSocket, fd_set *openSockets, int *maxfds);
void closeServer(int serverSocket, fd_set *openSockets, int *maxfds);
std::string listClients()
{
    std::string msg;
    if (clients.empty())
    {
        return msg = ("No clients registered on this server");
    }
    else
    {
        msg = ("Listing clients: ");
        for (auto const &x : clients)
        {
            std::ostringstream oss;
            oss << "\nKey: "
                << x.first
                << ", Name: "
                << x.second->name
                << ", Socket: "
                << x.second->sock;
            msg += oss.str();
        }
    }
    return msg;
}
std::string listServers()
{
    std::string msg;
    // if (servers.empty())
    // {
    //     return msg = ("No servers connected to this server");
    // }
    //else
    //{
       // msg = ("Listing servers connected to this one: ");
       msg = "SERVERS," + serverName + "," + "127.0.0.1" + "," + serverPort + ";";

       for (auto const &x : servers)
        {
            std::ostringstream oss;
            oss << x.second->groupID
                << ","
                << x.second->IP
                << ","
                << x.second->port
                << ";";
                
            msg += oss.str();
      //  }
        // for (auto const &x : servers)
        // {
        //     std::ostringstream oss;
        //     oss << "\nKey: "
        //         << x.first
        //         << ", groupID: "
        //         << x.second->groupID
        //         << ", IP: "
        //         << x.second->IP
        //         << ", Port: "
        //         << x.second->port
        //         << ", Socket: "
        //         << x.second->sock;
        //     msg += oss.str();
        // }
    }
    return msg;
}
// Process command from server on the server
void serverCommand(int serverSocket, fd_set *openSockets, int *maxfds,
                   char *buffer)
{
    std::vector<std::string> tokens;
    std::string token;

    std::string str(buffer);
    if (str.find(",") != std::string::npos)
    {
        std::cout << "Found , in the command" << std::endl;
        std::string temp;
        temp = constructCommand(str);
        str = temp;
    }

    // Split command from server into tokens for parsing
    std::stringstream stream(str);

    while (stream >> token)

        tokens.push_back(token);
    if (tokens[0].compare("CONNECT") == 0 && (tokens.size() == 4))
    {
        for (auto const &pair : servers)
        {
            if (pair.second->sock == serverSocket)
            {

                pair.second->groupID = tokens[1];
                pair.second->port = tokens[2];
                pair.second->IP = getIp();

                std::ostringstream oss;
                oss << pair.second->groupID << " "
                    << pair.second->port << " "
                    << pair.second->IP << " ";
                std::string temp = oss.str();
                std::cout << "the new connected server is " << temp << std::endl;
            }
        }
    }
    // This is slightly fragile, since it's relying on the order
    // of evaluation of the if statement.
    else if ((tokens[0].compare("MSG") == 0) && (tokens[1].compare("ALL") == 0))
    {
        if (!servers.empty())
        {

            std::string msg;
            for (auto i = tokens.begin() + 2; i != tokens.end(); i++)
            {
                msg += *i + " ";
            }

            for (auto const &pair : servers)
            {
                send(pair.second->sock, msg.c_str(), msg.length(), 0);
            }
        }
        else
        {
            std::cout << "There are no registered servers on this server" << std::endl;
        }
    }
    else if (tokens[0].compare("MSG") == 0)
    {
        if (!servers.empty())
        {
            for (auto const &pair : servers)
            {
                if (pair.second->groupID.compare(tokens[1]) == 0)
                {
                    std::string msg;
                    for (auto i = tokens.begin() + 2; i != tokens.end(); i++)
                    {
                        msg += *i + " ";
                    }
                    send(pair.second->sock, msg.c_str(), msg.length(), 0);
                }
            }
            std::cout << "There are no registered servers on this server" << std::endl;
        }
    }
    else if (tokens[0].compare("DIR") == 0)
    {
        std::string dircontent;
        dircontent = viewFiles();
        std::cout << dircontent << std::endl;
    }
    else if (tokens[0].compare("LISTSERVERS") == 0)
    {
        //Send to specific groupID
        std::string msg;
        msg = listServers();
        for (auto const &pair : servers)
        {
            sendCommand(pair.second->sock, msg);
        }
    }
    else if (tokens[0].compare("LEAVE") == 0)
    {
        if (!servers.empty())
        {
            for (auto const &pair : servers)
            {
                if ((pair.second->IP.compare(tokens[1]) && pair.second->port.compare(tokens[2])) == 0)
                {
                    close(pair.second->sock);
                }
            }
            std::cout << "LEAVE: There are no registered servers on this server" << std::endl;
        }
    }
    else if(tokens[0].compare("KEEPALIVE") == 0)
    {
        //DO command keepalive, KEEPALIVE,<# of Messages>
    }
    else if(tokens[0].compare("GET_MSG") == 0)
    {
        //DO command GET MSG,<GROUP_ID>
    }
    else if(tokens[0].compare("SEND_MSG") == 0)
    {
        std::cout << "Message from group: " << tokens[1] << std::endl;
        for (auto i = tokens.begin() + 3; i != tokens.end(); i++)
         {
             std::cout << *i << " ";
                            
        }
        std::cout.flush();
    }
    else if(tokens[0].compare("STATUSREQ") == 0)
    {
        //DO command STATUSREQ,FROM GROUP
    }
    else if(tokens[0].compare("STATUSRESP") == 0)
    {
        //DO command STATUSREQ,FROM GROUP
    }
    else
    {
        std::cout << "Unknown command from server:" << buffer << std::endl;
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
    
    if(tokens[0].compare("SM") == 0)
    {
        if(tokens[1].compare("LISTSERVERS") == 0)
        {
            if (!servers.empty())
            {
                std::cout << "Sending message to all connected servers " << std::endl;
                std::string msg = tokens[1];
                //  std::cout << "Before " << msg << std::endl;
                msg += "," + serverName;
                // std::cout << "Client LISTSERVERS: " << listServers() << std::endl;
                // std::cout << "Sending message to all connected servers " << msg << std::endl;
                for (auto const &pair : servers)
                {
                    sendCommand(pair.second->sock, msg);
                }
            }
            else
            {
                std::cout << "LISTSERVERS: There are servers connected to this server to recive this message" << std::endl;
            }
        }
        else if(tokens[1].compare("KEEPALIVE") == 0)
        {
            //DO command keepalive, KEEPALIVE,<# of Messages>
        }
        else if(tokens[1].compare("GET_MSG") == 0)
        {
            //DO command GET MSG,<GROUP_ID>
        }
        else if(tokens[1].compare("SEND_MSG") == 0)
        {
            std::string msg;
            std::cout << "Sending message to group: ";
             for (auto const &pair : servers)
            {
                if (pair.second->groupID.compare(tokens[2]) == 0)
                { 
                    msg ="SEND_MSG," + serverName + ",";
                    std::cout << "found server to send";
                        for (auto i = tokens.begin() + 2; i != tokens.end(); i++)
                        {
                            msg += *i;
                            msg += ",";
                        }
                        std::cout << msg;
                    }
                    sendCommand(pair.second->sock,msg);
            }
        }
        else if (tokens[0].compare("LEAVE") == 0)
        {
            //LEAVE,SERVER IP,PORT
        }
        else if(tokens[1].compare("STATUSREQ") == 0)
        {
            //DO command STATUSREQ,FROM GROUP
        }
        else if(tokens[1].compare("STATUSRESP") == 0)
        {
            //DO command STATUSREQ,FROM GROUP
        }
        else
        {
            std::cout << "Unknown SM command from client:" << buffer << std::endl;
        }
    }
    else
    {
        if (tokens[0].compare("LEAVE") == 0)
        {
            if (clients.empty())
            {
                std::cout << "There are no registered clients on this server" << std::endl;
            }
        
            else
            {
            // Close the socket, and leave the socket handling
            // code to deal with tidying up clients etc. when
            // select() detects the OS has torn down the connection.

                closeClient(clientSocket, openSockets, maxfds);
            }
        }
        else if (tokens[0].compare("WHO") == 0)
        {
            std::cout << "Who is logged on" << std::endl;
            std::string msg;
            if (!clients.empty())
            {
                for (auto const &names : clients)
                {
                    msg += names.second->name + ",";
                }
            // Reducing the msg length by 1 loses the excess "," - which
            // granted is totally cheating.
                send(clientSocket, msg.c_str(), msg.length() - 1, 0);
             }
         }

        else if (tokens[0].compare("MSG") == 0)
        {
            if (!clients.empty())
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
            std::cout << "There are no registered clients on this server" << std::endl;
            }
        }

        else if (tokens[0].compare("SC") == 0)
        {
            std::string ipAddress = tokens[1];
            std::string port = tokens[2];
            ConnectionToServers(ipAddress, port, clientSocket, openSockets);
        }
        else if (tokens[0].compare("DIR") == 0)
        {
            std::string dircontent;
            dircontent = viewFiles();
            std::cout << dircontent << std::endl;
        }
        else if (tokens[0].compare("QC") == 0) //Quick connect to 127.0.0.1 10003
        {
            ConnectionToServers("127.0.0.1", "10003", clientSocket, openSockets);
        }
        else if (tokens[0].compare("LISTCLIENTS") == 0)
        {
            for (auto const &pair : clients)
            {
                std::string msg;
                msg = listClients();
                //send(pair.second->sock, msg.c_str(), msg.length(), 0);
                sendCommand(pair.second->sock, msg);
            }
        }
        else if (tokens[0].compare("READ") == 0)
        {
            readFromFile();
        }
        else if(tokens[0].compare("GET") == 0)
        {
            //DO command GETMSG, GROUP ID Get a single message from the server for the GROUP ID
        }
        else if(tokens[0].compare("SEND") == 0)
        {
            //DO command SENDMSG, GROUP ID Send a message to the server for the GROUP ID
        }
        else if(tokens[0].compare("LISTSERVERS") == 0)
        {
            std::string msg;
            msg = listServers();
            for (auto const &pair : clients)
            {
               sendCommand(pair.second->sock, msg);
            }
        }
        else
        {
            std::cout << "Unknown command from client:" << buffer << std::endl;
        }
    }
}
/*===========================MAIN============================*/
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
    serverPort = argv[1];

    if (argc != 2)
    {
        printf("Usage: chat_server <ip port>\n");
        exit(0);
    }
    // Setup socket for server to listen to
    serverPort = argv[1];
    listenSSock = open_socket(atoi(argv[1]));
    listenCSock = open_socket(atoi(argv[1]) + 1);
    printf("Listening for servers on port: %d\n", atoi(argv[1]));
    printf("Listening for clients on port: %d\n", (atoi(argv[1]) + 1));

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
                        // std::string temp1 = buffer;
                        // std::cout << " now printing HEX after buffer " << std::endl;
                        // for (size_t i{1}; i <= temp1.size(); ++i)
                        // {
                        //     std::cout << std::hex << (size_t)temp1.at(i - 1) << ((i % 16 == 0) ? "\n" : " ");
                        // }
                        std::string temp = checkMessage(buffer);
                        // std::cout << "\nTemp : " << temp << " now printing HEX" << std::endl;
                        // for (size_t i{1}; i <= temp.size(); ++i)
                        // {
                        //     std::cout << std::hex << (size_t)temp.at(i - 1) << ((i % 16 == 0) ? "\n" : " ");
                        // }
                        std::cout << "Finishd checkMessage" << std::endl;
                        strcpy(buffer, temp.c_str());
                        //writeToFile(buffer);
                        serverCommand(server->sock, &openSockets, &maxfds, buffer);

                        // We don't check for -1 (nothing received) because select()
                        // only triggers if there is something on the socket for us.
                    }
                }
            }
        }
    }
}
std::string viewFiles()
{
    std::string filesInDir;
    struct dirent *de; // Pointer for directory entry
    // opendir() returns a pointer of DIR type.
    DIR *dr = opendir("./data");
    if (dr == NULL) // opendir returns NULL if couldn't open directory
    {
        filesInDir = ("Could not open current directory");
    }
    while ((de = readdir(dr)) != NULL)
    {
        //std::cout << de->d_name << std::endl;
        //files << (de->d_name + '\n');
        filesInDir = filesInDir + de->d_name + ' ';
    }
    closedir(dr);
    return filesInDir;
}
std::string constructCommand(std::string str)
{

    for (char &c : str)
    {
        if (c == ',')
        {
            c = ' ';
        }
    }
    return str;
}
std::string getTimeStamp()
{
    std::string timeStamp;
    std::stringstream temp;
    std::time_t t = std::time(0);
    std::tm *now = std::localtime(&t);
    temp << "\n"
         << (now->tm_year + 1900) << '/'
         << (now->tm_mon + 1) << '/'
         << now->tm_mday << '/'
         << now->tm_hour << '-'
         << now->tm_min << '-'
         << now->tm_sec << ": ";
    return timeStamp = temp.str();
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