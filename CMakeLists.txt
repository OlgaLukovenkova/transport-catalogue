cmake_minimum_required(VERSION 3.10)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)

find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto map_renderer.proto svg.proto graph.proto transport_router.proto)

set(GEO_FILES geo.h)
set(GRAPH_FILES graph.h router.h ranges.h graph.proto)
set(SVG_FILES svg.h svg.cpp svg.proto)
set(JSON_FILES json.h json.cpp json_builder.h json_builder.cpp)
set(TRANSPORT_CATALOGUE_FILES domain.h domain.cpp 
	transport_catalogue.h transport_catalogue.cpp 
	transport_router.h transport_router.cpp 
	main.cpp 
	transport_catalogue.proto transport_router.proto)
set(INTERFACE_FILES json_reader.h json_reader.cpp 
	map_renderer.h map_renderer.cpp 
	request_handler.h request_handler.cpp 
	serialization.h serialization.cpp 
	map_renderer.proto)


add_executable(transport_catalogue ${PROTO_SRCS} ${PROTO_HDRS} ${GEO_FILES} ${GRAPH_FILES} ${SVG_FILES} ${JSON_FILES} ${TRANSPORT_CATALOGUE_FILES} ${INTERFACE_FILES})
target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)