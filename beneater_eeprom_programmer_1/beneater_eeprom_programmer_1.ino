// Output Module - combinatorial logic for decimal -> 7 segment hex display
//

#define SHIFT_DATA 2
#define SHIFT_CLK 3
#define SHIFT_LATCH 4

#define EEPROM_D0 5
#define EEPROM_D7 12

#define WRITE_EN 13

void setup() {
  // put your setup code here, to run once:
  pinMode(SHIFT_DATA, OUTPUT);
  pinMode(SHIFT_CLK, OUTPUT);
  pinMode(SHIFT_LATCH, OUTPUT);

  digitalWrite(WRITE_EN, HIGH);
  pinMode(WRITE_EN, OUTPUT);

  Serial.begin(57600);

  writeDisplayModule();
  
  printContents();
}

void eraseEEPROM() {
  Serial.println("Erase EEPROM");
  
  for (int address=0; address <= 2048; address++) {
     writeEEPROM(address, 0);
  }
}

void writeDisplayModule() {
  Serial.println("Write EEPROM");

  byte digits[] = { 0x7e, 0x30, 0x6d, 0x79, 0x33, 0x5b, 0x5f, 0x70, 0x7f, 0x7b };

  ////////////////////////////////////////////////////
  // Positive Numbers 
  
  Serial.println("Programming ones place");
  for (int value=0; value < 256; value += 1) {
    writeEEPROM(value, digits[value % 10]);
  }

  Serial.println("Programming tens place");
  for (int value = 0; value < 256; value += 1) {
    writeEEPROM(value + 256, digits[(value/10) % 10]);
  }

  Serial.println("Programming hundreds place");
  for (int value = 0; value < 256; value += 1) {
    writeEEPROM(value + 512, digits[(value/100) % 10]);
  }

  Serial.println("Programming Sign");
  for (int value = 0; value < 256; value += 1) {
    writeEEPROM(value + 768, 0);
  }

  //////////////////////////////////////////////////////
  // Negative (two's complement) Numbers

  Serial.println("Programming ones place (twos complement)");
  for (int value = -128; value <= 127; value +=1) {
    writeEEPROM( byte(value) + 1024, digits[abs(value) % 10]);
  }

  Serial.println("Programming tens place (twos complement)");
  for (int value = -128; value <= 127; value += 1) {
    writeEEPROM( byte(value) + 1280, digits[(abs(value) / 10) % 10]);
  }

  Serial.println("Programming hundreds place (twos complement)");
  for (int value = -128; value <= 127; value += 1) {
    writeEEPROM( byte(value) + 1536, digits[(abs(value) / 100) % 10]);
  }

  Serial.println("Programming sign (twos complement)");
  for (int value = -128; value <= 127; value += 1) {
    if (value < 0) {
      writeEEPROM(byte(value) + 1792, 0x01);    // subtract sign as single hex segment
    } else {
      writeEEPROM(byte(value) + 1792, 0);
    }
  }

  
}

void printContents() {
  Serial.println("Read EEPROM");
  for (int base = 0; base < 256; base += 16) {
    byte data[16];
    for (int offset=0; offset <=15; offset+=1) {
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
