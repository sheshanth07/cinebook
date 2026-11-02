cmake_minimum_required(VERSION 3.15)
project(CineBook VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set(CINEBOOK_SOURCES
    src/main.cpp
    src/auth_service.cpp
    src/database.cpp
    src/movie_service.cpp
    src/booking_service.cpp
    src/admin_service.cpp
    src/http_server.cpp
    src/sqlite3.c
)

# Optional Oracle OCCI Support
option(USE_ORACLE "Enable Oracle Database via OCCI" OFF)
if(USE_ORACLE)
    add_definitions(-DUSE_ORACLE)
    # Search for Oracle Instant Client SDK
    if(DEFINED ENV{ORACLE_HOME})
        include_directories($ENV{ORACLE_HOME}/sdk/include)
        link_directories($ENV{ORACLE_HOME}/sdk/lib/msvc)
        set(ORACLE_LIBS oraocci19) # or oraocci21
    endif()
endif()

add_executable(cinebook_server
    src/main.cpp
    src/auth_service.cpp
    src/database.cpp
    src/movie_service.cpp
    src/booking_service.cpp
    src/admin_service.cpp
    src/http_server.cpp
    src/sqlite3.c
)

if(WIN32)
    target_link_libraries(cinebook_server PRIVATE ws2_32)
endif()

if(USE_ORACLE AND DEFINED ORACLE_LIBS)
    target_link_libraries(cinebook_server PRIVATE ${ORACLE_LIBS})
endif()
