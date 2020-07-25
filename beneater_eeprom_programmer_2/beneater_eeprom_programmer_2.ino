// Microcode - Bentium opcodes implemented as 5 stage microcode
//             where each stage is combination of control bits to set at each sub-step of instruction
// For EEPROM in 8bit computer kit
// - Arduino Nano
// - ATMEGA 328P ( old bootloader )
//
// Includes support for Condition Flags (Carry & Zero)

#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define SHIFT_LATCH 4

#define EEPROM_D0 5
#define EEPROM_D7 12

#define WRITE_EN 13

// control logic flags - where control flags are described as 16 bit word
#define HLT  uint16_t(1) << 15        // HALT - deactivate clock
#define MI   uint16_t(1) << 14        // Memory In - read address from Bus into Memory register
#define RI   uint16_t(1) << 13        // Ram In - read value from Bus into Ram at address in Memory register
#define RO   uint16_t(1) << 12        // Ram Out - write value to Bus from Ram at address in Memory register
#define IO   uint16_t(1) << 11        // Instruction Register Out - write value to Bus from Instruction Register (note: only low nibble, for opcode data/address)
#define II   uint16_t(1) << 10        // Instruction Register In - read value from Bus into Instruction Register (note: all 8 bits)
#define AI   uint16_t(1) << 9         // 'A' Data Register In - read value from Bus into 'A' Data Register
#define AO   uint16_t(1) << 8         // 'A' Data Regsiter Out - write value from 'A' Data Register to the Bus
#define EO   uint16_t(1) << 7         // Sigma Out - write value from ALU to the Bus
#define SU   uint16_t(1) << 6         // Subract - ALU mode - 0 == Addition (Sigma = 'A'+'B'), 1 == Subtraction (Sigma = 'A'-'B')
#define BI   uint16_t(1) << 5         // 'B' Data Register In - read value from Bus into 'B' Data Register
#define OI   uint16_t(1) << 4         // Output In - read value from Bus into Output Display
#define CE   uint16_t(1) << 3         // Counter Enable - increment the Program Counter register
#define CO   uint16_t(1) << 2         // Counter Output - write value from Program Counter register to the Bus
#define J    uint16_t(1) << 1         // JUMP - read value from Bus into the Program Counter register
#define FI   uint16_t(1) << 0         // Flags In - read flags (Zero/Carry) from ALU into Flags Register

#define FLAGS_Z0C0 0
#define FLAGS_Z0C1 1
#define FLAGS_Z1C0 2
#define FLAGS_Z1C1 3

// opcodes for conditional jump instructions
#define JC 0b0111
#define JZ 0b1000

// store UCODE_TEMPLATE in FLASH memory
const PROGMEM uint16_t UCODE_TEMPLATE[16][8] = {
  { MI|CO, RO|II|CE, 0,     0,      0,            0, 0, 0 },   // 0000 - NOP
  { MI|CO, RO|II|CE, IO|MI, RO|AI,  0,            0, 0, 0 },   // 0001 - LDA - load A register with value at address in memory
  { MI|CO, RO|II|CE, IO|MI, RO|BI,  AI|EO|FI,     0, 0, 0 },   // 0010 - ADD
  { MI|CO, RO|II|CE, IO|MI, RO|BI,  AI|EO|SU|FI,  0, 0, 0 },   // 0011 - SUB
  { MI|CO, RO|II|CE, IO|MI, AO|RI,  0,            0, 0, 0 },   // 0100 - STA
  { MI|CO, RO|II|CE, IO|AI, 0,      0,            0, 0, 0 },   // 0101 - LDI - load immediate
  { MI|CO, RO|II|CE, IO|J,  0,      0,            0, 0, 0 },   // 0110 - JMP
  { MI|CO, RO|II|CE, 0,     0,      0,            0, 0, 0 },   // 0111 - JC (as NOP)
  { MI|CO, RO|II|CE, 0,     0,      0,            0, 0, 0 },   // 1000 - JZ (as NOP)
  { MI|CO, RO|II|CE, 0,     0,      0,            0, 0, 0 },   // 1001 - 
  { MI|CO, RO|II|CE, 0,     0,      0,            0, 0, 0 },   // 1010 - 
  { MI|CO, RO|II|CE, 0,     0,      0,            0, 0, 0 },   // 1011 - 
  { MI|CO, RO|II|CE, 0,     0,      0,            0, 0, 0 },   // 1100 - 
  { MI|CO, RO|II|CE, 0,     0,      0,            0, 0, 0 },   // 1101 - 
  { MI|CO, RO|II|CE, AO|OI, 0,      0,            0, 0, 0 },   // 1110 - OUT
  { MI|CO, RO|II|CE, HLT,   0,      0,            0, 0, 0 },    // 1111 - HLT
};

// duplicates of microcode
// - so that conditional jumps can be implemented based on output of flags register
//   on eeprom address lines 8 and 9
// - 4 combinations of conditional jump flags
// - 16 instructions
// - 8 steps for each instruction
uint16_t ucode[4][16][8];

void initUCode() {
  // ZF = 0, CF = 0
  memcpy_P(ucode[FLAGS_Z0C0], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));

  // ZF = 0, CF = 1
  memcpy_P(ucode[FLAGS_Z0C1], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  // enable condition jump if Carry flag is set
  ucode[FLAGS_Z0C1][JC][2] = IO | J;

  // ZF = 1, CF = 0
  memcpy_P(ucode[FLAGS_Z1C0], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  // enable condition jump if zero flag is set
  ucode[FLAGS_Z1C0][JZ][2] = IO | J;

  // ZF = 1, CF = 1
  memcpy_P(ucode[FLAGS_Z1C1], UCODE_TEMPLATE, sizeof(UCODE_TEMPLATE));
  // enable condition jump if carry flag is set
  ucode[FLAGS_Z1C1][JC][2] = IO | J;
  // enable condition jump if zero flag is set
  ucode[FLAGS_Z1C1][JZ][2] = IO | J;
}

void setup() {
  initUCode();
  
  // put your setup code here, to run once:
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);

  digitalWrite(WRITE_EN, HIGH);
  pinMode(WRITE_EN, OUTPUT);

  Serial.begin(57600);

  writeMicrocode();
  
  printContents(0, 1024);
}

void eraseEEPROM() {
  Serial.println("Erase EEPROM");
  
  for (int address=0; address <= 2048; address++) {
     writeEEPROM(address, 0);
  }
}

void writeMicrocode() {
  Serial.println("Write EEPROM - Microcode");

  for (int address = 0; address < 1024; address += 1) {
    int flags       = (address & 0b1100000000) >> 8;        // 2 bit flags
    int byte_sel    = (address & 0b0010000000) >> 7;        // 1 bit byte select - to select between 2 EEPROMs
    int instruction = (address & 0b0001111000) >> 3;        // 4 bit opcode
    int step        = (address & 0b0000000111);             // 3 bit step

    const uint16_t microcode = ucode[flags][instruction][step];
    
    if (byte_sel != 0) {
      writeEEPROM(address, microcode);
    } else {
      writeEEPROM(address, microcode >> 8); 
    }

    if (address % 64 == 0) {
      Serial.print(".");
    }
  }

  Serial.println(" done");

}

void printContents(int start, int length) {
  Serial.println("Read EEPROM");
  for (int base = start; base < length; base += 16) {
    byte data[16];
    for (int offset=0; offset < 16; offset += 1) {
      data[offset] = readEEPROM(base + offset);
    }
    char buffer[80];
    sprintf(buffer, "%03x: %02x %02x %02x %02x %02x %02x %02x %02x   %02x %02x %02x %02x %02x %02x %02x %02x",
          base,
          data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7], 
          data[8], data[9], data[10], data[11], data[12], data[13], data[14], data[15]
          );
     Serial.println(buffer);
  }
}

byte readEEPROM(int address) {
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin++) {
    pinMode(pin, INPUT);
  }
  
  setAddress(address, /*outputEnable*/ true);

  byte data = 0;
  for (int pin = EEPROM_D7; pin >= EEPROM_D0; pin -= 1) {
    data = (data << 1) + digitalRead(pin);
  }

  return data;
}

void writeEEPROM(int address, byte data) {
  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin++) {
    pinMode(pin, OUTPUT);
  }
  
  setAddress(address, /*outputEnable*/ false);

  for (int pin = EEPROM_D0; pin <= EEPROM_D7; pin += 1) {
    digitalWrite(pin, data & 1);
    data >>= 1;
  }

  digitalWrite(WRITE_EN, LOW);
  delayMicroseconds(1);
  digitalWrite(WRITE_EN, HIGH);

  delay(5);
}

void setAddress(u16 address, bool outputEnable) {
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, ((address >> 8) & 0xff) | (outputEnable ? 0x00 : 0x80));
  shiftOut(SHIFT_DATA, SHIFT_CLK, MSBFIRST, (address) & 0xff);

  digitalWrite(SHIFT_LATCH, LOW);
  digitalWrite(SHIFT_LATCH, HIGH);
  digitalWrite(SHIFT_LATCH, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:

}
