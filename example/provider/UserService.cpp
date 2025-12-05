#include "UserService.h"

// 这是一个本地 Register 方法
bool UserService::Register(const std::string &name, const std::string &pwd)
{
    // 具体的注册业务
    return true;
}

// 这是一个本地 Login 方法
bool UserService::Login(const std::string &name, const std::string &pwd)
{
    // 具体的登录业务
    return true;
}

// 这是一个远程 Register 方法
void UserService::Register(::google::protobuf::RpcController *controller,
                           const ::RPCTest::RegisterRequest *request,
                           ::RPCTest::RegisterResponse *response,
                           ::google::protobuf::Closure *done)
{
    // 获取调用函数的参数
    std::string name = request->name();
    std::string pwd = request->password();

    // 调用本地的 Register 方法
    bool registerResult = Register(name ,pwd);

    // 根据调用结果设置 response
    RPCTest::ResultCode* resultCode = response->mutable_result();
    if (registerResult) // 注册成功
    {
        resultCode->set_errcode(0);
        resultCode->set_errmsg("");
    }
    else // 注册失败
    {
        resultCode->set_errcode(1);
        resultCode->set_errmsg("账号已存在");
    }
    response->set_success(registerResult);

    // 执行回调函数
    done->Run();
}

// 这是一个远程方法
void UserService::Login(google::protobuf::RpcController *controller,
                        const RPCTest::LoginRequest *request,
                        RPCTest::LoginResponse *response,
                        google::protobuf::Closure *done)
{
    std::string name = request->name();
    std::string password = request->password();

    // 调用本地的 Login 方法
    bool loginResult = Login(name, password);

    // 填写响应消息
    auto resultCode = response->mutable_result();
    if (loginResult)
    {
        resultCode->set_errcode(0);
        resultCode->set_errmsg("");
    }
    else
    {
        resultCode->set_errcode(1);
        resultCode->set_errmsg("账号或者密码错误");
    }
    response->set_success(loginResult);

    // 执行回调函数，将 result 序列化并通过网络模块发送给 caller 端
    done->Run();
}