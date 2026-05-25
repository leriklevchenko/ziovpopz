cmake_minimum_required(VERSION 3.20)

cmake_policy(SET CMP0091 NEW)

project(TrayBackgroundApp LANGUAGES CXX)

add_executable(TrayBackgroundApp WIN32
    src/main.cpp
)

target_compile_features(TrayBackgroundApp PRIVATE cxx_std_17)
target_compile_definitions(TrayBackgroundApp PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(TrayBackgroundApp PRIVATE
    Advapi32
    Shell32
)

if(MSVC)
    target_compile_options(TrayBackgroundApp PRIVATE /W4 /permissive- /utf-8)
    set_property(TARGET TrayBackgroundApp PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
    )
else()
    target_compile_options(TrayBackgroundApp PRIVATE -Wall -Wextra -Wpedantic)
endif()
