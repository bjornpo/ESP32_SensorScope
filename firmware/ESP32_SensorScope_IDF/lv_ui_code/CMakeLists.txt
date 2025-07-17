idf_component_register(SRCS 
    "appGUI.c"
    "serial_analyzer.c"
    "ui_settings.c"
    "Circuitboard_dark_landscape.c"
    "oscilloscope_icon_65.c"
    "analog_icon_65.c"
    "modbus_icon_65.c"
    "protocol_analyzer_icon_65.c"
    "SDI_12_icon_65.c"
    "serial_analyzer_icon_65.c"
    "Settings_icon_65.c"
    "terminal_icon_65.c"
    INCLUDE_DIRS "."
    REQUIRES lvgl
    REQUIRES main)