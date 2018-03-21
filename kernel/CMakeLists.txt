cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(storagemgr)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -Wno-unused-variable -Wno-implicit-function-declaration -O3 -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

add_executable(storagemgr
  main.c
)

target_link_libraries(storagemgr
  taihenForKernel_stub
  taihenModuleUtils_stub
  SceSblACMgrForDriver_stub
  ScePowerForDriver_stub
  SceIofilemgrForDriver_stub
  SceSysclibForDriver_stub
  SceModulemgrForKernel_stub
  SceModulemgrForDriver_stub
  SceThreadmgrForDriver_stub
  SceKernelSuspendForDriver_stub
)

vita_create_self(storagemgr.skprx storagemgr CONFIG exports.yml UNSAFE)

vita_create_stubs(stubs storagemgr ${CMAKE_SOURCE_DIR}/kernel/exports.yml KERNEL)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/stubs/
  DESTINATION lib
  FILES_MATCHING PATTERN "*.a"
)