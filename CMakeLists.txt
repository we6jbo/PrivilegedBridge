cmake_minimum_required(VERSION 3.21)

project(PrivilegedBridge VERSION 0.6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC OFF)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC OFF)

find_package(Qt6 REQUIRED COMPONENTS Core)

qt_add_executable(PrivilegedBridge
    main.cpp
    actionrunner.cpp
    actionrunner.h
)

target_link_libraries(PrivilegedBridge PRIVATE Qt6::Core)

install(TARGETS PrivilegedBridge
    RUNTIME DESTINATION bin
)

# AppStream metadata
install(FILES org.oneal.PrivilegedBridge.metainfo.xml
        DESTINATION share/metainfo)
