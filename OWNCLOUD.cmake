set( APPLICATION_NAME       "Ownnas" )
set( APPLICATION_SHORTNAME  "Ownnas" )
set( APPLICATION_EXECUTABLE "Ownnas" )
set( APPLICATION_DOMAIN     "Ownnas.com" )
set( APPLICATION_VENDOR     "Ownnas" )
set( APPLICATION_UPDATE_URL "https://updates.Ownnas.com/client/" CACHE string "URL for updater" )
set( APPLICATION_ICON_NAME  "Ownnas" )

set( LINUX_PACKAGE_SHORTNAME "Ownnas" )

set( THEME_CLASS            "ownNasTheme" )
set( APPLICATION_REV_DOMAIN "com.ownnas.desktopclient" )
set( WIN_SETUP_BITMAP_PATH  "${CMAKE_SOURCE_DIR}/admin/win/nsi" )

set( MAC_INSTALLER_BACKGROUND_FILE "${CMAKE_SOURCE_DIR}/admin/osx/installer-background.png" CACHE STRING "The MacOSX installer background image")

# set( THEME_INCLUDE          "${OEM_THEME_DIR}/mytheme.h" )
# set( APPLICATION_LICENSE    "${OEM_THEME_DIR}/license.txt )

option( WITH_CRASHREPORTER "Build crashreporter" OFF )
set( CRASHREPORTER_SUBMIT_URL "https://crash-reports.owncloud.com/submit" CACHE string "URL for crash reporter" )
set( CRASHREPORTER_ICON ":/owncloud-icon.png" )
