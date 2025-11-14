cmake_minimum_required (VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project(stl)
option(STL_HEADER_ONLY "Build as header-only" OFF)

if (STL_HEADER_ONLY)
    add_library(stl INTERFACE)
    target_include_directories(stl INTERFACE include)
else()
    add_library(stl "stl.cpp")
    target_include_directories(stl PUBLIC include)
    target_link_libraries(stl PUBLIC dbghelp)
    target_compile_features(stl PUBLIC cxx_std_20)
endif()

if (CMAKE_VERSION VERSION_GREATER 3.12)
	set_property(TARGET stl PROPERTY CXX_STANDARD 20)
endif()
