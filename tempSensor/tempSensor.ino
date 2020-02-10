#include <OneWire.h> 
#include <DallasTemperature.h>
#include <SPI.h>
#include <mcp2515.h>
/********************************************************************/
// Data wire is plugged into pin 2 on the Arduino 
#define ONE_WIRE_BUS 3
/********************************************************************/
// Setup a oneWire instance to communicate with any OneWire devices  
// (not just Maxim/Dallas temperature ICs) 
OneWire oneWire(ONE_WIRE_BUS); 
/********************************************************************/
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// BMS
#define current_pin A0
#define soc_pin A1

float maxCurrent = 5;
float current = 0;
float soc = 0;

// CAN
struct can_frame canMsg1;
struct can_frame canMsg2;
MCP2515 mcp2515(10);

typedef union {
  float number;
  uint8_t bytes[4];
} FLOATUNION_t;

void setup() {
  canMsg1.can_id  = 0x02;
  canMsg1.can_dlc = 8;
  canMsg2.can_id  = 0x03;
  canMsg2.can_dlc = 8;
  
  // start serial port 
  SPI.begin();
  Serial.begin(115200);

  // Start up the library 
  sensors.begin();

  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();

  pinMode(current_pin, INPUT);
  pinMode(soc_pin, INPUT);
}

void loop() {
  sensors.requestTemperatures(); // Send the command to get temperature readings 
  Serial.println(sensors.getTempCByIndex(0)); // Why "byIndex"?  
  FLOATUNION_t tempValue;
  tempValue.number = sensors.getTempCByIndex(0);

  soc = analogRead(soc_pin) / 1024.0 * 100.0;
  current = maxCurrent * analogRead(current_pin) / 1024.0;

  soc = 123.456;
  current = 7.89;

  FLOATUNION_t socValue;
  socValue.number = soc;

  FLOATUNION_t currentValue;
  currentValue.number = current;
  
  memcpy(canMsg1.data, tempValue.bytes, 4);
  mcp2515.sendMessage(&canMsg1);

  memcpy(canMsg2.data, socValue.bytes, 4);
  memcpy(canMsg2.data + 4, currentValue.bytes, 4);
  mcp2515.sendMessage(&canMsg2); 

  delay(100); 
}
