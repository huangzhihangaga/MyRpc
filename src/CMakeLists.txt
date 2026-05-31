#aux_source_directory(. SRC_LIST)
add_definitions(-DTHREADED)
set(SRC_LIST MprpcApplication.cpp
            MprpcConfig.cpp
            rpcheader.pb.cc
            MprpcProvider.cpp
            MprpcChannel.cpp
            MprpcController.cpp
            ServerConnectionPool.cpp
            ConnectionPool.cpp
            LogFile.cpp
            Logger.cpp
            AsyncLogging.cpp
            ZookeeperUtil.cpp)

add_library(mprpc ${SRC_LIST})
target_link_libraries(mprpc muduo_net muduo_base pthread zookeeper_mt)
