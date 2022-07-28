set(test_files
	main.cpp
)

add_executable(test ${test_files})

target_link_libraries(test passlang)
