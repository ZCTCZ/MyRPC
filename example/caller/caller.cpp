#include <iostream>
#include "RPCApplication.h"
#include "user.pb.h"
#include "friend.pb.h"
#include "RPCChannel.h"
#include "RPCController.h"
#include <memory>

int main(int argc, char **argv)
{
    RPCApplication::Init(argc, argv); // 初始化 rpc 框架
    std::unique_ptr<RPCChannel> pChannel = std::make_unique<RPCChannel>(); // 创建 Channel
    RPCTest::UserServiceRpc_Stub stub(pChannel.get()); // 创建 RPC 框架的客户端部分 stub
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 初始化 RegisterRequest
    RPCTest::RegisterRequest registerRequest;
    registerRequest.set_name("cz");
    registerRequest.set_password("zct010601");

    // 初始化 RegisterReponse
    RPCTest::RegisterResponse registerResponse;

    // 初始化RegisterController
    RPCController registerController;

    // 调用远程的 Register 函数
    stub.Register(&registerController, &registerRequest, &registerResponse, nullptr);

    if (registerController.Failed())
    {
        std::cout << "Register Failed:" << registerController.ErrorText() << std::endl;
    }
    else
    {
        if (0 == registerResponse.result().errcode())
        {
            std::cout << "Register Successfully" << std::endl;
        }
        else
        {
            std::cout << "Register Failed:" << registerResponse.result().errmsg() << std::endl;
        }
    }
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 初始化 LoginRequest
    RPCTest::LoginRequest request;
    request.set_name("cz");
    request.set_password("zct010601");

    // 初始化 LoginResponse，作为传出参数
    RPCTest::LoginResponse response;

    // 初始化 LoginController
    RPCController loginController;

    // 通过 stub 调用远程的Login函数
    stub.Login(&loginController, &request, &response, nullptr);

    if (loginController.Failed())
    {
        std::cout << "Login Failed:" << loginController.ErrorText() << std::endl;
    }
    else
    {
        if (0 == response.result().errcode())// 登录成功
        {
            std::cout << "Login Successfully" << std::endl;
        }
        else // 登录失败
        {
            std::cout << "Login Failed:" << response.result().errmsg() << std::endl;
        }
    }
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // 创建RPCChannel,之前的那个Channel已经被UserService服务使用
    std::unique_ptr<RPCChannel> pFriendServiceChannel = std::make_unique<RPCChannel>();

    // 创建FriendService服务的桩类
    RPCTest::FriendServiceRpc_Stub friendServiceStub(pFriendServiceChannel.get());

    // 初始化 GetFriendListRequest
    RPCTest::GetFriendListRequest gflRequest;
    gflRequest.set_id(601);
    
    // 初始化 GetFriendListResponse 作为传出参数
    RPCTest::GetFriendListResponse gflResponse;

    // 初始化 RPCController 控制器对象
    RPCController gflController;
    
    // 通过 friendServiceStub 调用远程函数
    friendServiceStub.GetFriendList(&gflController, &gflRequest, &gflResponse, nullptr);

    if (gflController.Failed())
    {
        std::cout << "获取用户列表失败:" << gflController.ErrorText() << std::endl;
    }
    else
    {
        if (0 == gflResponse.result().errcode())
        {
            int cnt = gflResponse.userinfo_size();
            for (int i=0; i<cnt; ++i)
            {
                std::cout << "index" << i+1 << " "
                << gflResponse.userinfo(i).name() << " "
                <<  gflResponse.userinfo(i).sex() << " "
                <<  gflResponse.userinfo(i).age() << std::endl;
            }
        }
        else
        {
            std::cout << "获取用户列表失败:" << gflResponse.result().errmsg() << std::endl;
        }   
    }

    return 0;
}