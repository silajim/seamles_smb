include(GenerateExportHeader)

set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS serialization filesystem)

add_library(seamless_smb_lib SHARED)
target_sources(seamless_smb_lib PRIVATE
    "nodes.h"
    "Context.h"
    "include/globals.h "
    "globals.cpp"
    "include/filemount.h"
    "filemount.cpp"
    "fileops.h"
    "fileops.cpp"
    "filenode.cpp"
    "filenode.h"
    "filesecurity.h"
    "filesecurity.cpp"
    "include/DbgPrint.h"
    "DbgPrint.cpp"
    "WinSec.cpp"
    "WinSec.h"
    "securityprocessor.h"
    "securityprocessor.cpp"
    )


target_include_directories(seamless_smb_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include/ ${CMAKE_CURRENT_SOURCE_DIR}/include)

target_link_directories(seamless_smb_lib PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../lib)
# Link with other targets.
target_link_libraries(seamless_smb_lib PRIVATE dokan2 Boost::filesystem Boost::serialization)

set_target_properties(seamless_smb_lib PROPERTIES  INTERPROCEDURAL_OPTIMIZATION_RELEASE "TRUE" )
#set_property(TARGET seamless_smb_lib PROPERTY  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_definitions(seamless_smb_lib PRIVATE  $<CONFIG:Debug>:"_DEBUG"> $<CONFIG:Release>"NDEBUG">  "WIN32;"        "_CONSOLE;"        "UNICODE;"        "_UNICODE"       )

generate_export_header(seamless_smb_lib)

target_include_directories(seamless_smb_lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
