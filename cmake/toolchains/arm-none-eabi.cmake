# cmake/toolchains/arm-none-eabi.cmake — Cortex-M cross-toolchain.
#
# PARITY: rlvgl/.cargo/config.toml + memory.x (Cortex-M7 + fpv5-d16).
# DELTA:  C++20 path, so we additionally call __libc_init_array() in
#         the reset handler — Rust's cortex-m-rt doesn't.
#
# Used by PLAT-02a+ targets (STM32H747I-DISCO). Other ARM Cortex-M
# boards may reuse this file; the cpu/fpu flag bundle is selectable
# by the consuming target via the LVGLPP_ARM_CPU / LVGLPP_ARM_FPU
# cache vars below (defaults match Cortex-M7 + double-precision).

set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# CMake test-compiles a small program when probing a new compiler.
# For a freestanding cross we cannot link against host libc, so tell
# CMake to verify the compiler by building a static library only.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Locate arm-none-eabi-* binaries. Search PATH first, then the most
# common Homebrew (macOS) and apt (Linux) install prefixes.
set(_lvglpp_arm_search_paths
    "$ENV{PATH}"
    /opt/homebrew/bin
    /opt/homebrew/Cellar/gcc-arm-embedded
    /usr/local/bin
    /usr/local/gcc-arm-none-eabi
    /usr/bin
)

find_program(LVGLPP_ARM_GCC      arm-none-eabi-gcc      PATHS ${_lvglpp_arm_search_paths})
find_program(LVGLPP_ARM_GXX      arm-none-eabi-g++      PATHS ${_lvglpp_arm_search_paths})
find_program(LVGLPP_ARM_AS       arm-none-eabi-as       PATHS ${_lvglpp_arm_search_paths})
find_program(LVGLPP_ARM_AR       arm-none-eabi-ar       PATHS ${_lvglpp_arm_search_paths})
find_program(LVGLPP_ARM_OBJCOPY  arm-none-eabi-objcopy  PATHS ${_lvglpp_arm_search_paths})
find_program(LVGLPP_ARM_OBJDUMP  arm-none-eabi-objdump  PATHS ${_lvglpp_arm_search_paths})
find_program(LVGLPP_ARM_SIZE     arm-none-eabi-size     PATHS ${_lvglpp_arm_search_paths})

if(NOT LVGLPP_ARM_GCC OR NOT LVGLPP_ARM_GXX)
    message(FATAL_ERROR
        "arm-none-eabi-gcc / arm-none-eabi-g++ not found.\n"
        "Install the GNU Arm Embedded Toolchain >= 11.0:\n"
        "  macOS:  brew install --cask gcc-arm-embedded\n"
        "  Linux:  sudo apt install gcc-arm-none-eabi g++-arm-none-eabi\n"
        "  Manual: https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads\n")
endif()

set(CMAKE_C_COMPILER       "${LVGLPP_ARM_GCC}"     CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER     "${LVGLPP_ARM_GXX}"     CACHE FILEPATH "" FORCE)
set(CMAKE_ASM_COMPILER     "${LVGLPP_ARM_GCC}"     CACHE FILEPATH "" FORCE)
set(CMAKE_AR               "${LVGLPP_ARM_AR}"      CACHE FILEPATH "" FORCE)
set(CMAKE_OBJCOPY          "${LVGLPP_ARM_OBJCOPY}" CACHE FILEPATH "" FORCE)
set(CMAKE_OBJDUMP          "${LVGLPP_ARM_OBJDUMP}" CACHE FILEPATH "" FORCE)
set(CMAKE_SIZE             "${LVGLPP_ARM_SIZE}"    CACHE FILEPATH "" FORCE)

# Cortex-M7 + double-precision FPU. STM32H747XIH6 is an M7 with
# fpv5-d16 (matches RM0399 §A.4).
set(LVGLPP_ARM_CPU        "cortex-m7"  CACHE STRING "Target Arm CPU")
set(LVGLPP_ARM_FPU        "fpv5-d16"   CACHE STRING "Target Arm FPU")
set(LVGLPP_ARM_FLOAT_ABI  "hard"       CACHE STRING "Target Arm float ABI")

set(_lvglpp_arm_cpu_flags
    "-mcpu=${LVGLPP_ARM_CPU}"
    "-mthumb"
    "-mfpu=${LVGLPP_ARM_FPU}"
    "-mfloat-abi=${LVGLPP_ARM_FLOAT_ABI}"
)
string(REPLACE ";" " " _lvglpp_arm_cpu_flags_str "${_lvglpp_arm_cpu_flags}")

set(CMAKE_C_FLAGS_INIT     "${_lvglpp_arm_cpu_flags_str} -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS_INIT   "${_lvglpp_arm_cpu_flags_str} -ffunction-sections -fdata-sections -fno-threadsafe-statics")
set(CMAKE_ASM_FLAGS_INIT   "${_lvglpp_arm_cpu_flags_str}")
set(CMAKE_EXE_LINKER_FLAGS_INIT
    "${_lvglpp_arm_cpu_flags_str} --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections -Wl,--print-memory-usage")

# Embedded posture is mandatory on a cross build. A consumer that
# overrides this is misconfiguring the target.
set(LVGLPP_EMBEDDED_POSTURE ON CACHE BOOL "Mirror rlvgl no_std + panic=abort" FORCE)

# Don't search the host filesystem for libraries / includes / programs.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Helper macro for downstream targets: produce .bin / .hex alongside
# the .elf, and print size summary. Macro (not function) so the
# add_custom_command() runs in the caller's directory scope, satisfying
# CMP0040's same-directory rule for TARGET-form custom commands.
macro(lvglpp_add_disco_artifacts target)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${target}> $<TARGET_FILE_DIR:${target}>/${target}.bin
        COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${target}> $<TARGET_FILE_DIR:${target}>/${target}.hex
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${target}>
        BYPRODUCTS
            $<TARGET_FILE_DIR:${target}>/${target}.bin
            $<TARGET_FILE_DIR:${target}>/${target}.hex
        VERBATIM)
endmacro()
