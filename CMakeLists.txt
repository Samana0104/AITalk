cmake_minimum_required(VERSION 3.21)

project(AITalk VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Svg)
qt_standard_project_setup()
set(CMAKE_AUTORCC ON) # res/resources.qrc (qt_standard_project_setup 은 AUTORCC 를 켜지 않는다)

# src/ 아래 폴더(app, core, storage, agent, engine, style, component, widget, panel, dialog, window)를
# 통째로 수집한다. 새 파일을 추가하면 다시 configure만 하면 된다.
file(GLOB_RECURSE AITALK_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)

qt_add_executable(AITalk
    ${AITALK_SOURCES}
    res/resources.qrc
    locale/locale.qrc
)

target_include_directories(AITalk PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(AITalk PRIVATE Qt6::Widgets Qt6::Svg)
target_compile_definitions(AITalk PRIVATE AITALK_VERSION="${PROJECT_VERSION}")

set_target_properties(AITalk PROPERTIES
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)

if(MSVC)
    target_compile_options(AITalk PRIVATE /utf-8 /W4 /permissive-)
else()
    target_compile_options(AITalk PRIVATE -Wall -Wextra)
endif()
