// ReSharper disable CppDFANullDereference
#include "./lib/log.h"
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdckdint.h>
#include <time.h>
#include <sys/ucontext.h>

#define MEMORY_SIZE 0x1000
#define STACK_SIZE 16
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define VARIABLE_REGISTER_SIZE 16
#define DISPLAY_SCALE 10
#define KEYBOARD_SIZE 16
#define FONT_BASE_MEMORY_ADDRESS 0x50
#define ROM_BASE_ADDRESS 0x200

typedef enum {
  CHIP,
  COSMAC
} ChipArch;

typedef struct
{
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

typedef struct
{
  u_int8_t memory[MEMORY_SIZE];
  GeneralRegister *generalRegister;
  u_int16_t programCounter;
  u_int16_t indexRegister;
  u_int16_t stackPointer;
  u_int16_t stack[STACK_SIZE];
  u_int8_t delayTimer;
  u_int8_t soundTimer;
  bool display[DISPLAY_HEIGHT][DISPLAY_WIDTH];
} Chip;


typedef struct {
  int raylibCharCode;
  u_int8_t chipCharCode;
} ChipCharMap;

static Chip *chip = nullptr;
static ChipArch chipArch = CHIP;
static ChipCharMap chipCharMaps[16];
static bool shouldIncrement = true;

void copy_font_to_memory(Chip *chip)
{

  const u_int8_t font[] = {
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
}

size_t load_rom()
{
  FILE *rom = fopen("../roms/corax.ch8", "rb");
  if (rom == NULL)
  {
    logevent(Fatal, 64, "Error while reading Rom %s", "");
    return -1;
  }

  fseek(rom, 0, SEEK_END);
  const long length = ftell(rom);
  fseek(rom, 0, SEEK_SET);
  auto const fileData =
      (unsigned char *)(malloc(sizeof(unsigned char) * length));

  const size_t bufferRead = fread(fileData, 1, length, rom);
  memcpy(chip->memory + ROM_BASE_ADDRESS, fileData, bufferRead);
  free(fileData);

  return bufferRead;
}

void setup_char_codes(){
  // ReSharper disable once CppTooWideScope
  const u_int8_t chipKeys[KEYBOARD_SIZE] = {
    0x01, 0x02, 0x03, 0x0C,
    0x04, 0x05, 0x06, 0x0D,
    0x07, 0x08, 0x09, 0x0E,
    0x0A, 0x00, 0x0B, 0x0F
  };

  constexpr int qwertyKeys[KEYBOARD_SIZE] = {
    KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR,
    KEY_Q, KEY_W, KEY_E, KEY_R,
    KEY_A, KEY_S, KEY_D, KEY_F,
    KEY_Z, KEY_X, KEY_C, KEY_V,
  };

  for (int i = 0; i < KEYBOARD_SIZE; i++) {
    chipCharMaps[i].chipCharCode = chipKeys[i];
    chipCharMaps[i].raylibCharCode = qwertyKeys[i];
  }
};

bool checkInputHeldMatchesVx(const u_int8_t vxRegisterValue) {
  for (int i = 0; i < KEYBOARD_SIZE; i++) {
    if (IsKeyDown(chipCharMaps[i].raylibCharCode)) {
      return chipCharMaps[i].chipCharCode == vxRegisterValue;
    }
  }
  return false;
}

int8_t get_pressed_char_code() {
  // TODO: Get Char Code
  for (int i =0; i < KEYBOARD_SIZE; i++) {
    if (IsKeyPressed(chipCharMaps[i].raylibCharCode)) {
      return (int8_t) chipCharMaps[i].chipCharCode;
    }
  }
  return -1;
}

void setup_chip()
{
  chip->generalRegister = (GeneralRegister *)malloc(sizeof(GeneralRegister));
  chip->stackPointer = 0;

  for (int i = 0; i <= 0xF; i++)
  {
    *((u_int8_t *)chip->generalRegister + (i)) = 0;
  }
  memset(chip->memory, 0, sizeof(chip->memory));
  memset(chip->display, 0, sizeof(chip->display));

  copy_font_to_memory(chip);
  setup_char_codes();
}

void display_to_screen() {
  for (int y= 0; y < DISPLAY_HEIGHT; y++){
    for (int x = 0; x < DISPLAY_WIDTH; x++){
      if (chip->display[y][x])
        DrawRectangle(x * DISPLAY_SCALE, y * DISPLAY_SCALE, DISPLAY_SCALE,
          DISPLAY_SCALE, RAYWHITE);
    }
  }
}

void parseInstructions(const u_int16_t code) {
  const u_int8_t opcode = (code >> 12) & 0xF;

  const u_int8_t Vx = *((u_int8_t *) chip->generalRegister + ((code >> 8) & 0x0F));
  const u_int8_t Vy = *((u_int8_t *) chip->generalRegister + ((code >> 4) & 0x00F));
  const u_int16_t nnn = code & 0x0FFF;
  const u_int8_t nn = code & 0x00FF;
  const u_int8_t n = code & 0x000F;

  switch (opcode) {
    case 0x0:
      // OOE0 => Clear Screen
      if (nnn == 0x0E0) {
        memset(chip->display, 0, sizeof(chip->display));
        ClearBackground(BLACK);
        break;
      }

      if (nnn == 0x0EE) {
        chip->stackPointer--;
        chip->programCounter = chip->stack[chip->stackPointer];
        chip->stack[chip->stackPointer] = 0;
        shouldIncrement = false;
        break;
      }
      break;

    case 0x1:
      chip->programCounter = nnn;
      shouldIncrement = false;
      break;

      //2NNN => Call Subroutine
    case 0x2:
      chip->stack[chip->stackPointer] = chip->programCounter + 2;
      chip->stackPointer++;
      chip->programCounter = nnn;
      shouldIncrement = false;
      break;

    case 0x3:
      chip->programCounter += ((int) Vx == (nn) ) * 2;
      break;

    case 0x4:
      chip->programCounter += ((int) Vx != (nn)) * 2;
      break;

    case 0x5:
      chip->programCounter += ((int) Vx == Vy ) * 2;
      break;

    case 0x6:
      *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) = nn;
      break;

    case 0x7:
      *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) += nn;
      break;

    case 0x8:
      // 8XY0 => Set Vx = Vy
      if ((code & 0x000F) == 0x0)
        *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) =
          *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F));

      // 8XY1 => Set Vx |= Vy
      else if ((code & 0x000F) == 0x1)
        *((u_int8_t *) chip->generalRegister + ((code >> 8) & 0x0F)) |=
          *((u_int8_t *) chip->generalRegister + ((code >> 4) & 0x00F));

      // 8XY2 => Set Vx &= Vy
      else if ((code & 0x000F) == 0x2)
        *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) &=
          *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F));

      // 8XY3 => Set Vx ^= Vy
      else if ((code & 0x000F) == 0x3)
        *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) ^=
          *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F)) ;

      // 8XY4 => Set Vx+=Vy (with carry flag VF Set)
      else if ((code & 0x000F) == 0x4) {
        const u_int8_t vx = *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F));
        const u_int8_t vy = *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F));
        u_int8_t result;
        const bool overflow = (u_int8_t) ckd_add(&result, vx, vy);

        if (overflow)
          chip->generalRegister->VF = 1;

        // Sets Vx to result
        *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) = result;
      }

      // 8XY5 => Set Vx = Vx - Vy (with carry flag VF set on underflow)
      else if ((code & 0x000F) == 0x5) {
        const u_int8_t vx = *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F));
        const u_int8_t vy = *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F));
        u_int8_t result;

        if (vx > vy)
          chip->generalRegister->VF = 1;

        // Sets Vx to result
        ckd_sub(&result, vx, vy);
        *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) = result;
      }

      // 8XY6 => Shift Vx 1 bit to right
      else if ((code & 0x000F) == 0x6) {
        // Ambiguous Instruction
        if (chipArch != COSMAC)
          *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F))  =
            *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F)); // Vx = Vy

        chip-> generalRegister->VF =  *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) & 1;
        *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) >>= 1;
      }

      //8XY6 => Set Vx = Vy - Vx ( with carry flag VF set on underflow ).
      else if ((code & 0x000F) == 0x7) {
        u_int8_t result;
        if (Vx > Vy)
          chip->generalRegister->VF = 1  ;
        ckd_sub(&result, Vy, Vx);
        *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) = result;
      }

      // 8XYE => Shift Vx 1 bit to right
      else if ((code & 0x000F) == 0xE) {
        // Ambiguous Instruction
        if (chipArch != COSMAC)
          *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F))  =
            *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F)); // Vx = Vy

        chip-> generalRegister->VF =  *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) >> 7;
        *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) <<= 1;
      }
      break;

    // 9XY0 =>  Skip instruction if Vx != Vy
    case 0x9:
      chip->programCounter += (Vx != Vy) * 2 ;
      break;

    // ANNN => Set Index Register to NNN
    case 0xA:
      chip->indexRegister = code & 0x0FFF;
      break;

    case 0xD:
      const u_int8_t x =
        *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x00F)) % DISPLAY_WIDTH;
      const u_int8_t y =
        *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x0F)) % DISPLAY_HEIGHT;

      for (int row = 0; row < n; row++) {
        const u_int8_t sprite_byte = chip->memory[chip->indexRegister + row];
        const int py =  (y + row) % DISPLAY_HEIGHT;
        for (int bitPosition = 0; bitPosition < 8; bitPosition++) {
          const int px =  (x + bitPosition) % DISPLAY_WIDTH;
          const bool sprite_pixel = (sprite_byte & (0x80 >> bitPosition)) !=0;

          if (sprite_pixel) {
            if (chip->display[py][px]) {
              chip->generalRegister->VF = 1;
            }
            chip->display[py][px] ^= 1;
          }
        }
      }
      break;

    default:
      break;
  }
}

int main(const int argv, char **argc)
{
  if (argv == 2 && strcmp(argc[1], "--cosmac") == 0) {
    chipArch = COSMAC;
    logevent(Info, 64, "Using COSMAC chip architecture");
  }

  LogSettings logSettings = {
      Warn,  // FILE
      Info, // Console
      Daily // Interval,
  };

  setup_logs(&logSettings);
  logevent(Info, 64, "Starting Chip-8 Emulator...", "");

  chip = (Chip *)(malloc(sizeof(Chip)));
  if (chip == nullptr)
    exit(-1);

  setup_chip();
  const u_int16_t rom_size = load_rom();

  SetTraceLogLevel(LOG_NONE);
  SetTargetFPS(60);
  InitWindow(DISPLAY_WIDTH * DISPLAY_SCALE , DISPLAY_HEIGHT * DISPLAY_SCALE,
             "CHIP-8");

  chip->programCounter = (u_int16_t) ROM_BASE_ADDRESS;

  while (!WindowShouldClose())
  {
    BeginDrawing();
    constexpr u_int8_t INSTRUCTIONS_PER_FRAME = 11;

    for (int i=0; i<INSTRUCTIONS_PER_FRAME; i++) {

      const bool shouldProgramContinue = chip->programCounter >= ROM_BASE_ADDRESS &&
        chip->programCounter < (u_int16_t)(rom_size + ROM_BASE_ADDRESS) - 1;

      if (shouldProgramContinue){

        const u_int16_t code = (chip->memory[chip->programCounter] << 8) +
          chip->memory[chip->programCounter + 1];

        shouldIncrement = true;

        logevent(Info, 64, "PC-> 0x%04X \tCode: 0x%04X", chip->programCounter, code);

        parseInstructions(code);

        if (shouldIncrement)
          chip->programCounter += 2;
      }

      else {
        break;
      }

      display_to_screen();
    }
    EndDrawing();
  }

  logevent(Info, 64, "Ending Chip-8 Emulator...", "");
  close_log();
}
