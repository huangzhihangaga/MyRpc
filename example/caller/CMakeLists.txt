set(SRC_LIST userserviceConsumer.cpp ../user.pb.cc)
#set(SRC_LIST friendserviceConsumer.cpp ../friend.pb.cc)

add_executable(consumer ${SRC_LIST})
target_link_libraries(consumer mprpc protobuf)
