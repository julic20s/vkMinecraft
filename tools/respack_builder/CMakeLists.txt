add_executable(respack_builder)

target_include_directories(respack_builder PRIVATE .)

target_link_libraries(respack_builder PRIVATE respack stb_image nlohmann_json)

aux_source_directory(. RESPACK_BUILDER_SRC)
aux_source_directory(interfaces RESPACK_BUILDER_INTERFACES_SRC)
aux_source_directory(collectors RESPACK_BUILDER_COLLECTORS_SRC)
target_sources(
    respack_builder PRIVATE
    ${RESPACK_BUILDER_SRC}
    ${RESPACK_BUILDER_INTERFACES_SRC}
    ${RESPACK_BUILDER_COLLECTORS_SRC}
)

if (MSVC)
    SET(
        RESPACK_BUILDER_BINARY
        ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/respack_builder
        CACHE STRING "respack builder binary path" FORCE
    )
else()
SET(
    RESPACK_BUILDER_BINARY
    ${CMAKE_CURRENT_BINARY_DIR}/respack_builder
    CACHE PATH "respack builder binary path" FORCE
)
endif()
