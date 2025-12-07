#include <string>
#include <chrono>

class RPCConnection
{
public:
    RPCConnection(const std::string& ip, uint16_t port);
    ~RPCConnection();

    bool Connect();
    bool IsConnected() const;
    void close();

    int Send(const std::string& data);
    int Recv(char* buffer, size_t bufferSize);

    const std::string& GetIp() const { return m_ip; }
    uint16_t GetPort() const { return m_port; }
    std::chrono::steady_clock::time_point GetLastUsedTime() const { return m_lastUsed; }
    void UpdateLastUsedTime() { m_lastUsed = std::chrono::steady_clock::now(); }
private:
    int m_fd;
    std::string m_ip;
    uint16_t m_port;
    bool m_connected;
    std::chrono::steady_clock::time_point m_lastUsed; // 连接最近一次使用时间
};