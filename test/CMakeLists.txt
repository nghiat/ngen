add_executable(core_test
  core/bit_stream_test.cpp
  core/command_line_test.cpp
  # core/dynamic_array_test.cpp
  core/hash_map_test.cpp
  core/intrusive_list_test.cpp
  core/linear_allocator_test.cpp
  core/loader/xml_test.cpp
  core/path_test.cpp
  core/string_test.cpp
  core/string_utils_test.cpp
  core/utils_test.cpp
  main.cpp
)

target_link_libraries(core_test core)
