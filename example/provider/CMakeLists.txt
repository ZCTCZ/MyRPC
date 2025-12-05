find_package(protobuf REQUIRED)

add_executable(provider 
                        provider.cpp 
                        UserService.cpp 
                        FriendService.cpp)

set_target_properties(provider PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/)

target_include_directories(provider 
                                PRIVATE
                                    ${PROJECT_BINARY_DIR}/example/proto/
                                    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(provider 
                            PRIVATE
                                rpc 
                                user_proto
                                friend_proto)