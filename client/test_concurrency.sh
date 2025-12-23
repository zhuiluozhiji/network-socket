#!/bin/bash

# ================= 配置区域 =================
LOG_DIR="client_log"
INPUT_FILE="auto_input.txt"
CLIENT_COUNT=100
REQUESTS_PER_CLIENT=100
EXPECTED_TOTAL=$((CLIENT_COUNT * REQUESTS_PER_CLIENT))
# ===========================================

# 1. 准备工作
echo "=== 初始化测试环境 ==="

# 创建存放日志的文件夹
if [ ! -d "$LOG_DIR" ]; then
    mkdir "$LOG_DIR"
    echo "已创建 $LOG_DIR 文件夹"
fi

# 清理旧日志
rm -f $LOG_DIR/client_*.log
echo "已清理旧日志文件"

# 创建自动输入指令: 1(连接) -> IP -> 端口 -> 2(发送) -> 6(退出)
cat > $INPUT_FILE <<EOF
1
127.0.0.1
6241
2
6
EOF

echo "=== 准备启动 $CLIENT_COUNT 个客户端 ==="

# 2. 并发启动客户端
for i in $(seq 1 $CLIENT_COUNT)
do
    echo "正在启动第 $i 个客户端..."
    # 启动后台进程，日志重定向到文件夹中
    ./client < $INPUT_FILE > "$LOG_DIR/client_$i.log" 2>&1 &
done

echo "=== 所有客户端已在后台运行，正在进行压力测试... ==="
echo "请观察服务器窗口的日志滚动情况！"

# 3. 等待所有后台任务结束
wait
echo "------------------------------------------------"
echo "=== 测试结束，正在统计数据... ==="

# 4. 统计结果 (新增的核心功能)
total_responses=0

echo -e "\n--- 各客户端接收情况 ---"
for i in $(seq 1 $CLIENT_COUNT)
do
    log_file="$LOG_DIR/client_$i.log"
    # 统计文件中包含 "[Response" 的行数
    count=$(grep -c "\[Response" "$log_file")
    
    # 累加总数
    total_responses=$((total_responses + count))
    
    # 打印单行结果
    if [ "$count" -eq "$REQUESTS_PER_CLIENT" ]; then
        echo "Client $i: $count/$REQUESTS_PER_CLIENT (Pass)"
    else
        echo "Client $i: $count/$REQUESTS_PER_CLIENT (FAIL!!)"
    fi
done

echo -e "\n--- 最终统计报告 ---"
echo "预期总响应数: $EXPECTED_TOTAL"
echo "实际总响应数: $total_responses"

if [ "$total_responses" -eq "$EXPECTED_TOTAL" ]; then
    echo "✅ 测试完美通过！服务器成功处理了所有 $EXPECTED_TOTAL 个并发请求，无丢包。"
else
    echo "❌ 测试存在异常！有 $((EXPECTED_TOTAL - total_responses)) 个请求丢失或未收到响应。"
fi

# 清理临时输入文件
rm $INPUT_FILE