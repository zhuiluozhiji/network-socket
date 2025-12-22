#ifndef APP_CLIENT_H
#define APP_CLIENT_H

#include <string>
#include <thread>
#include <atomic>
#include "../common/NetMsg.h" // 引入公共协议头文件

class AppClient {
private:
    int _sock;                      // Socket 句柄
    std::atomic<bool> _connected;   // 连接状态 (原子变量，线程安全)
    std::thread _recvThread;        // 后台接收线程对象

public:
    AppClient();
    ~AppClient();

    // 启动客户端主循环
    void run();

private:
    // 连接服务器
    bool connectServer(std::string ip, int port);
    
    // 断开连接
    void disconnect();
    
    // 接收线程的工作函数
    void recvLoop(); 

    // UI 相关
    void showMenu();
    void handleInput(int choice);
    
    // 辅助发送函数
    void sendRequest(char type, std::string data = "", int target = 0);
};

#endif