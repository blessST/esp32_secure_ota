cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(esp32_secure_ota VERSION 1.0.0)

set(APP_BIN "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bin")

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/esp32_secure_ota-signed.bin ${CMAKE_BINARY_DIR}/metadata.json
    COMMAND ${PYTHON} ${IDF_PATH}/components/esptool_py/esptool/espsecure.py
            sign_data --key ${CMAKE_SOURCE_DIR}/signing_key.pem --version 2
            --output ${CMAKE_BINARY_DIR}/esp32_secure_ota-signed.bin
            ${APP_BIN}
    COMMAND ${PYTHON} ${CMAKE_SOURCE_DIR}/tools/gen_metadata.py
            ${PROJECT_VERSION} esp32_secure_ota-signed.bin
            ${CMAKE_BINARY_DIR}/metadata.json
    DEPENDS app
    VERBATIM
)

add_custom_target(post_build ALL
    DEPENDS ${CMAKE_BINARY_DIR}/esp32_secure_ota-signed.bin ${CMAKE_BINARY_DIR}/metadata.json
)
