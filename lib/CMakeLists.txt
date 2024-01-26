find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

#zephyr_library_named(button)
#zephyr_library_sources(src/button.c)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})

zephyr_include_directories(include)

