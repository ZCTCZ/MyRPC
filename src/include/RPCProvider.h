#pragma once

#include "google/protobuf/service.h"
#include <muduo/net/EventLoop.h>
#include <muduo/net/TcpConnection.h>
#include <string>
#include <unordered_map>
#include <google/protobuf/descriptor.h>

// 用于发布 RPC 服务的类
class RPCProvider
{
public:
    // 框架暴露给外部的接口，用来发布 RPC 远程调用服务
    void NotifyService(google::protobuf::Service *);

    // 启动 RPC 服务结点，开始提供远程网络调用服务
    void Run();

private:
    muduo::net::EventLoop m_eventLoop;

    // 描述 service 对象
    struct ServiceInfo
    {
        google::protobuf::Service *m_pservice;                                                   // 指向service对象的指针，这里用的是基类指针
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> m_methodMap; // 记录service对象持有的方法，<方法名，方法描述>
    };

    std::unordered_map<std::string, struct ServiceInfo> m_serviceMap; // 记录所有注册的服务（service 对象）

    void OnConnect(const muduo::net::TcpConnectionPtr &pConn);

    void OnMessage(const muduo::net::TcpConnectionPtr &pConn, muduo::net::Buffer *buffer, muduo::Timestamp timestamp);

    // 回调函数，将response发送回客户端
    void SendRpcResponse(const muduo::net::TcpConnectionPtr &pConn, google::protobuf::Message *response);

    // RPC调用过程中出现问题，导致调用失败，给框架的客户端返回失败信息
    void SendErrorResponse(const muduo::net::TcpConnectionPtr &pConn, int error_code, const std::string &error_msg);
};