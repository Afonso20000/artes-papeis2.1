cmake_minimum_required(VERSION 3.16)
project(artes-papeis VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable Qt modules
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Allow Qt directory to be specified through command line or environment
if(DEFINED ENV{Qt6_DIR} AND NOT Qt6_DIR)
    set(Qt6_DIR $ENV{Qt6_DIR})
endif()

# If Qt6_DIR is not set, try to find Qt in standard locations
if(NOT Qt6_DIR)
    if(EXISTS "/Users/valass/Qt/6.10.0/macos/lib/cmake/Qt6")
        set(Qt6_DIR "/Users/valass/Qt/6.10.0/macos/lib/cmake/Qt6")
    elseif(EXISTS "/usr/local/opt/qt@6/lib/cmake/Qt6")
        set(Qt6_DIR "/usr/local/opt/qt@6/lib/cmake/Qt6")
    endif()
endif()

# Find Qt packages
find_package(Qt6 REQUIRED COMPONENTS 
    Core
    Gui
    Widgets
)

# Add Qt module include paths
include_directories(${Qt6Core_INCLUDE_DIRS})
include_directories(${Qt6Gui_INCLUDE_DIRS})
include_directories(${Qt6Widgets_INCLUDE_DIRS})

# Add executable
add_executable(${PROJECT_NAME}
    main.cpp
    mainwindow.cpp
    mainwindow.h
    productcard.cpp
    productcard.h
    productformdialog.cpp
    productformdialog.h
    productmanager.cpp
    productmanager.h
    produto.h
)

# Link Qt libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Add include directories
target_include_directories(${PROJECT_NAME} PRIVATE ${Qt6Core_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${Qt6Gui_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PRIVATE ${Qt6Widgets_INCLUDE_DIRS})

# Install rules
install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)