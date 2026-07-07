cmake_minimum_required(VERSION 3.22)

add_library(phonevr_shared STATIC
    PhoneVRNetwork.cpp
)

target_include_directories(phonevr_shared PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(phonevr_shared PRIVATE
    bcrypt
)
