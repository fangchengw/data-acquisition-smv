#include <SPI.h>
#include <mcp2515.h>

struct can_frame canMsg1;
struct can_frame canMsg2;
MCP2515 mcp2515(10);

int hall_pin = 53;
int prevValue = 0;
double startTime = 0;
double elapsedTime = 0;
float pinValue = 0;
float rpm = 0;
float avgRpm = 0;
float thresh = 250;
float diff = 0;

typedef union {
 float number;
 uint8_t bytes[4];
} FLOATUNION_t;



void setup() {
  canMsg1.can_id  = 0x01;
  canMsg1.can_dlc = 8;
  
  while (!Serial);
  Serial.begin(115200);
  SPI.begin();
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();
  
  Serial.println("Example: Write to CAN");

  pinMode(A0, INPUT);
  prevValue = analogRead(A0);
}

void loop() {

  double endTime = millis();
  pinValue = analogRead(A0);
  diff = pinValue - prevValue;
  if (diff < -600) {
    elapsedTime = endTime - startTime;
    rpm = 60000/elapsedTime;
    startTime = endTime;
  }
  
  prevValue = pinValue;

  float rpm2 = 123.586;

  FLOATUNION_t rpmFloatUnion;
  rpmFloatUnion.number = rpm;

  memcpy(canMsg1.data, rpmFloatUnion.bytes, 4);

  rpmFloatUnion.number = rpm2;

  memcpy(canMsg1.data + 4, rpmFloatUnion.bytes, 4);
 
  mcp2515.sendMessage(&canMsg1);

  Serial.println(canMsg1.data[0]);
}
