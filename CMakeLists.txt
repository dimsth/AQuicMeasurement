cmake_minimum_required(VERSION 3.26.4)
project(AQuicMeasurement)

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

add_executable(q_server ${SOURCES1})

add_executable(q_client ${SOURCES2})
