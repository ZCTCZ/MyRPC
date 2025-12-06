#include "ZkConnectionManager.h"

ZkConnectionManager* ZkConnectionManager::getInstance()
{
    static ZkConnectionManager instance;
    return &instance;
}

ZkClient* ZkConnectionManager::GetZkClient()
{
    if (!m_PZkClient || !m_isConnected.load())
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (!m_PZkClient || !m_isConnected.load())
        {
            m_PZkClient.reset(new ZkClient());
            m_PZkClient->Start();
            m_isConnected.store(true);
        }
    } 
    return m_PZkClient.get();
}

ZkConnectionManager::ZkConnectionManager()
: m_PZkClient(nullptr),
  m_isConnected(false)
{
}

ZkConnectionManager::~ZkConnectionManager()
{
}
