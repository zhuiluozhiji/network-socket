#include "TcpServer.h"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <ctime>

using namespace std;

// 构造函数：初始化 Socket
TcpServer::TcpServer() : _running(false), _idCounter(100) {
    // 1. 创建 Socket
    _listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (_listenSock == -1) {
        perror("Socket create failed");
        exit(1);
    }

    // 2. 设置端口复用 (防止重启时端口被占用)
    int opt = 1;
    setsockopt(_listenSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 3. 绑定端口
    sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY; // 监听所有网卡
    serverAddr.sin_port = htons(SERVER_PORT);

    if (bind(_listenSock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    // 4. 开始监听
    if (listen(_listenSock, 10) < 0) {
        perror("Listen failed");
        exit(1);
    }
}

TcpServer::~TcpServer() {
    close(_listenSock);
}

// 主循环：只负责 Accept 新连接
void TcpServer::start() {
    _running = true;
    cout << "[Server] Listening on port " << SERVER_PORT << "..." << endl;

    while (_running) {
        sockaddr_in clientAddr;
        socklen_t len = sizeof(clientAddr);
        
        // 阻塞等待连接
        int clientSock = accept(_listenSock, (struct sockaddr*)&clientAddr, &len);
        if (clientSock < 0) {
            perror("Accept failed");
            continue;
        }

        // 分配 ID 并记录
        int newId = _idCounter++; // ID 自增
        
        // 加锁操作 Map
        {
            lock_guard<mutex> lock(_mtx);
            ClientNode node;
            node.socket = clientSock;
            node.addr = clientAddr;
            node.id = newId;
            _clients[newId] = node;
        }

        cout << "[Server] New Client connected. ID: " << newId 
             << " IP: " << inet_ntoa(clientAddr.sin_addr) << endl;

        // 启动子线程处理该客户端
        // 【注意】使用 std::thread 替代 pthread，这是 C++11 特性，也是加分项
        std::thread t(&TcpServer::workerThread, this, clientSock, clientAddr, newId);
        t.detach(); // 分离线程，让它独立运行
    }
}

// 工作线程：接收数据并解析
// 在 server/TcpServer.cpp 中替换 workerThread 函数

void TcpServer::workerThread(int clientSock, sockaddr_in addr, int clientId) {
    char buffer[BUF_SIZE];
    std::string msgBuffer = ""; // 持久化缓冲区，用于处理粘包

    while (true) {
        memset(buffer, 0, BUF_SIZE);
        // 阻塞接收
        int bytesRead = recv(clientSock, buffer, BUF_SIZE - 1, 0);
        
        // 客户端断开或出错
        if (bytesRead <= 0) {
            cout << "[Server] Client " << clientId << " disconnected." << endl;
            close(clientSock);
            
            // 从列表中移除
            lock_guard<mutex> lock(_mtx);
            _clients.erase(clientId);
            break;
        }

        // 将收到的数据追加到缓冲区
        msgBuffer += buffer;

        // 循环处理缓冲区中所有完整的包（以 \n 结尾）
        size_t pos;
        while ((pos = msgBuffer.find('\n')) != std::string::npos) {
            // 提取第一条完整消息（不含 \n）
            std::string singlePacket = msgBuffer.substr(0, pos);
            // 从缓冲区移除已处理的部分（含 \n）
            msgBuffer.erase(0, pos + 1);

            // 解析并分发
            NetMsg msg;
            if (NetMsg::decode(singlePacket, msg)) {
                dispatchMessage(clientSock, msg, clientId);
            }
        }
    }
}

// 消息分发器
void TcpServer::dispatchMessage(int sock, NetMsg& msg, int clientId) {
    char type = msg.getType();
    
    switch (type) {
        case 'T': // Time Request
            handleTimeReq(sock, clientId); // 传入 clientId
            break;
        case 'N': // Name Request
            handleNameReq(sock, clientId); // 传入 clientId
            break;
        case 'L': // List Request
            handleListReq(sock, clientId); // 【修改】传入 clientId
            break;
        case 'S': // Send Message (Forward)
            handleForwardReq(sock, clientId, msg.getTargetId(), msg.getContent());
            break;
        case 'D': // Disconnect
            // 实际上 recv 返回 0 会自动处理断开，这里可以是主动退出的命令
            break;
        default:
            cout << "[Server] Unknown request type: " << type << endl;
            break;
    }
}

// 1. 处理时间
void TcpServer::handleTimeReq(int sock, int clientId) {
    time_t now = time(0);
    tm* ltm = localtime(&now);
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ltm);
    
    // 【新增日志】
    cout << "[Server] Client " << clientId << " requested Time. Sending: " << buf << endl;
    
    sendMsg(sock, 'T', std::string(buf));
}

// 2. 处理名字
// 2. 处理名字
void TcpServer::handleNameReq(int sock, int clientId) {
    char hostname[128];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        strcpy(hostname, "Server-Unknown");
    }

    // 【新增日志】
    cout << "[Server] Client " << clientId << " requested Name. Sending: " << hostname << endl;

    sendMsg(sock, 'N', std::string(hostname));
}

// 3. 处理列表
// 【修改】增加参数 int clientId
// 3. 处理列表
void TcpServer::handleListReq(int sock, int clientId) {
    // 【新增日志】
    cout << "[Server] Client " << clientId << " requested Client List." << endl;

    std::string listStr = "=== Online Clients === "; 
    lock_guard<mutex> lock(_mtx);
    for (auto& pair : _clients) {
        ClientNode& node = pair.second;
        listStr += "[ID:" + to_string(node.id) + 
                   " " + inet_ntoa(node.addr.sin_addr) + 
                   ":" + to_string(ntohs(node.addr.sin_port));
        if (node.id == clientId) listStr += "(You)";
        listStr += "] ";
    }
    
    sendMsg(sock, 'L', listStr);
}

// 4. 处理转发
// 4. 处理转发
void TcpServer::handleForwardReq(int sock, int sourceId, int targetId, std::string content) {
    // 【日志 1】收到请求
    // 格式：[1]handle request..
    cout << "[" << sourceId << "]handle request.." << endl;

    lock_guard<mutex> lock(_mtx);
    
    // 查找目标是否存在
    if (_clients.find(targetId) != _clients.end()) {
        int targetSock = _clients[targetId].socket;
        
        // 【日志 2】准备发送
        // 格式：send messsage to [2]:From [l]: hello
        cout << "send messsage to [" << targetId << "]:From [" << sourceId << "]: " << content << endl;
        
        // 组装消息: [来自 ID:101] 你好
        std::string forwardContent = "[From " + to_string(sourceId) + "]: " + content;
        
        // 复用 'S' 类型，TargetId 填 sourceId 告知接收方是谁发的
        NetMsg msg('S', forwardContent, sourceId); 
        std::string packet = msg.encode();
        send(targetSock, packet.c_str(), packet.length(), 0);
        
        // 【日志 3】发送成功
        // 格式：send messsage:already send the message!
        cout << "send messsage:already send the message!" << endl;

    } else {
        // 目标不存在的日志
        cout << "[Server] Error: Target " << targetId << " not found." << endl;
        sendMsg(sock, 'S', "[System] Error: Client " + to_string(targetId) + " not found.");
    }
}

// 辅助发送
void TcpServer::sendMsg(int sock, char type, std::string content, int targetId) {
    NetMsg msg(type, content, targetId);
    std::string packet = msg.encode();
    send(sock, packet.c_str(), packet.length(), 0);
}
//