add_executable(caller caller.cpp)

target_include_directories(caller PRIVATE 
                                    ${PROJECT_BINARY_DIR}/example/proto/)

target_link_libraries(caller PRIVATE 
                                rpc
                                user_proto
                                friend_proto)

set_target_properties(caller PROPERTIES 
                                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/)