# get_git_sha1(<var>): HEAD SHA1 of the repo rooted at CMAKE_CURRENT_SOURCE_DIR.
# Include by full path with no include guard, else a FetchContent parent's copy runs.

function(get_git_sha1 _var)
  set(_sha1 "GIT-hash-notfound")
  find_package(Git QUIET)
  if(Git_FOUND)
    # safe.directory: containers (cibuildwheel) build checkouts owned by another user
    set(_git "${GIT_EXECUTABLE}" -c safe.directory=*)
    execute_process(
      COMMAND ${_git} rev-parse --show-prefix
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      OUTPUT_VARIABLE _prefix
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
      RESULT_VARIABLE _res)
    # non-empty prefix: a git-less tree inside someone else's checkout
    if(_res EQUAL 0 AND _prefix STREQUAL "")
      execute_process(
        COMMAND ${_git} rev-parse HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE _head
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _res)
      if(_res EQUAL 0)
        set(_sha1 "${_head}")
      endif()

      execute_process(
        COMMAND ${_git} rev-parse --git-path logs/HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE _reflog
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
      get_filename_component(_reflog "${_reflog}" ABSOLUTE
        BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
      if(EXISTS "${_reflog}")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_reflog}")
      endif()
    endif()
  endif()
  set(${_var} "${_sha1}" PARENT_SCOPE)
endfunction()
