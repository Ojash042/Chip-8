// ReSharper disable CppDFANullDereference
#include "./lib/log.h"
#include <raylib.h>
#include <string.h>
#include <sys/types.h>
#include "./lib/cpu.h"

#define MEMORY_SIZE 0x1000
#define STACK_SIZE 16
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32
#define VARIABLE_REGISTER_SIZE 16
#define DISPLAY_SCALE 10
#define KEYBOARD_SIZE 16
#define FONT_BASE_MEMORY_ADDRESS 0x50
#define ROM_BASE_ADDRESS 0x200


int main(const int argv, char **argc)
{
  if (argv == 2 && strcmp(argc[1], "--cosmac") == 0) {
    setChipArchitecture(COSMAC);
    logevent(Info, 64, "Using COSMAC chip architecture");
  }

  LogSettings logSettings = {
      Warn,  // FILE
      Info, // Console
      Daily // Interval,
  };

  setup_logs(&logSettings);
  logevent(Info, 64, "Starting Chip-8 Emulator...", "");

  setupChip();
  const u_int16_t rom_size = loadRom();
  float elapsedTime = 0.0f;

  SetTraceLogLevel(LOG_NONE);
  SetTargetFPS(60);
  InitWindow(DISPLAY_WIDTH * DISPLAY_SCALE , DISPLAY_HEIGHT * DISPLAY_SCALE,
             "CHIP-8");

  while (!WindowShouldClose())
  {
    BeginDrawing();
    constexpr u_int8_t INSTRUCTIONS_PER_FRAME = 22;

    for (int i=0; i<INSTRUCTIONS_PER_FRAME; i++) {

      const u_int16_t programCounter = getProgramCounter();
      const bool shouldProgramContinue = programCounter >= ROM_BASE_ADDRESS &&
        programCounter < (u_int16_t)(rom_size + ROM_BASE_ADDRESS) - 1;

      if (shouldProgramContinue){

        const u_int16_t code = getNextCode();

        // logevent(Info, 64, "PC-> 0x%04X \tCode: 0x%04X", chip->programCounter, code);

        const InstructionState incrementState =  parseInstructions(code);

        incrementProgramCounter(incrementState);
      }

      else {
        break;
      }

      elapsedTime = GetFrameTime();
      if (elapsedTime >= 1) {
        elapsedTime = 0.0f;
        decrementCounters();
      }
      displayToScreen();
    }
    EndDrawing();
  }

  logevent(Info, 64, "Ending Chip-8 Emulator...", "");
  close_log();
}
