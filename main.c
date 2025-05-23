#include "./log.h"
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#define MEMORY_SIZE 0x1000
#define STACK_SIZE 16
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define VARIABLE_REGISTER_SIZE 16

#define ROM_BASE_ADDRESS 0x200

#define SCREEN_WIDTH_MULTIPLE 10
#define SCREEN_HEIGHT_MULTIPLE 10

typedef struct {
  u_int8_t V0;
  u_int8_t V1;
  u_int8_t V2;
  u_int8_t V3;
  u_int8_t V4;
  u_int8_t V5;
  u_int8_t V6;
  u_int8_t V7;
  u_int8_t V8;
  u_int8_t V9;
  u_int8_t VA;
  u_int8_t VB;
  u_int8_t VC;
  u_int8_t VD;
  u_int8_t VE;
  u_int8_t VF;
} GeneralRegister;

typedef struct {
  u_int8_t memory[MEMORY_SIZE];
  GeneralRegister *Generalregister;
  u_int16_t program_counter;
  u_int16_t index_register;
  u_int16_t stack_register;
  u_int16_t stack[STACK_SIZE];
  u_int8_t delay_timer;
  u_int8_t sound_timer;
  bool display[DISPLAY_WIDTH][DISPLAY_HEIGHT];
} Chip;

static Chip *chip = NULL;
bool force_close = false;

void copy_font_to_memory(Chip *chip) {
  const int FONT_BASE_MEMORY_ADDRESS = 0x50;
  int font[] = {
      0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
      0x20, 0x60, 0x20, 0x20, 0x70, // 1
      0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
      0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
      0x90, 0x90, 0xF0, 0x10, 0x10, // 4
      0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
      0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
      0xF0, 0x10, 0x20, 0x40, 0x40, // 7
      0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
      0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
      0xF0, 0x90, 0xF0, 0x90, 0x90, // A
      0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
      0xF0, 0x80, 0x80, 0x80, 0xF0, // C
      0xE0, 0x90, 0x90, 0x90, 0xE0, // D
      0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
      0xF0, 0x80, 0xF0, 0x80, 0x80  // F
  };

  memcpy(chip->memory + FONT_BASE_MEMORY_ADDRESS, font, sizeof(font));
  for (int i = 0; i < sizeof(font) / sizeof(font[0]); i++) {
    chip->memory[FONT_BASE_MEMORY_ADDRESS + i] = font[i];
  }
}

int load_rom() {
  FILE *rom = fopen("./roms/IBM_Logo.ch8", "rb");
  if (rom == NULL) {
    logevent(Fatal, 64, "Error while reading Rom %s", "");
    return -1;
  }

  fseek(rom, 0, SEEK_END);
  long length = ftell(rom);
  fseek(rom, 0, SEEK_SET);
  unsigned char *file_data =
      (unsigned char *)(malloc(sizeof(unsigned char) * length));

  size_t buffer_read = fread(file_data, 1, length, rom);
  memcpy(chip->memory + ROM_BASE_ADDRESS, file_data, buffer_read);

  return buffer_read;
}

void parse_instruction() {}

void setup_chip() {
  chip->Generalregister = (GeneralRegister *)malloc(sizeof(GeneralRegister));
  for (int i = 0; i <= 0xF; i++) {
    *((u_int8_t *)chip->Generalregister + (i)) = 0;
  }
  copy_font_to_memory(chip);
}

int main(int argv, char **argc) {

  LogSettings logSettings = {
      Warn,  // FILE
      Debug, // Console
      Daily,
  };

  setup_logs(&logSettings);
  logevent(Info, 64, "Starting Chip-8 Emulator...", "");

  chip = (Chip *)(malloc(sizeof(Chip)));
  setup_chip();
  u_int16_t rom_size = load_rom();

  SetTraceLogLevel(LOG_NONE);
  SetTargetFPS(60);
  InitWindow(DISPLAY_WIDTH * SCREEN_WIDTH_MULTIPLE,
             DISPLAY_HEIGHT * SCREEN_HEIGHT_MULTIPLE, "CHIP-8");

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);

    chip->program_counter = (u_int16_t)0x200;

    while (chip->program_counter <
           (u_int16_t)(rom_size + ROM_BASE_ADDRESS) - 1) {
      // logevent(Debug, 64, "Reading.. 0x%04x", chip->program_counter);

      u_int16_t code = (chip->memory[chip->program_counter] << 8) +
                       chip->memory[chip->program_counter + 1];

      if (code == 0x00e0) {
        ClearBackground(BLACK);
        // continue;
      }

      logevent(Debug, 64, "0x%04x", code);
      if ((code >> 12) == 0x1) {
        logevent(Info, 64, "Program Counter Jumped to: 0x%04x",
                 (code & 0x0FFF));
        chip->program_counter = (code & 0xFFF);
      }

      if ((code >> 12) == 0x6) {
        *((u_int8_t *)chip->Generalregister + ((code >> 8) & 0x0F)) =
            code & 0x00FF;
      }
      if ((code >> 12) == 0x7) {
        *((u_int8_t *)chip->Generalregister + ((code >> 8) & 0x0F)) +=
            (u_int8_t)(code & 0x00FF);
      }
      if ((code >> 12) == 0xA) {
        chip->index_register = code & 0x0FFF;
      }
      if ((code >> 12) == 0xD) {
        // DXYN
        u_int8_t x =
            *((u_int8_t *)chip->Generalregister + ((code >> 8) & 0x00F));
        u_int8_t y =
            *((u_int8_t *)chip->Generalregister + ((code >> 4) & 0x0F));
        u_int8_t n = (code & 0x000F);
        logevent(Info, 64, "Drawing At (%01X, %01X), for %d", x, y, n);
        // memcopy memory[IR] to memory[IR + N];
      }

      chip->program_counter += 2;
    }
    logevent(Info, 64, "Ended");

    EndDrawing();
    CloseWindow();
  }

  logevent(Info, 64, "Ending Chip-8 Emulator...", "");
  close_log();
}
