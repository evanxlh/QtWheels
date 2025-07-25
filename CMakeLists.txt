cmake_minimum_required(VERSION 3.5)

project(QtWheels VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Network)

set(PROJECT_SOURCES
        main.cpp
        MainWindow.cpp
        MainWindow.h
        MainWindow.ui
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(QtWheels
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        Source/Cache/EXMemoryCache.h
        Source/ImageLoader/EXImageProcessing.h Source/ImageLoader/EXImageProcessing.cpp
        Source/ImageLoader/EXImageLoaderGlobal.h
        Source/ImageLoader/EXImageProcessor.h Source/ImageLoader/EXImageProcessor.cpp
        Source/ImageLoader/EXImageLoaderConfiguration.h Source/ImageLoader/EXImageLoaderConfiguration.cpp
        Source/ImageLoader/EXImageRequest.h Source/ImageLoader/EXImageRequest.cpp
        Source/ImageLoader/EXImageRequestScheduler.h Source/ImageLoader/EXImageRequestScheduler.cpp
        Source/ImageLoader/EXImageLoader.h Source/ImageLoader/EXImageLoader.cpp
        Source/ImageLoader/EXImageLoaderPrivate.h



    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET QtWheels APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(QtWheels SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(QtWheels
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(QtWheels PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.QtWheels)
endif()
set_target_properties(QtWheels PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS QtWheels
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(QtWheels)
endif()
