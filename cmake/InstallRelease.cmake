# Debug 빌드에서 install 을 실행했을 때 build-release 를 Release 로 빌드해 dist/ 에 설치한다.
set(releaseDir "${SOURCE_DIR}/build-release")

if(NOT EXISTS "${releaseDir}/CMakeCache.txt")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${releaseDir}"
            -G "${GENERATOR}"
            -DCMAKE_BUILD_TYPE=Release
            -DCMAKE_CXX_COMPILER=${CXX_COMPILER}
            -DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}
            -DQt6_DIR=${QT6_DIR}
        RESULT_VARIABLE result
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "build-release 구성 실패 (exit ${result})")
    endif()
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --build "${releaseDir}" --config Release
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "build-release 빌드 실패 (exit ${result})")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" --install "${releaseDir}" --config Release --prefix "${INSTALL_PREFIX}"
    RESULT_VARIABLE result
)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "build-release 설치 실패 (exit ${result})")
endif()
