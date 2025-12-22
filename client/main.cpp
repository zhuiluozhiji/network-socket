#include "AppClient.h"
#include <iostream>

int main() {
    try {
        AppClient client;
        client.run(); // 启动客户端，进入菜单循环
    } catch (const std::exception& e) {
        std::cerr << "Client Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}