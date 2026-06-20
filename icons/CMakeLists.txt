install(
  FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/mdview.svg
  DESTINATION
    share/icons/hicolor/scalable/apps
)

foreach(ICON_SIZE 16 32 48 64 128 256)
  install(
    FILES
      ${CMAKE_CURRENT_SOURCE_DIR}/mdview${ICON_SIZE}.png
    DESTINATION
      share/icons/hicolor/${ICON_SIZE}x${ICON_SIZE}/apps
    RENAME
      mdview.png
  )
endforeach()
