cmake_minimum_required(VERSION 3.22)
project(SyncLens VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    WebEngineWidgets
    Network
)

qt_add_executable(synclens
    src/main.cpp

    src/app/Application.h
    src/app/Application.cpp
    src/app/MainWindow.h
    src/app/MainWindow.cpp
    src/app/SyncInstance.h
    src/app/SyncInstance.cpp

    src/webview/WebViewHost.h
    src/webview/WebViewHost.cpp

    src/health/HealthMonitor.h
    src/health/HealthMonitor.cpp

    src/tray/TrayController.h
    src/tray/TrayController.cpp

    src/settings/SettingsManager.h
    src/settings/SettingsManager.cpp
    src/settings/CredentialStore.h
    src/settings/CredentialStore.cpp

    src/ui/AboutDialog.h
    src/ui/AboutDialog.cpp
    src/ui/SettingsDialog.h
    src/ui/SettingsDialog.cpp
    src/ui/InstanceManagerDialog.h
    src/ui/InstanceManagerDialog.cpp

    resources/resources.qrc
)

target_link_libraries(synclens PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Network
)

# Optional: libsecret for secure credential persistence via the
# freedesktop Secrets Service (GNOME Keyring / KWallet).
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBSECRET QUIET libsecret-1)
    if(LIBSECRET_FOUND)
        message(STATUS "libsecret found — credential persistence enabled")
        target_compile_definitions(synclens PRIVATE HAS_LIBSECRET)
        target_include_directories(synclens PRIVATE ${LIBSECRET_INCLUDE_DIRS})
        target_link_libraries(synclens PRIVATE ${LIBSECRET_LIBRARIES})
    else()
        message(STATUS "libsecret not found — credentials not persisted across sessions (install libsecret-1-dev / libsecret to enable)")
    endif()
endif()

target_include_directories(synclens PRIVATE src)

install(TARGETS synclens DESTINATION bin)
install(FILES resources/synclens.desktop DESTINATION share/applications)
install(FILES resources/icons/synclens.png DESTINATION share/icons/hicolor/512x512/apps RENAME synclens.png)
# systemd user service — installed to the per-user unit directory so users
# can opt in with: systemctl --user enable synclens.service
install(FILES resources/synclens.service DESTINATION lib/systemd/user)
