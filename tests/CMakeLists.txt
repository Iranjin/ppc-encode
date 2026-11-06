add_executable(ppc-encode-test encode_test.cpp)
target_link_libraries(ppc-encode-test PRIVATE ppc::encode)
target_compile_options(ppc-encode-test PRIVATE
        $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic -Werror>)
add_test(NAME encode COMMAND ppc-encode-test)

# Each file must be rejected at compile time by encodeError, not by an unrelated mistake.
file(GLOB PPC_ENCODE_FAIL_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/fail/*.cpp)
foreach(source IN LISTS PPC_ENCODE_FAIL_SOURCES)
    get_filename_component(name ${source} NAME_WE)
    add_test(NAME fail.${name}
            COMMAND ${CMAKE_CXX_COMPILER} -std=c++20 -fsyntax-only
                    -I${PROJECT_SOURCE_DIR}/include ${source})
    set_tests_properties(fail.${name} PROPERTIES PASS_REGULAR_EXPRESSION "encodeError")
endforeach()
