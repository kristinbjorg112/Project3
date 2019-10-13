
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

class Message
{
public:
    std::string toGroupID;
    std::string fromGroupID;
    std::vector<std::string> vMsg;
    //Message(std::string toGroupID, std::string fromGroupID, std::vector<std::string> messages){
    Message(std::string newtoGroupID) : toGroupID(newtoGroupID) {}
};
std::map<int, Client *> clients;           // Lookup table for per Client information
std::map<int, Server *> servers;           // Lookup table for per Server information
std::map<std::string, Message *> messages; // Datastructure for messages
//std::map<std::string, std::map<std::string, std::vector<std::string>> msgMessage> msgMap; // Datastructure for messages

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
    std::cout << "Padding finished, sending message:" << buffer << std::endl;
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
        std::cout << "Now in checkMessage 'if (buffer[0] == 0x01)'" << std::endl;
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
            std::cout << "checkMessage: passed SOH and EOT check" << std::endl;
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
    }
    else
    {
        std::cout << "No padding on this message" << std::endl;
        std::string outcome = "1";
        return outcome;
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
            std::cout << line << std::endl;
        }
        myfile.close();
    }
    else
        std::cout << "Unable to open file";
}
std::string getnameinformation();
std::string getIp();

std::string getMessage(std::string groupId)
{
    std::cout << "Comes to getMessage: the token is " << groupId << std::endl;
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
                    std::cout << "All messages to  : " << x.second->toGroupID << std::endl;
                    std::cout << "All messages from: " << x.second->toGroupID << std::endl;
                    oss << "All messages to  : " << x.second->toGroupID << std::endl;
                    oss << "All messages from: " << x.second->toGroupID << std::endl;
                    for (auto i = x.second->vMsg.begin(); i < x.second->vMsg.end(); i++)
                    {
                        std::cout << "Message: ";
                        std::cout << *i << std::endl;
                        oss << *i << std::endl;
                        x.second->vMsg.pop_back();
                    }
                }
            }
        }
    }
    return oss.str();
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
    FD_SET(serverSocket, openSocekts);
    maxfds = std::max(maxfds, serverSocket);
    std::string hellomessage = "LISTSERVERS";

    // + serverName + "," + getIp() + "," + serverPort;
    nwrite = sendCommand(serverSocket, hellomessage);
    if (nwrite == -1)
    {
        perror("send() to server failed: ");
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
            oss << std::endl
                << "Key: "
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
    msg = "SERVERS," + serverName + "," + getIp() + "," + serverPort + ";";

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

void printAllMessagesInMap()
{
    // for (auto &x : msgMap)
    // {
    //     std::cout << "msgMap key: " << x.first << std::endl;
    //     if (x.second.size() == 0)
    //     {
    //         return;
    //     }
    //     else
    //     {
    //         for (auto i = x.second.begin(); i < x.second.end(); i++)
    //         {
    //             std::cout << "Message: ";
    //             std::cout << *i << std::endl;
    //         }
    //     }
    // }
}
void addMessageToMapByGroupID(std::string group, std::string str)
{
    std::string currentTime = getTimeStamp();
    std::string msg = currentTime + str;

    std::map<std::string, std::vector<std::string>>::iterator it;

    // it = msgMap.find(group);
    // if (it == msgMap.end())
    // {
    //     std::cout << "No groupID exsists for this group, making new msgMap" << std::endl;
    //     std::vector<std::string> newMessage;
    //     newMessage.push_back(msg);
    //     msgMap.emplace(group, newMessage);
    // }
    // else
    // {
    //     std::cout << "Adding message to msgMap" << std::endl;
    //     msgMap[group].push_back(msg);
    // }
}
void addMessageToMap(int serverSocket, std::string str)
{
    // std::string currentTime = getTimeStamp();
    // std::string msg = currentTime + str;
    // for (auto const &socket : servers)
    // {
    //     if (socket.second->sock == serverSocket)
    //     {
    //         std::map<std::string, std::vector<std::string>>::iterator it;

    //         it = msgMap.find(socket.second->groupID);
    //         if (it == msgMap.end())
    //         {
    //             std::cout << "No groupID exsists for this group, making new msgMap" << std::endl;
    //             std::vector<std::string> newMessage;
    //             newMessage.push_back(msg);
    //             msgMap.emplace(socket.second->groupID, newMessage);
    //         }
    //         else
    //         {
    //             std::cout << "Adding message to msgMap" << std::endl;
    //             msgMap[socket.second->groupID].push_back(msg);
    //         }
    //     }
    // }
    // printAllMessagesInMap();
}
bool checkIfGroupIdExsists(int serverSocket)
{
    // for (auto const &socket : servers)
    // {
    //     if (socket.second->sock == serverSocket)
    //     {
    //         std::map<std::string, std::vector<std::string>>::iterator it;

    //         it = msgMap.find(socket.second->groupID);
    //         if (it == msgMap.end())
    //         {
    //             return true;
    //         }
    //     }
    // }
    // return false;
}
bool checkIfTokenIsGroupId(std::string token)
{
    std::string validString = "P3_GROUP_";
    std::string substing;
    substing = token.substr(0, 8);
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
        std::cout << "Found , in the command" << std::endl;
        std::string temp;
        temp = constructCommand(str);
        str = temp;
    }
    // if (str.find(";") != std::string::npos)
    // {
    //     std::string withoutSemicolon = removeSemicolon(str);
    //     std::cout << "Listing servers: " << std::endl
    //               << buffer << std::endl;
    //     addMessageToMap(serverSocket, withoutSemicolon);
    //     return;
    // }
    // Split command from server into tokens for parsing
    std::stringstream stream(str);

    // if (checkIfGroupIdExsists(serverSocket))
    // {
    //     std::cout << "there is no GROUPID associated with this server socket. Unable to add message to map until thats done" << std::endl;
    // }
    // else
    // {
    //     addMessageToMap(serverSocket, str);
    // }
    while (stream >> token)
        tokens.push_back(token);


    if (tokens[0].compare("SERVERS") == 0)

    {
        std::cout << "serverCommand->SERVERS" << std::endl;
        if (servers.find(serverSocket) == servers.end())
        {
            //add server to map
            Server *nServer = new Server(serverSocket);
            std::cout << "tokens 1" << tokens[1] << std::endl;
            nServer->groupID = tokens[1];
            nServer->IP = tokens[2];
            nServer->port = tokens[3];
            servers.emplace(serverSocket, nServer);
            std::string serverlist = listServers();
            std::cout << "serverCommand->SERVERS->if (!checkIfGroupIdExsists(serverSocket))" << serverlist << std::endl;
        }
        else
        {
            std::cout << "serverCommand->SERVERS->else" << std::endl;
            for (auto const &pair : servers)
            {
                if (pair.second->sock == serverSocket)
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
        if ((tokens.size() == 4))
        {
            for (auto const &pair : servers)
            {
                if (pair.second->sock == serverSocket)
                {
                    pair.second->groupID = tokens[1];
                    pair.second->IP = tokens[2];
                    pair.second->port = tokens[3];

                    std::ostringstream oss;
                    oss << pair.second->groupID << " "
                        << pair.second->IP << " "
                        << pair.second->port << " ";
                    std::string temp = oss.str();
                    std::cout << "The connected server is: '" << temp << "'" << std::endl;
                    std::string msg = listServers();
                    sendCommand(serverSocket, msg);
                }
            }
        }
        else
        {
            std::string msg = listServers();
            sendCommand(serverSocket, msg);
            for (auto const &pair : servers)
            {
                if (pair.second->sock == serverSocket)
                {
                    if (pair.second->groupID.empty())
                    {
                        std::string listserversCommand = "LISTSERVERS";
                        sendCommand(serverSocket, listserversCommand);
                        std::cout << "sent listservers back, did not find in my map";
                    }
                }
            }
        }
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
        //GETMSG,<GROUPID>Get messages for the specified group
        //This may be for yourown group, or another group."
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
        if (tokens[2] == serverName)
        {
            std::cout << "serverCommand->SEND_MSG: Message from group: " << tokens[1] << std::endl;
            std::string msg;
            for (auto i = tokens.begin() + 3; i != tokens.end(); i++)
            {
                std::cout << *i << " ";
                msg += *i + " ";
            }
            //sending Message to connected client
            if (!clients.empty())
            {
                for (auto const &pair : clients)
                {
                    send(pair.second->sock, msg.c_str(), msg.length(), 0);
                }
            }
        }
        else
        {
            ///TODO crate a function
            std::cout << "serverCommand->SEND_MSG->else" << std::endl;
            Message *nMessage = new Message(tokens[1]);
            std::string msg;
            for (auto i = tokens.begin() + 3; i != tokens.end(); i++)
            {
                msg += *i + " ";
            }
            nMessage->toGroupID = tokens[1];
            nMessage->vMsg.push_back(msg);
            nMessage->fromGroupID = tokens[2];
            messages.emplace(tokens[1], nMessage);
            for (auto const &x : messages)
            {
                std::cout << "To Group   :" << x.second->toGroupID << std::endl;
                for (auto i = x.second->vMsg.begin(); i != x.second->vMsg.end(); i++)
                {
                    std::cout << *i << std::endl;
                }
                std::cout << "From Group :" << x.second->fromGroupID << std::endl;
            }
        }
    }
    else if (tokens[0].compare("STATUSREQ") == 0)
    {
        //DO command STATUSREQ,FROM GROUP
        std::cout << "serverCommand: DO command STATUSREQ,FROM GROUP" << std::endl;
    }
    else if (tokens[0].compare("STATUSRESP") == 0)
    {
        //DO command STATUSREQ,FROM GROUP
        std::cout << "serverCommand: DO command STATUSREQ,FROM GROUP" << std::endl;
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

    if (str.find("LISTSERVERS,V_GROUP_,") != std::string::npos)
    {
        std::string msg = "\nWrong port-hole, dummy. The right one is the port-hole above this one. Please try again\n";
        msg += "Closing connection...\n";
        sendCommand(clientSocket, msg);
        closeClient(clientSocket, openSockets, maxfds);
        return;
    }

    while (stream >> token)
        tokens.push_back(token);

    if (tokens[0].compare("GETMSG") == 0)
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
    else if (tokens[0].compare("SENDMSG") == 0 || tokens[0].compare("RS") == 0)
    {
        if (checkIfTokenIsGroupId(tokens[1]))
        {

            if (!checkIfServerWithThatGroupIdIsConnected(tokens[1]))
            {
                std::cout << "clientCommand->SENDMSG: unable to send command, no connected servers" << std::endl;
                std::string error = "Unable to send command, no connected servers, adding to map";
                send(clientSocket, error.c_str(), error.length() - 1, 0);
                Message *nMessage = new Message(tokens[1]);

                std::string msg;
                for (auto i = tokens.begin() + 2; i != tokens.end(); i++)
                {
                    msg += *i + " ";
                }
                nMessage->toGroupID = tokens[1];
                nMessage->vMsg.push_back(msg);
                nMessage->fromGroupID = serverName;
                messages.emplace(tokens[1], nMessage);
                for (auto const &x : messages)
                {
                    std::cout << "To Group   :" << x.second->toGroupID << std::endl;
                    for (auto i = x.second->vMsg.begin(); i != x.second->vMsg.end(); i++)
                    {
                        std::cout << *i << std::endl;
                    }
                    std::cout << "From Group :" << x.second->fromGroupID << std::endl;
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

                        std::cout << "Server buffer: " << buffer << std::endl;

                        std::string temp = checkMessage(buffer);

                        strcpy(buffer, temp.c_str());
                        std::string check = "1";
                        if (temp.compare(check) != 0)
                        {
                            serverCommand(server->sock, &openSockets, &maxfds, buffer);
                        }
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