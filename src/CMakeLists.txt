

add_library(rpc SHARED  RPCApplication.cpp
                        RPCProvider.cpp
                        RPCConfig.cpp
                        RPCChannel.cpp
                        RPCController.cpp
                        ZooKeeperUtil.cpp)

# 指定 CMAKE_PREFIX_PATH 包含 protobuf 安装路径
list(APPEND CMAKE_PREFIX_PATH "/usr/local")

# 使用 CONFIG 模式查找 Protobuf
find_package(protobuf CONFIG REQUIRED)

# 查找当前目录下的所有proto文件
set(PROTO_FILES 
            Header.proto 
            Response.proto)
# file(GLOB PROTO_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.proto")

# 使用 protobuf_generate 生成代码
protobuf_generate(
    TARGET rpc
    LANGUAGE cpp
    PROTOS ${PROTO_FILES}
)

# 指定链接需要的库
target_link_libraries(rpc 
                        PUBLIC 
                            muduo_net 
                            muduo_base 
                            protobuf::libprotobuf
                            zookeeper
                            hashtable
                            ssl 
                            crypto 
                            sasl2)

# 指定生成动态库所需要的头文件，使用PUBLIC限定符，若通过target_link_libraries连接库，会自动获取到这些头文件所在的路径
target_include_directories(rpc 
                            PUBLIC 
                                ${CMAKE_CURRENT_SOURCE_DIR}/include/
                            PRIVATE
                                ${CMAKE_CURRENT_BINARY_DIR})

target_compile_definitions(rpc PRIVATE THREADED)

# 设置生成的动态库属性
set_target_properties(rpc PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib) #指定输出路径
