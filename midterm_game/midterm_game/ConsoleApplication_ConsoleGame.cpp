#include <winsock2.h>
#include <ws2tcpip.h>
#include "MyUtil.h"
#include <cwchar>
#define _USE_MATH_DEFINES
#include <cmath>
#include <complex>
#include <Windows.h>
#include <WinUser.h>
#include <strsafe.h>
#include "KInput.h"
#include <vector>
#include <thread>
#include <mutex>
#include <iostream>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")

SOCKET ConnectSocket = INVALID_SOCKET;

double g_drawScale = 1.0;
std::string chatMessage;
std::string serverMessage;
bool chatMode = false;

std::mutex chatMutex;
std::mutex serverMutex;

class KVector2
{
public:
    double x;
    double y;
};
KVector2 g_characterPos{ 10,10 };

void DrawLine(double x, double y, double x2, double y2, char ch)
{
    KVector2 center{ g_width / 2.0, g_height / 2.0 };
    ScanLine(int(x * g_drawScale + center.x), int(-y * g_drawScale + center.y)
        , int(x2 * g_drawScale + center.x), int(-y2 * g_drawScale + center.y), ch);
}

void Update(double elapsedTime)
{
    g_drawScale = 1.0;
    DrawLine(-g_width / 2, 0, g_width / 2, 0, '.');
    DrawLine(0, -g_height / 2, 0, g_height / 2, '.');

    PutTextf(0, 0, "%g", elapsedTime);
    if (!chatMode)
    {
        if (Input.GetKeyDown(VK_LEFT))
            g_characterPos.x -= 1;
        if (Input.GetKeyDown(VK_RIGHT))
            g_characterPos.x += 1;
        if (Input.GetKeyDown(VK_UP))
            g_characterPos.y -= 1;
        if (Input.GetKeyDown(VK_DOWN))
            g_characterPos.y += 1;
    }
}

void DrawGameWorld() {
    float h = Input.GetAxis("Horizontal");
    float v = Input.GetAxis("Vertical");

    PutTextf(1, 1, "Simultaneous Key Processing:");
    PutTextf(1, 2, "h = %g", h);
    PutTextf(1, 3, "v = %g", v);
    PutText(g_characterPos.x, g_characterPos.y, "P");

    {
        std::lock_guard<std::mutex> lock(chatMutex);
        PutTextf(1, g_height - 5, std::string(1024, ' ').c_str());
        PutTextf(1, g_height - 5, "%s", chatMessage.c_str());
    }

    {
        std::lock_guard<std::mutex> lock(serverMutex);
        PutTextf(1, g_height - 4, std::string(1024, ' ').c_str());
        PutTextf(1, g_height - 4, "%s", serverMessage.c_str());
    }

    DrawBuffer();
}

bool InitializeClient() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        return false;
    }

    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(5150);
    inet_pton(AF_INET, "127.0.0.1", &serverAddress.sin_addr);

    if (connect(ConnectSocket, (sockaddr*)&serverAddress, sizeof(serverAddress)) == SOCKET_ERROR) {
        closesocket(ConnectSocket);
        WSACleanup();
        return false;
    }
    return true;
}

void ReceiveMessages() {
    char buffer[1024];
    while (true) {
        int bytesReceived = recv(ConnectSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';

            std::lock_guard<std::mutex> lock(serverMutex);
            serverMessage = std::string(buffer);
        }
        else {
            break;
        }
    }
}

void chat(bool& chatMode) {
    chatMode = true;
    char DataBuffer[1024] = "";

    std::cout << "Enter chat message: ";
    std::cin.getline(DataBuffer, 1024);

    int sendResult = send(ConnectSocket, DataBuffer, (int)strlen(DataBuffer), 0);

    {
        std::lock_guard<std::mutex> lock(chatMutex);
        chatMessage = DataBuffer;
    }

    chatMode = false;
}

int main(void)
{
    if (!InitializeClient()) {
        return 1;  // Exit if unable to connect
    }

    std::thread receiveThread(ReceiveMessages); 
    receiveThread.detach();

    g_hwndConsole = GetConsoleWindow();
    g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    ShowCursor(false);

    bool isGameLoop = true;
    clock_t prevClock = clock();
    clock_t currClock = prevClock;

    while (isGameLoop)
    {
        if (Input.GetKeyDown(VK_ESCAPE)) {
            isGameLoop = false;
        }
        if (Input.GetKeyDown(VK_RETURN) && !chatMode)
        {
            std::thread chat_thread(chat, std::ref(chatMode));
            chat_thread.detach();
        }
        prevClock = currClock;
        currClock = clock();
        const double elapsedTime = ((double)currClock - (double)prevClock) / CLOCKS_PER_SEC;
        ClearBuffer();
        Input.Update(elapsedTime);
        Update(elapsedTime);
        Sleep(10);
        DrawGameWorld();
    }

    closesocket(ConnectSocket);
    WSACleanup();
}
