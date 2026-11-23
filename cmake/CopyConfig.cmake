# config/ 의 파일을 실행 파일 옆 config/ 로 복사한다.
# 앱이 그 파일에 설정을 저장하므로, 이미 있는 파일은 덮어쓰지 않는다.
#   cmake -DSOURCE_DIR=<프로젝트>/config -DTARGET_DIR=<exe 폴더>/config -P CopyConfig.cmake
file(GLOB configFiles RELATIVE "${SOURCE_DIR}" "${SOURCE_DIR}/*")
foreach(configFile IN LISTS configFiles)
    if(NOT EXISTS "${TARGET_DIR}/${configFile}")
        file(COPY "${SOURCE_DIR}/${configFile}" DESTINATION "${TARGET_DIR}")
    endif()
endforeach()
