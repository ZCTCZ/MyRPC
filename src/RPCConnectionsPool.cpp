#include "RPCConnectionsPool.h"

RPCConnectionsPool::RPCConnectionsPool()
: m_maxIdleTime(300), // 默认最大空闲时间为5分钟
  m_maxConnectionsPerHost(10), // 默认单台主机最多10个连接
  m_stopCleaner(false),
  m_cleanerThread([this](){ RunCleaner(); })
{
}

RPCConnectionsPool::~RPCConnectionsPool()
{
    m_stopCleaner.store(true);
    m_cond.notify_one();
    if (m_cleanerThread.joinable()) // 等待子线程结束
    {
        m_cleanerThread.join();
    }
    CleanIdleConnections(); // 清理其他空闲连接
}

RPCConnectionsPool* RPCConnectionsPool::GetInstance()
{
    static RPCConnectionsPool instance;
    return &instance;
}

void RPCConnectionsPool::CleanIdleConnections()
{
    std::lock_guard<std::mutex> lock(m_mtx);
    for (auto& e : m_idleConnection)
    {
        while (!e.second.empty())
        {
            e.second.pop();
        }
    }
}

std::shared_ptr<RPCConnection> RPCConnectionsPool::GetConnection(const std::string& ip, uint16_t port)
{
    std::lock_guard<std::mutex> lock(m_mtx);
    struct ConnectionKey key({ip, port});

    // 首先在空闲连接map表里面查找
    auto it = m_idleConnection.find(key);
    if (it != m_idleConnection.end() && !it->second.empty())
    {
        auto pConn = std::move(it->second.front());
        it->second.pop();

        if (pConn->IsConnected())
        {
            return pConn;
        }
    }

    // 空闲连接map表里面没有找到，建立新的连接
    // 在建立新的连接之前，检查一下目标主机的连接数量是否超过上限
    if (m_connectionCount[key] >= m_maxConnectionsPerHost)
    {
        return nullptr; // 返回空指针
    }

    // 建立新的连接
    auto newConn = std::make_shared<RPCConnection>(ip, port);
    if (newConn->Connect()) // 连接建立成功
    {
        m_connectionCount[key]++; // 连接计数+1
        return newConn;
    }

    return nullptr;
}

void RPCConnectionsPool::ReturnConnection(std::shared_ptr<RPCConnection> pConn)
{
    if (!pConn)
    {
        return ;
    }

    ConnectionKey key({pConn->GetIp(), pConn->GetPort()});

    if (pConn->IsConnected()) // 当前连接是有效的
    {
        pConn->UpdateLastUsedTime();
        std::lock_guard<std::mutex> lock(m_mtx);

        m_idleConnection[key].push(std::move(pConn));
    }
    else // 当前连接无效
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        auto it = m_connectionCount.find(key);
        if (it != m_connectionCount.end()) 
        {
            if (--it->second == 0) // 将连接计数-1
            {
                // 当主机的连接数减为0，就从 空闲连接map表 和 连接计数map表 里删除该主机
                m_connectionCount.erase(key);
                m_idleConnection.erase(key);
            }
        }
    }
}

void RPCConnectionsPool::CleanTimeOutConnections()
{
    auto now = std::chrono::steady_clock::now();

    for (auto& e : m_idleConnection)
    {
        while (!e.second.empty())
        {
            // 从队头连接开始，获取它的空闲时间
            auto idleDuration = std::chrono::duration_cast<std::chrono::seconds>(now - e.second.front()->GetLastUsedTime()).count();
            if (idleDuration >= m_maxIdleTime) // 连接超时
            {
                ConnectionKey key = e.first;
                e.second.pop(); // 删除超时连接
                if (--m_connectionCount[key] == 0) // 连接计数-1，如果连接计数减为零，则删除该主机
                {
                    m_connectionCount.erase(key);
                    m_idleConnection.erase(key);
                }
            }
            else // 最老空闲连接都未超时，说明该队列里所有的连接都没有超时
            {
                break;
            }
        }
    }
}

void RPCConnectionsPool::RunCleaner()
{
    if (!m_stopCleaner.load())
    {
        // 每分钟检查一次是否有超时连接
        std::unique_lock<std::mutex> lock(m_mtx);
        m_cond.wait_for(lock, std::chrono::seconds(1));
        if (!m_stopCleaner.load())
        {
            CleanTimeOutConnections();
        }
    }
}



