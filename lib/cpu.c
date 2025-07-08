#include <raylib.h>
#include <sys/types.h>
#include "cpu.h"

#include <stdckdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log.h"


#define VARIABLE_REGISTER_SIZE 16
#define DISPLAY_SCALE 10
#define KEYBOARD_SIZE 16
#define FONT_BASE_MEMORY_ADDRESS 0x50
#define ROM_BASE_ADDRESS 0x200

static Chip *chip = nullptr;
static ChipArch chipArch = CHIP;
static ChipCharMap chipCharMaps[16];

void setChipArchitecture(const ChipArch arch) {
  chipArch = arch;
}

void copy_font_to_memory(Chip *chip) {
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

size_t loadRom()
{
  FILE *rom = fopen("../roms/flags.ch8", "rb");
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

void setup_chip()
{
  chip = (Chip *)(malloc(sizeof(Chip)));

  if (chip == nullptr)
    exit(-1);

  chip->generalRegister = (GeneralRegister *)malloc(sizeof(GeneralRegister));
  chip->stackPointer = 0;
  chip->programCounter = (u_int16_t) ROM_BASE_ADDRESS;

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

bool checkInputHeldMatchesVx(const u_int8_t vxRegisterValue) {
  for (int i = 0; i < KEYBOARD_SIZE; i++) {
    if (IsKeyDown(chipCharMaps[i].raylibCharCode)) {
      return chipCharMaps[i].chipCharCode == vxRegisterValue;
    }
  }
  return false;
}

int8_t get_pressed_char_code() {
  for (int i =0; i < KEYBOARD_SIZE; i++) {
    if (IsKeyPressed(chipCharMaps[i].raylibCharCode)) {
      return (int8_t) chipCharMaps[i].chipCharCode;
    }
  }
  return -1;
}

u_int16_t getProgramCounter() {
  return chip->programCounter;
}

void incrementProgramCounter() {
  chip->programCounter+=2;
}

u_int16_t getNextCode() {
  return (chip->memory[chip->programCounter] << 8) +
          chip->memory[chip->programCounter + 1];
}

bool parseInstructions(const u_int16_t code) {
  const u_int8_t opcode = (code >> 12) & 0xF;

  const u_int8_t testX = (code >> 8) & 0x0F;
  u_int8_t *p_Vx = (u_int8_t *) chip->generalRegister + ((code >> 8) & 0x0F);
  const u_int8_t *p_Vy = (u_int8_t *) chip->generalRegister + ((code >> 4) & 0x00F);
  const u_int8_t Vy = *p_Vy;
  const u_int8_t Vx = *p_Vx;

  const u_int16_t nnn = code & 0x0FFF;
  const u_int8_t nn = code & 0x00FF;
  const u_int8_t n = code & 0x000F;

  u_int8_t carryFlag = 0;
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
        return false;
      }
      break;

    case 0x1:
      chip->programCounter = nnn;
      return false;

      //2NNN => Call Subroutine
    case 0x2:
      chip->stack[chip->stackPointer] = chip->programCounter + 2;
      chip->stackPointer++;
      chip->programCounter = nnn;
      return false;

    case 0x3:
      chip->programCounter += ((int) Vx == nn ) * 2;
      break;

    case 0x4:
      chip->programCounter += ((int) Vx != nn) * 2;
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
        *p_Vx = *p_Vy;

      // 8XY1 => Set Vx |= Vy
      else if ((code & 0x000F) == 0x1)
        *p_Vx |= *p_Vy;

      // 8XY2 => Set Vx &= Vy
      else if ((code & 0x000F) == 0x2)
        *p_Vx &= *p_Vy;

      // 8XY3 => Set Vx ^= Vy
      else if ((code & 0x000F) == 0x3)
        *p_Vx ^= *p_Vy ;

      // 8XY4 => Set Vx+=Vy (with carry flag VF Set)
      else if ((code & 0x000F) == 0x4) {
        u_int8_t result;
        const bool overflow = (u_int8_t) ckd_add(&result, Vx, Vy);
        // Sets Vx to result
        *p_Vx = result;

        chip->generalRegister->VF = overflow ? 1 : 0;
      }

      // 8XY5 => Set Vx = Vx - Vy (with carry flag VF set on underflow)
      else if ((code & 0x000F) == 0x5) {
        u_int8_t result;

        // Sets Vx to result
        ckd_sub(&result, Vx, Vy);
        *p_Vx = result;
        chip->generalRegister->VF = Vx>=Vy ? 1 : 0;
      }

      // 8XY6 => Shift Vx 1 bit to right
      else if ((code & 0x000F) == 0x6) {
        // Ambiguous Instruction
        if (chipArch == COSMAC)
          *p_Vx  = *p_Vy; // Vx = Vy

        carryFlag =  *p_Vx & 1;
        *p_Vx >>= 1;
        chip-> generalRegister->VF = carryFlag;
      }

      //8XY7 => Set Vx = Vy - Vx ( with carry flag VF set on underflow ).
      else if ((code & 0x000F) == 0x7) {
        u_int8_t result;
        ckd_sub(&result, Vy, Vx);
        *p_Vx = result;
        chip->generalRegister->VF = Vy>=Vx? 1 : 0  ;
      }

      // 8XYE => Shift Vx 1 bit to right
      else if ((code & 0x000F) == 0xE) {
        // Ambiguous Instruction
        if (chipArch == COSMAC)
          *p_Vx  =* p_Vy; // Vx = Vy

        carryFlag =  Vx >> 7;
        *p_Vx <<= 1;
        chip-> generalRegister->VF =  carryFlag;
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

    // Code's wrong but it works
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

    case 0xC:
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      const unsigned long seed = ts.tv_nsec * 1000 + ts.tv_sec / 1000000;
      srand(seed);
      const u_int8_t rand = random() % (code & 0x00FF) && (code & 0x00FF);
      *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) = rand;
      break;

    case 0xF:
      if (nn == 0x07)
        *p_Vx = chip->delayTimer;

      if (nn == 0x015)
        chip->delayTimer = Vx;

      if (nn == 0x18)
        chip->soundTimer = Vx;

      if (nn == 0x1E) {
        chip->indexRegister += Vx;
        chip->generalRegister->VF = 0;
        if (chip->indexRegister >= 0x1000)
          chip->generalRegister->VF = 1;
      }

      if (nn == 0x0A) {
        const int8_t charCode = get_pressed_char_code();
        if (charCode != -1) {
          *p_Vx = charCode;
        }
        else {
          return false;
        }
      }
      if (nn == 0x33) {
        const u_int8_t binaryValue = Vx;
        const u_int16_t memoryLocation = chip->indexRegister;
        chip->memory[memoryLocation] = (u_int8_t) (binaryValue / 100 );
        chip->memory[memoryLocation + 1] = (u_int8_t) (((u_int8_t) (binaryValue / 10 )) % 10 );
        chip->memory[memoryLocation + 2] = (u_int8_t) (binaryValue % 10 );
      }

      if (nn == 0x55) {
        const u_int16_t memoryStartLocation = chip->indexRegister;
        const u_int8_t maxRegister = (code >> 8) & 0x0F;
        for (u_int8_t genRegister = 0; genRegister <= maxRegister ; genRegister++ ) {
          chip->memory[memoryStartLocation + genRegister] =  *((u_int8_t *)chip->generalRegister + genRegister );
          // Ambiguous Instruction
          if (chipArch == COSMAC) {
            chip->indexRegister++;
          }
        }
      }
      if (nn == 0x65) {
        const u_int16_t memoryStartLocation = chip->indexRegister;
        const u_int8_t maxRegister = (code >> 8) & 0x0F;
        for (u_int8_t genRegister = 0; genRegister <= maxRegister ; genRegister++ ) {
          *((u_int8_t *)chip->generalRegister + genRegister ) = chip->memory[memoryStartLocation + genRegister];
          // Ambiguous Instruction
          if (chipArch == COSMAC) {
            chip->indexRegister++;
          }
        }
      }

      break;

    default:
      break;
  }
  return true;
}
