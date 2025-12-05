#pragma once
#include <unordered_map>
#include <string>

class RPCConfig
{
public:
    RPCConfig() = default;
    ~RPCConfig() = default;
    
    // 读取配置文件信息
    void LoadConfigFile(const std::string& filePath);

    // 查询配置文件信息
    std::string Load(const std::string& key) const;
private:
    std::unordered_map<std::string, std::string> m_confmap;
};