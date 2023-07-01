cmake_minimum_required(VERSION 3.15)

project(mavpicam)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# set(DEPS_INSTALL_PATH "/home/kapz/px4/MAVSDK/build/linux-armv7/src/mavsdk")

add_definitions("-Wall -Wextra -Werror")

find_package(MAVSDK REQUIRED)

add_executable(mavpicam
    main.cpp
    # ... any other source files
)

target_link_libraries(mavpicam
    MAVSDK::mavsdk             #All apps link against mavsdk library
    # MAVSDK::mavsdk_action      # If action plugin used by app ...
    # MAVSDK::mavsdk_telemetry   #If telemetry plugin used by app ...
    # ... Any other linked libraries
)