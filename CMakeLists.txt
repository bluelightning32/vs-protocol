cmake_minimum_required(VERSION 3.21)
project(vs_protocol
        VERSION 1.0
        DESCRIPTION "Wireshark dissector the Vintage Story"
        )

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Wireshark 4.0 CONFIG REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZSTD REQUIRED libzstd)

SET(PLUGIN_DIR "${Wireshark_PLUGIN_INSTALL_DIR}" CACHE STRING
    "Directory to install the dissector plugin"
    )
SET(PROTOBUF_DIR
  "${Wireshark_INSTALL_PREFIX}/share/wireshark/protobuf"
    CACHE STRING "Directory to install protobuf files into")

# Turn on all warnings and set the warnings as errors.
if(MSVC)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  add_compile_options(/W3 /WX)
else()
  add_compile_options(-Wall -Werror)
endif()

# Declare the plugin that will be built, and list all of its cpp sources (do
# not include headers here).
add_library(vintage_story SHARED
            "client_id.cpp"
            "dissector.cpp"
            "server_id.cpp"
            )

# Link the plugin against the Wireshark epan library. The PRIVATE keyword means
# that other programs that link against the plugin will not also directly link
# against epan. Since nothing will compile time link against the plugin, the
# PRIVATE keyword basically has no effect for this library.
target_link_libraries(vintage_story PRIVATE epan)

# Include the zstd library. Wireshark will have builtin support for zstd
# through the `tvb_child_uncompress_zstd` function in the 4.2 release, but it
# is not in a stable release yet.
target_link_libraries(vintage_story PRIVATE ${ZSTD_LIBRARIES})
target_include_directories(vintage_story PRIVATE ${ZSTD_INCLUDE_DIRS})
target_compile_options(vintage_story PRIVATE ${ZSTD_CFLAGS_OTHER})

# This is an epan plugin. So install into ${CMAKE_INSTALL_LIBDIR}/epan, so that
# Wireshark loads it. If it were installed directly into the versioned plugins
# directory (if "epan" was left off), then Wireshark would ignore it.
install(TARGETS vintage_story LIBRARY DESTINATION "${PLUGIN_DIR}/epan")

install(FILES vintage_story.proto DESTINATION "${PROTOBUF_DIR}")
