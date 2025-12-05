#include "RPCConfig.h"
#include <fstream>
#include <iostream>
#include <algorithm>

// 删除字符串里开头和结尾的所有空白字符
static std::string &trim(std::string &str)
{
    // 从前往后，找到第一个不是空格的字符，返回它的迭代器位置
    auto pos1 = std::find_if(str.begin(), str.end(), [](char ch) -> bool
                             { return !std::isspace(ch); });

    // 删除str里开头的所有空白字符
    str.erase(str.begin(), pos1);

    // 从后往前，找到第一个不是空格的字符，返回它的迭代器位置
    auto pos2 = std::find_if(str.rbegin(), str.rend(), [](char ch) -> bool
                             { return !std::isspace(ch); });

    // 删除 str里尾部的所有空白字符
    str.erase(pos2.base(), str.end()); // pos2.base() 返回pos2的下一个迭代器位置
    return str;
}

// 读取配置文件信息
void RPCConfig::LoadConfigFile(const std::string &filePath)
{
    std::ifstream input(filePath);
    if (!input.is_open())
    {
        std::cout << "open " << filePath << " failed" << std::endl;
        exit(EXIT_FAILURE);
    }

    std::string line;
    while (std::getline(input, line))
    {
        // 跳过空行和注释行
        if (line.empty() || line.front() == '#')
        {
            continue;
        }
        
        // 在Windows中换行符是\r\n，而在Linux中换行符是\n。std::getline 会自动丢弃换行符
        // 所以，只需要处理Windows中剩下的\r就行
        if (line.back() == '\r')
        {
            continue;
        }

        auto pos = line.find('=');
        if (pos == std::string::npos) // 当前配置行中不存键值对
        {
            continue;
        }
        else
        {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            // 去除key和value中多余的空格
            trim(key);
            trim(value);

            // 将{key, value} 插入 m_confmap 里
            m_confmap.insert(std::make_pair(key, value));
        }
    }
}

// 查询配置文件信息
std::string RPCConfig::Load(const std::string &key) const
{
    auto it = m_confmap.find(key);
    if (it != m_confmap.end())
    {
        return it->second;
    }

    return "";
}