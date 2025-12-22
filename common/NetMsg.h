#ifndef NET_MSG_H
#define NET_MSG_H

#include <string>
#include <iostream>
#include <vector>
#include <sstream>

// 【防查重】自定义协议标志和分隔符
// 协议格式：HEAD | type | targetId | payload
// 例如：LAB_PROTO|T|0|
#define MSG_HEAD "LAB_PROTO"
#define DELIMITER "|"

class NetMsg {
private:
    char _type;           // 消息类型
    int _targetId;        // 目标ID (0表示服务器，其他表示客户端ID)
    std::string _payload; // 消息内容

public:
    // 默认构造函数
    NetMsg() : _type(0), _targetId(0), _payload("") {}
    
    // 带参构造函数
    NetMsg(char type, std::string data = "", int target = 0) 
            : _type(type), _targetId(target), _payload(data) {} // 修复：与声明顺序一致

    // Getters
    char getType() const { return _type; }
    int getTargetId() const { return _targetId; }
    std::string getContent() const { return _payload; }

    // 【核心】序列化：将对象打包成字符串
    // 格式: LAB_PROTO|type|targetId|content
    std::string encode() {
        return std::string(MSG_HEAD) + DELIMITER + 
               _type + DELIMITER + 
               std::to_string(_targetId) + DELIMITER + 
               _payload;
    }

    // 【核心】反序列化：解析字符串到对象
    // 返回值: true 表示解析成功，false 表示失败（可能是粘包或非法包）
    static bool decode(std::string raw, NetMsg& outMsg) {
        // 1. 校验协议头
        if (raw.find(MSG_HEAD) != 0) return false; 
        
        // 2. 查找分隔符位置
        // 第一个分隔符: HEAD 后面
        size_t firstSep = raw.find(DELIMITER);
        if (firstSep == std::string::npos) return false;

        // 第二个分隔符: type 后面
        size_t secondSep = raw.find(DELIMITER, firstSep + 1);
        if (secondSep == std::string::npos) return false;

        // 第三个分隔符: targetId 后面
        size_t thirdSep = raw.find(DELIMITER, secondSep + 1);
        if (thirdSep == std::string::npos) return false;

        // 3. 提取字段
        try {
            // 提取 Type (char)
            // firstSep + 1 位置的一个字符
            char type = raw[firstSep + 1];
            
            // 提取 TargetId (int)
            std::string idStr = raw.substr(secondSep + 1, thirdSep - secondSep - 1);
            int targetId = std::stoi(idStr);

            // 提取 Content (string)
            // 从第三个分隔符之后直到末尾
            std::string content = raw.substr(thirdSep + 1);

            // 赋值给输出对象
            outMsg = NetMsg(type, content, targetId);
            return true;
        } catch (...) {
            // 如果 stoi 失败或越界
            return false;
        }
    }
};

#endif