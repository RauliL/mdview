FIND_PACKAGE(PkgConfig)

PKG_CHECK_MODULES(GTKMM gtkmm-3.0 REQUIRED)
PKG_CHECK_MODULES(WEBKITGTK webkit2gtk-4.1)
IF(NOT ${WEBKITGTK_FOUND})
  PKG_CHECK_MODULES(WEBKITGTK webkit2gtk-4.0 REQUIRED)
ENDIF()

ADD_EXECUTABLE(
  mdview
  ./main.cpp
  ./window.cpp
  ../ext/hoedown/src/autolink.c
  ../ext/hoedown/src/buffer.c
  ../ext/hoedown/src/document.c
  ../ext/hoedown/src/escape.c
  ../ext/hoedown/src/html.c
  ../ext/hoedown/src/html_blocks.c
  ../ext/hoedown/src/html_smartypants.c
  ../ext/hoedown/src/stack.c
  ../ext/hoedown/src/version.c
)

TARGET_COMPILE_FEATURES(
  mdview
  PRIVATE
    cxx_std_11
)
TARGET_INCLUDE_DIRECTORIES(
  mdview
  PRIVATE
    ${GTKMM_INCLUDE_DIRS}
    ${WEBKITGTK_INCLUDE_DIRS}
)
TARGET_LINK_LIBRARIES(
  mdview
  ${GTKMM_LIBRARIES}
  ${WEBKITGTK_LIBRARIES}
)

INSTALL(
  TARGETS
    mdview
  RUNTIME DESTINATION
    bin
)
INSTALL(
  FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../mdview.desktop
  DESTINATION
    share/applications
)
