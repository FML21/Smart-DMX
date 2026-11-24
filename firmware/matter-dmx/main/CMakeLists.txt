idf_component_register(SRCS "app_main.cpp" "dmx_output.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES esp_matter esp_matter_console nvs_flash driver esp_timer)
target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
