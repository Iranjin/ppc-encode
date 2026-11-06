cmake_minimum_required(VERSION 3.20)

project(ppc-encode VERSION 1.0.0 LANGUAGES CXX)

add_library(ppc-encode INTERFACE)
add_library(ppc::encode ALIAS ppc-encode)
target_compile_features(ppc-encode INTERFACE cxx_std_20)
set_target_properties(ppc-encode PROPERTIES EXPORT_NAME encode)
target_include_directories(ppc-encode INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>)

option(PPC_ENCODE_BUILD_TESTS "Build the ppc-encode tests" ${PROJECT_IS_TOP_LEVEL})

if(PPC_ENCODE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(PROJECT_IS_TOP_LEVEL)
    include(GNUInstallDirs)
    include(CMakePackageConfigHelpers)

    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(TARGETS ppc-encode EXPORT ppc-encode-targets)
    install(EXPORT ppc-encode-targets
            NAMESPACE ppc::
            DESTINATION ${CMAKE_INSTALL_DATADIR}/ppc-encode)
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/ppc-encode-config.cmake
            "include(\"\${CMAKE_CURRENT_LIST_DIR}/ppc-encode-targets.cmake\")\n")
    write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/ppc-encode-config-version.cmake
            COMPATIBILITY SameMajorVersion ARCH_INDEPENDENT)
    install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/ppc-encode-config.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/ppc-encode-config-version.cmake
            DESTINATION ${CMAKE_INSTALL_DATADIR}/ppc-encode)
endif()
