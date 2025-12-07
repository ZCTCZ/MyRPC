#include "RPCConnection.h"
#include "Log.h"
#include <unistd.h>
#include <arpa/inet.h>

RPCConnection::RPCConnection(const std::string& ip, uint16_t port)
: m_fd(-1),
  m_ip(ip),
  m_port(port),
  m_lastUsed(std::chrono::steady_clock::now())
{
}

RPCConnection::~RPCConnection()
{
    close();
}

bool RPCConnection::Connect()
{
    if (m_fd != -1 && m_connected)
    {
        return true;
    }

    m_fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (m_fd == -1)
    {
        LOG(Log::error) << "socket() err";
        return false;
    }

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(m_port);

    if (inet_pton(AF_INET, m_ip.data(), &servaddr.sin_addr.s_addr) == -1)
    {
        LOG(Log::error) << "inet_pton() " << m_ip << " err";
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    if (::connect(m_fd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1)
    {
        LOG(Log::error) << "connect() " << m_ip << ":" << m_port << " err";
        ::close(m_fd);
        m_fd = -1;
        return false;
    }

    m_connected = true;
    return true;
}

bool RPCConnection::IsConnected() const
{
    return m_connected && m_fd != -1;
}

void RPCConnection::close()
{
    if (m_fd != -1) 
    {
        ::close(m_fd);
        m_fd = -1;
        m_connected = false;
    }
}

int RPCConnection::Send(const std::string& data)
{
    if (!m_connected)
    {
        return -1;
    }

    return ::send(m_fd, data.data(), data.size(), MSG_NOSIGNAL);
}   

int RPCConnection::Recv(char* buffer, size_t bufferSize)
{
    if (!m_connected)
    {
        return -1;
    }

    return ::recv(m_fd, buffer, bufferSize, 0);
}