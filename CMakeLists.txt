cmake_minimum_required(VERSION 3.26.4)
project(AQuicMeasurement)

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
