# Use make EmbedBrowser/fast for fast building!

set (CMAKE_CXX_STANDARD 11)

set(EMBEDBROWSER_DIR "${TOOLS_DIR}/EmbedBrowser")
set(DERIVED_SOURCES_EMBEDBROWSER_DIR "${CMAKE_BINARY_DIR}/DerivedSources/EmbedBrowser")

file(MAKE_DIRECTORY ${DERIVED_SOURCES_EMBEDBROWSER_DIR})

set(EmbedBrowser_SOURCES
    ${EMBEDBROWSER_DIR}/Browser.cpp
    ${EMBEDBROWSER_DIR}/Browser.h
    ${EMBEDBROWSER_DIR}/Extension.cpp
    ${EMBEDBROWSER_DIR}/Extension.h
)

set(EmbedBrowser_INCLUDE_DIRECTORIES
    ${DERIVED_SOURCES_EMBEDBROWSER_DIR}
    ${DERIVED_SOURCES_WEBKIT2GTK_DIR}
    ${FORWARDING_HEADERS_WEBKIT2GTK_DIR}
    ${FORWARDING_HEADERS_DIR}
    ${CMAKE_SOURCE_DIR}/Source
)

set(EmbedBrowser_SYSTEM_INCLUDE_DIRECTORIES
    ${GTK3_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
    ${LIBSOUP_INCLUDE_DIRS}
)

# OS X
#find_library(OSX_COREFOUNDATION CoreFoundation)
find_library(OSX_ICONV iconv)

set(EmbedBrowser_LIBRARIES
    ${JavaScriptCore_LIBRARY_NAME}
    WebKit2
    ${GTK3_LIBRARIES}
    ${GLIB_LIBRARIES}
    ${LIBSOUP_LIBRARIES}
    gnutls
    /usr/local/lib/gio/modules/libgiognomeproxy.a
    /usr/local/lib/gio/modules/libgiognutls.a
#    ${OSX_COREFOUNDATION}
    ${OSX_ICONV}
)

if (DEVELOPER_MODE)
    add_definitions(-DWEBKIT_INJECTED_BUNDLE_PATH="${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
endif ()

add_definitions(-DGDK_VERSION_MIN_REQUIRED=GDK_VERSION_3_6)

include_directories(${EmbedBrowser_INCLUDE_DIRECTORIES})
include_directories(SYSTEM ${EmbedBrowser_SYSTEM_INCLUDE_DIRECTORIES})
add_library(EmbedBrowser SHARED ${EmbedBrowser_SOURCES})
target_link_libraries(EmbedBrowser ${EmbedBrowser_LIBRARIES})

WEBKIT_ADD_TARGET_CXX_FLAGS(EmbedBrowser -Wno-unused-parameter -frtti -g)

install(
    TARGETS EmbedBrowser
    LIBRARY DESTINATION "${LIBEXEC_INSTALL_DIR}"
)
