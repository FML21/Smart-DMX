cmake_minimum_required(VERSION 3.16)
if(NOT DEFINED ENV{ESP_MATTER_PATH})
    message(FATAL_ERROR "Export the pinned ESP-Matter SDK before configuring this project")
endif()
set(ESP_MATTER_PATH $ENV{ESP_MATTER_PATH})
set(MATTER_SDK_PATH ${ESP_MATTER_PATH}/connectedhomeip/connectedhomeip)
set(PROJECT_VER "0.2.0")
set(PROJECT_VER_NUMBER 2)
set(EXTRA_COMPONENT_DIRS
    "${MATTER_SDK_PATH}/config/esp32/components"
    "${ESP_MATTER_PATH}/components")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(studio_matter_dmx)
idf_build_set_property(CXX_COMPILE_OPTIONS "-std=gnu++17;-Os;-DCHIP_HAVE_CONFIG_H;-Wno-overloaded-virtual" APPEND)
