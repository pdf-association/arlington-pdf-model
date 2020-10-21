cmake_minimum_required (VERSION 3.10)
SET (CMAKE_CXX_STANDARD 17)
SET (CMAKE_CXX_STANDARD_REQUIRED ON)
SET (CMAKE_REQUIRED_LIBRARIES stdc++fs)

PROJECT(TestGrammar)

set(SOURCES 
    src/CheckGrammar.cpp
	src/CheckDVA.cpp
    src/GrammarFile.cpp
    src/ParseObjects.cpp
    src/Utils.cpp
	)

if (WIN32)
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY 
            ${CMAKE_SOURCE_DIR}/bin/x86)
    else()
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY 
            ${CMAKE_SOURCE_DIR}/bin/x64)
    endif()
elseif(APPLE)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY 
		${CMAKE_SOURCE_DIR}/bin/darwin)
elseif(UNIX)
   set(CMAKE_RUNTIME_OUTPUT_DIRECTORY 
        ${CMAKE_SOURCE_DIR}/bin/linux) 
endif()

add_executable(TestGrammar
    src/Main.cpp
	${SOURCES}
	)
if(APPLE)
elseif (UNIX)
    target_link_libraries(TestGrammar dl stdc++fs)   
endif()
