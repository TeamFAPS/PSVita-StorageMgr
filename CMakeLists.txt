cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

set(SHORT_NAME storagemgr_installer)
project(${SHORT_NAME})
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "storagemgr")
set(VITA_TITLEID  "MLCL00006")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -fno-builtin-printf")
set(VITA_MAKE_FSELF_FLAGS "${VITA_MAKE_FSELF_FLAGS} -a 0x2800000000000001")

add_subdirectory(kernel)

add_executable(${SHORT_NAME}
  main.c
  debug_screen.c
  debug_screen_font.c
)

target_link_libraries(${SHORT_NAME}
  SceDisplay_stub
  SceCtrl_stub
  SceProcessmgr_stub
  ScePower_stub
  SceRegistryMgr_stub
  SceAppMgr_stub
  SceVshBridge_stub
)

vita_create_self(${SHORT_NAME}.self ${SHORT_NAME} UNSAFE)

vita_create_vpk(${SHORT_NAME}.vpk ${VITA_TITLEID} ${SHORT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE ${CMAKE_BINARY_DIR}/kernel/storagemgr.skprx storagemgr.skprx
)

add_dependencies(${SHORT_NAME}.vpk storagemgr.skprx)
