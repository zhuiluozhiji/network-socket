# 编译器设置
CXX = g++

# 编译选项
# -std=c++11: 使用 C++11 标准
# -Wall: 开启所有警告
# -I../common: 将 common 目录加入头文件搜索路径，否则找不到 NetMsg.h
# -pthread: 开启多线程支持 (必须！)
CXXFLAGS = -std=c++11 -Wall -I../common -pthread

# 生成的可执行文件名
TARGET = client

# 需要编译的源文件
SRCS = main.cpp AppClient.cpp

# 默认编译规则
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

# 清理规则 (执行 make clean 时调用)
clean:
	rm -f $(TARGET)