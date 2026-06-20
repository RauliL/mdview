find_package(PkgConfig)

pkg_check_modules(GTKMM gtkmm-4.0 REQUIRED)
pkg_check_modules(WEBKITGTK webkitgtk-6.0 REQUIRED)

add_executable(
  mdview
  ./highlight.cpp
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

target_compile_features(
  mdview
  PRIVATE
    cxx_std_17
)
target_include_directories(
  mdview
  PRIVATE
    ${GTKMM_INCLUDE_DIRS}
    ${WEBKITGTK_INCLUDE_DIRS}
)
target_link_libraries(
  mdview
  ${GTKMM_LIBRARIES}
  ${WEBKITGTK_LIBRARIES}
)

install(
  TARGETS
    mdview
  RUNTIME DESTINATION
    bin
)
install(
  FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/../mdview.desktop
  DESTINATION
    share/applications
)
