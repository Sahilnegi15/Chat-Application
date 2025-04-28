#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <thread>
#include <vector>
#include <algorithm>
#include <mutex>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define SERVER_ADDR "127.0.0.1"
using namespace std;

void setNonBlocking(SOCKET& socket) {
    u_long mode = 1;  
    if (ioctlsocket(socket, FIONBIO, &mode) != 0) {
        cerr << "Failed to set socket to non-blocking mode: " << WSAGetLastError() << endl;
        closesocket(socket);
        WSACleanup();
        exit(1);
    }
}


bool checkConnection(SOCKET clientSocket) {
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(clientSocket, &writefds);

    timeval timeout = { 0, 100000 };  
    int activity = select(0, NULL, &writefds, NULL, &timeout);

    if (activity == SOCKET_ERROR) {
        cerr << "Select error: " << WSAGetLastError() << endl;
        return false;
    }

    if (FD_ISSET(clientSocket, &writefds)) {
        int optval;
        int optlen = sizeof(optval);
        if (getsockopt(clientSocket, SOL_SOCKET, SO_ERROR, (char*)&optval, &optlen) == 0 && optval == 0) {
            return true;  
        }
    }
    return false; 
}

void sendMessage(SOCKET clientSocket) {
    cout << "Enter your chat name: ";
    
    string name;
    getline(cin, name);
    string message;

    while (true) {
        getline(cin, message);
        string msg = name + ": " + message;
        send(clientSocket, msg.c_str(), msg.length(), 0);
        if (msg == "Quit") {
            cout << "Stopping the application..." << endl;
            break;
        }
    }

    closesocket(clientSocket);
    WSACleanup();
}

void RecvMessage(SOCKET clientSocket) {
    char buffer[1024] = { 0 };

    while (true) {
      
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);

        timeval timeout = { 0, 100000 }; 
        int activity = select(0, &readfds, NULL, NULL, &timeout);
        if (activity == SOCKET_ERROR) {
            cerr << "Select error: " << WSAGetLastError() << endl;
            break;
        }

        if (FD_ISSET(clientSocket, &readfds)) {
            int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesReceived > 0) {
                cout << "Server: " << buffer << endl;
            } else if (bytesReceived == 0) {
                cout << "Server disconnected" << endl;
                break;
            } else if (WSAGetLastError() != WSAEWOULDBLOCK) {
                cerr << "Recv error: " << WSAGetLastError() << endl;
                break;
            }
        }

        
        this_thread::sleep_for(chrono::milliseconds(100));
    }

    closesocket(clientSocket);
    WSACleanup();
}

int main() {
    WSADATA wsaData;
    SOCKET clientSocket;
    struct sockaddr_in serverAddr;
    char buffer[1024] = { 0 };

   
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        return 1;
    }

    
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == INVALID_SOCKET) {
        cerr << "Socket creation failed" << endl;
        WSACleanup();
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

   
    setNonBlocking(clientSocket);

   
    if (connect(clientSocket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            cerr << "Connection failed with error: " << WSAGetLastError() << endl;
            closesocket(clientSocket);
            WSACleanup();
            return 1;
        }
       
        while (!checkConnection(clientSocket)) {
            cout << "Waiting for connection..." << endl;
            this_thread::sleep_for(chrono::milliseconds(500));  \
        }
    }

    cout << "Connected to server" << endl;

   
    thread sender(sendMessage, clientSocket);
    thread receiver(RecvMessage, clientSocket);

    sender.join();
    receiver.join();

    return 0;
}
