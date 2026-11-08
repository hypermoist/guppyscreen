# Builds grumpyscreen as libgrumpyscreen.so for the Android app. The sources
# are the same ones the Makefile builds; only main.cpp is swapped for the JNI
# entry point in grumpy_android.cpp, which renders into the app's Surface.
cmake_minimum_required(VERSION 3.22.1)
project(grumpyscreen C CXX)

if(NOT DEFINED GRUMPY_ROOT)
    get_filename_component(GRUMPY_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../../../.. ABSOLUTE)
endif()
if(NOT DEFINED GUPPYSCREEN_VERSION)
    set(GUPPYSCREEN_VERSION unknown)
endif()
if(NOT DEFINED GUPPYSCREEN_BRANCH)
    set(GUPPYSCREEN_BRANCH unknown)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE OR CMAKE_BUILD_TYPE STREQUAL "Debug")
    # LVGL software rendering is unusably slow unoptimised
    add_compile_options(-O2)
endif()

# --- libhv (websocket + http client) ---------------------------------------
# Same feature set as libhv/config.mk in the Makefile build.
set(BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC ON CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_UNITTEST OFF CACHE BOOL "" FORCE)
set(WITH_EVPP ON CACHE BOOL "" FORCE)
set(WITH_HTTP ON CACHE BOOL "" FORCE)
set(WITH_HTTP_SERVER ON CACHE BOOL "" FORCE)
set(WITH_HTTP_CLIENT ON CACHE BOOL "" FORCE)
# libhv's CMakeLists writes hconfig.h and include/hv back into its source
# tree. Build a private copy so the NDK's answers never reach the checkout the
# Makefile builds use, and each ABI gets its own.
set(LIBHV_COPY ${CMAKE_CURRENT_BINARY_DIR}/libhv-src)
file(COPY ${GRUMPY_ROOT}/libhv/ DESTINATION ${LIBHV_COPY}
     PATTERN .git EXCLUDE PATTERN include EXCLUDE PATTERN lib EXCLUDE)
add_subdirectory(${LIBHV_COPY} ${CMAKE_CURRENT_BINARY_DIR}/libhv)

# --- LVGL --------------------------------------------------------------------
# lvgl.mk compiles everything under lvgl/src; lv_conf.h at the repo root is
# found the same way (lvgl/src/lv_conf_internal.h looks two levels up).
file(GLOB_RECURSE LVGL_SRC ${GRUMPY_ROOT}/lvgl/src/*.c)
add_library(lvgl STATIC ${LVGL_SRC})
target_include_directories(lvgl PUBLIC ${GRUMPY_ROOT} ${GRUMPY_ROOT}/lvgl)

# --- wpa_ctrl --------------------------------------------------------------
# The wifi panel links against it; on Android the panel is never shown, but
# compiling the client keeps the shared sources free of #ifdefs.
set(WPA_DIR ${GRUMPY_ROOT}/wpa_supplicant/src)
add_library(wpa_client STATIC
    ${WPA_DIR}/common/wpa_ctrl.c
    ${WPA_DIR}/utils/os_unix.c
    ${WPA_DIR}/utils/common.c
    ${WPA_DIR}/utils/wpa_debug.c)
target_include_directories(wpa_client PUBLIC ${WPA_DIR} ${WPA_DIR}/utils ${WPA_DIR}/common)
target_compile_definitions(wpa_client PRIVATE CONFIG_CTRL_IFACE CONFIG_CTRL_IFACE_UNIX)
# ANDROID there means "built inside AOSP", with its private headers
target_compile_options(wpa_client PRIVATE -w -UANDROID)

# --- grumpyscreen ------------------------------------------------------------
# Icons: instead of the fixed-size bitmaps in assets/material*, the 256px PNGs
# in assets/material_png (assets/android-icons.sh), which grumpy_icons.c
# resamples to the screen's size, and the nav bar glyphs as a TTF for tiny_ttf.
set(ICON_PNG_DIR ${GRUMPY_ROOT}/assets/material_png)
set(NAV_TTF ${GRUMPY_ROOT}/assets/materialdesign_nav.ttf)
file(GLOB ICON_PNGS ${ICON_PNG_DIR}/*.png)
set(ICONS_C ${CMAKE_CURRENT_BINARY_DIR}/grumpy_icons_data.c)
add_custom_command(
    OUTPUT ${ICONS_C}
    COMMAND ${CMAKE_COMMAND} -DPNG_DIR=${ICON_PNG_DIR} -DNAV_TTF=${NAV_TTF} -DOUT=${ICONS_C}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/embed_assets.cmake
    DEPENDS ${ICON_PNGS} ${NAV_TTF} ${CMAKE_CURRENT_SOURCE_DIR}/embed_assets.cmake
    COMMENT "Embedding icons")
# nav glyphs at the nav bar's size (grumpy_icons.c)
target_compile_definitions(lvgl PUBLIC LV_USE_TINY_TTF=1)

file(GLOB GRUMPY_SRC
    ${GRUMPY_ROOT}/src/*.cpp
    ${GRUMPY_ROOT}/assets/*.c)
list(REMOVE_ITEM GRUMPY_SRC
    ${GRUMPY_ROOT}/src/main.cpp
    # COSMOS only, as in the Makefile
    ${GRUMPY_ROOT}/src/update_manager_client.cpp)

add_library(grumpyscreen SHARED ${GRUMPY_SRC} grumpy_android.cpp grumpy_icons.c ${ICONS_C})

target_include_directories(grumpyscreen PRIVATE
    # experimental/filesystem shim for the NDK's libc++
    ${CMAKE_CURRENT_SOURCE_DIR}/compat
    ${GRUMPY_ROOT}
    ${GRUMPY_ROOT}/src
    ${GRUMPY_ROOT}/fmt/include
    ${LIBHV_COPY}/include
    ${WPA_DIR}/common)

target_compile_definitions(grumpyscreen PRIVATE
    _GNU_SOURCE
    MMU_BACKEND_AFC
    MMU_BACKEND_HH
    GUPPYSCREEN_VERSION="${GUPPYSCREEN_VERSION}"
    GUPPYSCREEN_BRANCH="${GUPPYSCREEN_BRANCH}"
    SWITCH_TO_STOCK_BUTTON_TEXT="Switch to\\nStock"
    SWITCH_TO_STOCK_BUTTON_TITLE="Switch to Stock"
    SWITCH_TO_STOCK_BUTTON_PROMPT="Are you sure you want to switch to stock?"
    SWITCH_TO_STOCK_BUTTON_FAILURE="Failed to initiate switch to stock!"
    SWITCH_TO_STOCK_BUTTON_SUCCESS="Please power cycle your printer!"
    FACTORY_RESET_BUTTON_TEXT="Factory\\nReset"
    FACTORY_RESET_BUTTON_TITLE="Factory Reset"
    FACTORY_RESET_BUTTON_PROMPT="Are you sure you want to factory reset?"
    FACTORY_RESET_BUTTON_FAILURE="Failed to initiate factory reset!"
    FACTORY_RESET_BUTTON_SUCCESS="Your printer will restart shortly!")
if(GUPPY_SMALL_SCREEN)
    target_compile_definitions(grumpyscreen PRIVATE GUPPY_SMALL_SCREEN)
endif()

target_compile_options(grumpyscreen PRIVATE
    -Wno-unused-parameter -Wno-missing-field-initializers -Wno-sign-compare
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-declarations>)

target_link_libraries(grumpyscreen
    lvgl
    hv_static
    wpa_client
    android
    log
    m)
