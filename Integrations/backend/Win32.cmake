# Legacy Win32-api backend for Gamebryo3

set(GB3_BACKEND_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../Source" CACHE PATH "" FORCE)

function(post_backend_operations)
endfunction()

function(pre_backend_operations)
endfunction()

function(add_backend_target_defines target link)
    # We don't need to define EE_PLATFORM_WIN32 as it's always implied
endfunction()
