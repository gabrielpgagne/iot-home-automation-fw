# This CMake file is picked by the Zephyr build system because it is defined
# as the module CMake entry point (see zephyr/module.yml).

#zephyr_include_directories(lib)

#find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
#project(alarme_frigo)
add_subdirectory(lib)
#add_subdirectory(app)