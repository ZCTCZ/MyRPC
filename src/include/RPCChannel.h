#pragma once
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <string>

class RPCChannel final : public google::protobuf::RpcChannel
{
public:
    // 重写google::protobuf::RpcChannel::CallMethod
    void CallMethod(const google::protobuf::MethodDescriptor *method,
                    google::protobuf::RpcController *controller,
                    const google::protobuf::Message *request,
                    google::protobuf::Message *response,
                    google::protobuf::Closure *done) override;


private:
    // 通过网络将sendStr发送给框架的服务端
    void SendToServer(const std::string& serviceName, const std::string& methodName, const std::string& sendStr, google::protobuf::Message *response, google::protobuf::RpcController *controller);
};