add_executable(fairdicetoken.wasm ${CMAKE_CURRENT_SOURCE_DIR}/src/fairdicetoken.cpp)
target_include_directories(fairdicetoken.wasm
   PUBLIC 
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(fairdicetoken.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

add_custom_target(fairdicetoken.abi ALL)

set(ABI_FILE "${CMAKE_CURRENT_BINARY_DIR}/fairdicetoken.abi")

add_custom_command(
    TARGET fairdicetoken.abi
    COMMAND eosiocpp -g  ${ABI_FILE}   "${CMAKE_CURRENT_SOURCE_DIR}/include/fairdicetoken.hpp"
)

add_custom_command(TARGET fairdicetoken.abi POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${ABI_FILE} "${CMAKE_CURRENT_SOURCE_DIR}/abi/fairdicetoken.abi"
)