#!/bin/env bash

PROJECT=Chip-8

flags=(
  -Wall
  -Wextra
)

src_files=(
  ./lib/log.c
  ./main.c
)

libs=(
  -lraylib
)

gcc "${flags[@]}" "${src_files[@]}" "${libs[@]}" -o ./out/${PROJECT}
