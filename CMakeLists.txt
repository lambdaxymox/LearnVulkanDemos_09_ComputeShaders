cmake_minimum_required(VERSION 3.28)

project("LearnVulkanDemos_09_ComputeShaders" VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

include(NoInSourceBuilds)
CheckNoInSourceBuilds()

find_package(Vulkan REQUIRED)

add_subdirectory(external/glfw-3.4)
add_subdirectory(external/glm-1.0.1)
add_subdirectory(external/fmt-10.2.0)
add_subdirectory(external/stb)
add_subdirectory(external/tiny_obj_loader)

add_subdirectory(compile_glsl_shaders)
add_subdirectory(compile_hlsl_shaders)

add_executable(LearnVulkanDemos_09_ComputeShaders)
target_sources(LearnVulkanDemos_09_ComputeShaders PRIVATE
    src/main.cpp
    src/engine.cpp
    src/engine_impl_fmt.cpp
)
target_link_libraries(LearnVulkanDemos_09_ComputeShaders PRIVATE Vulkan::Vulkan)
target_link_libraries(LearnVulkanDemos_09_ComputeShaders PRIVATE glfw)
target_link_libraries(LearnVulkanDemos_09_ComputeShaders PRIVATE glm)
target_link_libraries(LearnVulkanDemos_09_ComputeShaders PRIVATE fmt)
target_link_libraries(LearnVulkanDemos_09_ComputeShaders PRIVATE stb)
target_link_libraries(LearnVulkanDemos_09_ComputeShaders PRIVATE tiny_obj_loader)
target_link_libraries(LearnVulkanDemos_09_ComputeShaders
    PRIVATE
        compile_glsl_shaders
        compile_hlsl_shaders
)

add_custom_target(run
    COMMAND ${CMAKE_COMMAND} -E env $<TARGET_FILE:LearnVulkanDemos_09_ComputeShaders>
    DEPENDS "LearnVulkanDemos_09_ComputeShaders"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    VERBATIM
)
