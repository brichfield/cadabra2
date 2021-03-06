cmake_minimum_required(VERSION 2.8)

project(Cadabra)
set(CADABRA_VERSION_MAJOR 2)
set(CADABRA_VERSION_MINOR 0)
set(COPYRIGHT_YEARS "2001-2016")
execute_process(COMMAND git rev-parse --short HEAD OUTPUT_VARIABLE GIT_SHORT_SHA     OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND git rev-list --count HEAD  OUTPUT_VARIABLE GIT_COMMIT_SERIAL OUTPUT_STRIP_TRAILING_WHITESPACE)
set(CADABRA_VERSION_BUILD "${GIT_COMMIT_SERIAL}.${GIT_SHORT_SHA}")
message("-- Build id ${CADABRA_VERSION_BUILD}")

enable_testing()

# Generate config file with CMake variables.
configure_file("${PROJECT_SOURCE_DIR}/core/Config.hh.in" "${PROJECT_BINARY_DIR}/core/Config.hh")
include_directories(${PROJECT_BINARY_DIR}/core)

# Determine what type of system we are on, for packaging.
set(LINUX_NAME "")
if(EXISTS "/etc/redhat-release")
   file(READ "/etc/redhat-release" LINUX_ISSUE)
   if(LINUX_ISSUE MATCHES "CentOS")
       set(CPACK_GENERATOR "RPM")
       set(CPACK_SYSTEM_NAME "centos")
       message("-- This is a CentOS system")
   endif(LINUX_ISSUE MATCHES "CentOS")
   if(LINUX_ISSUE MATCHES "Fedora")
       set(CPACK_GENERATOR   "RPM")
       set(CPACK_SYSTEM_NAME "fedora")
       message("-- This is a Fedora system")
   endif(LINUX_ISSUE MATCHES "Fedora")
else(EXISTS "/etc/redhat-release")
   if(EXISTS "/etc/issue")
      file(READ "/etc/issue" LINUX_ISSUE)
      if(LINUX_ISSUE MATCHES "Ubuntu")
        set(CPACK_GENERATOR   "DEB")
   	  set(CPACK_SYSTEM_NAME "trusty")
   	  message("-- This is an Ubuntu system")
      endif(LINUX_ISSUE MATCHES "Ubuntu")
      if(LINUX_ISSUE MATCHES "Mint")
        set(CPACK_GENERATOR "DEB")
   	  message("-- This is a Linux Mint system")
      endif(LINUX_ISSUE MATCHES "Mint")
   endif(EXISTS "/etc/issue")
endif(EXISTS "/etc/redhat-release")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/config/postinst.in" "${CMAKE_CURRENT_BINARY_DIR}/postinst")

# Logic to build packages (RPM/DEB) using CPack; see https://cmake.org/Wiki/CMake:Packaging_With_CPack
# 
include (InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME           "cadabra2")
set(CPACK_RESOURCE_FILE_LICENSE  "${CMAKE_CURRENT_SOURCE_DIR}/doc/license.txt")
set(CPACK_RPM_PACKAGE_LICENSE    "GPLv3")
set(CPACK_PACKAGE_VERSION_MAJOR  "${CADABRA_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR  "${CADABRA_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH  "${CADABRA_VERSION_BUILD}")
set(CPACK_PACKAGE_VENDOR         "Kasper Peeters")
set(CPACK_PACKAGE_CONTACT        "Kasper Peeters <kasper.peeters@phi-sci.com>")
set(CPACK_STRIP_FILES            ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA ${CMAKE_CURRENT_BINARY_DIR}/postinst)
set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE ${CMAKE_CURRENT_BINARY_DIR}/postinst)
set(CPACK_RPM_EXCLUDE_LIST /usr /usr/lib64 /usr/lib64/python2.7 /usr/bin /usr/lib /usr/share /usr/share/texlive)

#xdg-desktop-menu install <install>/share/applications/MyApp.desktop

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libpcre3, libpcrecpp0, libgmpxx4ldbl, libboost-python1.54.0, libboost-system1.54.0, libboost-filesystem1.54.0, libboost-program-options1.54.0, libboost-regex1.54.0, libstdc++6, uuid-runtime, libgtkmm-3.0-1, texlive, texlive-latex-extra, python-matplotlib, python-mpmath, dvipng, python-sympy")
set(CPACK_RPM_PACKAGE_REQUIRES   "python-libs, pcre, gmp, libuuid, sqlite, gtkmm30, boost-system, boost-filesystem, boost-program-options, boost-regex, libstdc++, python-matplotlib, dvipng, texlive-latex")

set(CPACK_DEBIAN_PACKAGE_SECTION "math")
set(CPACK_RPM_PACKAGE_GROUP      "Applications/Productivity")

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "field-theory motivated computer algebra system")
set(CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/doc/description)
include(CPack)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")
# -I/usr/local/Cellar/python/2.7.8_2/Frameworks/Python.framework/Versions/2.7/include/python2.7")
endif()


# Set path to additional cmake files
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")

add_subdirectory(core)
add_subdirectory(client_server)
add_subdirectory(frontend)
add_subdirectory(tests)
add_subdirectory(web2 EXCLUDE_FROM_ALL)

