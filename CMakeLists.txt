cmake_minimum_required(VERSION 3.17.4)

project(AQuicMeasurement)

set(SRC_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/src)
set(SAMPLE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/sample)

include_directories(
    ${CMAKE_SOURCE_DIR}/../msquic/src/inc/
)
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
set(SOURCES0
    src/quic_server.c
)

# Sources for client
set(SOURCES1
    src/quic_client.c
)

# Sources for sample
set(SOURCES2
    sample/sample.c
)

#Server
add_executable(q_server ${SOURCES0})

set_target_properties(q_server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${SRC_OUTPUT_DIRECTORY})

# Include the msquic library header file directory
target_include_directories(q_server PRIVATE ${CMAKE_SOURCE_DIR}/../msquic/src/inc)

#Client
add_executable(q_client ${SOURCES1})

set_target_properties(q_client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${SRC_OUTPUT_DIRECTORY})

# Include the msquic library header file directory
target_include_directories(q_client PRIVATE ${CMAKE_SOURCE_DIR}/../msquic/src/inc)

#==========Sample===========
add_executable(ms_sample ${SOURCES2})

set_target_properties(ms_sample PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${SAMPLE_OUTPUT_DIRECTORY})

target_include_directories(ms_sample PRIVATE ${CMAKE_SOURCE_DIR}/../msquic/src/inc)

# Link against the msquic library
link_directories(${CMAKE_SOURCE_DIR}/../msquic/build/bin/Release)

target_link_libraries(q_server PRIVATE ${CMAKE_SOURCE_DIR}/../msquic/build/bin/Release/libmsquic.so.2.3.0)
target_link_libraries(q_client PRIVATE ${CMAKE_SOURCE_DIR}/../msquic/build/bin/Release/libmsquic.so.2.3.0)
target_link_libraries(ms_sample PRIVATE ${CMAKE_SOURCE_DIR}/../msquic/build/bin/Release/libmsquic.so.2.3.0)

target_compile_options(q_server PRIVATE -g)
target_compile_options(q_client PRIVATE -g)
target_compile_options(ms_sample PRIVATE -g)
