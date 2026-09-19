cmake_minimum_required(VERSION 3.16)
project(MonitorSimulation LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

qt_standard_project_setup()

add_executable(MonitorSimulation
    main.cpp
    mainwindow.cpp
    mainwindow.h
)

target_link_libraries(MonitorSimulation PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)