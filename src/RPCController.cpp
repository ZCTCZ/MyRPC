#include "RPCController.h"

RPCController::RPCController()
:m_failed(false), m_errMsg("")
{
}

// 将 RpcController 重置为初始状态以便复用。只能在没有 RPC 进行时调用。
void RPCController::Reset()
{
    m_failed = false;
    m_errMsg.clear();
}

// 判断 RPC 调用是否失败。必须在调用完成后才能调用此方法
bool RPCController::Failed() const
{
    return m_failed;
}

// 当 RPCController::Failed() 返回值为 true 时，通过该函数获取错误信息
std::string RPCController::ErrorText() const
{
    return m_errMsg;
}

// 取消 RPC 调用。RPC 系统可能会立即取消、延迟取消或者不取消
void RPCController::StartCancel()
{
}

// 设置调用失败及失败原因，错误信息会反映在客户端的 ErrorText() 中
void RPCController::SetFailed(const std::string& reason)
{
    m_failed = true;
    m_errMsg = reason;
}

// 检查客户端是否已取消请求。如果是，则服务器可以放弃响应
bool RPCController::IsCanceled() const
{
    return false;
}

// 注册一个回调函数，在 RPC 被取消时调用，每个请求只能调用一次
void RPCController::NotifyOnCancel(google::protobuf::Closure* callback)
{
}



