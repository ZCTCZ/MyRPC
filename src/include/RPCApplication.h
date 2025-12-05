#pragma once
#include "RPCConfig.h"

class RPCApplication
{
public:
    // 初始化操作
    static void Init(int argc, char** argv);

    // 获取框架类对象的单例
    static RPCApplication& GetInstance();

    // 返回 config 配置类静态成员对象
    const RPCConfig& GetConfig();
private:
    RPCApplication() = default;
    RPCApplication(const RPCApplication&) = delete;
    RPCApplication(RPCApplication&&) = delete;
    RPCApplication& operator=(const RPCApplication&) = delete;
    RPCApplication& operator=(const RPCApplication&&) = delete;
    static RPCConfig config; // 读取配置文件的实例类
};