include(GNUInstallDirs)

project(qtmc CXX)

add_executable( ${PROJECT_NAME}
    "src/main.cpp"
)
target_include_directories( ${PROJECT_NAME} 
    PRIVATE $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/external>
)
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)
target_link_libraries(${PROJECT_NAME} PRIVATE qualisys_cpp_sdk)

set(CMAKE_DEFAULT_STARTUP_PROJECT ${PROJECT_NAME})

# 'make install' to the correct locations (provided by GNUInstallDirs).
install(
	TARGETS ${PROJECT_NAME}
	EXPORT ${PROJECT_NAME}-targets
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR} # This is for Windows
)