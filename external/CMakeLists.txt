include(FetchContent)

set(BUILD_SHARED_LIBS OFF)

option(GLFW_BUILD_TESTS "" OFF)
option(GLFW_BUILD_DOCS "" OFF)
option(GLFW_INSTALL "" OFF)
option(GLFW_BUILD_EXAMPLES "" OFF)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw
        GIT_TAG 3.3.2
)

option(ZLIB_ENABLE_TESTS "" OFF)
option(WITH_NATIVE_INSTRUCTIONS "" ON)
FetchContent_Declare(
        zlib
        GIT_REPOSITORY https://github.com/zlib-ng/zlib-ng
        GIT_TAG 2.0.2
)

FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm
        GIT_TAG 0.9.9.8
)

FetchContent_MakeAvailable(glfw zlib glm)