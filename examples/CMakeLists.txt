CMAKE_MINIMUM_REQUIRED(VERSION 3.5)

PROJECT(Examples)

SET(CMAKE_CXX_STANDARD 14)

# search RTI package as a CMake module
SET(CMAKE_MODULE_PATH "$ENV{NDDSHOME}/resource/cmake")
FIND_PACKAGE(RTIConnextDDS 6.0.0 MODULE REQUIRED)

SET(COMM_SRC 
	CommPosition.cpp
	CommPositionDDS.cpp 
	CommOrientation.cpp
	CommOrientationDDS.cpp
	CommPose6d.cpp  
	CommPose6dDDS.cpp
	CommText.cpp 
	CommTextDDS.cpp
)

ADD_LIBRARY(CommTests ${COMM_SRC})
TARGET_LINK_LIBRARIES(CommTests RTIConnextDDS::cpp2_api)

ADD_EXECUTABLE(PushClient example_push_client.cpp)
TARGET_LINK_LIBRARIES(PushClient RTI-DDS-SmartSoft CommTests)

ADD_EXECUTABLE(PushServer example_push_server.cpp)
TARGET_LINK_LIBRARIES(PushServer RTI-DDS-SmartSoft CommTests)

ADD_EXECUTABLE(SendClient example_send_client.cpp)
TARGET_LINK_LIBRARIES(SendClient RTI-DDS-SmartSoft CommTests)

ADD_EXECUTABLE(SendServer example_send_server.cpp)
TARGET_LINK_LIBRARIES(SendServer RTI-DDS-SmartSoft CommTests)

ADD_EXECUTABLE(QueryClient example_query_client.cpp)
TARGET_LINK_LIBRARIES(QueryClient RTI-DDS-SmartSoft CommTests)

ADD_EXECUTABLE(QueryServer example_query_server.cpp)
TARGET_LINK_LIBRARIES(QueryServer RTI-DDS-SmartSoft CommTests)

ADD_EXECUTABLE(EventClient example_event_client.cpp)
TARGET_LINK_LIBRARIES(EventClient RTI-DDS-SmartSoft CommTests)

ADD_EXECUTABLE(EventServer example_event_server.cpp)
TARGET_LINK_LIBRARIES(EventServer RTI-DDS-SmartSoft CommTests)
