project(seamless_smb_service_tester_settings)
add_executable(seamless_smb_service_tester_settings)

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set_target_properties(seamless_smb_service_tester_settings PROPERTIES AUTOMOC TRUE)
set_target_properties(seamless_smb_service_tester_settings PROPERTIES AUTOUIC TRUE)
set_target_properties(seamless_smb_service_tester_settings PROPERTIES AUTORCC TRUE)

find_package(Qt6 COMPONENTS Core Network REQUIRED)

target_include_directories(seamless_smb_service_tester_settings PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include/ ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(seamless_smb_service_tester_settings PUBLIC Qt6::Core Qt6::Network QtService seamless_smb_service_lib)

#set_property(TARGET seamless_smb_service_tester PROPERTY  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_definitions(seamless_smb_service_tester_settings PRIVATE  $<CONFIG:Debug>:"_DEBUG"> $<CONFIG:Release>"NDEBUG">  "WIN32;"        "_CONSOLE;"        "UNICODE;"        "_UNICODE"       )


target_sources(seamless_smb_service_tester_settings PRIVATE "main.cpp")
