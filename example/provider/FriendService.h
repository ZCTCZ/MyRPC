#include "friend.pb.h"
#include <string>
#include <vector>

class FriendService final : public RPCTest::FriendServiceRpc
{
public:
    // 远程 GetFriendList 方法
    void GetFriendList(::google::protobuf::RpcController *controller,
                       const ::RPCTest::GetFriendListRequest *request,
                       ::RPCTest::GetFriendListResponse *response,
                       ::google::protobuf::Closure *done) override;
private:
    struct UserInfo
    {
        std::string name;
        std::string sex;
        uint32_t age;

        UserInfo(const std::string &Name, const std::string &Sex, uint32_t Age)
        : name(Name), sex(Sex), age(Age) {}
    };

    // 本地 GetFriendList 方法
    std::vector<UserInfo> GetFriendList(uint32_t id) const;
};