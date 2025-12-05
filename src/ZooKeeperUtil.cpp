#include "ZooKeeperUtil.h"
#include "RPCApplication.h"

#include <iostream>
#include <semaphore.h>

ZkClient::ZkClient()
: m_zhandle(nullptr, deleter)
{
}

ZkClient::~ZkClient()
{
}
/**
 * @brief 全局回调函数 运行在与 zkServer 通信的IO线程内
 * 
 * @param zh 本次回调所对应的 ZooKeeper 句柄。
 * @param type  ZOO_CREATED_EVENT（节点创建）
 *              ZOO_DELETED_EVENT（节点删除）
 *              ZOO_CHANGED_EVENT（节点数据变更）
 *              ZOO_CHILD_EVENT（子节点列表变化）
 *              ZOO_SESSION_EVENT（会话状态变化）
 *              ZOO_NOTWATCHING_EVENT（服务器端移除 watcher）
 * 
 * @param state ZOO_EXPIRED_SESSION_STATE 会话过期
 *              ZOO_AUTH_FAILED_STATE 认证失败
 *              ZOO_CONNECTING_STATE 正在连接
 *              ZOO_ASSOCIATING_STATE 正在握手
 *              ZOO_CONNECTED_STATE 已连接（正常）
 *              
 * @param path 触发事件的节点路径。
 * @param watcherCtx 在 zookeeper_init 时传进去的 watcherCtx 参数，用来把“自定义数据”带进回调。
 */
void watcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
    if (type == ZOO_SESSION_EVENT) // 触发回调的事件类型是 会话状态发生改变
    {
        if (state == ZOO_CONNECTED_STATE) // 事件状态为已连接
        {
            sem_t* pIsReady = (sem_t*)(watcherCtx);
            sem_post(pIsReady); // 将信号量加1
        }
    }
}


void ZkClient::Start()
{
    std::string zkAddr = std::move(RPCApplication::GetInstance().GetConfig().Load("zookeeperAddr"));
    std::string zkPort = RPCApplication::GetInstance().GetConfig().Load("zookeeperPort");
    std::string host = zkAddr + ":" + zkPort;

    sem_t isReady;
    sem_init(&isReady, 0, 0); 

    /*
    zookeeper_init() 会创建一个网络I/O线程，watcher回调函数也在该线程里执行。该线程的创建是一个异步的过程。
    zookeeper_init() 返回一个 zkClient 的句柄，通过该句柄就可以和 zkServer 通信。
    */
    zhandle_t* pZhandle = zookeeper_init(host.data(), watcher, 30000, nullptr, (void*)&isReady, 0);
    if (pZhandle == nullptr) // 初始化句柄失败
    {
        std::cerr << "zookeeper_init() err" << std::endl;
        exit(EXIT_FAILURE);
    }

    // 等待 和 zkServer 服务器连接成功
    sem_wait(&isReady);
    
    m_zhandle.reset(pZhandle);
    std::cout << "Connect to zkServer Successfully"  << std::endl;
}

void ZkClient::Create(const char *path, const char *data, int dataLen, int state)
{
    char path_buffer[128] = {0};
    int bufferLen = sizeof(path_buffer);
    int flag = zoo_exists(m_zhandle.get(), path, 0, nullptr);
    if (flag == ZNONODE) // 路径为 path 的结点不存在，可以创建
    {
        /**
         * @brief 创建指定 path 的结点 
         * 参数1：zooKeepet 句柄
         * 参数2：新结点所在路径
         * 参数3：新结点里的数据
         * 参数4：新结点里数据的大小
         * 参数5：访问控制列表，一般用 ZOO_OPEN_ACL_UNSAFE
         * 参数6：创建的结点状态，永久结点或者暂时性结点
         * 参数7：传出参数，用于接收实际创建的结点名称
         */
        int res = zoo_create(m_zhandle.get(), path, data, dataLen, &ZOO_OPEN_ACL_UNSAFE, state, path_buffer, bufferLen);
        if (res == ZOK)
        {
            std::cout << "Create New ZNode Successfully... path=" << path << std::endl;
        }
        else
        {
            std::cerr << "Create New ZNode Failed... path=" << path << " flag=" << res << std::endl;
        }
    }
}

std::string ZkClient::GetData(const char *path)
{
    char buffer[256] = {0};
    int bufferLen = sizeof(buffer);
    int res = zoo_get(m_zhandle.get(), path, 0, buffer, &bufferLen, nullptr);
    if (res == ZOK)
    {
        return std::string(buffer);
    }
    else
    {
        std::cerr << "zoo_get err... path=" << path << " flag=" << res << std::endl;
        return "";
    }
}