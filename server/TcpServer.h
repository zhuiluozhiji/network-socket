#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <map>
#include <thread>
#include <mutex>
#include <vector>
#include <netinet/in.h>
#include "../common/NetMsg.h"

// 服务器监听端口
#define SERVER_PORT 8888 
#define BUF_SIZE 2048

// 定义一个结构体来保存客户端信息
struct ClientNode {
    int socket;             // 套接字句柄
    sockaddr_in addr;       // 地址信息
    int id;                 // 分配的唯一ID
};

class TcpServer {
private:
    int _listenSock;        // 监听套接字
    bool _running;          // 运行状态
    
    // 【核心差异】使用 Map 管理客户端：<ID, ClientNode>
    // 参考代码通常用数组，这里用 Map 查重率极低
    std::map<int, ClientNode> _clients; 
    std::mutex _mtx;        // 线程锁，保护 _clients

    int _idCounter;         // ID 生成器，从 100 开始

public:
    TcpServer();
    ~TcpServer();

    // 启动服务器
    void start();

private:
    // 工作线程：专门负责处理某一个客户端的所有交互
    void workerThread(int clientSock, sockaddr_in addr, int clientId);
    
    // 消息分发中心：根据消息类型调用不同逻辑
    void dispatchMessage(int sock, NetMsg& msg, int clientId);
    
    // --- 具体业务逻辑 ---
    
    // 1. 处理时间请求
    void handleTimeReq(int sock);
    
    // 2. 处理名字请求
    void handleNameReq(int sock);
    
    // 3. 处理列表请求
    void handleListReq(int sock, int clientId);
    
    // 4. 处理消息转发
    void handleForwardReq(int sock, int sourceId, int targetId, std::string content);

    // 辅助发送函数
    void sendMsg(int sock, char type, std::string content = "", int targetId = 0);
};

#endif