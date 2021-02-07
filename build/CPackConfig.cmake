# This file will be configured to contain variables for CPack. These variables
# should be set in the CMake list file of the project before CPack module is
# included. The list of available CPACK_xxx variables and their associated
# documentation may be obtained using
#  cpack --help-variable-list
#
# Some variables are common to all generators (e.g. CPACK_PACKAGE_NAME)
# and some are specific to a generator
# (e.g. CPACK_NSIS_EXTRA_INSTALL_COMMANDS). The generator specific variables
# usually begin with CPACK_<GENNAME>_xxxx.


set(CPACK_BINARY_7Z "OFF")
set(CPACK_BINARY_IFW "OFF")
set(CPACK_BINARY_NSIS "ON")
set(CPACK_BINARY_NUGET "OFF")
set(CPACK_BINARY_WIX "OFF")
set(CPACK_BINARY_ZIP "OFF")
set(CPACK_BUILD_SOURCE_DIRS "D:/dev/С++/Projects_C++/Черный пояс С++/(Курсовой проект) Электронная таблица;D:/dev/С++/Projects_C++/Черный пояс С++/(Курсовой проект) Электронная таблица/build")
set(CPACK_CMAKE_GENERATOR "Visual Studio 15 2017")
set(CPACK_COMPONENTS_ALL "Unspecified;dev")
set(CPACK_COMPONENT_UNSPECIFIED_HIDDEN "TRUE")
set(CPACK_COMPONENT_UNSPECIFIED_REQUIRED "TRUE")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_FILE "D:/dev/utils/CMAKE/cmake-3.19.0-rc1-win64-x64/share/cmake-3.19/Templates/CPack.GenericDescription.txt")
set(CPACK_DEFAULT_PACKAGE_DESCRIPTION_SUMMARY "spreadsheet built using CMake")
set(CPACK_GENERATOR "NSIS")
set(CPACK_INSTALL_CMAKE_PROJECTS "D:/dev/С++/Projects_C++/Черный пояс С++/(Курсовой проект) Электронная таблица/build;spreadsheet;ALL;/")
set(CPACK_INSTALL_PREFIX "C:/Program Files/spreadsheet")
set(CPACK_MODULE_PATH "")
set(CPACK_NSIS_DISPLAY_NAME "spreadsheet 4.7.2")
set(CPACK_NSIS_INSTALLER_ICON_CODE "")
set(CPACK_NSIS_INSTALLER_MUI_ICON_CODE "")
set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
set(CPACK_NSIS_PACKAGE_NAME "spreadsheet 4.7.2")
set(CPACK_NSIS_UNINSTALL_NAME "Uninstall")
set(CPACK_OUTPUT_CONFIG_FILE "D:/dev/С++/Projects_C++/Черный пояс С++/(Курсовой проект) Электронная таблица/build/CPackConfig.cmake")
set(CPACK_PACKAGE_CONTACT "antlr-discussion@googlegroups.com")
set(CPACK_PACKAGE_DEFAULT_LOCATION "/")
set(CPACK_PACKAGE_DESCRIPTION_FILE "D:/dev/utils/CMAKE/cmake-3.19.0-rc1-win64-x64/share/cmake-3.19/Templates/CPack.GenericDescription.txt")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "spreadsheet built using CMake")
set(CPACK_PACKAGE_FILE_NAME "spreadsheet-4.7.2-win64")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "spreadsheet 4.7.2")
set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY "spreadsheet 4.7.2")
set(CPACK_PACKAGE_NAME "spreadsheet")
set(CPACK_PACKAGE_RELOCATABLE "true")
set(CPACK_PACKAGE_VENDOR "Humanity")
set(CPACK_PACKAGE_VERSION "4.7.2")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "1")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_RESOURCE_FILE_LICENSE "D:/dev/utils/CMAKE/cmake-3.19.0-rc1-win64-x64/share/cmake-3.19/Templates/CPack.GenericLicense.txt")
set(CPACK_RESOURCE_FILE_README "D:/dev/utils/CMAKE/cmake-3.19.0-rc1-win64-x64/share/cmake-3.19/Templates/CPack.GenericDescription.txt")
set(CPACK_RESOURCE_FILE_WELCOME "D:/dev/utils/CMAKE/cmake-3.19.0-rc1-win64-x64/share/cmake-3.19/Templates/CPack.GenericWelcome.txt")
set(CPACK_SET_DESTDIR "OFF")
set(CPACK_SOURCE_7Z "ON")
set(CPACK_SOURCE_GENERATOR "7Z;ZIP")
set(CPACK_SOURCE_OUTPUT_CONFIG_FILE "D:/dev/С++/Projects_C++/Черный пояс С++/(Курсовой проект) Электронная таблица/build/CPackSourceConfig.cmake")
set(CPACK_SOURCE_ZIP "ON")
set(CPACK_SYSTEM_NAME "win64")
set(CPACK_TOPLEVEL_TAG "win64")
set(CPACK_WIX_SIZEOF_VOID_P "8")

if(NOT CPACK_PROPERTIES_FILE)
  set(CPACK_PROPERTIES_FILE "D:/dev/С++/Projects_C++/Черный пояс С++/(Курсовой проект) Электронная таблица/build/CPackProperties.cmake")
endif()

if(EXISTS ${CPACK_PROPERTIES_FILE})
  include(${CPACK_PROPERTIES_FILE})
endif()
