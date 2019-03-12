include_directories(${ADR_SOURCE_DIR}/main)
link_directories(${ADR_SOURCE_DIR}/main)

add_executable(adr  main.c
                    adr.c)

find_package(Matlab COMPONENTS MAT_LIBRARY MX_LIBRARY)
target_include_directories(adr PUBLIC
  ${Matlab_INCLUDE_DIRS})
target_link_libraries(adr ${Matlab_MAT_LIBRARY} ${Matlab_MX_LIBRARY} m)