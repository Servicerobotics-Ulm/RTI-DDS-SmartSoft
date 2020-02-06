CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

PROJECT(RTI-DDS-SmartSoft VERSION 1.1)

# find the SmartSoft Component-Developer API package
FIND_PACKAGE(SmartSoft_CD_API 1.1 PATHS $ENV{SMART_ROOT_ACE}/modules)

# search RTI package as a CMake module
SET(CMAKE_MODULE_PATH "$ENV{NDDSHOME}/resource/cmake")
FIND_PACKAGE(RTIConnextDDS 6.0.1 MODULE REQUIRED COMPONENTS messaging_api)

# collect IDL files
FILE(GLOB IDL_FILES "${PROJECT_SOURCE_DIR}/*.idl")

FOREACH( IDL_FILE ${IDL_FILES} )
  GET_FILENAME_COMPONENT(BASE_NAME ${IDL_FILE} NAME_WE)
  GET_FILENAME_COMPONENT(BASE_DIR ${IDL_FILE} DIRECTORY)

  SET(CURR_IDL_HPP "${BASE_DIR}/${BASE_NAME}.hpp")
  SET(CURR_IDL_SRC "${BASE_DIR}/${BASE_NAME}.cxx")
  SET(CURR_IDL_PLUGIN_HPP "${BASE_DIR}/${BASE_NAME}Plugin.hpp")
  SET(CURR_IDL_PLUGIN_SRC "${BASE_DIR}/${BASE_NAME}Plugin.cxx")

  LIST(APPEND IDL_SRCS "${CURR_IDL_SRC}")
  LIST(APPEND IDL_SRCS "${CURR_IDL_PLUGIN_SRC}")

  IF(IDL_COMPILER)
    ADD_CUSTOM_COMMAND(
      OUTPUT "${CURR_IDL_HPP}" "${CURR_IDL_SRC}" "${CURR_IDL_PLUGIN_HPP}" "${CURR_IDL_PLUGIN_SRC}" 
      COMMAND ${IDL_COMPILER} ARGS -language C++11 -update typefiles ${IDL_FILE} 
      DEPENDS ${IDL_FILE} COMMENT "Run IDL compiler for ${IDL_FILE}"
    )
  ENDIF(IDL_COMPILER)
ENDFOREACH( IDL_FILE ${IDL_FILES} )

FILE(GLOB HDRS "${PROJECT_SOURCE_DIR}/*.h")
FILE(GLOB SRCS "${PROJECT_SOURCE_DIR}/*.cpp")

# create the main library
ADD_LIBRARY(${PROJECT_NAME} SHARED ${SRCS} ${IDL_SRCS})

# set library target options
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES 
  CXX_STANDARD 14
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)

# set library dependencies
TARGET_LINK_LIBRARIES(${PROJECT_NAME} 
	SmartSoft_CD_API # this is the SmartSoft Component Developer (CD) API
	RTIConnextDDS::cpp2_api # this is the RTI's Modern C++ core library
	RTIConnextDDS::messaging_cpp2_api # this is the RTI's Modern C++ Request-Response extension library
)

TARGET_INCLUDE_DIRECTORIES(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${RTI_DDS_SmartSoft_ROOT}>
  $<INSTALL_INTERFACE:include>  # <prefix>/include
)

# set the export-name used in the ${PROJECT_NAME}Config.cmake.in and for exporting and installing the target
SET(EXPORT_NAME ${PROJECT_NAME}Target)

# generate CMake package-config file
CONFIGURE_FILE(
  ${PROJECT_SOURCE_DIR}/${PROJECT_NAME}Config.cmake.in
  ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  @ONLY
)

# create a build-tree targets file
EXPORT(TARGETS ${PROJECT_NAME} FILE ${EXPORT_NAME}.cmake)

# generate ConfigVersion file
INCLUDE(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  COMPATIBILITY AnyNewerVersion
)

# install headers and the library
INSTALL(FILES ${HDRS} DESTINATION include/${PROJECT_NAME})
INSTALL(TARGETS ${PROJECT_NAME} EXPORT ${EXPORT_NAME} LIBRARY DESTINATION lib)
# install cmake configuration files
INSTALL(EXPORT ${EXPORT_NAME} DESTINATION lib/cmake/${PROJECT_NAME})
INSTALL(FILES ${PROJECT_BINARY_DIR}/${PROJECT_NAME}Config.cmake DESTINATION lib/cmake/${PROJECT_NAME})
INSTALL(FILES "${PROJECT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake" DESTINATION lib/cmake/${PROJECT_NAME})
