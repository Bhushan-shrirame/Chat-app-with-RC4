#include <iostream>
#include <cstring>
#include <string>
#include <thread>
#include <unistd.h>
#include "dh.h"
#include "rc4.h"

#ifdef _WIN32

#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")

#elif __linux__
#include <netinet/in.h>
#include <arpa/inet.h>
#define SOCKET int
#define closesocket close
#endif

const char *SERVER_IP = "127.0.0.1"; // Change to the server's IP
const int SERVER_PORT = 10069;

int state = 0;
int secretKey = -1;
int publicKeyP = 3001;
int publicKeyG = 2749;
int sharedKey = -1;

int decodeRecievedMessage(std::string &msg, SOCKET clientSocket)
{
    if (msg.substr(0, 3) == "000")
    {
        state = 0;
        return 0;
    }
    std::string decode = "";
    int index = 0;
    for (index = 0; index < msg.size(); index++)
        if (msg[index] == ':')
            break;
    index += 2;
    for (int i = index; i < msg.size(); i++)
    {
        if (msg[i] == ' ')
        {
            break;
        }
        decode += msg[i];
    }
    if ((decode == "enable-rc4") && state == 0)
    {
        std::cout << "Securing the channel-exchanging keys" << std::endl;
        std::string key = "";
        int otherKey = -1;
        for (int i = index + decode.size() + 1; i < msg.size(); i++)
        {
            if (msg[i] == ' ')
            {
                break;
            }
            key += msg[i];
        }
        secretKey = rand() % 3000;
        otherKey = std::stoi(key);
        secretKey = compute(publicKeyG, secretKey, publicKeyP);
        sharedKey = compute(otherKey, secretKey, publicKeyP);
        std::string message = "enable-rc4 " + std::to_string(secretKey);
        send(clientSocket, message.c_str(), message.length(), 0);
        std::cout << "Key Exchange Complete" << std::endl;
        state = 2;
        return 0;
    }
    else if (decode == "enable-rc4" && state == 1)
    {
        std::string key = "";
        int otherKey = -1;
        for (int i = index + decode.size() + 1; i < msg.size(); i++)
        {
            if (msg[i] == ' ')
            {
                break;
            }
            key += msg[i];
        }
        otherKey = std::stoi(key);
        sharedKey = compute(secretKey, otherKey, publicKeyP);
        std::cout << "Key Exchange Complete" << std::endl;
        state = 2;
        return 0;
    }
    else if (decode == "disable-rc4")
    {
        state = 0;
        std::cout << "Switching to unsecure Channel";
    }
    else if (state == 2)
    {
        std::string to_decode = "";
        for (int i = index; i < msg.size(); i++)
        {
            to_decode += msg[i];
        }
        msg = msg.substr(0, index) + encryptDecrypt(to_decode, sharedKey);
    }
    return 1;
}

void receiveMessages(SOCKET clientSocket)
{
    char buffer[1024];
    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0)
        {
            std::cerr << "Server disconnected." << std::endl;
            closesocket(clientSocket);
            exit(1);
        }
        std::string message(buffer);
        if (state == 2)
        {
            std::cout << message << std::endl;
        }
        if (decodeRecievedMessage(message, clientSocket))
            std::cout << message << std::endl;
    }
}

int decodeSentMessage(std::string &msg)
{
    if (msg == "quit")
    {
        return 1;
    }
    else if (msg == "enable-rc4" && state == 0)
    {
        std::cout << "Securing the channel-exchanging keys" << std::endl;
        state = 1;
        secretKey = abs(rand() % 3000);
        secretKey = compute(publicKeyG, secretKey, publicKeyP);
        msg = msg + " " + std::to_string(secretKey);
    }
    else if (msg == "disable-rc4")
    {
        state = 0;
        std::cout << "Switching to unsecure Channel" << std::endl;
    }
    else if (state == 2)
    {
        msg = encryptDecrypt(msg, sharedKey);
    }
    return 0;
}

int main()
{
#ifdef __WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return -1;
    }
#endif
    srand((unsigned)time(NULL));
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1)
    {
        std::cerr << "Error creating socket" << std::endl;
        return -1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(SERVER_PORT);
    serverAddress.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (connect(clientSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        std::cerr << "Error connecting to server" << std::endl;
#ifdef __WIN32
        WSACleanup();
#endif
        return -1;
    }

    std::cout << "Connected to the server" << std::endl;

    // Input the user's name
    std::string userName;
    std::cout << "Enter your name: ";
    std::getline(std::cin, userName);
    send(clientSocket, userName.c_str(), userName.length(), 0);

    std::thread receiveThread(receiveMessages, clientSocket);
    receiveThread.detach();

    while (true)
    {
        std::string message;
        std::getline(std::cin, message);

        if (decodeSentMessage(message))
        {
            break;
        }

        std::string fullMessage = message;
        send(clientSocket, fullMessage.c_str(), fullMessage.length(), 0);
    }

    closesocket(clientSocket);
#ifdef __WIN32
    WSACleanup();
#endif

    return 0;
}
