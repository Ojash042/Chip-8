//
// Created by ojash on 7/6/25.
//

#ifndef CPU_H
#define CPU_H
#include <sys/types.h>

#define MEMORY_SIZE 0x1000
#define STACK_SIZE 16
#define DISPLAY_WIDTH 64
#define DISPLAY_HEIGHT 32

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

void setChipArchitecture(const ChipArch arch);
size_t loadRom();
void setup_chip();
void display_to_screen();
bool parseInstructions(u_int16_t code);
u_int16_t getProgramCounter();
void incrementProgramCounter();
u_int16_t getNextCode();

#endif //CPU_H
