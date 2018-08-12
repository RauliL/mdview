CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(mdview C CXX)

FIND_PACKAGE(PkgConfig)

PKG_CHECK_MODULES(GTKMM gtkmm-3.0 REQUIRED)
PKG_CHECK_MODULES(WEBKITGTK webkit2gtk-4.0 REQUIRED)

ADD_EXECUTABLE(
  mdview
  mdview.cpp
  ext/hoedown/src/autolink.c
  ext/hoedown/src/buffer.c
  ext/hoedown/src/document.c
  ext/hoedown/src/escape.c
  ext/hoedown/src/html.c
  ext/hoedown/src/html_blocks.c
  ext/hoedown/src/html_smartypants.c
  ext/hoedown/src/stack.c
  ext/hoedown/src/version.c
)

TARGET_COMPILE_FEATURES(mdview PRIVATE cxx_std_11)
TARGET_INCLUDE_DIRECTORIES(
  mdview
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ext/maddy/include
    ${GTKMM_INCLUDE_DIRS}
    ${WEBKITGTK_INCLUDE_DIRS}
)
TARGET_LINK_LIBRARIES(
  mdview
  ${GTKMM_LIBRARIES}
  ${WEBKITGTK_LIBRARIES}
)
INSTALL(TARGETS mdview RUNTIME DESTINATION bin)
