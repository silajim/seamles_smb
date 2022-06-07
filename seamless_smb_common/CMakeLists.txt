project(seamless_smb_common)
add_library(seamless_smb_common STATIC)
target_sources(seamless_smb_common PRIVATE common.cpp)
target_sources(seamless_smb_common PUBLIC common.h)


set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set_target_properties(seamless_smb_common PROPERTIES AUTOMOC TRUE)
set_target_properties(seamless_smb_common PROPERTIES AUTOUIC TRUE)
set_target_properties(seamless_smb_common PROPERTIES AUTORCC TRUE)

find_package(Qt5 COMPONENTS Core Network REQUIRED)

target_link_libraries(seamless_smb_common PUBLIC Qt5::Core Qt5::Network)
target_include_directories(seamless_smb_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include/ ${CMAKE_CURRENT_SOURCE_DIR}/include)
