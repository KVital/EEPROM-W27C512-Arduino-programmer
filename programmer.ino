#define DS     A0    // Data pin
#define LATCH  A1    // Latch pin
#define CLOCK  A2    // Clock pin

#define D0 2
#define D1 3
#define D2 4
#define D3 5
#define D4 6
#define D5 7
#define D6 8
#define D7 10

// define the IO lines for the eeprom control
#define CE     A3
#define OE     A4

#define VH     12     //Erase Pin (A9)  12v
#define VPP    11     //OE Pin = VPP    12v


#define BUFFERSIZE 256
#define BLOCK_SIZE 256

byte buffer[BUFFERSIZE];
String mode;

void setup() {
  pinMode(DS, OUTPUT);
  pinMode(LATCH, OUTPUT);
  pinMode(CLOCK, OUTPUT);
  pinMode(VH, OUTPUT);
  pinMode(VPP, OUTPUT);
  digitalWrite(VH, LOW); 
  digitalWrite(VPP, LOW); 
  pinMode(CE, OUTPUT);
  pinMode(OE, OUTPUT);
  digitalWrite(CE, HIGH); 
  digitalWrite(OE, HIGH); 

  Serial.begin(115200);       // Start serial communication

}

void loop() {

  if (Serial.available()) {
    char c = toupper(Serial.read());
    mode = String(c);
  }

  if (mode == "R") { Read(); reset(); }
  if (mode == "W") { Write(); reset(); }
  if (mode == "V") { Verify(); reset(); }
  if (mode == "E") { Erase(); reset(); }
  if (mode == "B") { BlankCheck(); reset(); }
}


// ================= VERIFY ======================

void Verify() {
  data_bus_input();    // <<< ADD THIS
  data_bus_input();

  for (long addr = 0; addr <= 65535; addr++) {
    byte value = read_byte(addr);
    Serial.write(value); // send directly to PC
  }
}

// ================= WRITE =======================

void Write() {
  data_bus_output();

  digitalWrite(OE, HIGH);       // must be HIGH for programming
  digitalWrite(VPP, HIGH);  // 12.5V ON
  delay(10);
  byte block[BLOCK_SIZE];

    for (long block_start = 0; block_start <= 0xFFFF; block_start += BLOCK_SIZE) {
        // Receive a block of 256 bytes from Python
        for (int i = 0; i < BLOCK_SIZE; i++) {
            while (Serial.available() == 0);
            block[i] = Serial.read();
        }

        // Receive 1-byte checksum from Python
        while (Serial.available() == 0);
        byte expected_checksum = Serial.read();

        // Compute checksum on Arduino
        byte sum = 0;
        for (int i = 0; i < BLOCK_SIZE; i++) sum += block[i];

        // Compare checksum
        if (sum != expected_checksum) {
            // checksum mismatch → request retransmit
            Serial.write(0x00); // NACK
            block_start -= BLOCK_SIZE; // retry same block
            continue;
        }

        // Checksum OK → write block to chip
        for (int i = 0; i < BLOCK_SIZE; i++) {
            uint16_t addr = block_start + i;
            writeAdress(addr);
            write_data_bus(block[i]);

            digitalWrite(CE, LOW);
            delayMicroseconds(100);
            digitalWrite(CE, HIGH);
            delayMicroseconds(1);

            Serial.write(0xFF); // ACK per byte
        }

        // ACK block
        Serial.write(0xFF);
    }

  digitalWrite(VPP, LOW);   // turn off 12.5V
  data_bus_input();

}



// ================= READ ========================
void Read() {
  Serial.println("Reading...");

  long Start_Address = 0x0000;
  long BlockSize = 0x10000;   // 64 KB
 data_bus_input();
  for (long y = 0; y < BlockSize / 256; y++) {
   
    for (int x = 0; x < 256; x++) {
      buffer[x] = read_byte(Start_Address + y * 256 + x);
    }

    // PRINT
    printHex(Start_Address + y * 256, 4);
    Serial.print(": ");
    for (int i = 0; i < 16; i++) {
      printHex(buffer[i], 2);
      Serial.print(" ");
    }
    Serial.println();
  }

  Serial.println("Read done.");
}

// ================= ERASE =======================
void Erase() {
  digitalWrite(CE, HIGH); 
  digitalWrite(OE, HIGH);
  digitalWrite(VH, HIGH); 
  digitalWrite(VPP, HIGH);
  delay(1);

  //erase pulse
  digitalWrite(CE, LOW); 
  delay(350);
  digitalWrite(CE, HIGH); 
  delayMicroseconds(1);

  //Turning Off
  digitalWrite(VH, LOW); 
  digitalWrite(VPP, LOW);
  delayMicroseconds(1);

  Serial.println("Erase complete. Run a blank check");
}

// ================= BLANK =======================
void BlankCheck() {
  Serial.println("Blank check...");
    data_bus_input();    // <<< ADD THIS

  for (long a = 0; a < 65536; a++) {
    if (read_byte(a) != 0xFF) {
      printHex(a, 4);
      Serial.println("Not blank");
      return;
    }
  }
  Serial.println("OK");
}


void reset() {
  mode = "";
  Serial.println("R/W/V/E/B ?");
}
//###############################################################
// DATA BUS functions
//###############################################################
// Function to write 16 bits to two daisy-chained 595s
void writeAdress(uint16_t value) {
  digitalWrite(LATCH, LOW);          // Start sending data
  shiftOut(DS, CLOCK, MSBFIRST, (value >> 8) & 0xFF);
  shiftOut(DS, CLOCK, MSBFIRST, value & 0xFF);
  digitalWrite(LATCH, HIGH);         // Latch data to output pins
}

inline byte read_data_bus() {
    // Read D0-D5 (PD2-PD7)
    byte lower = (PIND & 0b11111100) >> 2;  // shift PD2-PD7 to bits 0-5

    // Read D6 (PB0) and D7 (PB2)
    byte upper = ((PINB & (1 << PB0)) << 6)  // PB0 → bit 6
               | ((PINB & (1 << PB2)) << 5); // PB2 → bit 7

    return lower | upper;
}
byte read_byte(unsigned int address) {
  writeAdress(address);

  digitalWrite(CE, LOW);
  digitalWrite(OE, LOW);
  delayMicroseconds(5);   // allow access time

  byte data = read_data_bus();

  digitalWrite(OE, HIGH);
  digitalWrite(CE, HIGH);

  return data;
}

void printHex(int num, int precision) {
  char tmp[16];
  char format[128];

  sprintf(format, "%%.%dX", precision);
  sprintf(tmp, format, num);
  Serial.print(tmp);
}

void data_bus_input() {
  pinMode(D0, INPUT);
  pinMode(D1, INPUT);
  pinMode(D2, INPUT);
  pinMode(D3, INPUT);
  pinMode(D4, INPUT);
  pinMode(D5, INPUT);
  pinMode(D6, INPUT);
  pinMode(D7, INPUT);
}

void data_bus_output() {
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
}

inline void write_data_bus(byte data)
{
    // D0-D5 → PD2-PD7
    // Clear PD2-PD7 bits
    PORTD = (PORTD & 0b00000011) | (data << 2);  

    // D6 → PB0, D7 → PB2
    // Clear PB0 and PB2
    PORTB = (PORTB & 0b11111010) 
            | ((data & 0x40) >> 6)    // D6 → PB0
            | ((data & 0x80) >> 5);   // D7 → PB2
}
