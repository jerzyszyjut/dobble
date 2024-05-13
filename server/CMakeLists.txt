cmake_minimum_required(VERSION 3.5)
  
project(
  Dobble
  VERSION 0.1
  LANGUAGES C)

set(CMAKE_C_FLAGS "-Wall -Wextra -Werror -std=c99")

add_library(game game.c game.h server.c server.h)
add_executable(dobble.out main.c)

target_link_libraries(dobble.out game)
