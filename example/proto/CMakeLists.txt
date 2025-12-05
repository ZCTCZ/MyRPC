# 指定 CMAKE_PREFIX_PATH 包含 protobuf 安装路径
list(APPEND CMAKE_PREFIX_PATH "/usr/local")

# 使用 CONFIG 模式查找 Protobuf
find_package(Protobuf CONFIG REQUIRED)

# ==============================
# 定义函数：为单个 .proto 文件创建 OBJECT 库
# ==============================
function(add_proto_library PROTO_NAME PROTO_FILE)      
    #1. 创建OBJECT库
    add_library(${PROTO_NAME} OBJECT)

    #2. 生成C++代码
    protobuf_generate(
        TARGET ${PROTO_NAME}
        LANGUAGE cpp
        PROTOS ${PROTO_FILE}
    )

    #3. 连接 protobuf 依赖
    target_link_libraries(${PROTO_NAME}
        PUBLIC
            protobuf::libprotobuf)

    #4. 添加生成目录到 include 路径
    target_include_directories(${PROTO_NAME}
        PUBLIC
            ${CMAKE_CURRENT_BINARY_DIR})
endfunction(add_proto_library PROTO_NAME PROTO_FILE)       

# ==============================
# 批量调用：为每个 .proto 文件生成库
# ==============================
add_proto_library(user_proto ${CMAKE_CURRENT_SOURCE_DIR}/user.proto)
add_proto_library(friend_proto ${CMAKE_CURRENT_SOURCE_DIR}/friend.proto)


##################################################
# # 指定 CMAKE_PREFIX_PATH 包含 protobuf 安装路径
# list(APPEND CMAKE_PREFIX_PATH "/usr/local")

# # 使用 CONFIG 模式查找 Protobuf
# find_package(Protobuf CONFIG REQUIRED)

# # 创建目标库 user_proto
# add_library(user_proto OBJECT)

# # 使用 protobuf_generate 生成代码
# protobuf_generate(
#     TARGET user_proto
#     LANGUAGE cpp
#     PROTOS user.proto
# )

# # 链接 protobuf 库并传播依赖
# target_link_libraries(user_proto
#     PUBLIC
#         protobuf::libprotobuf
# )

# # 添加生成文件目录到 include 路径
# target_include_directories(user_proto
#     PUBLIC
#         ${CMAKE_CURRENT_BINARY_DIR}
# )

###################################################
# find_package(Protobuf REQUIRED) #需要使用 Module 模式

# # 查找当前目录下的所有proto文件
# file(GLOB PROTO_FILES "*.proto")

# # 从proto文件生成cpp文件和h文件
# protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# # 将生成的代码编译成一个静态库
# add_library(user_proto STATIC ${PROTO_SRCS} ${PROTO_HDRS})

# # 为 user_proto 目标添加 protobuf 的包含目录
# target_include_directories(user_proto PUBLIC ${CMAKE_CURRENT_BINARY_DIR})