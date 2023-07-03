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


#Server
add_executable(q_server ${SOURCES1})

# Include the ngtcp2 library header file directory
target_include_directories(q_server PRIVATE /usr/local/include)

# Link against the ngtcp2 library
target_link_libraries(q_server PRIVATE ngtcp2)

target_compile_options(q_server PRIVATE -Wall -Wextra -pedantic)

#GDB
add_executable(gdb_server ${SOURCES1})

# Include the ngtcp2 library header file directory
target_include_directories(gdb_server PRIVATE /usr/local/include)

# Link against the ngtcp2 library
target_link_libraries(gdb_server PRIVATE ngtcp2)

target_compile_options(gdb_server PRIVATE -Wall -Wextra -pedantic -g)

#Client
add_executable(q_client ${SOURCES2})

# Include the ngtcp2 library header file directory
target_include_directories(q_client PRIVATE /usr/local/include)

# Link against the ngtcp2 library
target_link_libraries(q_client PRIVATE ngtcp2)

target_compile_options(q_client PRIVATE -Wall -Wextra -pedantic)

#GDB
add_executable(gdb_client ${SOURCES2})

# Include the ngtcp2 library header file directory
target_include_directories(gdb_client PRIVATE /usr/local/include)

# Link against the ngtcp2 library
target_link_libraries(gdb_client PRIVATE ngtcp2)

target_compile_options(gdb_client PRIVATE -Wall -Wextra -pedantic -g)
