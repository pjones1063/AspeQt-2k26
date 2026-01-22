# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "AspeQt_autogen"
  "CMakeFiles/AspeQt_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/AspeQt_autogen.dir/ParseCache.txt"
  )
endif()
