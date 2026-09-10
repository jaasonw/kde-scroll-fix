cmake_minimum_required(VERSION 3.25)
project(scrollfix LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(QT_DEFAULT_MAJOR_VERSION 6)
find_package(ECM REQUIRED NO_MODULE)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH})
include(KDEInstallDirs6)
find_package(Qt6 REQUIRED COMPONENTS Core DBus Widgets Quick)
find_package(KWin 6.3 REQUIRED)
add_library(scrollfix MODULE src/plugin.cpp)
target_link_libraries(scrollfix PRIVATE KWin::kwin)
set_target_properties(scrollfix PROPERTIES PREFIX "")
install(TARGETS scrollfix DESTINATION ${KDE_INSTALL_PLUGINDIR}/kwin/plugins)
add_executable(scroll-fix-settings src/settings.cpp)
target_link_libraries(scroll-fix-settings PRIVATE Qt6::Widgets Qt6::DBus KF6::ConfigCore)
install(TARGETS scroll-fix-settings DESTINATION ${KDE_INSTALL_BINDIR} COMPONENT Settings EXCLUDE_FROM_ALL)
install(FILES scroll-fix-settings.desktop DESTINATION ${KDE_INSTALL_APPDIR} COMPONENT Settings EXCLUDE_FROM_ALL)
set(FORMAT_SOURCES src/policy.h src/plugin.cpp src/settings.cpp tests/policy_test.cpp)
set(TIDY_SOURCES src/plugin.cpp src/settings.cpp tests/policy_test.cpp)
find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format)
find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)
if(CLANG_FORMAT_EXECUTABLE)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${FORMAT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM)
    add_custom_target(format-check
        COMMAND ${CLANG_FORMAT_EXECUTABLE} --dry-run --Werror ${FORMAT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM)
endif()

if(CLANG_TIDY_EXECUTABLE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_custom_target(tidy
        COMMAND ${CLANG_TIDY_EXECUTABLE} -p ${CMAKE_BINARY_DIR} ${TIDY_SOURCES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM)
    add_dependencies(tidy scrollfix_autogen)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(policy_test tests/policy_test.cpp)
    target_link_libraries(policy_test PRIVATE Qt6::Core)
    add_test(NAME policy COMMAND policy_test)
    add_test(NAME settings_smoke COMMAND scroll-fix-settings --smoke-test)
    set_tests_properties(settings_smoke PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
