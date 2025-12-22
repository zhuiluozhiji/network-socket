# 编译器设置
CXX = g++
# 编译选项：包含 common 目录，启用 C++11，启用 pthread 支持
CXXFLAGS = -std=c++11 -Wall -I../common -pthread

# 目标文件名
TARGET = server

# 源文件列表
SRCS = main.cpp TcpServer.cpp

# 编译规则
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

# 清理规则
clean:
	rm -f $(TARGET)