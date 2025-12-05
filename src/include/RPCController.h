#pragma once

#include "google/protobuf/service.h"
#include <string>

class RPCController final : public google::protobuf::RpcController
{
public:
    RPCController();
    ~RPCController() = default;
    ////////////////////客户端方法//////////////////////////
    void Reset() override;
    bool Failed() const override;
    std::string ErrorText() const override;
    void StartCancel() override;

    ////////////////////服务端方法//////////////////////////
    void SetFailed(const std::string& reason) override;
    bool IsCanceled() const override;
    void NotifyOnCancel(google::protobuf::Closure* callback) override;
private:
    bool m_failed; // 是否发生错误的标志
    std::string m_errMsg; // 发生错误后的错误信息
};