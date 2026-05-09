QT += core widgets network

CONFIG += c++17
CONFIG -= app_bundle

TEMPLATE = app
TARGET = air_race_hid_tool

INCLUDEPATH += $$PWD/src

SOURCES += \
    src/main.cpp \
    src/core/race_packet.cpp \
    src/core/race_command.cpp \
    src/transport/hid_transport.cpp \
    src/service/race_service.cpp \
    src/service/serial_number_service.cpp \
    src/ui/main_window.cpp \
    src/ui/sn_mode_settings.cpp \
    src/ui/sn_mode_settings_dialog.cpp \
    src/ui/app_paths.cpp

HEADERS += \
    src/core/race_packet.h \
    src/core/race_command.h \
    src/transport/hid_transport.h \
    src/service/race_service.h \
    src/service/serial_number_service.h \
    src/ui/main_window.h \
    src/ui/sn_mode_settings.h \
    src/ui/sn_mode_settings_dialog.h \
    src/ui/app_paths.h

# hidapi setup:
# A) Preferred: use local source tree under usb_tool/hidapi (no prebuilt lib needed).
# B) Optional: use prebuilt library by setting HIDAPI_ROOT.
HIDAPI_LOCAL_ROOT = $$PWD/hidapi

exists($$HIDAPI_LOCAL_ROOT/windows/hid.c):exists($$HIDAPI_LOCAL_ROOT/hidapi/hidapi.h) {
    message("Using local hidapi source from $$HIDAPI_LOCAL_ROOT")
    INCLUDEPATH += $$HIDAPI_LOCAL_ROOT/hidapi
    INCLUDEPATH += $$HIDAPI_LOCAL_ROOT/windows
    SOURCES += $$HIDAPI_LOCAL_ROOT/windows/hid.c
    HEADERS += $$HIDAPI_LOCAL_ROOT/hidapi/hidapi.h

    win32 {
        LIBS += -lsetupapi
    }
} else {
    message("Local hidapi source not found, fallback to HIDAPI_ROOT/prebuilt mode")
    isEmpty(HIDAPI_ROOT) {
        message("HIDAPI_ROOT is not set. Using default linker/include search path.")
    } else {
        INCLUDEPATH += $$HIDAPI_ROOT/include
        INCLUDEPATH += $$HIDAPI_ROOT/include/hidapi
        LIBS += -L$$HIDAPI_ROOT/lib
    }

    # Optional local 3rdparty fallback:
    exists($$PWD/3rdparty/hidapi/include) {
        INCLUDEPATH += $$PWD/3rdparty/hidapi/include
        INCLUDEPATH += $$PWD/3rdparty/hidapi/include/hidapi
    }
    exists($$PWD/3rdparty/hidapi/lib) {
        LIBS += -L$$PWD/3rdparty/hidapi/lib
    }

    win32 {
        LIBS += -lhidapi
    }
}
