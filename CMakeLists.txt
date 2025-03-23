cmake_minimum_required(VERSION 3.13)
project(g13)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-std=c++17")

option(DEBUG "Compile for debugging" OFF)
if(DEBUG)
        add_compile_options(-g)
        add_link_options(-g)
endif()

set(CONTROL_DIR "/tmp" CACHE STRING
        "Absolute directory of default pipe files (/tmp)")
add_compile_definitions(-DCONTROL_DIR="${CONTROL_DIR}")

include_directories(.)

add_executable(pbm2lpbm
        pbm2lpbm.cpp)

add_executable(g13d
        g13.hpp
        g13_action.hpp
        g13_action.cpp
        g13_device.hpp
        g13_device.cpp
        g13_fonts.hpp
        g13_fonts.cpp
        g13_hotplug.hpp
        g13_hotplug.cpp
        g13_keys.hpp
        g13_keys.cpp
        g13_lcd.hpp
        g13_lcd.cpp
        g13_log.hpp
        g13_log.cpp
        g13_main.cpp
        g13_manager.hpp
        g13_manager.cpp
        g13_profile.hpp
        g13_profile.cpp
        g13_stick.hpp
        g13_stick.cpp
        g13_test.py
        helper.hpp
        helper.cpp
        logo.hpp)

add_custom_target(version
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND ${CMAKE_SOURCE_DIR}/GIT-VERSION-GEN
        DEPENDS GIT-VERSION-GEN
        COMMENT "Checking version header ..."
)
add_dependencies(g13d version)
set_property(DIRECTORY PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/GIT-VERSION.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/GIT-VERSION-FILE")

add_executable(runtests
        g13.hpp
        g13_action.hpp
        g13_action.cpp
        g13_device.hpp
        g13_device.cpp
        g13_fonts.hpp
        g13_fonts.cpp
        g13_hotplug.hpp
        g13_hotplug.cpp
        g13_keys.hpp
        g13_keys.cpp
        g13_lcd.hpp
        g13_lcd.cpp
        g13_log.hpp
        g13_log.cpp
        g13_manager.hpp
        g13_manager.cpp
        g13_profile.hpp
        g13_profile.cpp
        g13_stick.hpp
        g13_stick.cpp
        g13_test.py
        helper.hpp
        helper.cpp
        logo.hpp
        testKeys.cpp)

target_link_libraries (g13d usb-1.0 log4cpp evdev)
target_link_libraries (runtests usb-1.0 log4cpp evdev gtest gmock)
