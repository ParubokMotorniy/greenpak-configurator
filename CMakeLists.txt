cmake_minimum_required(VERSION 3.26)
project(bridge-cli LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

include(GNUInstallDirs)
if(WIN32)
  set(CMAKE_INSTALL_BINDIR ".")
  set(CMAKE_INSTALL_LIBEXECDIR ${CMAKE_INSTALL_BINDIR})
  set(CMAKE_INSTALL_DATAROOTDIR ${CMAKE_INSTALL_BINDIR})
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/Out/${CMAKE_BUILD_TYPE}/${CMAKE_INSTALL_BINDIR}"
)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/Out/${CMAKE_BUILD_TYPE}/${CMAKE_INSTALL_LIBDIR}"
)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/Out/${CMAKE_BUILD_TYPE}/${CMAKE_INSTALL_LIBDIR}"
)

if(CMAKE_CXX_COMPILER_ID EQUAL GNU)
  add_compile_options(-Wall -Wextra)
endif()

set(HEADERS ${CMAKE_CURRENT_LIST_DIR}/include/scriptprocessing.h ${CMAKE_CURRENT_LIST_DIR}/include/serial.h ${CMAKE_CURRENT_LIST_DIR}/common/protocol.h)
set(SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/main.cpp ${CMAKE_CURRENT_LIST_DIR}/src/scriptprocessing.cpp ${CMAKE_CURRENT_LIST_DIR}/src/serial.cpp)

find_package(Boost REQUIRED COMPONENTS system program_options)

add_executable(${PROJECT_NAME} ${HEADERS} ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include PRIVATE ${CMAKE_CURRENT_LIST_DIR}/common)
target_link_libraries(${PROJECT_NAME} Boost::system Boost::program_options)

