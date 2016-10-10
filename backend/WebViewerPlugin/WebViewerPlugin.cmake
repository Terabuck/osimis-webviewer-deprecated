# Orthanc - A Lightweight, RESTful DICOM Store
# Copyright (C) 2012-2016 Sebastien Jodogne, Medical Physics
# Department, University Hospital of Liege, Belgium
# Copyright (C) 2016 OSIMIS SA
#
# This program is free software: you can redistribute it and/or
# modify it under the terms of the GNU Affero General Public License
# as published by the Free Software Foundation, either version 3 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Affero General Public License for more details.
# 
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

# Build the plugin as a shared library.
#
# Usage:
#   (within CMakeLists.txt)
#   # Set all required variables
#   set(ORTHANC_DIR ${CMAKE_SOURCE_DIR}/Orthanc)
#   set(VIEWER_PLUGIN_DIR ${CMAKE_SOURCE_DIR}/WebViewerPlugin)
#   set(VIEWER_FRONTEND_DIR ${CMAKE_SOURCE_DIR}/../frontend)
#   # Make sure WebViewerLibrary target is available (see `WebViewerLibrary/WebViewerLibrary.cmake`)
#   # Build unit tests
#   include(${WebViewerPlugin}/WebViewerPlugin.cmake)

# Include version (taken from git tag)
include(${RESOURCES_DIR}/CMake/GetProductVersionFromGitTag.cmake) # see target_compile_definitions below target for more info

MESSAGE( STATUS "PRODUCT_VERSION_BRANCH:         " ${PRODUCT_VERSION_BRANCH} )
MESSAGE( STATUS "PRODUCT_VERSION_SHORT_STRING:   " ${PRODUCT_VERSION_SHORT_STRING} )
MESSAGE( STATUS "JS_FRONTEND_VERSION:            " ${JS_FRONTEND_VERSION} )

# Set build parameters
set(STANDALONE_BUILD ON CACHE BOOL "Standalone build (all the resources are embedded, necessary for releases)")
set(JS_CLIENT_PATH "${VIEWER_FRONTEND_DIR}/build" CACHE STRING "Path of the front-end build folder")
if(EXISTS ${JS_CLIENT_PATH}) # If file exists (not var)
  # Set frontend version based on local build if available
  # when building inside the docker container, the frontend/build folder is already there and we don't want to override it.
  set(JS_FRONTEND_VERSION "LOCAL")
else()
  # Set frontend version based on the commit id for all other builds including the release version
  set(JS_FRONTEND_VERSION ${PRODUCT_VERSION_COMMIT_SHA1_STRING})
endif()

# Download the frontend lib
if(NOT ${JS_FRONTEND_VERSION} STREQUAL "LOCAL") 
  # ORTHANC_ROOT & DownloadPackage.cmake are included within BuildDependencies.cmake
  DownloadPackage(FALSE "http://orthanc.osimis.io/public/osimisWebViewer/${JS_FRONTEND_VERSION}.zip" ${JS_CLIENT_PATH} TRUE)
endif()

# Check JS_FRONTEND content exists
if(NOT EXISTS ${JS_CLIENT_PATH}/js/lib.js OR
  NOT EXISTS ${JS_CLIENT_PATH}/js/app.js OR
  NOT EXISTS ${JS_CLIENT_PATH}/styles/lib.css OR
  NOT EXISTS ${JS_CLIENT_PATH}/styles/app.css)
  message(FATAL_ERROR "Bad frontend folder content in ${JS_CLIENT_PATH}")
endif()

# Generate embedded resources
# Always embed at least ORTHANC_EXPLORER, even if STANDALONE_BUILD is off?
if (STANDALONE_BUILD)
EmbedResources(
  ORTHANC_EXPLORER  ${RESOURCES_DIR}/OrthancExplorer.js
  WEB_VIEWER  ${JS_CLIENT_PATH}
  )
else()
EmbedResources(
  ORTHANC_EXPLORER  ${RESOURCES_DIR}/OrthancExplorer.js
  )
endif()

# Append resources.rc to windows' build
if (${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
  list(APPEND VIEWER_PLUGIN_WINDOWS_SOURCES ${VIEWER_PLUGIN_DIR}/resources.rc) 
endif()

# Create OsimisWebViewer Plugin as a dynamic library
add_library(OsimisWebViewer
  SHARED

  # Frontend code
  ${AUTOGENERATED_SOURCES}

  ${VIEWER_PLUGIN_WINDOWS_SOURCES}
  ${VIEWER_PLUGIN_DIR}/WebViewer.cpp
  ${VIEWER_PLUGIN_DIR}/Plugin.cpp
  )

if (STANDALONE_BUILD)
  target_compile_definitions(OsimisWebViewer PRIVATE
    -DORTHANC_STANDALONE=1
    )
else()
  target_compile_definitions(OsimisWebViewer PRIVATE
    -DORTHANC_STANDALONE=0
    -DWEB_VIEWER_PATH="${JS_CLIENT_PATH}/"
    )
endif()

# Include version (taken from git tag)
# @todo create a macro instead
target_compile_definitions(OsimisWebViewer PRIVATE -DPRODUCT_VERSION_BRANCH=${PRODUCT_VERSION_BRANCH})
target_compile_definitions(OsimisWebViewer PRIVATE -DPRODUCT_VERSION_MAJOR=${PRODUCT_VERSION_MAJOR})
target_compile_definitions(OsimisWebViewer PRIVATE -DPRODUCT_VERSION_MINOR=${PRODUCT_VERSION_MINOR})
target_compile_definitions(OsimisWebViewer PRIVATE -DPRODUCT_VERSION_PATCH=${PRODUCT_VERSION_PATCH})
target_compile_definitions(OsimisWebViewer PRIVATE -DPRODUCT_VERSION_COMMIT_NUMBER=${PRODUCT_VERSION_COMMIT_NUMBER})
target_compile_definitions(OsimisWebViewer PRIVATE -DPRODUCT_VERSION_COMMIT_SHA1_STRING=\"${PRODUCT_VERSION_COMMIT_SHA1_STRING}\")
target_compile_definitions(OsimisWebViewer PRIVATE -DPRODUCT_VERSION_BUILD_YEAR_STRING=\"${PRODUCT_VERSION_BUILD_YEAR}\")
target_compile_definitions(OsimisWebViewer PRIVATE -DPRODUCT_VERSION_BUILD_MONTH_STRING=\"${PRODUCT_VERSION_BUILD_MONTH}\")
target_compile_definitions(OsimisWebViewer PRIVATE -DPRODUCT_VERSION_BUILD_DAY_STRING=\"${PRODUCT_VERSION_BUILD_DAY}\")

# Check & rebuild if embedded resources has changed?
if (STATIC_BUILD)
  add_dependencies(OsimisWebViewer EmbeddedResourcesGenerator)
endif()

add_dependencies(OsimisWebViewer WebViewerLibrary)
target_link_libraries(OsimisWebViewer WebViewerLibrary)

message("Setting the version of the library to ${PRODUCT_VERSION_SHORT_STRING}")

set_target_properties(OsimisWebViewer PROPERTIES
    VERSION ${PRODUCT_VERSION_SHORT_STRING}
    SOVERSION ${PRODUCT_VERSION_SHORT_STRING})

install(
  TARGETS OsimisWebViewer
  RUNTIME DESTINATION lib                      # Destination for Windows
  LIBRARY DESTINATION share/orthanc/plugins    # Destination for Linux
  )