
#idf_component_register(SRCS "settings.c" "Circuitboard_dark_landscape.c" "appGUI.c" "display_driver.c" "main.c"
#                    INCLUDE_DIRS ".")
idf_component_register(SRCS 
    #"Circuitboard_dark_landscape.c" 
    #"appGUI.c" 
    "display_driver.c" 
    "main.c"
    "settings.c"
    "serial_analyzer_rmt.c"
    "oscilloscope.c"
    INCLUDE_DIRS ".")

