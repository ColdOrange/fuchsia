find_package(Catch2 3 REQUIRED)
include(Catch)

function(fuchsia_add_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE fuchsia Catch2::Catch2WithMain)
    catch_discover_tests(${name})
endfunction()

fuchsia_add_test(test_epoll_context)
fuchsia_add_test(test_buffer)
fuchsia_add_test(test_address)
fuchsia_add_test(test_endpoint)
