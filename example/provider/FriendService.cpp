#include "FriendService.h"

// 本地 GetFriendList 方法
std::vector<FriendService::UserInfo> FriendService::GetFriendList(uint32_t id) const
{
    // 具体业务逻辑......
    std::vector<FriendService::UserInfo> vec;
    vec.emplace_back(FriendService::UserInfo("灰太狼", "男", 23));
    vec.emplace_back(FriendService::UserInfo("猪猪侠", "男", 21));
    vec.emplace_back(FriendService::UserInfo("光头强", "男", 30));

    return vec;
}

// 远程 GetFriendList 方法
void FriendService::GetFriendList(::google::protobuf::RpcController *controller,
                                  const ::RPCTest::GetFriendListRequest *request,
                                  ::RPCTest::GetFriendListResponse *response,
                                  ::google::protobuf::Closure *done)
{
    // 获取参数
    uint32_t id = request->id();

    // 调用本地函数
    std::vector<FriendService::UserInfo> vecs = std::move(GetFriendList(id));

    // 将结果封装到response里面
    if (!vecs.empty())
    {
        response->mutable_result()->set_errcode(0);
        response->mutable_result()->set_errmsg("");
        response->set_success(true);
    }
    else
    {
        response->mutable_result()->set_errcode(1);
        response->mutable_result()->set_errmsg("该用户不存在");
        response->set_success(false);
    }

    for (const auto &info : vecs)
    {
        auto pInfo = response->add_userinfo();
        pInfo->set_name(info.name);
        pInfo->set_sex(info.sex);
        pInfo->set_age(info.age);
    }

    // 执行回调
    done->Run();
}