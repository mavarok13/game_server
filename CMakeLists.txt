cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(Boost 1.78.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(GameLib STATIC
	src/model.cpp
	src/sdk.h
	src/tagged.h
)

#target_link_libraries(GameLib PUBLIC CONAN_PKG::boost)

add_executable(game_server
	src/main.cpp
	src/command_line_args.h
	src/http_server.h
	src/http_server.cpp
	src/request_handler.h
	src/request_handler.cpp
	src/http_path_utils.h
	src/http_path_utils.cpp
	src/http_content_type.h
	src/http_content_type.cpp
	src/app.h
	src/boost_json.cpp
	src/json_fields.h
	src/json_loader.h
	src/json_loader.cpp
	src/json_builder.h
	src/json_builder.cpp
	src/logger.hpp
	src/logger.cpp
	src/loot_generator.h
	src/loot_generator.cpp
	src/collision_detector.h
	src/collision_detector.cpp
	src/game_item_player_provider.h
	src/game_office_player_provider.h
	src/extra_data.h
	src/extra_data.cpp
	src/endpoint.h
	src/geom.h
)

add_executable(loot_generator_test
	tests/loot_generator_tests.cpp
	src/loot_generator.h
	src/loot_generator.cpp
)

add_executable(collision_detector_test
	tests/collision-detector-tests.h
	tests/collision-detector-tests.cpp
	src/collision_detector.h
	src/collision_detector.cpp
)

target_link_libraries(game_server PUBLIC GameLib PRIVATE Threads::Threads PUBLIC CONAN_PKG::boost)
target_link_libraries(loot_generator_test PRIVATE CONAN_PKG::catch2)
target_link_libraries(collision_detector_test PRIVATE CONAN_PKG::catch2)
