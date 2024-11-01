#include <winsock2.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "ws2_32.lib")

void main(void)
{
    WSADATA              wsaData;
    SOCKET               ListeningSocket;
    SOCKET               NewConnection;
    SOCKADDR_IN          ServerAddr;
    SOCKADDR_IN          ClientAddr;
    int                  ClientAddrLen = sizeof(ClientAddr);
    int                  Port = 5150;
    int                  Ret;
    char                 DataBuffer[1024];
    char                 ResponseBuffer[1024];

    if ((Ret = WSAStartup(MAKEWORD(2, 2), &wsaData)) != 0) {
        printf("WSAStartup failed with error %d\n", Ret);
        return;
    }

    if ((ListeningSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET) {
        printf("socket failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_port = htons(Port);
    ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(ListeningSocket, (SOCKADDR*)&ServerAddr, sizeof(ServerAddr)) == SOCKET_ERROR) {
        printf("bind failed with error %d\n", WSAGetLastError());
        closesocket(ListeningSocket);
        WSACleanup();
        return;
    }

    if (listen(ListeningSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("listen failed with error %d\n", WSAGetLastError());
        closesocket(ListeningSocket);
        WSACleanup();
        return;
    }

    printf("We are awaiting a connection on port %d.\n", Port);

    if ((NewConnection = accept(ListeningSocket, (SOCKADDR*)&ClientAddr, &ClientAddrLen)) == INVALID_SOCKET) {
        printf("accept failed with error %d\n", WSAGetLastError());
        closesocket(ListeningSocket);
        WSACleanup();
        return;
    }

    printf("We successfully got a connection from %s:%d.\n", inet_ntoa(ClientAddr.sin_addr), ntohs(ClientAddr.sin_port));
    closesocket(ListeningSocket);

    printf("We are waiting to receive data...\n");

    while (true) {

        memset(DataBuffer, 0, sizeof(DataBuffer));
        memset(ResponseBuffer, 0, sizeof(ResponseBuffer));

        Ret = recv(NewConnection, DataBuffer, sizeof(DataBuffer) - 1, 0);
        if (Ret == SOCKET_ERROR) {
            printf("recv failed with error %d\n", WSAGetLastError());
            break;
        }
        else if (Ret == 0) {
            printf("Connection closed by client.\n");
            break;
        }

        DataBuffer[Ret] = '\0';
        printf("Client: %s\n", DataBuffer);

        snprintf(ResponseBuffer, sizeof(ResponseBuffer), "Server: %s", DataBuffer);

        Ret = send(NewConnection, ResponseBuffer, strlen(ResponseBuffer), 0);
        printf("%s\n", ResponseBuffer);
        if (Ret == SOCKET_ERROR) {
            printf("send failed with error %d\n", WSAGetLastError());
            break;
        }
    }

    printf("We are now going to close the client connection.\n");
    closesocket(NewConnection);
    WSACleanup();
}
