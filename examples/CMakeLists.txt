option(ENABLE_THREAD_SANITIZER "Enable thread sanitizer" OFF)

function(fuchsia_add_example name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE fuchsia)
    target_compile_options(${name} PRIVATE -fconcepts-diagnostics-depth=10) # for diagnostics on concepts

    if (ENABLE_THREAD_SANITIZER)
        target_compile_options(${name} PRIVATE -fsanitize=thread)
        target_link_options(${name} PRIVATE -fsanitize=thread)
    endif ()
endfunction()

fuchsia_add_example(hello)
fuchsia_add_example(hello_coro)
fuchsia_add_example(echo_server)
fuchsia_add_example(echo_server_coro)
