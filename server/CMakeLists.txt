cmake_minimum_required(VERSION 3.5)
  
project(
  Dobble
  VERSION 0.1
  LANGUAGES C)

set(CMAKE_C_FLAGS "-Wall -Wextra -Werror -std=c99")

add_executable(dobble main.c server.c server.h game.c game.h)

target_link_libraries(dobble)
