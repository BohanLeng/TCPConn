cmake_minimum_required(VERSION 3.20)

project(TCPConn)

add_library(${PROJECT_NAME} SHARED TCPConnImpl.cpp TCPClientImpl.cpp TCPServerImpl.cpp TCPRawMsgSenderImpl.cpp)
target_compile_definitions(${PROJECT_NAME} PRIVATE TCPCONN_DLL)

target_include_directories(${PROJECT_NAME} PRIVATE ${ROOT_DIR}/include)

if (WIN32)
    set_target_properties(TCPConn PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_definitions(-D_WIN32_WINNT=0x0A00)
    set(Boost_NO_WARN_NEW_VERSIONS 1)
    set(Boost_USE_STATIC_LIBS ON)
    set(Boost_USE_MULTITHREAD ON)
    find_package(Boost REQUIRED system)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES} ws2_32)
elseif (UNIX)
    find_package(Boost REQUIRED system)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
endif ()