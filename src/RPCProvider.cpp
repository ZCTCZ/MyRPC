#include "RPCProvider.h"
#include "RPCApplication.h"
#include "Header.pb.h"
#include "Response.pb.h"
#include "RPCController.h"
#include "ZooKeeperUtil.h"

#include "TcpServer.h"
#include "Log.h"

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

    TcpServer tcpServer(ip, port, 4); // 定义TcpServer 对象，设置服务器ip、端口和子线程个数

    // 设置通信的回调函数
    tcpServer.sethandlemessage([this](std::shared_ptr<Connection> pConn, Buffer *buffer)
                                { OnMessage(pConn, buffer); });

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

    // 启动服务器
    tcpServer.start();
}

/**
 * 客户端框架发送过来的 一条完整的rpc请求 的数据格式：4字节前缀长度 + rpcHeaderSize(4字节) + rpcHeader + 参数
 */

void RPCProvider::OnMessage(std::shared_ptr<Connection> pConn, Buffer* buffer)
{
    while (buffer->readableBytes() > 4)
    {
        uint32_t len = buffer->peekInt32(); // 读取 rpc 请求报文的长度
        if (buffer->readableBytes() >= 4 + len) // 缓冲区里有完整的 rpc 请求报文
        {
            buffer->retrieve(4); // 消费掉 4 字节的报文长度
            uint32_t rpcHeaderSize = buffer->readInt32(); // 获取 rpcHeader 的长度
            std::string rpcHeaderStr = buffer->retrieveAsString(rpcHeaderSize); // 获取 rpcHeader 的字符串形式
            MyRPC::RpcHeader rpcHeader;// rpcHeader由三部分组成: serviceName, methodName, argvSize。
            if (!rpcHeader.ParseFromString(rpcHeaderStr)) // 反序列化 protobuf
            {
                LOG(Log::error) << "ParseFromString() err";
                SendErrorResponse(pConn, MyRPC::RPCResponseError::PARSE_ERROR, "反序列化错误");
                return ;
            }

            std::string serviceName = rpcHeader.servicename(); // 获取服务名称
            std::string methodName = rpcHeader.methodname(); // 获取方法名称
            uint32_t argvSize = rpcHeader.argvsize(); // 获取参数大小

            std::string argvStr = buffer->retrieveAsString(argvSize); // 获取参数

            // 在 m_serviceMap 里面查找服务
            auto servicePos = m_serviceMap.find(serviceName);
            if (servicePos == m_serviceMap.end())
            {
                std::string msg("未注册" + serviceName + "服务");
                LOG(Log::error) << msg;
                SendErrorResponse(pConn, MyRPC::RPCResponseError::SERVICE_NOT_FOUND, msg);
                return ;
            }

            // 在 m_servceMap 里面查找方法
            auto methodPos = servicePos->second.m_methodMap.find(methodName);
            if (methodPos == servicePos->second.m_methodMap.end())
            {
                std::string msg("未定义" + methodName + "方法");
                LOG(Log::error) << msg;
                SendErrorResponse(pConn, MyRPC::RPCResponseError::METHOD_NOT_FOUND, msg);
                return ;
            }

            // 调用指定服务的指定方法
            google::protobuf::Service* pService = servicePos->second.m_pservice; // 获取Service 句柄
            const google::protobuf::MethodDescriptor *pMethodDesc = methodPos->second; // 获取 MethodDescriptor 句柄
            std::unique_ptr<google::protobuf::Message> pRequest(pService->GetRequestPrototype(pMethodDesc).New()); // 获取相应的request
            if (!pRequest->ParseFromString(argvStr)) // 反序列化 protobuf
            {
                LOG(Log::error) << "ParseFromString() err";
                SendErrorResponse(pConn, MyRPC::RPCResponseError::PARSE_ERROR, "反序列化错误");
                return ;
            }

            std::unique_ptr<google::protobuf::Message> pResponse(pService->GetResponsePrototype(pMethodDesc).New()); // 获取相应的response

            google::protobuf::Closure* done = google::protobuf::NewCallback<RPCProvider, 
                                                                            std::shared_ptr<Connection>, 
                                                                            google::protobuf::Message *>
                                            (this, &RPCProvider::SendRpcResponse, pConn, pResponse.get());

            pService->CallMethod(pMethodDesc, nullptr, pRequest.get(), pResponse.get(), done);
        }
        else if (len >= 64 * 1024 * 1024) // 超过 64M，则关闭连接，防止炸弹
        {
            LOG(Log::error) << "有炸弹包!";
            pConn->closeconnection(); // 断开和对端的连接
            return;
        }
        else // 不是一条完整的rpcHeader
        {
            return;
        }
    }
}

// 回调函数，将response发送回客户端
void RPCProvider::SendRpcResponse(std::shared_ptr<Connection> pConn, google::protobuf::Message *response)
{
    MyRPC::RPCResponseWrapper wrapper;
    wrapper.set_success(true);
    wrapper.mutable_error()->set_error_code(MyRPC::RPCResponseError::SUCCESS);
    wrapper.mutable_error()->set_error_message("");

    std::string responseStr;
    if (response->SerializeToString(&responseStr))
    {
        // 将responseData塞进 wrapper的data字段
        wrapper.set_data(responseStr);

        std::string wrapperStr; 
        if (wrapper.SerializeToString(&wrapperStr))
        {
            pConn->send(wrapperStr);
        }
        else // 序列化失败
        {
            LOG(Log::error) << "SerializeToString() err";
        }
    }
    else // 序列化失败
    {
        LOG(Log::error) << "SerializeToString() err";
    }
}


// RPC调用过程中出现问题，导致调用失败，给框架的客户端返回失败信息
void RPCProvider::SendErrorResponse(std::shared_ptr<Connection> pConn, int error_code, const std::string &error_msg)
{
    MyRPC::RPCResponseWrapper wrapper;
    wrapper.set_success(false);
    wrapper.mutable_error()->set_error_code(error_code);
    wrapper.mutable_error()->set_error_message(error_msg);

    std::string wrapperStr;
    if (wrapper.SerializeToString(&wrapperStr))
    {
        pConn->send(wrapperStr);
    }
    else // 序列化失败，一般不会发生
    {
        LOG(Log::warn) << "SerializeToString() err";
    }
}