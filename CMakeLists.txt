cmake_minimum_required(VERSION 3.15.0 FATAL_ERROR)

set(CMAKE_SYSTEM_VERSION 10.0.22000.0 CACHE STRING "" FORCE)

project(dokan_mirror C CXX)

list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/cmake")
set(Boost_USE_STATIC_RUNTIME ON)
find_package(Boost REQUIRED COMPONENTS serialization filesystem)

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

################################################################################
# Sub-projects
################################################################################

#set(PROJECT_NAME dokan_mirror)

################################################################################
# Source groups
################################################################################
set(Sources
    "mirror.cpp"
    "filenode.cpp"
    "filenode.h"
    "DbgPrint.h"
)
source_group("Sources" FILES ${Sources})

set(ALL_FILES
    ${Sources}
)

################################################################################
# Target
################################################################################
add_executable(${PROJECT_NAME} ${ALL_FILES})


target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/ )

target_link_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/lib)
# Link with other targets.
target_link_libraries(${PROJECT_NAME} PRIVATE  dokan2 Boost::filesystem Boost::serialization)


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
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/"
        OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/"
        OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        OUTPUT_DIRECTORY_DEBUG   "${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/"
        OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/${CMAKE_VS_PLATFORM_NAME}/$<CONFIG>/"
    )
endif()
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE"
    )
endif()
################################################################################
# MSVC runtime library
################################################################################
get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
    string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
        $<$<CONFIG:Debug>:
            MultiThreadedDebug
        >
        $<$<CONFIG:Release>:
            MultiThreaded
        >
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
        $<$<CONFIG:Debug>:
            MultiThreadedDebug
        >
        $<$<CONFIG:Release>:
            MultiThreaded
        >
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
        $<$<CONFIG:Debug>:
            MultiThreadedDebug
        >
        $<$<CONFIG:Release>:
            MultiThreaded
        >
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
        $<$<CONFIG:Debug>:
            MultiThreadedDebug
        >
        $<$<CONFIG:Release>:
            MultiThreaded
        >
        $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Release>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
    )
endif()
set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

################################################################################
# Include directories
################################################################################
if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../sys"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../sys"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../sys"
    )
elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
    target_include_directories(${PROJECT_NAME} PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/../../sys"
    )
endif()

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
    target_compile_definitions(${PROJECT_NAME} PRIVATE  $<CONFIG:Debug>:"_DEBUG"> $<CONFIG:Release>"NDEBUG">  "WIN32;"        "_CONSOLE;"        "UNICODE;"        "_UNICODE"
    )
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

################################################################################
# Compile and link options
################################################################################
#if(MSVC)
#    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
#        target_compile_options(${PROJECT_NAME} PRIVATE
#            $<$<CONFIG:Debug>:
#                /Od;
#                /W4;
#                /Zi
#            >
#            $<$<CONFIG:Release>:
#                /O2;
#                /Oi;
#                /Gy;
#                /W3;
#                ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT}
#            >
#            /MP;
#            ${DEFAULT_CXX_EXCEPTION_HANDLING};
#            /Y-
#        )
#    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
#        target_compile_options(${PROJECT_NAME} PRIVATE
#            $<$<CONFIG:Debug>:
#                /Od;
#                /W4;
#                /Zi
#            >
#            $<$<CONFIG:Release>:
#                /O2;
#                /Oi;
#                /Gy;
#                /W3;
#                ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT}
#            >
#            /MP;
#            ${DEFAULT_CXX_EXCEPTION_HANDLING};
#            /Y-
#        )
#    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
#        target_compile_options(${PROJECT_NAME} PRIVATE
#            $<$<CONFIG:Debug>:
#                /Od;
#                /W4;
#                /Zi
#            >
#            $<$<CONFIG:Release>:
#                /O2;
#                /Oi;
#                /Gy;
#                /W3;
#                ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT}
#            >
#            /MP;
#            ${DEFAULT_CXX_EXCEPTION_HANDLING};
#            /Y-
#        )
#    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
#        target_compile_options(${PROJECT_NAME} PRIVATE
#            $<$<CONFIG:Debug>:
#                /Od;
#                /W4;
#                /Zi
#            >
#            $<$<CONFIG:Release>:
#                /O2;
#                /Oi;
#                /Gy;
#                /W3;
#                ${DEFAULT_CXX_DEBUG_INFORMATION_FORMAT}
#            >
#            /MP;
#            ${DEFAULT_CXX_EXCEPTION_HANDLING};
#            /Y-
#        )
#    endif()
#    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM")
#        target_link_options(${PROJECT_NAME} PRIVATE
#            $<$<CONFIG:Debug>:
#                /INCREMENTAL
#            >
#            $<$<CONFIG:Release>:
#                /OPT:REF;
#                /OPT:ICF;
#                /INCREMENTAL:NO
#            >
#            /DEBUG;
#            /SUBSYSTEM:CONSOLE
#        )
#    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "ARM64")
#        target_link_options(${PROJECT_NAME} PRIVATE
#            $<$<CONFIG:Debug>:
#                /INCREMENTAL
#            >
#            $<$<CONFIG:Release>:
#                /OPT:REF;
#                /OPT:ICF;
#                /INCREMENTAL:NO
#            >
#            /DEBUG;
#            /SUBSYSTEM:CONSOLE
#        )
#    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
#        target_link_options(${PROJECT_NAME} PRIVATE
#            $<$<CONFIG:Debug>:
#                /INCREMENTAL
#            >
#            $<$<CONFIG:Release>:
#                /OPT:REF;
#                /OPT:ICF;
#                /INCREMENTAL:NO
#            >
#            /DEBUG;
#            /SUBSYSTEM:CONSOLE
#        )
#    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x86")
#        target_link_options(${PROJECT_NAME} PRIVATE
#            $<$<CONFIG:Debug>:
#                /INCREMENTAL
#            >
#            $<$<CONFIG:Release>:
#                /OPT:REF;
#                /OPT:ICF;
#                /INCREMENTAL:NO
#            >
#            /DEBUG;
#            /SUBSYSTEM:CONSOLE
#        )
#    endif()
#endif()

################################################################################
# Dependencies
################################################################################
#add_dependencies(${PROJECT_NAME}
#    dokan
#)



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

