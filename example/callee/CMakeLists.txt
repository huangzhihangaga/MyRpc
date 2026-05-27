set(SRC_LIST userserviceProvider.cpp ../user.pb.cc)
#set(SRC_LIST friendserviceProvider.cpp ../friend.pb.cc)

add_executable(provider ${SRC_LIST})
target_link_libraries(provider mprpc protobuf)