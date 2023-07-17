cmake_minimum_required(VERSION 3.17.4)
project(AQuicMeasurement)

include(FetchContent)

# Remove all files from the build folder
file(GLOB files_to_remove ${CMAKE_BINARY_DIR}/*)
foreach(file ${files_to_remove})
    if(IS_DIRECTORY ${file})
        file(REMOVE_RECURSE ${file})
    else()
        file(REMOVE ${file})
    endif()
endforeach()

# Sources for server
set(SOURCES1
    src/quic_server.c
)

# Sources for client
set(SOURCES2
    src/quic_client.c
)


#Server
add_executable(q_server ${SOURCES1})

# Include the msquic library header file directory
target_include_directories(q_server PRIVATE /usr/local/include/)

#Client
add_executable(q_client ${SOURCES2})

# Include the msquic library header file directory
target_include_directories(q_client PRIVATE /usr/local/include/)

FetchContent_GetProperties(msquic)

FetchContent_Declare(
  msquic
  GIT_REPOSITORY https://github.com/microsoft/msquic.git
  GIT_TAG main
  GIT_PROGRESS TRUE)

if(NOT msquic_POPULATED)
  FetchContent_Populate(msquic)
  add_subdirectory(${msquic_SOURCE_DIR} ${msquic_BINARY_DIR})
  FetchContent_MakeAvailable(msquic)
endif()

FetchContent_MakeAvailable(msquic)

# Link against the msquic library
target_link_libraries(q_server PRIVATE msquic)
target_link_libraries(q_client PRIVATE msquic)

target_compile_options(q_server PRIVATE -g)
target_compile_options(q_client PRIVATE -g)
