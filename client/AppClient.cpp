#include "AppClient.h"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <limits> // 用于清理输入缓冲区

// 【Bonus 修改】用于统计收到的时间响应个数
static int g_timeRespCount = 0;

using namespace std;

// 构造函数
AppClient::AppClient() : _sock(-1), _connected(false) {}

// 析构函数
AppClient::~AppClient() { 
    disconnect(); 
}

// 主循环：显示菜单并处理输入
void AppClient::run() {
    int choice;
    while (true) {
        showMenu();
        if (!(cin >> choice)) {
            // 防止输入非数字导致死循环
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        // 处理回车残留，防止干扰后续 getline
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (choice == 6) { // 退出
            disconnect();
            break;
        }
        
        handleInput(choice);
        
        // 简单延时，防止菜单刷新太快冲掉输出信息
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

// 连接服务器
bool AppClient::connectServer(std::string ip, int port) {
    if (_connected) {
        cout << "[Info] Already connected." << endl;
        return true;
    }

    // 1. 创建 Socket
    _sock = socket(AF_INET, SOCK_STREAM, 0);
    if (_sock == -1) {
        perror("Socket creation failed");
        return false;
    }

    // 2. 设置服务器地址
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) <= 0) {
        cout << "[Error] Invalid IP address" << endl;
        return false;
    }

    // 3. 连接
    if (connect(_sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Connection failed");
        return false;
    }

    _connected = true;
    cout << "[Info] Connected to server successfully!" << endl;

    // 4. 启动接收线程
    // 使用 std::thread 创建后台线程，专门负责 recv
    _recvThread = std::thread(&AppClient::recvLoop, this);
    
    return true;
}

// 断开连接
void AppClient::disconnect() {
    if (_connected) {
        _connected = false;
        
        // 【新增】强制关闭读写通道，这能确保阻塞的 recv 立即返回 0 或 -1
        shutdown(_sock, SHUT_RDWR); 
        
        close(_sock);
        _sock = -1;
        
        // 等待接收线程结束
        if (_recvThread.joinable()) {
            _recvThread.join();
        }
        cout << "[Info] Disconnected." << endl;
    }
}

// 接收线程循环
// 在 client/AppClient.cpp 中替换 recvLoop 函数

// void AppClient::recvLoop() {
//     char buffer[2048];
//     std::string msgBuffer = ""; // 【added】持久化缓冲区

//     while (_connected) {
//         memset(buffer, 0, sizeof(buffer));
//         int bytesRead = recv(_sock, buffer, sizeof(buffer) - 1, 0);
        
//         if (bytesRead <= 0) {
//             if (_connected) { 
//                 cout << "\n[Error] Server disconnected." << endl;
//                 _connected = false;
//                 close(_sock);
//                 _sock = -1;
//             }
//             break;
//         }

//         // 修改：追加数据并循环切割
//         msgBuffer += buffer;

//         size_t pos;
//         while ((pos = msgBuffer.find('\n')) != std::string::npos) {
//             std::string raw = msgBuffer.substr(0, pos);
//             msgBuffer.erase(0, pos + 1);

//             NetMsg msg;
//             if (NetMsg::decode(raw, msg)) {
//                 // 根据消息类型显示不同内容
//                 if (msg.getType() == 'S') {
//                     cout << "\n>>> [New Message] " << msg.getContent() << endl;
//                 } else if (msg.getType() == 'L') {
//                     cout << "\n" << msg.getContent() << endl;
//                 } else {
//                     cout << "\n>>> [Server Response]: " << msg.getContent() << endl;
//                 }
//                 cout << ">>> "; 
//                 flush(cout);
//             }
//         }
//     }
// }

// bonus部分

// 接收线程循环：负责后台接收服务器消息
void AppClient::recvLoop() {
    char buffer[2048];
    std::string msgBuffer = ""; // 持久化缓冲区，用于解决粘包

    while (_connected) {
        // 1. 阻塞等待接收数据
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(_sock, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesRead <= 0) {
            if (_connected) { 
                cout << "\n[Error] Server disconnected." << endl;
                _connected = false;
                close(_sock);
                _sock = -1;
            }
            break;
        }

        // 2. 将新数据追加到缓冲区
        msgBuffer += buffer;

        // 3. 循环切割处理完整的数据包（以 \n 为界）
        size_t pos;
        while ((pos = msgBuffer.find('\n')) != std::string::npos) {
            std::string raw = msgBuffer.substr(0, pos);
            msgBuffer.erase(0, pos + 1); // 移除已处理部分

            // 4. 反序列化并显示
            NetMsg msg;
            if (NetMsg::decode(raw, msg)) {
                // 根据消息类型显示不同内容
                if (msg.getType() == 'T') {
                    // 【Bonus 修改】计数并显示
                    g_timeRespCount++;
                    cout << "\n>>> [Response " << g_timeRespCount << "/100]: " << msg.getContent() << endl;
                    
                    if (g_timeRespCount == 100) {
                         cout << ">>> [Success] All 100 responses received!" << endl;
                    }
                } 
                else if (msg.getType() == 'S') {
                    // 处理转发消息
                    cout << "\n>>> [New Message] " << msg.getContent() << endl;
                } 
                else if (msg.getType() == 'L') {
                    // 处理列表消息（直接打印，不加额外前缀，因为内容里已经格式化好了）
                    cout << "\n" << msg.getContent() << endl;
                } 
                else {
                    // 其他类型（如 'N' 名字响应）
                    cout << "\n>>> [Server Response]: " << msg.getContent() << endl;
                }

                // 恢复提示符
                cout << ">>> "; 
                flush(cout);
            }
        }
    }
}

void AppClient::showMenu() {
    if (!_connected) {
        cout << "\n=== OFFLINE MENU ===" << endl;
        cout << "1. Connect to Server" << endl;
        cout << "6. Exit" << endl;
    } else {
        cout << "\n=== ONLINE MENU ===" << endl;
        cout << "2. Get Server Time" << endl;
        cout << "3. Get Server Name" << endl;
        cout << "4. Get Client List" << endl;
        cout << "5. Send Message" << endl;
        cout << "6. Disconnect & Exit" << endl;
    }
    cout << "Select: ";
}

void AppClient::handleInput(int choice) {
    if (!_connected) {
        if (choice == 1) {
            string ip;
            int port;
            cout << "Enter Server IP (e.g. 127.0.0.1): ";
            cin >> ip;
            cout << "Enter Server Port (e.g.学号后四位): ";
            cin >> port;
            connectServer(ip, port);
        } else {
            cout << "Please connect first." << endl;
        }
        return;
    }

    // 已连接状态下的功能
    switch (choice) {
        case 1:
            cout << "Already connected." << endl;
            break;
        // case 2: // 获取时间
        //     sendRequest('T');
        //     break;
case 2: // 获取时间
        {
            cout << "=== Starting 100 Requests Stress Test ===" << endl;
            g_timeRespCount = 0; 
            
            for (int i = 0; i < 100; i++) {
                sendRequest('T');
                
                // 【关键修改】每次发送后暂停 50 毫秒
                // 100次 x 50ms = 持续约 5 秒
                // 这样能保证 Client 1 还在发的时候，Client 2 肯定也加入进来了
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            cout << ">>> Sent 100 requests. Waiting for responses..." << endl;
            break;
        }
        case 3: // 获取名字
            sendRequest('N');
            break;
        case 4: // 获取列表
            sendRequest('L');
            break;
        case 5: // 发送消息
        {
            int targetId;
            string content;
            cout << "Enter Target Client ID (Check List first): ";
            cin >> targetId;
            
            // 【关键修改】增加这一行，清除输入数字后残留的换行符
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); 
            
            cout << "Enter Message: ";
            // 之前的注释有误导性，这里必须清理完 buffer 才能 getline
            getline(cin, content);
            
            if (!content.empty()) {
                sendRequest('S', content, targetId);
            } else {
                cout << "Message cannot be empty." << endl;
            }
            break;
        }
        default:
            cout << "Invalid option." << endl;
            break;
    }
}

// 辅助发送函数
void AppClient::sendRequest(char type, std::string data, int target) {
    if (!_connected) return;
    
    NetMsg msg(type, data, target);
    std::string packet = msg.encode();
    
    int sent = send(_sock, packet.c_str(), packet.length(), 0);
    if (sent < 0) {
        perror("Send failed");
    }
}