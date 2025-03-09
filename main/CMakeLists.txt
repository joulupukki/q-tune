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

    # utils/lcd.c
    # utils/touch.c
    utils/I2C_Driver.c
    utils/ST7701S.c
    utils/Buzzer.c
    utils/TCA9554PWR.c
    utils/multi_button.c

#    utils/ebd4/Buzzer/Buzzer.c
#    utils/ebd4/EXIO/TCA9554PWR.c
#    utils/ebd4/I2C_Driver/I2C_Driver.c
#    utils/ebd4/LCD_Driver/ST7701S.c
#    utils/ebd4/LVGL_Driver/LVGL_Driver.c
)

set(INCLUDE_DIRS
    .
    fonts
    standby-ui
    tuning-ui
    utils
    # utils/ebd4/Buzzer
    # utils/ebd4/EXIO
    # utils/ebd4/I2C_Driver
    # utils/ebd4/LCD_Driver
    # utils/ebd4/LVGL_Driver
)

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${INCLUDE_DIRS}
    REQUIRES esp_lcd driver esp_adc nvs_flash
)

target_include_directories(${COMPONENT_LIB} PRIVATE ${CMAKE_SOURCE_DIR}/extra_components/q/q_lib/include ${CMAKE_SOURCE_DIR}/extra_components/q/infra/include)
add_compile_definitions(PROJECT_VERSION="${PROJECT_VER}")