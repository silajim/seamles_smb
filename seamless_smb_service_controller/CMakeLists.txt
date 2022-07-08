cmake_minimum_required(VERSION 3.14)

project(seamless_smb_service_controller VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Quick REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Quick REQUIRED)

set(PROJECT_SOURCES
        main.cpp
        qml.qrc
        "applicationui.h"
        "applicationui.cpp"
        "mountlistmodel.h"
        "mountlistmodel.cpp"
        "mounteditor.h"
        "mounteditor.cpp"
        "client.h"
        "client.cpp"
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(seamless_smb_service_controller
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET seamless_smb_service_controller APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    add_executable(seamless_smb_service_controller   ${PROJECT_SOURCES} )

endif()

set_target_properties(seamless_smb_service_controller PROPERTIES AUTOMOC TRUE)
set_target_properties(seamless_smb_service_controller PROPERTIES AUTOUIC TRUE)
set_target_properties(seamless_smb_service_controller PROPERTIES AUTORCC TRUE)

target_compile_definitions(seamless_smb_service_controller PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)
target_link_libraries(seamless_smb_service_controller PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Quick QtService seamless_smb_common seamless_smb_service_lib)

set_target_properties(seamless_smb_service_controller PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_import_qml_plugins(seamless_smb_service_controller)
    qt_finalize_executable(seamless_smb_service_controller)
endif()
