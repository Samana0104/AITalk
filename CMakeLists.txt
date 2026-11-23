cmake_minimum_required(VERSION 3.21)

project(AITalk VERSION 0.1.0 LANGUAGES CXX)

# `cmake --install build-release` 는 실행에 필요한 파일만 프로젝트의 dist/ 에 모은다 (exe·DLL·플러그인·QML 모듈·config).
# dist/ 는 배포본이라 Release 빌드만 설치한다.
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_CURRENT_SOURCE_DIR}/dist" CACHE PATH "Install directory" FORCE)
endif()
set(CMAKE_INSTALL_BINDIR ".") # exe 와 DLL 을 dist/ 바로 아래에 둔다

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Widgets 는 트레이 메뉴와 아직 옮기지 않은 설정·진단 다이얼로그가 쓴다.
find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets Svg Quick)
qt_standard_project_setup(REQUIRES 6.8)
set(CMAKE_AUTORCC ON) # res/resources.qrc (qt_standard_project_setup 은 AUTORCC 를 켜지 않는다)

# src/ 아래 폴더(app, core, storage, agent, engine, manager, ui, dialog, util)를
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

# 설정 파일(config/)을 빌드마다 실행 파일 옆 config/ 로 복사한다. 이미 있으면 사용자 설정이므로 두고 넘어간다.
add_custom_target(AITalkConfig ALL
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/config
        -DTARGET_DIR=$<TARGET_FILE_DIR:AITalk>/config
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyConfig.cmake
    COMMENT "Copying config files"
    VERBATIM
)
add_dependencies(AITalkConfig AITalk)

# 설치(dist/): exe → Qt·컴파일러 런타임 DLL, 플러그인, QML 모듈(Qt 배포 도구) → config(이미 있으면 사용자 설정이므로 유지).
# 첫 규칙에서 Release 가 아니면 멈춰 Debug 빌드가 dist/ 를 덮어쓰지 않게 한다.
install(CODE "
    if(NOT CMAKE_INSTALL_CONFIG_NAME STREQUAL \"Release\")
        message(FATAL_ERROR \"dist/ 에는 Release 빌드만 설치한다. 지금 빌드: \${CMAKE_INSTALL_CONFIG_NAME}\\n\"
            \"  cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release\\n\"
            \"  cmake --build build-release\\n\"
            \"  cmake --install build-release\")
    endif()
")
install(TARGETS AITalk
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
qt_generate_deploy_qml_app_script(
    TARGET AITalk
    OUTPUT_SCRIPT deployScript
    NO_TRANSLATIONS
    DEPLOY_TOOL_OPTIONS --skip-plugin-types qmltooling # QML 디버거는 배포본에 필요 없다
)
install(SCRIPT ${deployScript})
install(CODE "
    set(SOURCE_DIR \"${CMAKE_CURRENT_SOURCE_DIR}/config\")
    set(TARGET_DIR \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/config\")
    include(\"${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopyConfig.cmake\")
")

if(MSVC)
    target_compile_options(AITalk PRIVATE /utf-8 /W4 /permissive-)
else()
    target_compile_options(AITalk PRIVATE -Wall -Wextra)
endif()
