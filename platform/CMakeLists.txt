# lvglpp::platform — display, input, and storage backends.
#
# PARITY: rlvgl/platform (v0.2.0). Each backend lands per-chapter and
# is opt-in via its own CMake option.

# ---------------------------------------------------------------------------
# Per-backend options
# ---------------------------------------------------------------------------
option(LVGLPP_PLATFORM_HOST_SDL
    "Build the host SDL2 backend (PLAT-01). Requires SDL2."
    OFF)

# Future:
# option(LVGLPP_PLATFORM_STM32H747I_DISCO ... PLAT-02)
# option(LVGLPP_PLATFORM_BEAGLEBONE_BLACK ... PLAT-03)
# option(LVGLPP_PLATFORM_ESP32_LCD       ... PLAT-04)

# Embedded posture is mutually exclusive with any host backend.
if(LVGLPP_EMBEDDED_POSTURE AND LVGLPP_PLATFORM_HOST_SDL)
    message(FATAL_ERROR
        "LVGLPP_EMBEDDED_POSTURE=ON is incompatible with "
        "LVGLPP_PLATFORM_HOST_SDL=ON — see "
        "docs/platform-host-sdl/00-host-sdl-backend.md §5.5.")
endif()

# ---------------------------------------------------------------------------
# Backend selection
# ---------------------------------------------------------------------------
set(LVGLPP_PLATFORM_SOURCES "")

if(LVGLPP_PLATFORM_HOST_SDL)
    find_package(SDL2 QUIET CONFIG)
    if(NOT SDL2_FOUND)
        # Fall back to pkg-config / older Find module.
        find_package(SDL2 QUIET)
    endif()
    if(NOT SDL2_FOUND)
        message(FATAL_ERROR
            "LVGLPP_PLATFORM_HOST_SDL=ON but SDL2 was not found. "
            "Install SDL2 (macOS: 'brew install sdl2'; Debian/Ubuntu: "
            "'apt install libsdl2-dev') or set SDL2_DIR.")
    endif()
    list(APPEND LVGLPP_PLATFORM_SOURCES src/host_sdl/host_sdl.cpp)
endif()

# ---------------------------------------------------------------------------
# Library target
# ---------------------------------------------------------------------------
if(LVGLPP_PLATFORM_SOURCES)
    add_library(lvglpp_platform ${LVGLPP_PLATFORM_SOURCES})

    target_include_directories(lvglpp_platform
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(lvglpp_platform
        PUBLIC
            lvglpp::core
        PRIVATE
            lvglpp_warnings
    )
    target_compile_features(lvglpp_platform PUBLIC cxx_std_20)

    if(LVGLPP_PLATFORM_HOST_SDL)
        target_compile_definitions(lvglpp_platform
            PUBLIC LVGLPP_PLATFORM_HOST_SDL_ENABLED=1)
        # Prefer the imported target SDL2::SDL2 (modern CONFIG mode);
        # fall back to SDL2_LIBRARIES + SDL2_INCLUDE_DIRS if older
        # find module supplied them.
        if(TARGET SDL2::SDL2)
            target_link_libraries(lvglpp_platform PUBLIC SDL2::SDL2)
        else()
            target_include_directories(lvglpp_platform PUBLIC ${SDL2_INCLUDE_DIRS})
            target_link_libraries(lvglpp_platform PUBLIC ${SDL2_LIBRARIES})
        endif()
    endif()
else()
    # No backend selected — keep an INTERFACE umbrella so consumers
    # can still link `lvglpp::platform` without having opted into a
    # backend.
    add_library(lvglpp_platform INTERFACE)
    target_include_directories(lvglpp_platform
        INTERFACE
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
    )
    target_link_libraries(lvglpp_platform INTERFACE lvglpp::core)
    target_compile_features(lvglpp_platform INTERFACE cxx_std_20)
endif()

add_library(lvglpp::platform ALIAS lvglpp_platform)

# ---------------------------------------------------------------------------
# Tests — SDL-independent (Screen value type, etc.). Gated only on
# LVGLPP_BUILD_TESTS, never on a backend option, so they run on a plain
# host configure. enable_testing() is called by the top-level CMakeLists
# before this directory is added.
# ---------------------------------------------------------------------------
if(LVGLPP_BUILD_TESTS)
    add_subdirectory(tests)
endif()
