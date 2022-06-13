cmake_minimum_required(VERSION 3.10)

project(TransportCatalogue CXX)
set(CMAKE_CXX_STANDARD 17)

set(JSON_FILES json.h json.cpp json_builder.h json_builder.cpp)
set(SVG_FILES svg.h svg.cpp)
set(TRANSPORT_FILES domain.h domain.cpp transport_catalogue.h transport_catalogue.cpp geo.h geo.cpp)
set(REQUESTS_FILES request_handler.h request_handler.cpp map_renderer.h map_renderer.cpp json_reader.h json_reader.cpp)
set(ROUTER_FILES graph.h ranges.h router.h transport_router.h transport_router.cpp)
set(SERIALIZATION_FILES serialization.h serialization.cpp transport_catalogue.proto
                        map_renderer.proto svg.proto graph.proto transport_router.proto)

set(CMAKE_PREFIX_PATH "../protobuf_package")
find_package(Protobuf REQUIRED)
find_package(Threads REQUIRED)

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS transport_catalogue.proto map_renderer.proto svg.proto
                                            transport_router.proto graph.proto)

add_executable(transport_catalogue main.cpp ${PROTO_SRCS} ${PROTO_HDRS} ${JSON_FILES} ${SVG_FILES} ${TRANSPORT_FILES} /
                                            ${REQUESTS_FILES} ${ROUTER_FILES} ${SERIALIZATION_FILES})

target_include_directories(transport_catalogue PUBLIC ${Protobuf_INCLUDE_DIRS})
target_include_directories(transport_catalogue PUBLIC ${CMAKE_CURRENT_BINARY_DIR})

string(REPLACE "protobuf.lib" "protobufd.lib" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")
string(REPLACE "protobuf.a" "protobufd.a" "Protobuf_LIBRARY_DEBUG" "${Protobuf_LIBRARY_DEBUG}")

target_link_libraries(transport_catalogue "$<IF:$<CONFIG:Debug>,${Protobuf_LIBRARY_DEBUG},${Protobuf_LIBRARY}>" Threads::Threads)
