#pragma
#include "ZooKeeperUtil.h"

#include <memory>
#include <mutex>
#include <atomic>

class ZkConnectionManager
{
public:
    static ZkConnectionManager* getInstance();
    ZkClient* GetZkClient();

private:
    ZkConnectionManager();
    ~ZkConnectionManager();
    ZkConnectionManager(const ZkConnectionManager&) = delete;
    ZkConnectionManager(const ZkConnectionManager&&) = delete;
    ZkConnectionManager& operator=(const ZkConnectionManager& ) = delete;
    ZkConnectionManager& operator=(const ZkConnectionManager&& ) = delete;

    std::unique_ptr<ZkClient> m_PZkClient;
    std::atomic<bool> m_isConnected;
    std::mutex m_mtx;
};