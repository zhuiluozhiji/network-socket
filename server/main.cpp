#include "TcpServer.h"
#include <iostream>

int main() {
    // 实例化并启动
    try {
        TcpServer server;
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Server crashed: " << e.what() << std::endl;
    }
    return 0;
}