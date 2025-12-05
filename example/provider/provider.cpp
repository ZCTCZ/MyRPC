#include "UserService.h"
#include "FriendService.h"
#include "RPCApplication.h"
#include "RPCProvider.h"

int main(int argc, char **argv)
{
    // 框架的初始化工作
    RPCApplication::Init(argc, argv);

    // RPCProvider 是框架用于发布 RPC 服务的类
    RPCProvider provider;

    // 注册 UserService 服务
    UserService userService;
    provider.NotifyService(&userService);

    // 注册 FriendService 服务
    FriendService friendService;
    provider.NotifyService(&friendService);

    // 启动一个 RPC 服务节点，Run() 函数调用之后，进程进入阻塞状态，等待远程的 RPC 调用请求
    provider.Run();

    return 0;
}