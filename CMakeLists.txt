cmake_minimum_required(VERSION 3.20)

project(TCPConn)

add_library(TCPConn SHARED TCPConnImpl.cpp TCPClientImpl.cpp TCPServerImp.cpp)
target_compile_definitions(TCPConn PRIVATE TCPCONN_DLL)

include_directories(${ROOT_DIR}/include)

if (WIN32)
    set_target_properties(TCPConn PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_definitions(-D_WIN32_WINNT=0x0A00)
    set(Boost_NO_WARN_NEW_VERSIONS 1)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREAD ON)
    find_package(Boost REQUIRED system)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(TCPConn PRIVATE ${Boost_LIBRARIES} ws2_32)
elseif (UNIX)
    include_directories(/opt/homebrew/Cellar/boost/1.85.0/include)
endif ()