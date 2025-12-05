#pragma once

#include <zookeeper/zookeeper.h>
#include <memory>
#include <functional>
#include <string>

// zookeeper的客户端类
class ZkClient
{
public:
    ZkClient();
    ~ZkClient();

    // 启动 zookeeper 的客户端程序zkClient，和 zookeeper的服务端 zkServer 建立连接
    void Start();

    
    /**
     * @brief 在 zkServer 上根据指定的path创建zookeeper结点
     * 
     * @param path zookeeper 结点 在 zkServer 上的路径
     * @param data zookeeper 结点里的数据
     * @param dataLen data 的长度
     * @param state zookeeprt 结点的状态。state = 0:永久性结点
     */
    void Create(const char* path, const char* data, int dataLen, int state = 0);

    // 获取指定 path 的结点的值
    std::string GetData(const char* path);

private:
    std::function<void(zhandle_t*)> deleter = [](zhandle_t* p){ if (p) zookeeper_close(p); };
    std::unique_ptr<zhandle_t, decltype(deleter)> m_zhandle;//zookeeper的客户端句柄 
};