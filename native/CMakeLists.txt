cmake_minimum_required(VERSION 3.25)
project(Wallfall CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---------- Ready-made libraries, downloaded once into deps/ ----------
include(FetchContent)
set(FETCHCONTENT_BASE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps)
FetchContent_Declare(skia DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  URL https://github.com/JetBrains/skia-pack/releases/download/m144-e2e6623374-4/Skia-m144-e2e6623374-4-windows-Release-x64.zip
  URL_HASH SHA256=50869b098a4f72f5843687093a1a8fdd94e5d61d98397024bb4e521227fedcba)
FetchContent_Declare(sdl3 DOWNLOAD_EXTRACT_TIMESTAMP TRUE
  URL https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-devel-3.4.18-VC.zip
  URL_HASH SHA256=78d84602ae616cfe26b33a73b7d3b9b6b56e8a6650e53d92c9c6ca938f747acd)
FetchContent_MakeAvailable(skia sdl3)

set(SDL3_DIR ${sdl3_SOURCE_DIR}/cmake)
find_package(SDL3 REQUIRED CONFIG)
set(SKIA_LIBS ${skia_SOURCE_DIR}/out/Release-windows-x64)

# ---------- The proof ----------
add_executable(wallfall-proof src/proof.cpp src/canvas.cpp src/sound.cpp src/replay.cpp)
target_include_directories(wallfall-proof SYSTEM PRIVATE ${skia_SOURCE_DIR} ${skia_SOURCE_DIR}/third_party/externals/harfbuzz/src)
target_compile_definitions(wallfall-proof PRIVATE SK_RELEASE NOMINMAX WIN32_LEAN_AND_MEAN _CRT_SECURE_NO_WARNINGS)
target_compile_options(wallfall-proof PRIVATE /utf-8 /W3 /external:W0)
target_link_libraries(wallfall-proof PRIVATE SDL3::SDL3
  ${SKIA_LIBS}/skia.lib ${SKIA_LIBS}/libpng.lib ${SKIA_LIBS}/zlib.lib ${SKIA_LIBS}/skcms.lib ${SKIA_LIBS}/harfbuzz.lib
  dwrite user32 gdi32 ole32 usp10 advapi32 d3d12 dxgi d3dcompiler opengl32 windowscodecs)
add_custom_command(TARGET wallfall-proof POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:wallfall-proof>
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:wallfall-proof>/assets)
