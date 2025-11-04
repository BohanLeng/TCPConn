cmake_minimum_required(VERSION 3.20)

project(TCPConn)
set(CMAKE_CXX_STANDARD 20)

if(APPLE)
    set(OPERATING_SYSTEM "macOS")
elseif(UNIX AND NOT APPLE)
    set(OPERATING_SYSTEM "Linux")
elseif(WIN32)
    set(OPERATING_SYSTEM "Windows")
else()
    set(OPERATING_SYSTEM "Other")
endif()

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
    target_link_libraries(${PROJECT_NAME} PRIVATE ${Boost_LIBRARIES} ws2_32 mswsock)
elseif (UNIX)
    find_package(Boost REQUIRED system)
    target_include_directories(${PROJECT_NAME} PRIVATE ${Boost_INCLUDE_DIRS})
endif ()

# pybind for python clients
find_package(pybind11 REQUIRED)
pybind11_add_module(tcpconn_py MODULE pybind_module.cpp)
# Link against the core TCPConn library
if (WIN32)
    target_link_libraries(tcpconn_py PRIVATE TCPConn ${Boost_LIBRARIES} ws2_32)
else()
    target_link_libraries(tcpconn_py PRIVATE TCPConn ${Boost_LIBRARIES})
endif()

target_include_directories(tcpconn_py PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})