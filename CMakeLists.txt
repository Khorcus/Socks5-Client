cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(Socks5-Client LANGUAGES CXX)

add_executable(${PROJECT_NAME}
        src/main.cpp src/kqueue/KQueue.cpp
        src/kqueue/KQueue.hpp src/socks/SocksClient.cpp
        src/socks/SocksClient.hpp src/socket/ClientSocket.cpp
        src/socket/ClientSocket.hpp
        src/utils.hpp src/socks/SocksActions.cpp
        src/socks/SocksActions.hpp
        src/kqueue/Actions.hpp
        src/socket/SocketActions.cpp
        src/socket/SocketActions.hpp
        )
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
set_target_properties(${PROJECT_NAME} PROPERTIES CXX_EXTENSIONS OFF)
