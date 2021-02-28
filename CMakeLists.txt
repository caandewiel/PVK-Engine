project(pvk)
cmake_minimum_required(VERSION 3.14)

IF(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DVK_USE_PLATFORM_MACOS_MVK")
    SET(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for Mac OS X" FORCE)
ENDIF()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -Wall -Wextra -Wshadow -Wno-unused-parameter -Werror")
#set(CMAKE_CXX_FLAGS_RELEASE "-O2")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fno-omit-frame-pointer ")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Threads REQUIRED)
find_package(Boost REQUIRED)
find_package(GTest REQUIRED)

FILE(GLOB application_src)

include_directories(external)
include_directories(external/gli)
include_directories(external/glfw-3.3.2)
include_directories(external/include)
include_directories(external/include/tiny_gltf)

SET(PUBLIC_HEADERS
        lib/application/application.hpp
        lib/buffer/buffer.hpp
        lib/buffer/uniformBuffer.hpp
        lib/camera/camera.hpp
        lib/commandBuffer/commandBuffer.hpp
        lib/context/context.hpp
        lib/debug/debug.hpp
        lib/device/logicalDevice.hpp
        lib/device/physicalDevice.hpp
        lib/gltf/GLTFLoader.hpp
        lib/gltf/GLTFObject.hpp
        lib/gltf/GLTFNode.hpp
        lib/gltf/GLTFPrimitive.hpp
        lib/gltf/GLTFAnimation.hpp
        lib/image/image.hpp
        lib/ktx/KTXLoader.hpp
        lib/mesh/mesh.hpp
        lib/mesh/vertex.hpp
        lib/object/object.hpp
        lib/pipeline/pipeline.hpp
        lib/pipeline/pipelineBuilder.hpp
        lib/shader/shader.hpp
        lib/texture/texture.hpp
        lib/util/util.hpp
        lib/gltf/GLTFSkin.hpp
        lib/gltf/GLTFMaterial.hpp lib/pipeline/pipelineParser.hpp lib/gltf/Drawable.cpp lib/gltf/Drawable.h)

SET(SOURCES
        lib/buffer/buffer.cpp
        lib/buffer/uniformBuffer.cpp
        lib/camera/camera.cpp
        lib/context/context.cpp
        lib/debug/debug.cpp
        lib/device/logicalDevice.cpp
        lib/device/physicalDevice.cpp
        lib/gltf/GLTFLoader.cpp
        lib/gltf/GLTFObject.cpp
        lib/gltf/GLTFNode.cpp
        lib/gltf/GLTFPrimitive.cpp
        lib/gltf/GLTFAnimation.cpp
        lib/image/image.cpp
        lib/ktx/KTXLoader.cpp
        lib/mesh/vertex.cpp
        lib/object/object.cpp
        lib/pipeline/pipeline.cpp
        lib/pipeline/pipelineBuilder.cpp
        lib/shader/shader.cpp
        lib/texture/texture.cpp
        lib/util/util.cpp
        lib/gltf/GLTFSkin.cpp
        external/proxy/gli.h
        external/proxy/tiny_gltf.h
        lib/gltf/Drawable.cpp
        lib/gltf/Drawable.h)

add_executable(${PROJECT_NAME} main.cpp ${PUBLIC_HEADERS} ${SOURCES})
add_executable(runTests test/pvk_test.cpp ${PUBLIC_HEADERS} ${SOURCES} test/MockApplication.hpp)

if (VULKAN_FOUND)
    message(STATUS "Found Vulkan, Including and Linking now")
    message(${Vulkan_INCLUDE_DIRS} " " ${Vulkan_LIBRARIES})
    message(${ASSIMP_INCLUDE_DIRS} " " ${ASSIMP_LIBRARIES})
    message(${GTEST_INCLUDE_DIRS} " " ${GTEST_LIBRARIES})
    include_directories(${Vulkan_INCLUDE_DIRS})
    include_directories(${Boost_INCLUDE_DIR})
    include_directories(${GTEST_INCLUDE_DIRS})
    target_link_libraries (${PROJECT_NAME} ${Vulkan_LIBRARIES} ${BOOST_LIBRARIES} glfw)
endif (VULKAN_FOUND)

target_link_libraries(runTests /usr/local/lib/libgtest.a /usr/local/lib/libgtest_main.a ${Vulkan_LIBRARIES} ${BOOST_LIBRARIES} glfw)

target_link_libraries(pvk Vulkan::Vulkan Threads::Threads)