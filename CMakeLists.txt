cmake_minimum_required( VERSION 3.22.1 )

#-----------------------------------------------------
# Cmake debug flag
#-----------------------------------------------------
set(CMAKE_VERBOSE_MAKEFILE OFF)

#--------------------------------------------------------
# Check cmake option conflicts
#--------------------------------------------------------
include(cmake/CheckOptions.cmake)

#--------------------------------------------------------
# Configure compiler/project
#--------------------------------------------------------
project(proxyapp LANGUAGES CXX)
include(cmake/CompilerConfig.cmake) # This must go *after* project()
include(cmake/ProxyConfig.cmake)    # This must go *before* other configurations

#--------------------------------------------------------
# Configure
#--------------------------------------------------------
include(cmake/KokkosConfig.cmake)

#--------------------------------------------------------
# Custom cmake functions
#--------------------------------------------------------
include(cmake/KokkosUtils.cmake)
include(cmake/ProxyUtils.cmake)

#--------------------------------------------------------
# Subdirectories
#--------------------------------------------------------
add_subdirectory(src)

#--------------------------------------------------------
# Print configuration summary
#--------------------------------------------------------
print_configuration_summary()
