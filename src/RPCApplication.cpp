#include "RPCApplication.h"
#include <unistd.h>
#include <string>
#include <iostream>

// 定义静态成员变量
RPCConfig RPCApplication::config;

// 初始化操作
void RPCApplication::Init(int argc, char** argv) 
{
    if (argc != 3)
    {
        std::cout << "用法: " << argv[0] << " -i <configfile>" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string file;
    int opt = 0;
    while ((opt = getopt(argc, argv, "i:")) != -1) // 检测是否有 -i 选项，并且携带参数。
    {
        switch (opt)
        {
        case 'i':
            file = optarg;
            break;
        case '?': // 遇到无法识别的选项
            exit(EXIT_FAILURE);
        default:
            exit(EXIT_FAILURE);
        }
    }

    // 加载配置文件
    // std::cout << file << std::endl;
    config.LoadConfigFile(file);
}

// 获取框架类对象的单例
RPCApplication& RPCApplication::GetInstance()
{
    static RPCApplication rpcApp = RPCApplication();
    return rpcApp;
}

// 返回 config 配置类静态成员对象
const RPCConfig& RPCApplication::GetConfig()
{
    return RPCApplication::config;
}