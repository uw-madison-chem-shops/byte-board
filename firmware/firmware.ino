int SPI_MOSI = 0;
int SPI_CLK = 2;
int SPI_CS = 1; 

int analogValue = 0;
volatile int value = 0;
volatile int lastEncoded = 0;
bool hex = false;
bool clock = true;
unsigned long latest_tick = millis();



void spiTransfer(volatile byte opcode, volatile byte data)  {
  digitalWrite(SPI_CS,LOW);
  shiftOut(opcode);
  shiftOut(data);
  digitalWrite(SPI_CS,HIGH);
}

void shiftOut(byte _send) {

  for(int i=7; i>=0; i--)  // 8 bits in a byte
  {
    digitalWrite(SPI_MOSI, bitRead(_send, i));    // Set MOSI
    digitalWrite(SPI_CLK, HIGH);                  // SCK high
    delay(1);
    digitalWrite(SPI_CLK, LOW);                   // SCK low
    delay(1);
  } 
}

void clearDisplay() {
  for(int i=0;i<8;i++) {
    spiTransfer(i+1, 0x00); 
  }
}

void write7Seg(int count) {
  if (hex) {
    spiTransfer(0x01, 0b0000110);  // decode mode
    // d2
    spiTransfer(0x62, count & 0x0000000F);
    // d1
    count = count >> 4;
    spiTransfer(0x61, count & 0x0000000F);
    // d0
    count = count >> 4;
    spiTransfer(0x60, 0x00);
  }
  else {
    spiTransfer(0x01, 0b0000111);  // decode mode
    // d2
    spiTransfer(0x62, (count % 10) & 0x0000000F);
    // d1
    count = count / 10;
    spiTransfer(0x61, (count % 10) & 0x0000000F);
    // d0
    count = count / 10;;
    spiTransfer(0x60, (count % 10) & 0x0000000F); 
  }
}

void setup() {
  pinMode(SPI_MOSI, OUTPUT);
  pinMode(SPI_CLK, OUTPUT);
  pinMode(SPI_CS, OUTPUT);
  digitalWrite(SPI_CS, HIGH);
  //
  spiTransfer(0x04, 0b00000000);  // enable shutdown
  spiTransfer(0x01, 0b0000111);  // decode mode
  spiTransfer(0x02, 0x7F);  // intensity
  spiTransfer(0x03, 0x05);  // display 8 digits
  spiTransfer(0x04, 0b00100001);  // disable shutdown, "clear"
  spiTransfer(0x04, 0b00000001);  // disable shutdown, "unclear"
  //
  pinMode(3, INPUT);
  pinMode(4, INPUT);
  pinMode(1, OUTPUT);
  digitalWrite(3, HIGH);
  digitalWrite(4, HIGH);
  GIMSK = 0b00100000;       // Enable pin change interrupts
  PCMSK = 0b00011000;       // Enable pin change interrupt for PB3 and PB4
  sei();                    // Turn on interrupts
}


byte flipByte(byte c){
  char r=0;
  for(byte i = 0; i < 8; i++){
    r <<= 1;
    r |= c & 1;
    c >>= 1;
  }
  return r;
}

int rotateOne(int input) {
  int c = (input >> 1) | (input << (8 - 1));
  return c;
}

void writeIndicators() {
  char out = 0;
  if (hex) {
    out = out | (1 << 7);
  }
  else {
    out = out | 1;
  }
  out = out | (clock << 1);
  spiTransfer(0x64, out); 
}

void loop() {
  analogValue = analogRead(0);
  if (analogValue < 978) {
    if (hex) {
      hex = false;
    }
    else {
      hex = true;
    }
  }
  spiTransfer(0x63, rotateOne(flipByte(value/4)));
  write7Seg(value/4);
  writeIndicators();
  if (clock) {
    if ((millis() - latest_tick) > 1000) {
      value += 4;
      latest_tick = millis();
    }
  }
  else {
    if ((millis() - latest_tick) > 60000) {
      clock = true;
    }
  }
}


ISR(PCINT0_vect) {
  int MSB = digitalRead(4); //MSB = most significant bit
  int LSB = digitalRead(3); //LSB = least significant bit
 
  int encoded = (MSB << 1) |LSB; //converting the 2 pin value to single number
  int sum  = (lastEncoded << 2) | encoded; //adding it to the previous encoded value
 
  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    value++;
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    value--;
 
  lastEncoded = encoded; //store this value for next time
 
  value &= 0b1111111111;
  clock = false;
  latest_tick = millis();
}