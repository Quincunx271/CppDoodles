set(_indirect_call_file "${CMAKE_BINARY_DIR}/_indirect_call.cmake" CACHE INTERNAL "" FORCE)

# Untested. This is a model of the idea of how one would do this.

# Calls a function whose name is stored in a string. One cannot write:
#
#     "${name}"(some args)
#
# However, with this function, one can write:
#
#    indirect_call("${name}" some args)
macro(indirect_call _indirect_call_name)
  # Create a string which holds the code which would call the function
  #
  #     "${name}"(some args)
  list(JOIN ARGN " " _indirect_call_args)
  
  # Write this to a file (finishing creating the function call as well)
  file(WRITE "${_indirect_call_file}" "${_indirect_call_name}(${_indirect_call_args})")
  
  # include(...) the file we just wrote.
  include("${_indirect_call_file}")
  
  # One might consider hashing the arguments and using
  #
  #     "${CMAKE_BINARY_DIR}/_indirect_call/${name}-${hash}.cmake"
  #
  # However, since this is just a function call and nothing more complex,
  # I'd say there's no need to do that. It's not important to be able to
  # see exactly what code CMake executed by include(...)ing the file, since
  # it should already be clear from the indirect_call(...) invocation.
endmacro()
