#include "RPCProvider.h"
#include "RPCApplication.h"
#include "Header.pb.h"
#include "Response.pb.h"
#include "RPCController.h"
#include "ZooKeeperUtil.h"

#include <muduo/net/TcpServer.h>
#include <muduo/net/InetAddress.h>
#include <memory>

// 框架暴露给外部的接口，用来发布（注册） RPC 远程调用服务
void RPCProvider::NotifyService(google::protobuf::Service *gService)
{
    ServiceInfo serviceInfo;

    // 获取待注册的服务对象的描述信息
    const google::protobuf::ServiceDescriptor *serviceDesc = gService->GetDescriptor();

    // 将service对象的所有方法都记录到 serviceInfo
    int methodCnt = serviceDesc->method_count();
    for (int i = 0; i < methodCnt; ++i)
    {
        const google::protobuf::MethodDescriptor *methodDesc = serviceDesc->method(i); // 获取每个method的描述信息
        serviceInfo.m_methodMap.insert(std::make_pair(methodDesc->name(), methodDesc));
    }

    // 将 service 指针记录到 serviceInfo
    serviceInfo.m_pservice = gService;

    // 将 service 对象添加到 m_serviceMap 里
    m_serviceMap.insert(std::make_pair(serviceDesc->name(), std::move(serviceInfo)));
}

// 启动 RPC 服务结点，开始提供远程网络调用服务
void RPCProvider::Run()
{
    std::string ip = RPCApplication::GetInstance().GetConfig().Load("rpcAddr");
    uint16_t port = std::stoi(RPCApplication::GetInstance().GetConfig().Load("rpcPort").data());

    muduo::net::InetAddress addr(ip, port);

    // 定义 TcpServer 对象
    muduo::net::TcpServer tcpserver(&m_eventLoop, addr, "RPCProvider");

    // 设置连接建立和断开的回调函数
    tcpserver.setConnectionCallback([this](const muduo::net::TcpConnectionPtr &pConn)
                                    { OnConnect(pConn); });

    // 设置通信的回调函数
    tcpserver.setMessageCallback([this](const muduo::net::TcpConnectionPtr &pConn, muduo::net::Buffer *buffer, muduo::Timestamp timestamp)
                                 { OnMessage(pConn, buffer, timestamp); });

    tcpserver.setThreadNum(4);
    
    // 向 zkServer 上发布服务
    
    ZkClient zk; // 定义 zkClient 对象，通过该对象和 zkServer 通信
    zk.Start(); // 连接 zkServer 服务器

    for (const auto& e1 : m_serviceMap)
    {
        std::string servicePath("/" + e1.first); // 服务结点所在路径: /serviceName
        zk.Create(servicePath.data(), nullptr, 0, 0); // 服务结点需要作为父节点，所以创建为永久性结点
        for (const auto& e2 : e1.second.m_methodMap)
        {
            std::string methodPath(servicePath + "/" + e2.first); // 方法结点路径：/serviceName/methodName
            std::string nodeData(ip + ":" + std::to_string(port)); // 方法结点里的数据："IP:Port"
            zk.Create(methodPath.data(), nodeData.data(), nodeData.size(), ZOO_EPHEMERAL); // 方法结点创建为临时性结点
        }
    }

    tcpserver.start();
    m_eventLoop.loop();
}

void RPCProvider::OnConnect(const muduo::net::TcpConnectionPtr &pConn)
{
    if (pConn->disconnected())
    {
        pConn->shutdown();
    }
}

/**
 * 客户端框架发送过来的 一条完整的rpc请求 的数据格式：4字节前缀长度 + rpcHeaderSize + rpcHeader + 参数
 */
void RPCProvider::OnMessage(const muduo::net::TcpConnectionPtr &pConn, muduo::net::Buffer *buffer, muduo::Timestamp timestamp)
{
    while (buffer->readableBytes() > 4)
    {
        uint32_t len = buffer->peekInt32(); // 读出报文的长度，不需要手动转换成主机字节序，peekInet32()内部已经做过了
        if (buffer->readableBytes() >= len + 4) // 完整接收到了rpc请求报文
        {
            buffer->retrieve(4); // 消耗4字节的报文长度
            uint32_t rpcHeaderSize = buffer->readInt32(); // 获取 rpcHeader 的长度，不需要手动转换成主机字节序，readInt32内部已经做过了
            std::string rpcHeaderStr(buffer->retrieveAsString(rpcHeaderSize)); // 获取 rpcHeader ,并转成string
            MyRPC::RpcHeader rpcHeader;// rpcHeader由三部分组成: serviceName, methodName, argvSize。
            if (!rpcHeader.ParseFromString(rpcHeaderStr)) // 反序列化protobuf
            {
                // 输出日志
                std::cout << "ParseFromString() err" << std::endl;

                SendErrorResponse(pConn, MyRPC::RPCResponseError::PARSE_ERROR, "反序列化错误");
                return ;
            }
            
            std::string serviceName(rpcHeader.servicename());  // 获取服务名称
            std::string methodName(rpcHeader.methodname());    // 获取方法名称
            uint32_t argvSize = rpcHeader.argvsize();          // 获取参数所占字节数

            std::string argv(buffer->retrieveAsString(argvSize)); // 获取参数字符串
            
            // 在 m_serviceMap 里面查找指定名称的服务
            auto servicePos = m_serviceMap.find(serviceName);
            if (servicePos == m_serviceMap.end())
            {
                // 输出日志
                std::string msg = "未注册 " + serviceName + " 服务";
                std::cout << msg<< std::endl;

                SendErrorResponse(pConn, MyRPC::RPCResponseError::SERVICE_NOT_FOUND, msg);
                return ;
            }

            // 查找指定名称的方法
            auto methodPos = servicePos->second.m_methodMap.find(methodName);
            if (methodPos == servicePos->second.m_methodMap.end())
            {
                // 输出日志
                std::string msg = "未定义 " + methodName + " 方法";
                std::cout << msg << std::endl;
                
                SendErrorResponse(pConn, MyRPC::RPCResponseError::METHOD_NOT_FOUND, msg);
                return ;
            }

            // 调用指定服务的指定方法
            google::protobuf::Service *pService = servicePos->second.m_pservice; // 获取 service 指针
            const google::protobuf::MethodDescriptor *pMethodDesc = methodPos->second; // 获取 MethodDescriptor指针
            std::unique_ptr<google::protobuf::Message> pRequest(pService->GetRequestPrototype(pMethodDesc).New()); // 生成request
            if (!pRequest->ParseFromString(argv))
            {
                // 输出日志
                std::cout << "ParseFromString() err" << std::endl;

                SendErrorResponse(pConn, MyRPC::RPCResponseError::PARSE_ERROR, "反序列化错误");
                return ;
            }

            std::unique_ptr<google::protobuf::Message> pResponse(pService->GetResponsePrototype(pMethodDesc).New()); // 生成response

            std::unique_ptr<google::protobuf::Closure> done(google::protobuf::NewCallback<RPCProvider, 
                                                                        const muduo::net::TcpConnectionPtr &, 
                                                                        google::protobuf::Message *>
                                                                        (this, &RPCProvider::SendRpcResponse, 
                                                                        pConn, pResponse.get())); // 设置回调函数

            pService->CallMethod(pMethodDesc, nullptr, pRequest.get(), pResponse.get(), done.release()); // 使用done.get()的话会导致资源重复delete

        }
        else if (len >= 64 * 1024 * 1024) // 超过 64M，则关闭连接，防止炸弹
        {
            // 输出日志
            std::cout << "有炸弹！" << std::endl;
            pConn->shutdown();
            return;
        }
        else // 不是一条完整的rpcHeader
        {
            return;
        }
    }
}

// 回调函数，将response发送回客户端
void RPCProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &pConn, google::protobuf::Message *response)
{
    // 定义框架内部的响应类，封装远程函数调用的response数据，以及RPC调用成功还是失败等信息
    MyRPC::RPCResponseWrapper wrapper;
    wrapper.set_success(true);
    wrapper.mutable_error()->set_error_code(MyRPC::RPCResponseError::SUCCESS);
    wrapper.mutable_error()->set_error_message("");

    // 将response 序列化之后，封装到 RPCResponseWrapper 对象里
    std::string responseData;
    if (response->SerializeToString(&responseData))
    {
        wrapper.set_data(responseData);

        // 序列化 RPCResponseWrapper 对象，并发送给框架的客户端
        std::string wrapperData;
        if (wrapper.SerializeToString(&wrapperData))
        {
            muduo::net::Buffer buffer;
            buffer.append(wrapperData);
            pConn->send(&buffer);
        }
        else // 序列化失败，一般来说不会发生
        {
            // 输出日志
        }
    }
    else // 序列化失败，一般来说不会发生
    {
        // 输出日志
    }

    pConn->shutdown();
}

// RPC调用过程中出现问题，导致调用失败，给框架的客户端返回失败信息
void RPCProvider::SendErrorResponse(const muduo::net::TcpConnectionPtr &pConn, int error_code, const std::string &error_msg)
{
    MyRPC::RPCResponseWrapper wrapper;
    wrapper.set_success(false);
    wrapper.mutable_error()->set_error_code(error_code);
    wrapper.mutable_error()->set_error_message(error_msg);

    std::string wrapperData;
    if (wrapper.SerializeToString(&wrapperData))
    {
        muduo::net::Buffer buffer;
        buffer.append(wrapperData);
        pConn->send(&buffer);
    }
    else // 序列化失败，一般不会发生
    {
        // 输出日志
    }

    pConn->shutdown();
}
