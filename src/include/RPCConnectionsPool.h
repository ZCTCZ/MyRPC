#pragma once
#include "RPCConnection.h"
#include <unordered_map>
#include <queue>
#include <string>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

class RPCConnectionsPool
{
public:
    static RPCConnectionsPool* GetInstance();
    std::shared_ptr<RPCConnection> GetConnection(const std::string& ip, uint16_t port);
    void ReturnConnection(std::shared_ptr<RPCConnection> pConn);
    void SetMaxIdleTime(int seconds) { m_maxIdleTime = seconds; }
    void SetMaxConnectionsPerHost(int count) { m_maxConnectionsPerHost = count; }

private:
    RPCConnectionsPool();
    ~RPCConnectionsPool();

    RPCConnectionsPool(const RPCConnectionsPool&) = delete;
    RPCConnectionsPool(const RPCConnectionsPool&&) = delete;
    RPCConnectionsPool& operator=(const RPCConnectionsPool&) = delete;
    RPCConnectionsPool& operator=(const RPCConnectionsPool&&) = delete;

    void CleanIdleConnections(); // 清空所有连接
    void CleanTimeOutConnections(); // 清空超时连接
    void RunCleaner(); // 清理线程运行的函数

    struct ConnectionKey
    {
        std::string ip;
        uint16_t port;

        bool operator==(const ConnectionKey& other) const
        {
            return ip == other.ip && port == other.port;
        }
    };
    
    struct KeyHash
    {
        size_t operator()(const ConnectionKey& key) const
        {
            return std::hash<std::string>()(key.ip) ^ (std::hash<uint16_t>()(key.port) << 1);
        }
    };

    std::unordered_map<ConnectionKey, std::queue<std::shared_ptr<RPCConnection>>, KeyHash> m_idleConnection; //记录空闲连接的哈希表
    std::unordered_map<ConnectionKey, int, KeyHash> m_connectionCount; // 记录每台主机上连接的个数


    int m_maxIdleTime; // 连接的最大空闲时间
    int m_maxConnectionsPerHost; // 每个主机最大连接个数
    std::mutex m_mtx;
    std::atomic<bool> m_stopCleaner;
    std::thread m_cleanerThread; // 清理超时连接的线程
    std::condition_variable m_cond;
};