project(seamless_smb_service_process)

add_executable(seamless_smb_service_process
    otherlogger.h otherlogger.cpp)

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set_target_properties(seamless_smb_service_process PROPERTIES AUTOMOC TRUE)
set_target_properties(seamless_smb_service_process PROPERTIES AUTOUIC TRUE)
set_target_properties(seamless_smb_service_process PROPERTIES AUTORCC TRUE)

find_package(Qt6 COMPONENTS Core Network REQUIRED)

target_sources(seamless_smb_service_process PRIVATE main.cpp logger.cpp logger.h dokanrunner.h dokanrunner.cpp otherlogger.cpp otherlogger.h)

target_include_directories(seamless_smb_service_process PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../../include/)
target_link_libraries(seamless_smb_service_process PRIVATE Qt6::Core Qt6::Network seamless_smb_common seamless_smb_lib FreeDrives dokan2 user32)


#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")

#message(${CMAKE_CXX_FLAGS})

target_link_directories(seamless_smb_service_process PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../../lib)



