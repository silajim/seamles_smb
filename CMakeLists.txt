cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

set(CMAKE_SYSTEM_VERSION 10.0.22000.0 CACHE STRING "" FORCE)

project(seamless_smb  C CXX)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/cmake")

find_package(Qt5 COMPONENTS Core REQUIRED)

#add_subdirectory(rtl)

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

################################################################################
# Set target arch type if empty. Visual studio solution generator provides it.
################################################################################
if(NOT CMAKE_VS_PLATFORM_NAME)
    set(CMAKE_VS_PLATFORM_NAME "x64")
endif()
message("${CMAKE_VS_PLATFORM_NAME} architecture in use")

if(NOT ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM"
     OR "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64"
     OR "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64"
     OR "${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86"))
    message(FATAL_ERROR "${CMAKE_VS_PLATFORM_NAME} arch is not supported!")
endif()

################################################################################
# Global configuration types
################################################################################
set(CMAKE_CONFIGURATION_TYPES
    "Debug"
    "Release"
    CACHE STRING "" FORCE
)



################################################################################
# Use solution folders feature
################################################################################
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
add_subdirectory(seamless_smb_lib)
add_subdirectory(qt-solutions/qtservice/src)
add_subdirectory(seamless_smb_common)
add_subdirectory(seamless_smb_service)
add_subdirectory(seamless_smb_service_tester)
add_subdirectory(seamless_smb_service_controller)

################################################################################
# Sub-projects
################################################################################

#set(PROJECT_NAME dokan_mirror)

################################################################################
# Source groups
################################################################################
set(Sources
    "mirror.cpp"
)
source_group("Sources" FILES ${Sources})

set(ALL_FILES
    ${Sources}
)

#add_library(seamless_smb_lib SHARED)
#target_sources(seamless_smb_lib PUBLIC
#    "nodes.h"
#    "Context.h"
#    "globals.h "
#    "globals.cpp"
#    "filemount.h"
#    "filemount.cpp"
#    "fileops.h"
#    "fileops.cpp"
#    "filenode.cpp"
#    "filenode.h"
#    "filesecurity.h"
#    "filesecurity.cpp"
#    "DbgPrint.h"
#    "DbgPrint.cpp"
#    "WinSec.cpp"
#    "WinSec.h"
#    "securityprocessor.h"
#    "securityprocessor.cpp"
#    )



################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})


target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/ )

target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lib)
# Link with other targets.
target_link_libraries(${PROJECT_NAME} PUBLIC  dokan2)
target_link_libraries(${PROJECT_NAME} PRIVATE seamless_smb_lib)


#use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
#set(ROOT_NAMESPACE mirror)

#set_target_properties(${PROJECT_NAME} PROPERTIES
#    VS_GLOBAL_KEYWORD "Win32Proj"
#)
################################################################################
# Target name
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        TARGET_NAME_DEBUG   "mirror"
        TARGET_NAME_RELEASE "mirror"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        TARGET_NAME_DEBUG   "mirror"
        TARGET_NAME_RELEASE "mirror"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        TARGET_NAME_DEBUG   "mirror"
        TARGET_NAME_RELEASE "mirror"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        TARGET_NAME_DEBUG   "mirror"
        TARGET_NAME_RELEASE "mirror"
    )
endif()
################################################################################
# Output directory
################################################################################
#if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
#    set_target_properties(${PROJECT_NAME} PROPERTIES
#        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/"
#        OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/"
#    )
#elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
#    set_target_properties(${PROJECT_NAME} PROPERTIES
#        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/"
#        OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/"
#    )
#elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
#    set_target_properties(${PROJECT_NAME} PROPERTIES
#        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/"
#        OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/"
#    )
#endif()

    set_target_properties(${PROJECT_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE" )

################################################################################
# MSVC runtime library
################################################################################
get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
#set_property(TARGET ${PROJECT_NAME} PROPERTY  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")


################################################################################
# Compile definitions
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "WIN32;"
        "_CONSOLE;"
        "UNICODE;"
        "_UNICODE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "WIN32;"
        "_CONSOLE;"
        "UNICODE;"
        "_UNICODE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_compile_definitions(${PROJECT_NAME} PRIVATE  $<CONFIG:Debug>:"_DEBUG"> $<CONFIG:Release>"NDEBUG">  "WIN32;"        "_CONSOLE;"        "UNICODE;"        "_UNICODE"       )
#    target_compile_definitions(seamless_smb_lib PRIVATE  $<CONFIG:Debug>:"_DEBUG"> $<CONFIG:Release>"NDEBUG">  "WIN32;"        "_CONSOLE;"        "UNICODE;"        "_UNICODE"       )

elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "_DEBUG"
        ">"
        "$<$<CONFIG:Release>:"
            "NDEBUG"
        ">"
        "WIN32;"
        "_CONSOLE;"
        "UNICODE;"
        "_UNICODE"
    )
endif()



if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "ntdll;"
        "advapi32"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "ntdll;"
        "advapi32"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "ntdll"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    set(ADDITIONAL_LIBRARY_DEPENDENCIES
        "ntdll"
    )
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
    target_link_directories(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "${CMAKE_CURRENT_SOURCE_DIR}/../debug"
        ">"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    target_link_directories(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "${CMAKE_CURRENT_SOURCE_DIR}/../debug"
        ">"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_link_directories(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "${CMAKE_CURRENT_SOURCE_DIR}/../debug"
        ">"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    target_link_directories(${PROJECT_NAME} PRIVATE
        "$<$<CONFIG:Debug>:"
            "${CMAKE_CURRENT_SOURCE_DIR}/../debug"
        ">"
    )
endif()


get_target_property(_qmake_executable Qt5::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")


add_custom_target(RunWideployQt ALL)
add_dependencies(RunWideployQt seamless_smb_service seamless_smb_service_lib seamless_smb_service_controller)

add_custom_command(TARGET RunWideployQt POST_BUILD
    #COMMAND powershell -ExecutionPolicy Bypass -File "$<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/runcommand.ps1>" "${WINDEPLOYQT_EXECUTABLE}" --no-translations "$<SHELL_PATH:$<TARGET_FILE:${atarget}>>"
    # "cd ." command is used to force true command doing anything else in cases windeployqt fails
    COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations "$<SHELL_PATH:$<TARGET_FILE:seamless_smb_service>>" || cd .
    COMMENT "Running windeployqt..."
    )

add_custom_command(TARGET RunWideployQt POST_BUILD
    #COMMAND powershell -ExecutionPolicy Bypass -File "$<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/runcommand.ps1>" "${WINDEPLOYQT_EXECUTABLE}" --no-translations "$<SHELL_PATH:$<TARGET_FILE:${atarget}>>"
    # "cd ." command is used to force true command doing anything else in cases windeployqt fails
    COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations "$<SHELL_PATH:$<TARGET_FILE:seamless_smb_service_lib>>" || cd .
    COMMENT "Running windeployqt..."
    )

add_custom_command(TARGET RunWideployQt POST_BUILD
    #COMMAND powershell -ExecutionPolicy Bypass -File "$<SHELL_PATH:${CMAKE_CURRENT_SOURCE_DIR}/runcommand.ps1>" "${WINDEPLOYQT_EXECUTABLE}" --no-translations "$<SHELL_PATH:$<TARGET_FILE:${atarget}>>"
    # "cd ." command is used to force true command doing anything else in cases windeployqt fails
    COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-translations "$<SHELL_PATH:$<TARGET_FILE:seamless_smb_service_controller>>" --qmldir "$<SHELL_PATH:$<TARGET_PROPERTY:seamless_smb_service_controller,SOURCE_DIR>>" || cd .
    COMMENT "Running windeployqt..."
    )
