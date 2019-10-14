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

//using namespace std;

#define BACKLOG 5 // Allowed length of queue of waiting connections
int maxfds;       // Passed to select() as max fd in set
std::string serverName = "P3_GROUP_20";
std::string serverPort;
std::string serverIp;

// Help functions, are below main()
std::string viewFiles();
std::string constructCommand(std::string str);
std::string removeSemicolon(std::string str);
std::string getTimeStamp();
std::string getnameinformation();
std::string getIp();

// Simple class for handling connections from clients.
class Client
{
public:
    int sock;         // socket of client connection
    std::string name; // Limit length of name of client's user

    Client(int socket) : sock(socket) {}

    ~Client() {} // Virtual destructor defined for base class
};

// Simple class for handling connections from servers.
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

// Simple class for handling messages from other groups.
class Message
{
public:
    std::string toGroupID;
    std::string fromGroupID;
    std::vector<std::string> vMsg;
    Message(std::string newtoGroupID) : toGroupID(newtoGroupID) {}
};
std::map<int, Client *> clients;           // Lookup table for per Client information
std::map<int, Server *> servers;           // Lookup table for per Server information
std::map<std::string, Message *> messages; // Datastructure for messages

//Function to write to file
int writeToFile(char *buffer)
{
    std::ofstream file;
    file.open("./log.txt", std::ios::in | std::ios::app | std::ios::out);
    file << getTimeStamp();
    file << buffer << std::endl;
    file.close();
}

//Function to set padding on a message
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
    std::cout << "Padding finished, sending message:" << buffer << std::endl;
    writeToFile(buffer);
    return send(clientSocket, buffer, sizeof(buffer), 0);
}

//Function to remove padding on incoming messaages
std::string removePadding(std::string msg)
{
  
    std::string removePadding;
    msg.erase((msg.length() - 1), 1);
    msg.erase(0, 1);
    removePadding = msg;
    return msg;
}
std::string checkMessage(char *buffer)
{

    if (buffer[0] == 0x01)
    {
        std::string msg = buffer;
   
        if (msg[0] == 0x01 && msg[msg.length() - 1] == 0x04)
        {
            std::string msg = buffer;
            std::cout << "checkMessage: passed SOH and EOT check" << std::endl;
            std::string outcome;
            outcome = removePadding(msg);
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
        }
    }
    else
    {
        std::cout << "No padding on this message" << std::endl;
        std::string outcome = "1";
        return outcome;
    }
}

//Function to get a single message for a given groupID
std::string getMessage(std::string groupId)
{
    std::ostringstream oss;
    if (messages.find(groupId) == messages.end())
    {
        std::cout << "There are no messages from this group" << std::endl;
        oss << "There are no messages from this group";
    }
    else
    {
        for (auto &x : messages)
        {
            if (x.second->toGroupID == groupId)
            {

                if (x.second->vMsg.size() == 0)
                {
                    std::cout << "There are no new messages on this server" << std::endl;
                    oss << "There are no new messages on this server";
                }
                else
                {
                    //Changed this from x.second->fromGroupID << std::endl;
                    std::cout << "Messages to  : " << x.second->toGroupID << std::endl;
                    std::cout << "Messages from: " << x.second->fromGroupID << std::endl;
                    std::cout << x.second->vMsg.front();
                    oss << "Messages to  : " << x.second->toGroupID << std::endl;
                    oss << "Messages from: " << x.second->fromGroupID << std::endl;
                    oss << "Message fetched at " + getTimeStamp() << std::endl;
                    oss << x.second->vMsg.front();
                    x.second->vMsg.erase(x.second->vMsg.begin());
                }
            }
        }
    }
    return oss.str();
}


//Function based on main's client.cpp to force a connenction to another server
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

    //set new server into the map
    Server *nServer = new Server(serverSocket);
    nServer->sock = serverSocket;
    nServer->IP = ipAddress;
    nServer->port = port;
    servers.emplace(serverSocket, nServer);
    FD_SET(serverSocket, openSocekts);
    maxfds = std::max(maxfds, serverSocket);
    std::string hellomessage = "LISTSERVERS," + serverName;
    nwrite = sendCommand(serverSocket, hellomessage);
    if (nwrite == -1)
    {
        perror("send() to server failed: ");
    }
}
int open_socket(int portno);
void closeClient(int clientSocket, fd_set *openSockets, int *maxfds);
void closeServer(int serverSocket, fd_set *openSockets, int *maxfds);

//function to find all servers connected to this server
std::string listServers()
{
    std::string msg;
    msg = "SERVERS," + serverName + "," + getIp() + "," + serverPort + ";";

    for (auto const &x : servers)
    {
        if(!x.second->groupID.empty()){
            
            std::ostringstream oss;
            oss << x.second->groupID
                << ","
                << x.second->IP
                << ","
                << x.second->port
                << ";";

            msg += oss.str();
        }
        return msg;
    }
}

//Function that takes in a groupID and checks if it is a valid groupID
bool checkIfTokenIsGroupId(std::string token)
{
    std::string validString = "P3_GROUP_";
    std::string substing;
    substing = token.substr(0, 9);
    if (validString.compare(substing) == 0)
    {
        return true;
    }
    return false;
}


bool checkIfServerWithThatGroupIdIsConnected(std::string token)
{
    for (auto const &socket : servers)
    {
        if (socket.second->groupID == token)
        {
            return true;
        }
    }
    return false;
}
//Function to get a socket for a groupID
int getServerSocketFromGroupID(std::string gID)
{
    for (auto const &x : servers)
    {
        std::cout << "getServerSocketFromGroupID: " << x.second->groupID << " == " << gID << std::endl;
        if (x.second->groupID == gID)
        {
            int socket = x.second->sock;
            std::cout << "Socket: " << x.second->sock << std::endl;
            return socket;
        }
    }
    return -1;
}

// Process command from server on the server
void serverCommand(int serverSocket, fd_set *openSockets, int *maxfds,
                   char *buffer)
{
    std::cout << "serverCommand: " << std::endl;
    for (int i = 0; i < 10; i++)
    {
        std::cout << buffer[i];
    }
    std::vector<std::string> tokens;
    std::string token;

    std::string str(buffer);
    if (str.find(",") != std::string::npos)
    {
        std::string temp;
        temp = constructCommand(str);
        str = temp;
    }
    std::stringstream stream(str);
    while (stream >> token)
        tokens.push_back(token);

    if (tokens[0].compare("SERVERS") == 0)
    {
        std::cout << "serverCommand->SERVERS" << std::endl;
        if (servers.find(serverSocket) == servers.end()
        && (tokens[1].compare(serverName) != 0))
        {
            //add server to map
            Server *nServer = new Server(serverSocket);
            nServer->sock = serverSocket;
            
            nServer->groupID = tokens[1];
            nServer->IP = tokens[2];
            nServer->port = tokens[3];
            servers.emplace(serverSocket, nServer);
            std::string serverlist = listServers();
            int n = serverlist.length(); 
  
            char char_array[n + 1]; 
        
            strcpy(char_array, serverlist.c_str());
            writeToFile(char_array);
        }
        else
        {
            for (auto const &pair : servers)
            {
                if (pair.second->sock == serverSocket && (tokens[1].compare(serverName) != 0))
                {
                    pair.second->groupID = tokens[1];
                    pair.second->IP = tokens[2];
                    pair.second->port = tokens[3];
                }
            }
            std::string serverlist = listServers();
            std::cout << serverlist << std::endl;
        }
    }
else if (tokens[0].compare("LISTSERVERS") == 0)
    {
        std::string msg = listServers();
        sendCommand(serverSocket, msg);
        for (auto const &pair : servers)
        {
            if (pair.second->sock == serverSocket)
            {
                if (pair.second->groupID.empty())
                {
                    std::string listserversCommand = "LISTSERVERS," + serverName;
                    sendCommand(serverSocket, listserversCommand);
                    std::cout << "sent listservers back, did not find in my map";
                }
            }
        }
    }
    else if (tokens[0].compare("GET_MSG") == 0)
    {
       std::string msg = getMessage(tokens[1]);
       sendCommand(serverSocket,msg);

    }
    else if (tokens[0].compare("LEAVE") == 0)
    {
        if (tokens.size() == 3)
        {
            if (!servers.empty())
            {
                for (auto const &pair : servers)
                {
                    if ((pair.second->IP.compare(tokens[1]) && pair.second->port.compare(tokens[2])) == 0)
                    {
                        std::cout << "SERVERCOMMAND Found Server and will disconenct" << std::endl;
                        closeServer(pair.second->sock, openSockets, maxfds);
                        close(pair.second->sock);
                    }
                }
            }
            else
            {
                std::cout << "LEAVE SERVERCOMMAND This server is not connected to any servers" << std::endl;
            }
        }
        else
        {
            std::cout << "LEAVE SERVERCOMMAND You need to write the command with LEAVE ip port";
        }
    }
    else if (tokens[0].compare("KEEPALIVE") == 0)
    {
        //DO command keepalive, KEEPALIVE,<# of Messages>
        std::cout << "serverCommand: DO command keepalive, KEEPALIVE,<# of Messages>" << std::endl;
    }
    else if (tokens[0].compare("GET_MSG") == 0)
    {
        std::cout << "serverCommand->GET_MSG" << std::endl;
        if (tokens.size() < 2)
        {
            sendCommand(serverSocket, "Please insert groupID\n");
        }
        else
        {
            std::string msg = getMessage(tokens[1]);
            sendCommand(serverSocket, msg);
        }
    }
    else if (tokens[0].compare("SEND_MSG") == 0)
    {
        if (clients.empty())
        {
            std::cout << "serverCommand->SEND_MSG: unable to send command, server with that groupID not connected or no client is connected. Adding to map" << std::endl;
            if (messages.find(tokens[2]) == messages.end())
            {
                //make a new message map
                Message *nMessage = new Message(tokens[2]);

                std::string msg = "Server recived message at " + getTimeStamp();
                for (auto i = tokens.begin() + 3; i != tokens.end(); i++)
                {
                    msg += *i + " ";
                }
                nMessage->toGroupID = tokens[2];
                nMessage->vMsg.push_back(msg);
                nMessage->fromGroupID = tokens[1];
                messages.emplace(tokens[2], nMessage);
            }
            else
            {
                //if group already has a message add the message to the map
                for (auto const &x : messages)
                {
                    if (x.second->toGroupID == tokens[2])
                    {
                        std::string msg = "Server recived message at " + getTimeStamp();
                        for (auto i = tokens.begin() + 3; i != tokens.end(); i++)
                        {
                            msg += *i + " ";
                        }
                        x.second->vMsg.push_back(msg);
                        x.second->fromGroupID = tokens[1];
                    }
                }
            }
            std::cout << "printing all messages in map" << std::endl;
            for (auto const &x : messages)
            {
                std::cout << "To Group   :" << x.second->toGroupID << std::endl;
                std::cout << "From Group :" << x.second->fromGroupID << std::endl;
                for (auto i = x.second->vMsg.begin(); i != x.second->vMsg.end(); i++)
                {
                    std::cout << *i << std::endl;
                }
            }
        }
        else
        {
            std::cout << "serverCommand->SEND_MSG: Message from group: " << tokens[1] << std::endl;
            std::string msg = "New message from: " + token[1] + '\n';
            msg += "Recived from: " + getTimeStamp() + '\n';
            for (auto i = tokens.begin() + 3; i != tokens.end(); i++)
            {
                std::cout << *i << " ";
                msg += *i + " ";
            }
            //sending Message to all connected clients
            if (!clients.empty())
            {
                for (auto const &pair : clients)
                {
                    send(pair.second->sock, msg.c_str(), msg.length(), 0);
                }
            }
        }
    }
    else if (tokens[0].compare("STATUSRESP") == 0)
    {
        std::cout << "serverCommand: DO command STATUSRESP,FROM GROUP" << std::endl;
        std::string msg = "STATUSRESP from " + tokens[1] + ',';
        for (auto i = tokens.begin() + 3; i != tokens.end(); i++)
        {
            msg += *i + " ";
        }
        for (auto const &pair : clients)
        {
            send(pair.second->sock, msg.c_str(), msg.length(), 0);
        }
    }
    else if (tokens[0].compare("STATUSREQ") == 0)
    {
        std::cout << "serverCommand: DO command STATUSREQ,FROM GROUP" << std::endl;
        std::string msg = "STATUSRESP," + serverName + ',' + tokens[1] + ',';

        for (auto &z : servers)
        {
            std::cout << "serverCommand: autoz" << std::endl;
            std::string groupID = z.second->groupID;

            for (auto &x : messages)
            {
                std::cout << x.second->toGroupID << " AND "
                          << "groupID" << std::endl;
                if (x.second->toGroupID.compare(groupID) == 0)
                {
                    std::cout << "got in makeing the message" << std::endl;
                    std::string toGroupID = x.second->toGroupID;
                    std::string nrOfMessages = std::to_string(x.second->vMsg.size());
                    msg += toGroupID + ',' + nrOfMessages + ',';
                }
            }
        }
        std::cout << "this is the statusrsp" << msg << std::endl;
        sendCommand(serverSocket, msg);
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
    std::cout << "clientCommand" << std::endl;
    std::vector<std::string> tokens;
    std::string token;

    // Split command from client into tokens for parsing
    std::stringstream stream(buffer);
    std::string str(buffer);

    while (stream >> token)
        tokens.push_back(token);

    if (tokens[0].compare("GETMSG") == 0)
    {
        if (checkIfTokenIsGroupId(tokens[1]))
        {
            std::cout << "clientCommand->GETMSG" << std::endl;
            if (tokens.size() != 2)
            {
                std::string error = "Please insert groupID";
                send(clientSocket, error.c_str(), error.length(), 0);
            }
            else
            {
                std::string msg = getMessage(tokens[1]);
                send(clientSocket, msg.c_str(), msg.length(), 0);
            }
        }
        else
        {
            std::cout << "clientCommand->GETMSG: Not valid groupID" << std::endl;
        }
    }
    if (tokens[0].compare("GET_MSG") == 0)
    {
        std::string mssg = tokens[0] + ',' + serverName;
        int sock = getServerSocketFromGroupID(tokens[1]);

        sendCommand(sock,mssg);
    }
    else if (tokens[0].compare("SENDMSG") == 0 || tokens[0].compare("RS") == 0)
    {
        if (checkIfTokenIsGroupId(tokens[1]))
        {

            if (!checkIfServerWithThatGroupIdIsConnected(tokens[1]))
            {
                std::cout << "clientCommand->SENDMSG: unable to send command, no connected servers" << std::endl;
                std::string error = "Unable to send command, no connected servers, adding to map";
                if (messages.find(tokens[1]) == messages.end())
                {
                    //make a new message map
                    send(clientSocket, error.c_str(), error.length() - 1, 0);
                    Message *nMessage = new Message(tokens[1]);

                    std::string msg = "Server recived message at " + getTimeStamp();
                    for (auto i = tokens.begin() + 2; i != tokens.end(); i++)
                    {
                        msg += *i + " ";
                    }
                    nMessage->toGroupID = tokens[1];
                    nMessage->vMsg.push_back(msg);
                    nMessage->fromGroupID = serverName;
                    messages.emplace(tokens[1], nMessage);
                }
                else
                {
                    //if group already has a message add the message to the map
                    for (auto const &x : messages)
                    {
                        if (x.second->toGroupID == tokens[1])
                        {
                            std::string msg = "Server recived message at " + getTimeStamp();
                            for (auto i = tokens.begin() + 2; i != tokens.end(); i++)
                            {
                                msg += *i + " ";
                            }
                            x.second->vMsg.push_back(msg);
                            x.second->fromGroupID = serverName;
                        }
                    }
                }
                std::cout << "printing all messages in map" << std::endl;
                for (auto const &x : messages)
                {
                    std::cout << "To Group   :" << x.second->toGroupID << std::endl;
                    std::cout << "From Group :" << x.second->fromGroupID << std::endl;
                    for (auto i = x.second->vMsg.begin(); i != x.second->vMsg.end(); i++)
                    {
                        std::cout << *i << std::endl;
                    }
                }
            }
            else
            {
                std::cout << "clientCommand->SENDMSG: Sending the message '";
                std::string msg = "SEND_MSG," + serverName + ',' + tokens[1] + ',';
                for (auto i = tokens.begin() + 2; i != tokens.end(); i++)
                {
                    msg += *i + " ";
                }
                std::cout << "Message is  " << msg << std::endl;
                int socket = getServerSocketFromGroupID(tokens[1]);
                if (socket == -1)
                {
                    std::cout << "clientCommand->SENDMSG: unable to find serversocket from GROUPID" << std::endl;
                }
                sendCommand(socket, msg);
            }
        }
        else
        {
            std::cout << "clientCommand->SENDMSG: Not valid groupID" << std::endl;
        }
    }
    else if (tokens[0].compare("LISTSERVERS") == 0)
    {
        std::string msg = listServers();
        send(clientSocket, msg.c_str(), msg.length() - 1, 0);
    }
    else if (tokens[0].compare("SC") == 0)
    {
        if (tokens.size() == 3)
        {
            std::string ipAddress = tokens[1];
            std::string port = tokens[2];
            ConnectionToServers(ipAddress, port, clientSocket, openSockets);
        }
        else
        {
            std::cout << "You neeed to write SC ip port" << std::endl;
        }
    }
    else if (tokens[0].compare("QC") == 0) //Quick connect to 127.0.0.1 10002
    {
        std::string base = "100";
        std::string port = tokens[1];
        base += port;
        ConnectionToServers("127.0.0.1", base, clientSocket, openSockets);
    }
    else if (tokens[0].compare("LEAVE") == 0)
    {
        if (!servers.empty())
        {
            if (tokens.size() == 4)
            {
                int socket = getServerSocketFromGroupID(tokens[3]);
                std::string msg = tokens[0] + ',' + tokens[1] + ',' + tokens[2];
                if (socket == -1)
                {
                    std::cout << "unable to find serversocket from GROUPID" << std::endl;
                }
                sendCommand(socket, msg);
            }
            else if (tokens.size() == 3)
            {
                for (auto const &pair : servers)
                {
                    if ((pair.second->IP.compare(tokens[1]) && pair.second->port.compare(tokens[2])) == 0)
                    {
                        std::cout << "SERVERCOMMAND Found Server and will disconenct" << std::endl;
                        closeServer(pair.second->sock, openSockets, maxfds);
                        close(pair.second->sock);
                    }
                }
            }
            else
            {
                std::cout << "Input for LEAVE command not on a right format" << std::endl;
            }
        }
        else
        {
            std::cout << "IN LEAVE CLIENTCOMMAND: no servers are connected to this server" << std::endl;
        }
    }
    else if (tokens[0].compare("STATUSREQ") == 0)
    {
        //checkIfTokenIsGroupId
        std::cout << "CLIENDCOMMAND statusREQ send forward" << std::endl;
        std::string msg = tokens[0] + ',' + serverName;

        int sock = getServerSocketFromGroupID(tokens[1]);
        std::cout << "sent it forward to " << sock << std::endl;
        sendCommand(sock, msg);
    }
    else
    {
        std::cout << "Unknown command from client:" << buffer << std::endl;
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

                // And update the maximum file descriptor
                maxfds = std::max(maxfds, serverSock);

                // If there are more than 5 servers connected.
                if (servers.size() >= 5)
                {
                    // Close connection to a random connected server to make space
                    // for a new connection
                    int first = servers.begin()->second->sock;
                    closeServer(first, &openSockets, &maxfds);
                    close(first);
                    servers[serverSock] = new Server(serverSock);
                }
                else
                {
                    servers[serverSock] = new Server(serverSock);
                    // Decrement the number of sockets waiting to be dealt with
                    n--;
                }

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
                        std::cout << "Server buffer: " << buffer << std::endl;

                        std::string temp = checkMessage(buffer);
                        strcpy(buffer, temp.c_str());
                        std::string check = "1";
                        if (temp.compare(check) != 0)
                        {
                            writeToFile(buffer);
                            serverCommand(server->sock, &openSockets, &maxfds, buffer);
                        }
                    }
                }
            }
        }
    }
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
std::string removeSemicolon(std::string str)
{
    for (char &c : str)
    {
        if (c == ';')
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
    temp << (now->tm_year + 1900) << '/'
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
void closeServer(int serverSocket, fd_set *openSockets, int *maxfds)
{
    // Remove Server from the Server list
    servers.erase(serverSocket);
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