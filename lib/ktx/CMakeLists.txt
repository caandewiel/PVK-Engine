file(GLOB BASE_SRC "*.cpp")
file(GLOB BASE_HEADERS "*.hpp")

add_library(lib STATIC ${BASE_SRC} ${BASE_HEADERS})
set_target_properties(lib PROPERTIES LINKER_LANGUAGE CXX)
target_link_libraries(lib ${Vulkan_LIBRARY} ${XCB_LIBRARIES} ${WAYLAND_CLIENT_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})