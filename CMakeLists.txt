cmake_minimum_required(VERSION 3.21)
project(air_race_hid_tool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
find_package(hidapi CONFIG REQUIRED)

add_executable(air_race_hid_tool
    src/main.cpp
    src/core/race_packet.h
    src/core/race_packet.cpp
    src/core/race_command.h
    src/core/race_command.cpp
    src/transport/hid_transport.h
    src/transport/hid_transport.cpp
    src/service/race_service.h
    src/service/race_service.cpp
    src/service/serial_number_service.h
    src/service/serial_number_service.cpp
    src/ui/main_window.h
    src/ui/main_window.cpp
)

target_link_libraries(air_race_hid_tool
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        hidapi::hidapi
)

target_include_directories(air_race_hid_tool PRIVATE src)
