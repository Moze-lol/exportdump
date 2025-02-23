cmake_minimum_required (VERSION 3.20)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_DEBUG OFF)

if (MSVC)
	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		#clang-cl
	elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
		#cl
		add_compile_options(/Zc:preprocessor) #Fix for __VA_OPT__
		add_compile_options(/external:anglebrackets /external:W0 /external:templates-) #For some reason these dont work under clang-cl
	endif()
endif()

project(exportdump VERSION 1.2.0 LANGUAGES CXX)
file(GLOB SRC_FILES CONFIGURE_DEPENDS  ${PROJECT_SOURCE_DIR}/src/*.cpp  ${PROJECT_SOURCE_DIR}/src/*.hpp) #Yes, GLOB is bad. No, I do not care.

add_executable(${PROJECT_NAME} ${SRC_FILES})
target_link_libraries(${PROJECT_NAME} PRIVATE Dbghelp.lib)

target_compile_definitions(${PROJECT_NAME} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

if (${CMAKE_BUILD_TYPE} MATCHES "Debug")
	set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "exportdumpd")
else()
	set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "exportdump")
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX})