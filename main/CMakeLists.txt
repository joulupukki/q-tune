# ESP32-CYD
set(SRCS
    main.cpp
    gpio_task.cpp
    pitch_detector_task.cpp
    tuner_gui_task.cpp
    tuner_controller.cpp
    user_settings.cpp

    fonts/fontawesome_48.c
    fonts/raleway_128.c
    fonts/tuner_font_images.c
    fonts/tuner_font_image_a2x.c
    fonts/tuner_font_image_b2x.c
    fonts/tuner_font_image_c2x.c
    fonts/tuner_font_image_d2x.c
    fonts/tuner_font_image_e2x.c
    fonts/tuner_font_image_f2x.c
    fonts/tuner_font_image_g2x.c
    fonts/tuner_font_image_none2x.c
    fonts/tuner_font_image_sharp2x.c

    standby-ui/standby_ui_blank.cpp

    tuning-ui/tuner_ui_attitude.cpp
    tuning-ui/tuner_ui_needle.cpp
    tuning-ui/tuner_ui_strobe.cpp
    tuning-ui/tuner_ui_note_quiz.cpp

    utils/OneEuroFilter.cpp

    waveshare/CST328.c
#    waveshare/esp_lcd_touch/esp_lcd_touch.c
    waveshare/I2C_Driver.c
    waveshare/LVGL_Driver.c
    waveshare/ST7789.c
    waveshare/Vernon_ST7789T/Vernon_ST7789T.c
    waveshare/waveshare.c
)

set(INCLUDE_DIRS
    .
    fonts
    hardware
    standby-ui
    tuning-ui
    utils
    waveshare
#    waveshare/esp_lcd_touch
    waveshare/Vernon_ST7789T
)

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES esp_lcd driver esp_adc nvs_flash
)

target_include_directories(${COMPONENT_LIB} PRIVATE ${CMAKE_SOURCE_DIR}/extra_components/q/q_lib/include ${CMAKE_SOURCE_DIR}/extra_components/q/infra/include)
add_compile_definitions(PROJECT_VERSION="${PROJECT_VER}")