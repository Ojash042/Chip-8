// ReSharper disable CppDFANullDereference
#include "./lib/log.h"
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdckdint.h>
#include <time.h>
#include <sys/time.h>

#define MEMORY_SIZE 0x1000
#define STACK_SIZE 16
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define VARIABLE_REGISTER_SIZE 16
#define DISPLAY_SCALE 10

#define ROM_BASE_ADDRESS 0x200

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
  u_int16_t program_counter;
  u_int16_t index_register;
  u_int16_t stack_pointer;
  u_int16_t stack[STACK_SIZE];
  u_int8_t delay_timer;
  u_int8_t sound_timer;
  bool display[DISPLAY_HEIGHT][DISPLAY_WIDTH];
} Chip;

static Chip *chip = nullptr;

void copy_font_to_memory(Chip *chip)
{
  constexpr int FONT_BASE_MEMORY_ADDRESS = 0x50;
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
  for (int i = 0; i < sizeof(font) / sizeof(font[0]); i++)
  {
    chip->memory[FONT_BASE_MEMORY_ADDRESS + i] = font[i];
  }
}

size_t load_rom()
{
  FILE *rom = fopen("./roms/IBM_Logo.ch8", "rb");
  if (rom == NULL)
  {
    logevent(Fatal, 64, "Error while reading Rom %s", "");
    return -1;
  }

  fseek(rom, 0, SEEK_END);
  const long length = ftell(rom);
  fseek(rom, 0, SEEK_SET);
  const auto file_data =
      (unsigned char *)(malloc(sizeof(unsigned char) * length));

  const size_t buffer_read = fread(file_data, 1, length, rom);
  memcpy(chip->memory + ROM_BASE_ADDRESS, file_data, buffer_read);
  free(file_data);

  return buffer_read;
}

void parse_instruction(u_int8_t code)
{
}

void setup_chip()
{
  chip->generalRegister = (GeneralRegister *)malloc(sizeof(GeneralRegister));
  chip->stack_pointer = 0;

  for (int i = 0; i <= 0xF; i++)
  {
    *((u_int8_t *)chip->generalRegister + (i)) = 0;
  }
  memset(chip->memory, 0, sizeof(chip->memory));
  memset(chip->display, 0, sizeof(chip->display));

  copy_font_to_memory(chip);
}

int main(int argv, char **argc)
{

  LogSettings logSettings = {
      Warn,  // FILE
      Warn, // Console
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

  while (!WindowShouldClose())
  {
    BeginDrawing();
    ClearBackground(BLACK);

    chip->program_counter = (u_int16_t)0x200;

    while (chip->program_counter <
           (u_int16_t)(rom_size + ROM_BASE_ADDRESS) - 1)
    {

      const u_int16_t code = (chip->memory[chip->program_counter] << 8) +
                       chip->memory[chip->program_counter + 1];

      if (code == 0x00e0)
      {
        memset(chip->display, 0, sizeof(chip->display));
        ClearBackground(BLACK);
      }

      else if (code == 0x00EE) {
        chip->program_counter = chip->stack[chip->stack_pointer];
        chip->stack[chip->stack_pointer] = 0;
        chip->stack_pointer--;
      }

      else if ((code >> 12) == 0x1)
      {
        logevent(Info, 64, "Program Counter Jumped to: 0x%04x", (code & 0x0FFF));
        chip->program_counter = (code & 0xFFF);
      }

      else if (code >> 12 == 0x2) {
        chip->stack[chip->stack_pointer] = chip->program_counter;
        chip->stack_pointer++;
        chip->program_counter = code & 0x0FFF;
        chip->program_counter-=2;
      }

      else if (code >> 12 == 0x3) {
        const u_int8_t vx = *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F));
        if (vx == code & 0x00FF) {
          chip->program_counter +=2;
        }
      }

      else if (code >> 12 == 0x4) {
        const u_int8_t vx = *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F));
        if (vx != code & 0x00FF) {
          chip->program_counter +=2;
        }
      }

      else if (code >> 12 == 0x5) {
        const u_int8_t vx = *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F));
        const u_int8_t vy = *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F));
        if (vx == vy) {
          chip->program_counter +=2;
        }
      }

      else if ((code >> 12) == 0x6)
      {
        *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) = code & 0x00FF;
      }

      else if ((code >> 12) == 0x7)
      {
        *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) +=
            (u_int8_t)(code & 0x00FF);
      }
      else if (code >> 12 == 0x8 ) {
        if ((code & 0x000F) == 0x0)
          *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) = *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F)) ;
        else if ((code & 0x000F) == 0x1)
          *((u_int8_t *) chip->generalRegister + ((code >> 8) & 0x0F)) |= *(
            (u_int8_t *) chip->generalRegister + ((code >> 4) & 0x00F));
        else if ((code & 0x000F) == 0x2)
          *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) &= *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F)) ;
        else if ((code & 0x000F) == 0x3)
          *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) ^= *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F)) ;
        else if ((code & 0x000F) == 0x4) {
          const u_int8_t vx = *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F));
          const u_int8_t vy = *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F));
          u_int8_t result;
          chip->generalRegister->VF = (u_int8_t) ckd_add(&result, vx, vy);

          // Sets Vx to result
          *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) = result;
        }
        else if ((code & 0x000F) == 0x5) {
          const u_int8_t vx = *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F));
          const u_int8_t vy = *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F));
          u_int8_t result;
          chip->generalRegister->VF = vx > vy;
          // Sets Vx to result
          ckd_sub(&result, vx, vy);
          *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) = result;
        }
        else if ((code & 0x000F) == 0x6) {
          // Ambiguous Instruction
          // *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F))  = *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F)); // Vx = Vy
          chip-> generalRegister->VF =  *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) & 1;
          *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) >>=1;
        }
        else if ((code & 0x000F) == 0x7) {
          const u_int8_t vx = *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F));
          const u_int8_t vy = *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F));
          u_int8_t result;
          chip->generalRegister->VF = (u_int8_t) (vy > vx);
          ckd_sub(&result, vy, vx);

          // Sets Vx to result
          *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) = result;
        }
        else if ((code & 0x000F) == 0xE) {
          // Ambiguous Instruction
          //*((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F))  = *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F)); // Vx = Vy
          chip-> generalRegister->VF =  *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) >> 7;
          *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F)) <<=1;
        }
      }

      else if (code >> 12 == 0x9) {
        const u_int8_t vx = *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F));
        const u_int8_t vy = *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x00F));
        if (vx != vy) {
          chip->program_counter +=2;
        }
      }

      else if ((code >> 12) == 0xA)
        chip->index_register = code & 0x0FFF;
      else if ((code >> 12) == 0xB) {
        // Ambiguous
        // chip->program_counter =  chip->generalRegister->V0 + code & 0x0FFF;
        chip->program_counter =  (code & 0x0FFF) + *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x0F));;
        chip->program_counter -=2;
      }
      else if ((code >> 12) == 0xC) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        const unsigned long seed = ts.tv_nsec * 1000 + ts.tv_sec / 1000000;
        srand(seed);
        u_int8_t random = random() % (code & 0x00FF) && code && 0x00FF;
        *((u_int8_t *)chip->generalRegister + (code >> 8 & 0x0F)) = random;
      }


      else if ((code >> 12) == 0xD){
        // DXYN
        const u_int8_t x =
            *((u_int8_t *)chip->generalRegister + ((code >> 8) & 0x00F)) % DISPLAY_WIDTH;
        const u_int8_t y =
            *((u_int8_t *)chip->generalRegister + ((code >> 4) & 0x0F)) % DISPLAY_HEIGHT;
        u_int8_t const n = code & 0x000F;

        for (int row = 0; row < n; row++) {
          const u_int8_t sprite_byte = chip->memory[chip->index_register + row];
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

      }

      for (int y= 0; y < DISPLAY_HEIGHT; y++)
      {
        for (int x = 0; x < DISPLAY_WIDTH; x++)
        {
          if (chip->display[y][x])
            DrawRectangle(x * DISPLAY_SCALE, y * DISPLAY_SCALE, DISPLAY_SCALE,
                          DISPLAY_SCALE, RAYWHITE);
        }
      }
      chip->program_counter += 2; 
    }
    logevent(Info, 64, "Ended");

    EndDrawing();
  }

  logevent(Info, 64, "Ending Chip-8 Emulator...", "");
  close_log();
}
