#include <string>
#include <iostream>
#include "user.pb.h"

class UserService final : public RPCTest::UserServiceRpc
{
public:
    //这是一个本地 Register 方法
    bool Register(const std::string &name, const std::string &pwd);

    // 这是一个本地 Login 方法
    bool Login(const std::string &name, const std::string &pwd);

    // 这是一个远程 Register 方法
    void Register(::google::protobuf::RpcController* controller,
                        const ::RPCTest::RegisterRequest* request,
                        ::RPCTest::RegisterResponse* response,
                        ::google::protobuf::Closure* done) override;

    // 这是一个远程 Login 方法
    void Login(google::protobuf::RpcController *controller,
               const ::RPCTest::LoginRequest *request,
               RPCTest::LoginResponse *response,
               google::protobuf::Closure *done) override;
};