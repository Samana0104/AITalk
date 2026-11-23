cmake_minimum_required(VERSION 3.21)

project(AITalk VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Widgets 는 트레이 메뉴와 아직 옮기지 않은 설정·진단 다이얼로그가 쓴다.
find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Svg Quick)
qt_standard_project_setup(REQUIRES 6.8)
set(CMAKE_AUTORCC ON) # res/resources.qrc (qt_standard_project_setup 은 AUTORCC 를 켜지 않는다)

# src/ 아래 폴더(app, core, storage, agent, engine, style, ui, dialog, util)를
# 통째로 수집한다. 새 파일을 추가하면 다시 configure만 하면 된다.
file(GLOB_RECURSE AITALK_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h
)
# qml/ 하위 폴더(components, panels, chat)까지 수집한다. 타입 이름은 파일 이름이다.
file(GLOB_RECURSE AITALK_QML_FILES CONFIGURE_DEPENDS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/qml/*.qml
)

qt_add_executable(AITalk
    ${AITALK_SOURCES}
    res/resources.qrc
    res/locale/locale.qrc
)

# QML 화면(qml/)과 QML_ELEMENT 로 등록하는 C++ 타입(src/ui/)을 AITalk 모듈로 묶는다.
qt_add_qml_module(AITalk
    URI AITalk
    QML_FILES ${AITALK_QML_FILES}
)

# qmltyperegistrar 가 QML_ELEMENT 헤더를 파일 이름만으로 include 하므로 src/ui 도 경로에 넣는다.
target_include_directories(AITalk PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
)
target_link_libraries(AITalk PRIVATE Qt6::Widgets Qt6::Svg Qt6::Quick)
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
