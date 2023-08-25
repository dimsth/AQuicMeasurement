cmake_minimum_required(VERSION 3.17.4)

project(AQuicMeasurement)

set(SRC_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/src)
set(SAMPLE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/sample)

include_directories(
    ${CMAKE_SOURCE_DIR}/../msquic/src/inc/
)

# Set the build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Remove all files from the build folder
file(GLOB files_to_remove ${CMAKE_BINARY_DIR}/*)
foreach(file ${files_to_remove})
    if(IS_DIRECTORY ${file})
        file(REMOVE_RECURSE ${file})
    else()
        file(REMOVE ${file})
    endif()
endforeach()

# Sources for quic w/ stream
set(SOURCES0
    src/quic_w_stream.c
)

# Sources for quic w/ datagram
set(SOURCES1
  src/quic_w_datag.c
)

# Sources for sample
set(SOURCES2
    ms_sample/ms_sample.c
)

# Sources for TCP/IP
set(SOURCES3
    src/tcp_conn.c
)

#===========Stream==========
add_executable(q_stream ${SOURCES0})

set_target_properties(q_stream PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${SRC_OUTPUT_DIRECTORY})

# Include the msquic library header file directory
target_include_directories(q_stream PRIVATE ${CMAKE_SOURCE_DIR}/../msquic/src/inc)

#==========Datagram==========
add_executable(q_datag ${SOURCES1})

set_target_properties(q_datag PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${SRC_OUTPUT_DIRECTORY})

# Include the msquic library header file directory
target_include_directories(q_datag PRIVATE ${CMAKE_SOURCE_DIR}/../msquic/src/inc)

#==========Sample===========
add_executable(ms_sample ${SOURCES2})

set_target_properties(ms_sample PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${SAMPLE_OUTPUT_DIRECTORY})

target_include_directories(ms_sample PRIVATE ${CMAKE_SOURCE_DIR}/../msquic/src/inc)

#==========TCP/IP===========
add_executable(tcp_conn ${SOURCES3})

set_target_properties(tcp_conn PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${SRC_OUTPUT_DIRECTORY})

# Link against the msquic library
link_directories(${CMAKE_SOURCE_DIR}/../msquic/build/bin/Release)

target_link_libraries(q_stream PRIVATE ${CMAKE_SOURCE_DIR}/../msquic/build/bin/Release/libmsquic.so.2.3.0 -fsanitize=address -lasan)
target_link_libraries(q_datag PRIVATE ${CMAKE_SOURCE_DIR}/../msquic/build/bin/Release/libmsquic.so.2.3.0 -fsanitize=address -lasan)
target_link_libraries(ms_sample PRIVATE ${CMAKE_SOURCE_DIR}/../msquic/build/bin/Release/libmsquic.so.2.3.0 -fsanitize=address -lasan)
target_link_libraries(tcp_conn PRIVATE -fsanitize=address -lasan)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  target_compile_options(q_stream PRIVATE -funit-at-a-time -O2 -finline-functions -DNDEBUG)
  target_compile_options(q_datag PRIVATE -funit-at-a-time -O2 -finline-functions -DNDEBUG)
  target_compile_options(ms_sample PRIVATE -funit-at-a-time -O2 -finline-functions -DNDEBUG)
  target_compile_options(tcp_conn PRIVATE -funit-at-a-time -O2 -finline-functions -DNDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Sanitizer")
  target_compile_options(q_stream PRIVATE -g -Wall -Wextra -pedantic -fsanitize=address -fno-omit-frame-pointer)
  target_compile_options(q_datag PRIVATE -g -Wall -Wextra -pedantic -fsanitize=address -fno-omit-frame-pointer)
  target_compile_options(ms_sample PRIVATE -g -Wall -Wextra -pedantic -fsanitize=address -fno-omit-frame-pointer)
  target_compile_options(tcp_conn PRIVATE -g -Wall -Wextra -pedantic -fsanitize=address -fno-omit-frame-pointer)
else()
  target_compile_options(q_stream PRIVATE -g -Wall -Wextra -pedantic)
  target_compile_options(q_datag PRIVATE -g -Wall -Wextra -pedantic)
  target_compile_options(ms_sample PRIVATE -g -Wall -Wextra -pedantic)
  target_compile_options(tcp_conn PRIVATE -g -Wall -Wextra -pedantic)
endif()
