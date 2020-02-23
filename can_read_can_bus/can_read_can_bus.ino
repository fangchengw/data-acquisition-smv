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
int dataCollectionRate = 800; //data collection period (ms)and screen refresh rate
int brightness = 120; // brightness of lcd 0-255


unsigned long timer = 0;
int runnum;

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
  pinMode(switch_pin, INPUT);
  resetVar();
  
  
  Serial.println("------- CAN Read ----------");
  Serial.println("ID  DLC   DATA");
}

//use to convert byte to float
typedef union {
 float number;
 uint8_t bytes[4];
} FLOATUNION_t;

void loop() {
  //get data
  timer = millis() - startTime;
  if (digitalRead(switch_pin)) {
    //close file
    myFile.close();
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(millis());
    lcd.setCursor(0, 0);
    lcd.print("File Closed");
    delay(2000);
    resetVar();
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

  //save the file every 10s
  if (timer % 10000 == 0) {
    myFile.flush();
  }

  //display the data and write to the file at dataCollectionRate
  if (timer % dataCollectionRate == 0) {
        //values on lcd
        lcd.setCursor(0, 0); 
        String firstline = "R" + String(runnum) + " Time:" + String(timer / 1000.0);
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
        String toFile = String(timer / 1000.0) + "," + String(fahr) + "," + String(rpm) + "," + String(current) + "," + String(soc) + "," + String(millis() / 1000.0) + "\n";
        myFile.print(toFile);
    }

}

//return the run number for new run
int getHeaderNum() {
  if (SD.exists("header.txt")) {
    int num = 1;
    File readfile = SD.open("header.txt", FILE_READ);
    while (readfile.available()) {
      num = readfile.read() - '0';
      //Serial.print(num);
    }
    
    return num + 1;
  } else {
    return 1;
  }
}

//write the run number to the sd card, return -1 if fails, 0 if success
int writeToHeader(int headerNo) {
  File header = SD.open("header.txt", FILE_WRITE);
  if(header) {
    header.print(headerNo);
    header.close();
    return 0;
  } else {
    header.close();
    return -1;
  }
}

//reset all variable and wait for the button press to begin opening file
void resetVar() {
  lcd.clear();
  voltage = 0;
  soc = 0;
  current = 0;
  timer = 0;
  lcd.setCursor(0, 0);
  lcd.print("Hold To Begin");
  bool breakLoop = false;
  while (!breakLoop) {
    if (digitalRead(switch_pin)) {
      int index = 0;
      while (true) {
        //initiate SD card and the file
        SD.begin(4);
        runnum = getHeaderNum();
        //Serial.print(runnum);
        myFile = SD.open("Run_" + String(runnum) +".csv", FILE_WRITE); 
        if (myFile) {
          //Serial.print("Success in Opening Files");
          //TODO: add hear file for run number. display it somewhere and use it for file name
          if(writeToHeader(runnum) == -1) {
            goto failure;
          }
          String header = "Time,Temp,RPM,Current,State of Charge,Timer\n";
          myFile.print(header);
          break;
        } else {
          // if the file didn't open, print an error:
          goto failure;
        }
        failure:
        lcd.setCursor(0, 0);
        lcd.print("Error Opening File");
        lcd.setCursor(0, 1);
        lcd.print("Trying To Reopen");
        lcd.setCursor(0, 3);
        lcd.print("Hold To Restart");
        lcd.setCursor(index, 2);
        lcd.print(".");
        index++;
        if (index >= 20) {
          index = 0;
          lcd.setCursor(0, 2);
          lcd.print("                    ");
        }
        //Serial.println("error opening test.txt");
        //delay(500);
        if (digitalRead(switch_pin)) {
          lcd.clear();
          lcd.print("Restarting");
          setup();
          return; 
        }
       
        delay(500);
      }
      breakLoop = true;
    }
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Start in 3...");
  delay(1000);
  lcd.print("2...");
  delay(1000);
  lcd.print("1..");
  delay(1000);
  startTime = millis();
  lcd.clear();
}
