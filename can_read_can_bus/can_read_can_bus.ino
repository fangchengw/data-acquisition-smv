#include <SPI.h>
#include <mcp2515.h>
#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD
#include <SD.h>

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4); // Change to (0x27,16,2) for 16x2 LCD.

#define BRIGHTNESS_PIN 9   // Must be a PWM pin
#define switch_pin 3

// display A4, A5

struct can_frame canMsg;
MCP2515 mcp2515(10);

// Can modify
int dataCollectionRate = 400; //data collection period (ms)and screen refresh rate
int brightness = 120; // brightness of lcd 0-255

bool closeFile = false;
bool closedisplay = false;
unsigned long timer = 0;

//file
File myFile;

float rpm = 0;
float fahr = 0;
float soc = 0;
float current = 0;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  
  mcp2515.reset();
  mcp2515.setBitrate(CAN_125KBPS);
  mcp2515.setNormalMode();

  // Initiate the LCD:
  lcd.init();
  lcd.backlight();
  analogWrite(BRIGHTNESS_PIN, brightness);

  pinMode(switch_pin, INPUT_PULLUP);

  SD.begin(4);
  myFile = SD.open("test.csv", FILE_WRITE);
  if (myFile) {
    //Serial.print("Success in Opening Files");
    String header = "Time, Temp, RPM, Current, State of Charge\n";
    myFile.print(header);
  } else {
    // if the file didn't open, print an error:
    lcd.setCursor(0, 0);
    lcd.print("Error Opening File");
    //Serial.println("error opening test.txt");
    delay(20000);
  }
  
  Serial.println("------- CAN Read ----------");
  Serial.println("ID  DLC   DATA");
}

typedef union {
 float number;
 uint8_t bytes[4];
} FLOATUNION_t;

void loop() {
  //get data
  timer = millis();
  if (!closeFile){
    closeFile = digitalRead(switch_pin);
  }

  FLOATUNION_t value;
  FLOATUNION_t value2;
  
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
      
    Serial.print(canMsg.can_id, HEX); // print ID
    Serial.print(" "); 
    Serial.print(canMsg.can_dlc, HEX); // print DLC
    Serial.print(" ");

    memcpy(value.bytes, canMsg.data, 4);
    memcpy(value2.bytes, canMsg.data + 4, 4);
    Serial.print(value.number, 4);
    Serial.print(" ");
    Serial.print(value2.number, 4);

    Serial.println();      
  }

  if (canMsg.can_id == 0x01) {
    rpm = value.number;
  }
  if (canMsg.can_id == 0x02) {
    fahr = value.number;
  }
  if (canMsg.can_id == 0x03) {
    soc = value.number;
    current = value2.number;
  }

  if (timer % dataCollectionRate == 0) {
    if (!closeFile) {
      //values on lcd
      lcd.setCursor(0, 0); 
      String firstline = "Time:" + String(timer / 1000.0);
      lcd.print(firstline); // Print the string "Hello World!"
      //Serial.print(timer);
      //Serial.print("  ");
      //Serial.println(timer/1000.0 );
          
      lcd.setCursor(0, 1); 
      String secondline = "Temp:" + String(fahr) + " SOC:" + String(soc);
      lcd.print(secondline);
    
      lcd.setCursor(0, 2);
      String thirdline = "RPM:" + String(rpm);
      lcd.print(thirdline);
    
      lcd.setCursor(0, 3);
      String forthline = "Current:" + String(current);
      lcd.print(forthline);

      //print value to file
      String toFile = String(timer / 1000.0) + "," + String(fahr) + "," + String(rpm) + "," + String(current) + "," + String(soc) + "\n";
      myFile.print(toFile);
      
    } else {
      if (!closedisplay) {
        closedisplay = true;
        //close file
        //Serial.println(timer);
        myFile.close();
        lcd.clear();
        lcd.setCursor(0, 1);
        lcd.print("File Closed");
        //Serial.println("File Closed");
      }
    }
  }

}
