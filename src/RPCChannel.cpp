#include "RPCChannel.h"
#include "Header.pb.h"
#include "Response.pb.h"
#include "RPCApplication.h"
#include "ZooKeeperUtil.h"
#include <string>
#include <arpa/inet.h>
#include <errno.h>
#include <memory>


void RPCChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                            google::protobuf::RpcController *controller,
                            const google::protobuf::Message *request,
                            google::protobuf::Message *response,
                            google::protobuf::Closure *done)
{
//1.将被调用的函数和参数信息封装成 rpcHeader ==> serviceName + methodName + argvSize

    // 获取服务名称 serviceName
    const google::protobuf::ServiceDescriptor *service = method->service();
    std::string serviceName = static_cast<std::string>(service->name());

    // 获取方法名称 methodName 
    std::string methodName = static_cast<std::string>(method->name());

    // 将 request 序列化成字符串
    std::string requestStr;
    if (!request->SerializeToString(&requestStr))
    {
        // 输出日志
        controller->SetFailed("SerializeToString() err");
        return ;
    }

    // 获取参数长度 argvSize
    uint32_t argvSize = requestStr.size();

    // 将 serviceName、methodName 和 argcSize 封装成Header
    MyRPC::RpcHeader Header;
    Header.set_servicename(serviceName);
    Header.set_methodname(methodName);
    Header.set_argvsize(argvSize);

    // 将 Header 序列化成字符串
    std::string headerStr;
    if (!Header.SerializeToString(&headerStr))
    {
        // 输出日志
        controller->SetFailed("SerializeToString() err");
        return ;
    }

    // 计算headerStr的大小，用大端序存储
    uint32_t headerSize = htonl(headerStr.size());

//2.将headerSize headerStr requestStr 封装成一个发送流 sendStr 发送给框架的服务端RPCProvider
    
    //初始化发送流
    std::string sendStr;

    // 填充 sendStr
    sendStr.insert(0, reinterpret_cast<const char*>(&headerSize), 4); // 添加headerSize
    sendStr += headerStr;
    sendStr += requestStr;

    // 将已经封装好了的 sendStr 发送给框架的服务端
    SendToServer(serviceName, methodName, sendStr, response, controller); // sendStr = headerSize (4字节)+ headerStr + requestStr
}

// 通过网络将sendStr发送给框架的服务端
// 目前是一对一通信
void RPCChannel::SendToServer(const std::string& serviceName, const std::string& methodName, const std::string& sendStr, google::protobuf::Message *response, google::protobuf::RpcController *controller)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == fd)
    {
        // 输出日志
        controller->SetFailed("send err");
        return ;
    }

    // 使用智能指针(自定义删除器)管理通信套接字
    auto del = [](int* p){ if(p) ::close(*p); };
    std::unique_ptr<int, decltype(del)> pfd(new int(fd), del);

    // 从 zooKepper 的服务器上获取 RPC 框架服务端 RPCProvider 的IP和端口号
    ZkClient zk; // 定义 zkClient 对象
    zk.Start();  // 连接 zkServer 服务器
    std::string path("/" + serviceName + "/" + methodName); // 生成查找结点所在的路径: /serviceName/methodName
    std::string data = zk.GetData(path.data()); // 查找指定路径结点的数据
    if (data.empty()) // 指定路径的结点不存在，即未注册所指定的服务或者方法 
    {
        // 输出日志
        controller->SetFailed(path + " Not Exit In ZooKeeperServer");
        return ;
    }

    size_t pos = data.find(':');
    if (pos == std::string::npos)
    {
        // 输出日志
        controller->SetFailed(path + " Is Invalid");
        return ;
    }

    std::string ip(data.begin(), data.begin()+pos);
    std::string port(data.begin()+pos+1, data.end());

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    if (-1 == inet_pton(AF_INET, ip.data(), &servaddr.sin_addr.s_addr))
    {
        // 输出日志
        controller->SetFailed("inet_pton() err");
        return ;
    }
    servaddr.sin_port = htons(stoi(port));

    if (-1 == connect(fd, (struct sockaddr*)&servaddr, sizeof(servaddr)))
    {
        // 输出日志
        controller->SetFailed("connect() err");
        return ;
    }

    // 发送sendStr之前需要添加长度前缀
    std::string str;

    // 用大端序存储待发送的数据的长度
    uint32_t sz = htonl(sendStr.size());
    str.insert(0, reinterpret_cast<const char*>(&sz), 4);
    str += sendStr;

    // 发送 str
    if (-1 == send(fd, str.data(), str.size(), 0))
    {
        // 输出日志
        controller->SetFailed("send() err");
        return ;
    }

    // 阻塞等待 RPCProvider 返回函数调用的结果
    char buf[65535] = {0};
    ssize_t recvLen = recv(fd, buf, sizeof(buf), 0);
    if (-1 == recvLen)
    {
        // 输出日志
        controller->SetFailed("recv() err");
    }
    else if (0 == recvLen)
    {
        // 输出日志
        controller->SetFailed("连接异常断开");
    }
    else
    {
        // 将接收到的数据拷贝到string里
        std::string recvStr(buf, recvLen);
        
        MyRPC::RPCResponseWrapper wrapper;
        if (wrapper.ParseFromString(recvStr))
        {
            if (wrapper.success()) // RPC 调用成功
            {
                // 反序列化
                response->ParseFromString(wrapper.data());
            }
            else // RPC 调用失败
            {
                controller->SetFailed(wrapper.error().error_message());
            }
        }
        else
        {
            controller->SetFailed("ParseFromString() err");
        }
    }
}