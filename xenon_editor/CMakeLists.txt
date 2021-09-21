cmake_minimum_required(VERSION 3.10)

project(xenon_editor CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ImGui CMake
add_library(imgui
    libs/imgui/imgui.cpp
    libs/imgui/imgui_draw.cpp
    libs/imgui/imgui_widgets.cpp
    libs/imgui/imgui_demo.cpp
    libs/imgui/imgui_tables.cpp
    libs/imgui/backends/imgui_impl_glfw.cpp
    libs/imgui/backends/imgui_impl_opengl3.cpp
    libs/imgui/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imgui PUBLIC libs/imgui/)
target_link_libraries(imgui PUBLIC glfw)

# ImGuizmo CMake
add_library(imguizmo
    libs/ImGuizmo/ImGuizmo.cpp
)
target_include_directories(imguizmo PUBLIC libs/ImGuizmo/)
target_link_libraries(imguizmo PUBLIC imgui)


add_executable(xenon_editor "src/main.cpp" "src/orbit_camera.h" "src/orbit_camera.cpp"   "src/ui_elements.h" "src/ui_elements.cpp")
target_link_libraries(xenon_editor PUBLIC xenon)
target_link_libraries(xenon_editor PUBLIC imgui)
target_link_libraries(xenon_editor PUBLIC imguizmo)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${CMAKE_CURRENT_LIST_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    $<TARGET_FILE_DIR:xenon>/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets)
