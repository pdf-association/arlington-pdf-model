cmake_minimum_required (VERSION 3.10)
SET (CMAKE_CXX_STANDARD 17)
SET (CMAKE_CXX_STANDARD_REQUIRED ON)
SET (CMAKE_REQUIRED_LIBRARIES stdc++fs)
SET (CMAKE_DEBUG_POSTFIX _d)

PROJECT(TestGrammar)

set(SOURCES
    src/ArlingtonPDFShimPDFix.cpp
    src/ArlingtonTSVGrammarFile.cpp
    src/CheckDVA.cpp
    src/CheckGrammar.cpp
    src/ParseObjects.cpp
    src/Utils.cpp
    src/Sarge/src/sarge.cpp
    )

if (WIN32)
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/x86)
    else()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/x64)
    endif()
elseif(APPLE)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/darwin)
elseif(UNIX)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/linux)
endif()

add_executable(TestGrammar
    src/Main.cpp
    ${SOURCES}
    )

set_target_properties(TestGrammar PROPERTIES DEBUG_POSTFIX ${CMAKE_DEBUG_POSTFIX})

if (UNIX)
    target_link_libraries(TestGrammar dl stdc++fs)
endif()
