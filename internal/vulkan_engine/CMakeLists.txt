cmake_minimum_required(VERSION 3.28)
project(vulkan_engine LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)

add_library(vulkan_engine SHARED)
target_sources(vulkan_engine PRIVATE
    vulkan_engine/vulkan_platform/physical_device_properties.cpp
    vulkan_engine/vulkan_platform/physical_device_requirements_builder.cpp
    vulkan_engine/vulkan_platform/physical_device_requirements.cpp
    vulkan_engine/vulkan_platform/vulkan_instance_properties.cpp
    vulkan_engine/vulkan_platform/vulkan_platform.cpp
    vulkan_engine/vulkan_platform_impl_fmt.cpp
)

find_package(Vulkan REQUIRED)
# add_subdirectory(../../external/fmt-10.2.0)

target_link_libraries(vulkan_engine PUBLIC Vulkan::Vulkan)
target_link_libraries(vulkan_engine PUBLIC fmt)
target_link_libraries(vulkan_engine PRIVATE glfw)

target_include_directories(vulkan_engine INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
