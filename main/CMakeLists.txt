# ESP32-CYD
set(SRCS
    main.cpp
    globals.cpp
    gpio_task.cpp
    pitch_detector_task.cpp
    tuner_gui_task.cpp
    tuner_controller.cpp
    user_settings.cpp

    fonts/fontawesome_48.c
    fonts/raleway_128.c
    fonts/tuner_font_images.c

    standby-ui/standby_ui_blank.cpp

    tuning-ui/tuner_ui_attitude.cpp
    tuning-ui/tuner_ui_needle.cpp
    tuning-ui/tuner_ui_strobe.cpp

    utils/lcd.c
    utils/touch.c
    utils/OneEuroFilter.cpp
)

set(INCLUDE_DIRS
    .
    fonts
    standby-ui
    tuning-ui
    utils
)

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES esp_lcd driver esp_adc nvs_flash
)

target_include_directories(${COMPONENT_LIB} PRIVATE ${CMAKE_SOURCE_DIR}/extra_components/q/q_lib/include ${CMAKE_SOURCE_DIR}/extra_components/q/infra/include)
add_compile_definitions(PROJECT_VERSION="${PROJECT_VER}")