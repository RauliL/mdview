find_package(PkgConfig)

include(FetchContent)

pkg_check_modules(GTKMM gtkmm-4.0 REQUIRED)
pkg_check_modules(WEBKITGTK webkitgtk-6.0 REQUIRED)

fetchcontent_declare(
  maddy
  URL https://github.com/progsource/maddy/releases/download/1.5.0/maddy-src.zip
  URL_HASH SHA256=eae8990ab796185547ef06dbb752d0d7cbc38067b327a4c7e5141c280aa633fd
)
fetchcontent_makeavailable(maddy)

add_executable(
  mdview
  ./highlight.cpp
  ./main.cpp
  ./window.cpp
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
  PUBLIC
    ${GTKMM_LIBRARIES}
    ${WEBKITGTK_LIBRARIES}
    maddy
)

install(
  TARGETS
    mdview
  RUNTIME DESTINATION
    bin
)
install(
  FILES
    ../mdview.desktop
  DESTINATION
    share/applications
)
