file(GLOB BASE_SRC "*.cpp")
file(GLOB BASE_HEADERS "*.hpp")

add_library(base STATIC ${BASE_SRC} ${BASE_HEADERS})
target_link_libraries(base ${Vulkan_LIBRARY} ${XCB_LIBRARIES} ${WAYLAND_CLIENT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})