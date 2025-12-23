#!/bin/bash

# 1. 准备工作
# 创建存放日志的文件夹（如果不存在）
if [ ! -d "client_log" ]; then
    mkdir client_log
    echo "已创建 client_log 文件夹"
fi

# 清理旧的日志文件
rm -f client_log/client_*.log

# 创建自动输入文件
# 内容含义：1(连接) -> IP -> 端口 -> 2(发送100次请求) -> 6(退出)
cat > auto_input.txt <<EOF
1
127.0.0.1
6241
2
6
EOF

echo "=== 准备启动 100 个客户端进行并发测试 ==="

# 2. 循环启动 100 个后台进程
for i in {1..100}
do
    echo "正在启动第 $i 个客户端..."
    # 【修改点】将输出路径改为 client_log/client_$i.log
    ./client < auto_input.txt > client_log/client_$i.log 2>&1 &
done

echo "=== 10 个客户端已全部启动，正在后台运行... ==="
echo "请观察服务器窗口的日志输出！"

# 3. 等待所有后台任务结束
wait

echo "=== 测试结束！所有客户端已退出 ==="
echo "日志文件已保存在 client_log/ 目录下"