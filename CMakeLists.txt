cmake_minimum_required(VERSION 3.28)

project(SoulEngine LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
	enable_language(OBJC)
endif()

set(BOX2D_BUILD_UNIT_TESTS FALSE)
set(BOX2D_BUILD_TESTBED FALSE)
set(SDLSHADERCROSS_VENDORED ON)
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS OFF)
set(SDLIMAGE_BUILD_SHARED_LIBS OFF)

add_definitions(-DGLM_ENABLE_EXPERIMENTAL)

add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/flecs)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/box2d)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/spdlog)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/glm)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/SDL3)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/SDL3_image)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/SDL3_ttf)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/glaze)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/SDL_shadercross)
add_subdirectory(${CMAKE_SOURCE_DIR}/vendor/meshoptimizer)

add_library(${PROJECT_NAME} STATIC
	src/core/application.h
	src/core/application.cpp
	src/core/tiny_obj_loader.cpp
	src/game_module.h
	src/game_module.cpp
	src/texture.h
	src/texture.cpp
	src/font.h
	src/font.cpp
	src/dag.h
	src/rect.h
	src/ecsModule/assetModule/module.h
	src/ecsModule/assetModule/module.cpp
	src/ecsModule/ui_module/layout.h
	src/ecsModule/ui_module/layout.cpp
	src/ecsModule/ui_module/module.h
	src/ecsModule/ui_module/module.cpp
	#src/ecsModule/ui_render_module/components.h
	#src/ecsModule/ui_render_module/module.h
	#src/ecsModule/ui_render_module/module.cpp
	src/ecsModule/ui_render_module_new/offsets.h
	src/ecsModule/ui_render_module_new/components.h
	src/ecsModule/ui_render_module_new/module.h
	src/ecsModule/ui_render_module_new/module.cpp
	src/ecsModule/ui_render_module_new/node_helpers.h
	src/ecsModule/ui_render_module_new/node_helpers.cpp
	src/ecsModule/ui_render_module_new/text_helpers.h
	src/ecsModule/ui_render_module_new/text_helpers.cpp
	#src/ecsModule/common.h
	#src/ecsModule/uiModule/module.h
	#src/ecsModule/uiModule/module.cpp
	#src/ecsModule/physicsModule/module.h
	#src/ecsModule/physicsModule/module.cpp
	#src/ecsModule/timeModule/module.h
	#src/ecsModule/timeModule/module.cpp
	src/ecsModule/cameraModule/module.h
	src/ecsModule/cameraModule/module.cpp
	src/ecsModule/inputModule/module.h
	src/ecsModule/inputModule/module.cpp
	#src/ecsModule/playerModule/module.h
	#src/ecsModule/playerModule/module.cpp
	src/ecsModule/transformModule/module.h
	src/ecsModule/transformModule/module.cpp
	src/ecsModule/windowModule/components.h
	src/ecsModule/windowModule/components.cpp
	src/ecsModule/windowModule/module.h
	src/ecsModule/windowModule/module.cpp
	#src/ecsModule/sceneModule/module.h
	#src/ecsModule/sceneModule/module.cpp
	#src/ecsModule/renderModule/module.h
	#src/ecsModule/renderModule/module.cpp
	src/ecsModule/render_module/material.h
	src/ecsModule/render_module/render_phase.h
	src/ecsModule/render_module/render_phase.cpp
	src/ecsModule/render_module/module.h
	src/ecsModule/render_module/module.cpp
	src/ecsModule/spriteModule/module.h
	src/ecsModule/spriteModule/module.cpp
	src/ecsModule/sprite_render_module/module.h
	src/ecsModule/sprite_render_module/module.cpp
	src/ecsModule/textModule/module.h
	src/ecsModule/textModule/module.cpp
	src/ecsModule/text_render_module/text_vertex.h
	src/ecsModule/text_render_module/module.h
	src/ecsModule/text_render_module/module.cpp
	src/ecsModule/meshModule/primitives.h
	src/ecsModule/meshModule/primitives.cpp
	src/ecsModule/meshModule/components.h
	src/ecsModule/meshModule/module.h
	src/ecsModule/meshModule/module.cpp
	src/ecsModule/mesh3d_module/components.h
	src/ecsModule/mesh3d_module/module.h
	src/ecsModule/mesh3d_module/module.cpp
	src/ecsModule/debug_module/module.h
	src/ecsModule/debug_module/module.cpp
	src/ecsModule/editor_module/docking.h
	src/ecsModule/editor_module/docking.cpp
	src/ecsModule/editor_module/immediate.h
	src/ecsModule/editor_module/immediate.cpp
	src/ecsModule/editor_module/components.h
	src/ecsModule/editor_module/module.h
	src/ecsModule/editor_module/module.cpp
	src/ecsModule/default_modules.h
	src/ecsModule/default_modules.cpp
	#src/ecsModule/tweenModule/module.h
	#src/ecsModule/tweenModule/module.cpp
	src/ecsModule/utils.h
	src/ecsModule/utils.cpp
	src/utils/sdl.h
	src/utils/sdl.cpp
	src/color.h
	src/color.cpp
	src/file.h
	src/file.cpp
	src/texture_atlas.h
	src/utils/visit.h
)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/vendor/glaze/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/vendor/box2d/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/vendor/spdlog/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/vendor/glm)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/vendor/matchit)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/vendor/SDL_shadercross/include)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/vendor/tinyobjloader)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/vendor/meshoptimizer)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/src)

target_link_libraries(${PROJECT_NAME} PUBLIC flecs::flecs_static)
target_link_libraries(${PROJECT_NAME} PUBLIC box2d)
target_link_libraries(${PROJECT_NAME} PUBLIC spdlog)
target_link_libraries(${PROJECT_NAME} PUBLIC glm::glm)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::Headers)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3::SDL3-static)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3_image::SDL3_image)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3_ttf::SDL3_ttf)
target_link_libraries(${PROJECT_NAME} PUBLIC SDL3_shadercross::SDL3_shadercross)
target_link_libraries(${PROJECT_NAME} PUBLIC glaze::glaze)
target_link_libraries(${PROJECT_NAME} PUBLIC meshoptimizer)

add_executable(Plague-Survivors
	src/main.cpp
)

target_link_libraries(Plague-Survivors PRIVATE SoulEngine)

get_target_property(LINK_OPTS Plague-Survivors LINK_OPTIONS)
message(STATUS "Plague-Survivors LINK_OPTIONS: ${LINK_OPTS}")

set_target_properties(Plague-Survivors PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR} )
set_target_properties(Plague-Survivors PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR} )
set_target_properties(Plague-Survivors PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})


#set_target_properties(${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++${CMAKE_CXX_STANDARD}" )

#if(WIN32)
#	add_custom_command(
#		TARGET ${PROJECT_NAME}
#		COMMENT "Copy OpenAL DLL"
#		PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SFML_SOURCE_DIR}/extlibs/bin/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/openal32.dll $<TARGET_FILE_DIR:${PROJECT_NAME}>
#		VERBATIM)
#endif()

install(TARGETS ${PROJECT_NAME} Plague-Survivors)
