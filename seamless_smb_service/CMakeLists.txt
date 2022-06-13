project(seamless_smb_service)

add_executable(seamless_smb_service)
add_library(seamless_smb_service_lib STATIC)
target_sources(seamless_smb_service PRIVATE "main.cpp")
target_sources(seamless_smb_service_lib PRIVATE "seamless_smb_service.h" "seamless_smb_service.cpp" "daemon.h" "daemon.cpp" "logger.h" "logger.cpp")

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set_target_properties(seamless_smb_service PROPERTIES AUTOMOC TRUE)
set_target_properties(seamless_smb_service PROPERTIES AUTOUIC TRUE)
set_target_properties(seamless_smb_service PROPERTIES AUTORCC TRUE)


set_target_properties(seamless_smb_service_lib PROPERTIES AUTOMOC TRUE)
set_target_properties(seamless_smb_service_lib PROPERTIES AUTOUIC TRUE)
set_target_properties(seamless_smb_service_lib PROPERTIES AUTORCC TRUE)

find_package(Qt5 COMPONENTS Core Network REQUIRED)

target_include_directories(seamless_smb_service_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../include/ ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(seamless_smb_service_lib PUBLIC Qt5::Core Qt5::Network QtService seamless_smb_common seamless_smb_lib dokan2 user32)

target_link_libraries(seamless_smb_service PRIVATE seamless_smb_service_lib )

target_link_directories(seamless_smb_service_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../lib)

#set_property(TARGET seamless_smb_service PROPERTY  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_definitions(seamless_smb_service PRIVATE  $<CONFIG:Debug>:"_DEBUG"> $<CONFIG:Release>"NDEBUG">  "WIN32;"        "_CONSOLE;"        "UNICODE;"        "_UNICODE"       )


#set_property(TARGET seamless_smb_service_lib PROPERTY  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_definitions(seamless_smb_service_lib PRIVATE  $<CONFIG:Debug>:"_DEBUG"> $<CONFIG:Release>"NDEBUG">  "WIN32;"        "_CONSOLE;"        "UNICODE;"        "_UNICODE"       )

