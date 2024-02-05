
set(output ${CMAKE_BINARY_DIR}/lightwave_blender.zip)
add_custom_target(lightwave_blender ALL
    COMMAND ${CMAKE_COMMAND} -E tar cf "${output}" --format=zip -- "${CMAKE_CURRENT_SOURCE_DIR}/lightwave_blender"
    COMMENT "Creating blender plugin"
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
install(FILES "${output}" DESTINATION plugins COMPONENT plugins)
