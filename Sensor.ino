#include <SoftwareSerial.h> // Software Serial library to define own serial communication
//#include <Servo.h> // servo library
#include <DHT.h> // dht11 library
#include <Stepper.h> // stepper library
#include<LiquidCrystal.h> // lcd library

#define DEBUG true
#define DHType DHT11 // type of DHT used 

SoftwareSerial esp(50,3); // make RX Arduino line pin 50, make TX Arduino line pin 3.
                          // This means that you need to connect the TX line from the esp to the Arduino's pin 50
                          // and the RX line from the esp to the Arduino's pin 3 

/*// Servo
int servoPin = 12; // servo motor control pin 
int servoPos = 0; // servoPos - position of servo(10 - 170)
Servo myServo; // creating myServo object of type Servo*/

// LCD
int rs = 32, en = 33, d4 = 34, d5 = 35, d6 = 36, d7 = 37;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); // create lcd object

// LED's                          
int bedroomLED = 24, kitchenLED = 25, livingroomLED = 26;


// Stepper motor
int stepsPerRevolution = 2048, motSpeed = 15, motDir = 0;
Stepper myStepper(stepsPerRevolution, 8, 10, 9, 11);
int state = 0; 

// Photoresistor
int lightPin = A0; // photoresistor Analog pin for reading 
int lightVal; // variable to hold the analog value of the sensor

// DHT11
int sensePin = 2; // DHT sense pin
float humidity, tempC, tempF; //variables to hold temperature, humidity values
DHT HT(sensePin, DHType); // create DHT object..
unsigned long lastDHTupdate = 0; // will store last time DHT11 was updated
long interval = 0; // interval at which to update (milliseconds)

// MQ-2
int MQ2buzzer = 51; // buzzer pin
int smokeSensor = A15; // smoke sensor read pin
int sensorThres = 850; // threshold value of MQ-2

// PIR
int PIRbuzzer = 27; // pin for the buzzer
int pirPin = 28; // input pin (for PIR sensor)
int PIRstateLED = 29; // led pin to visually show state of PIR
int pirState; // variable to store state of PIR

// Tilt Switch
int tiltPin = 23; // tiltPin
int tiltVal; // variable to store tiltPin Value 
int TiltstateLED = 7; // led pin to visually show state of Tilt switch
int TiltReset = 12; // to reset the tilt switch and tiltstateLED
int resetTilt = 0; // keep track of switch value(TiltReset);

// Water Sensor
int Val = 0; // variable for storing sensor value
int WSPower = 30; // Sensor power pin
int WSPin = A14; // Sensor read pin

//Water Pump
int WPSpeedPin = 22; // Water pump speed pin
int WPSpeed; // WPSpeed 0-255

// DC motor
int DCSpeedPin = 6; // DC motor speed pin
int dir1 = 5, dir2 = 4; // dir1 and dir2 direction pins 
int DCSpeed = 0; // DCSpeed 0-255

// put your setup code here, to run once:
void setup() {

    esp.begin(9600); // esp's baud rate (might be different depending on esp version)

    
    //Stepper motor
    myStepper.setSpeed(motSpeed); // define at what speed we want to run the stepper
    
    // LED'S
    pinMode(bedroomLED,OUTPUT); // declare bedroomLED as OUTPUT
    digitalWrite(bedroomLED,LOW); // set bedroomLED off initially
    pinMode(kitchenLED,OUTPUT); // declare kitchenLED as OUTPUT
    digitalWrite(kitchenLED,LOW); // set kitchenLED off initially
    pinMode(livingroomLED,OUTPUT); // declare livingroomLED as OUTPUT
    digitalWrite(livingroomLED,LOW); // set livingroomLED off initially

    // LCD
    lcd.begin(16, 2); // begin lcd(16 charters per line, 2 lines);

    // DHT11
    HT.begin(); // start the HT object

    // Photoresistor
    pinMode(lightPin, INPUT); // declare lightPin as INPUT

    
    // MQ-2
    pinMode(MQ2buzzer, OUTPUT);
    pinMode(smokeSensor, INPUT);
    Serial.begin(9600);

    // PIR
    pinMode(PIRbuzzer, OUTPUT);      // declare PIRbuzzer as output
    pinMode(pirPin, INPUT);     // declare sensor as input
    pinMode(PIRstateLED, OUTPUT); // declare stateLED as output
    digitalWrite(PIRstateLED, LOW); // un-armed state of PIR
    pirState = LOW; // we start, assuming no motion detected

    // Tilt switch
    pinMode(tiltPin, INPUT); // declare tiltPin as INPUT
    digitalWrite(tiltPin, HIGH); //internal pull-up resistor
    pinMode(TiltstateLED, OUTPUT);  // declare TiltstateLED as OUTPUT
    pinMode(TiltReset, OUTPUT); // declare TiltReset as output
   
    // Water sensor
    pinMode(WSPower, OUTPUT); // Set WSPower as an OUTPUT
    pinMode(WSPin, INPUT); // Set WSPin as an INPUT
    digitalWrite(WSPower, LOW); // Set to LOW so no power flows through the sensor

    // Water Pump
    pinMode(WPSpeedPin, OUTPUT); // declare WPSpeed pin as output
    WPSpeed = 0; // off water pump

    // DC motor
    pinMode(DCSpeedPin, OUTPUT); // declare DCSpeed pin as output
    pinMode(dir1, OUTPUT);  // declare dir1 as output
    pinMode(dir2, OUTPUT);  //declare dir2 as output

    sendCommand("AT+RST\r\n",2000,DEBUG); // reset module
    sendCommand("AT+CWMODE=3\r\n",1000,DEBUG); // 1:configure as station point, 2:access point, 3: both SP and AP
    sendCommand("AT+CWJAP=\"Dalam WiFi\",\"321@wireless\"\r\n",3000,DEBUG); // join to access point(wifi router)
    delay(5000); // wait for connection and get IP
    sendCommand("AT+CIFSR\r\n",1000,DEBUG); // get ip address
    sendCommand("AT+CIPMUX=1\r\n",1000,DEBUG); // configure for multiple connections
    sendCommand("AT+CIPSERVER=1,80\r\n",1000,DEBUG); // turn on server on port 80
    Serial.println("Server Ready");
}

/*class Sensor() {

    // Class member
    unsigned long lastUpdate; // will store lastime update check
    long updateInterval; // interval between updates

    // Constructor - initializes the member variables
    public:
    Sensor(long updateInterval) {

       this->updateInterval = updateInterval;
    }
    
    void Update() {

        MQ2();
        PIR();
        Tilt();
        WaterPump();
    }
};*/

// put your main code here, to run repeatedly:
void loop() { 

    unsigned long currentMillis = millis(); // start timer
    lightVal = analogRead(lightPin); // reading value of lightPin and store it in lightVal
    int temp = digitalRead(TiltReset); // read TiltReset value
    if(temp == 1)

        resetTilt = temp;
        
    MQ2();
    PIR();
    Tilt();
    WaterPump();
    
    if((currentMillis - lastDHTupdate > interval) || interval == 0) {

        interval = 600000; // change interval(10 mins(in ms))
        //interval = 10000;
        dht11();
        lastDHTupdate = currentMillis;
        
    }

    /*if(PIRstateLED == HIGH && lightVal < 50)

        digitalWrite(livingroomLED, HIGH);
    else
        digitalWrite(livingroomLED, LOW);*/
    
    if(esp.available()) { // check if the esp is sending a message 
     
        if(esp.find("+IPD,")) { // check for client
      
            delay(1000); // wait for the serial buffer to fill up (read all the serial data)
         
            // get the connection id so that we can then disconnect
            int connectionId = esp.read()-48; // subtract 48 because the read() function returns 
                                              // the ASCII decimal value and 0 (the first decimal number) starts at 48
              
             esp.find("pin="); // advance cursor to "pin="
                  
             int pinNumber = (esp.read()-48); // get first number i.e. if the pin 13 then the 1st number is 1
             int secondNumber = (esp.read()-48); // get second number

             if(secondNumber>=0 && secondNumber<=9) {
               
                pinNumber *= 10;
                pinNumber += secondNumber; // get second number, i.e. if the pin number is 13 then the 2nd number is 3, then add to the first number
             }
          
                //DC motor
                if(pinNumber == 6) {
                
                    digitalWrite(dir1, HIGH); // control direction of the DC motor either: CW or anti-CW
                    digitalWrite(dir2, LOW); //both HIGH or LOW stops motor.. Either one, moves motor in a specific direction
                    
                    if(!digitalRead(pinNumber)) {
                        
                        DCSpeed = 255;
                        analogWrite(DCSpeedPin, DCSpeed); // writes motor to run at what speed
                    }else {

                        DCSpeed = 0;
                        analogWrite(DCSpeedPin, DCSpeed);
                    }
                }
                 // Stepper motor
                 /*if(pinNumber == 41) {

                   
                  digitalWrite(pinNumber, !(digitalRead(pinNumber)));
              
                  if(digitalRead(pinNumber)) {
                        
                        motDir = (8)*2048;
                        myStepper.step(motDir);
                        motDir = 0;
                  }else {

                     motDir = -8*2048; 
                     myStepper.step(motDir);
                     motDir = 0; 
                   }
                }*/
                else     
                digitalWrite(pinNumber, !(digitalRead(pinNumber))); //toggle pin 
               
             // build string that is sent back to device that is requesting pin toggle
             String content;
             content = "Pin ";
             content += pinNumber;
             content += " is ";
               
             if(digitalRead(pinNumber))
                
                content += "ON";
             else 
              
                content += "OFF";
            
            sendHTTPResponse(connectionId, content);
            
            // make close command
            String closeCommand = "AT+CIPCLOSE="; 
            closeCommand+=connectionId; // append connection id
            closeCommand+="\r\n";
            sendCommand(closeCommand,500,DEBUG); // close connection
        } 
    }
}

/*
* Name: dht11
* Description: Function used to check temperature and humidity
*/
void dht11() {

    //lcd.clear();
    lcd.setCursor(0, 0);
    humidity = HT.readHumidity(); // read humidity
    tempC = HT.readTemperature(); // read temperature
    //tempF = HT.readTemperature(true); // true to read in fahrenhite
    lcd.print("Humidity:");
    lcd.print(humidity);
    lcd.print("%");
    lcd.setCursor(0, 1);
    lcd.print("Temp:");
    lcd.print(tempC);
    lcd.print("C");
    //lcd.print(tempF);
    //lcd.print("F");
    //lcd.clear();
}

/*
* Name: MQ2
* Description: Function used to check if MQ-2 has exceeded threshold value. If 'yes' sound buzzer
*/
void MQ2() {
    
    int sensorVal = analogRead(smokeSensor);

    //Serial.print("Pin A0: ");
    //Serial.println(sensorVal);
    // Checks if it has reached the threshold value
    if(sensorVal > sensorThres)
   
        tone(MQ2buzzer, 1000, 200);
    else
        
        noTone(MQ2buzzer);
  
  //delay(2000);
}

/*
* Name: PIR
* Description: Function used to check if PIR detects motion. If 'yes' sound buzzer  
*/
void PIR() {
    
    int pirVal = digitalRead(pirPin);  // read input value

    if(digitalRead(PIRstateLED)) {
      
        if (pirVal == HIGH) { // check if the input is HIGH
              
            digitalWrite(PIRbuzzer, HIGH);  // turn buzzer on
            if (pirState == LOW) { // check state of PIR sensor
      
                //Serial.println("Motion detected!"); // print on output change
                pirState = HIGH; // change state
            }
        }else {
      
            digitalWrite(PIRbuzzer, LOW); // turn buzzer OFF
    
            if (pirState == HIGH) { // check state of PIR sensor
            
                //Serial.println("Motion ended!");  // print on output change
                pirState = LOW; // change state
           }
        }
    }
}

/*
* Name: Tilt
* Description: Function used to check tilt switch. 
*/
void Tilt() {
  
    tiltVal = digitalRead(tiltPin); // read tiltPin value i.e., 0 or 1
    //Serial.println(tiltVal);  // prints tiltVal to serial monitor
    
    if(tiltVal == 0) {  // checks tiltVal value(whether switch is tilted or not)
    
        digitalWrite(TiltstateLED, HIGH); // turns on led
     }/*else {
    
        digitalWrite(greenLED, HIGH); // turns on green led
        digitalWrite(redLED, LOW); // turns off red led
     }*/
     if((digitalRead(TiltstateLED) && resetTilt) == 1) {

        digitalWrite(TiltstateLED, LOW);
        resetTilt = 0;
     }
}

/* 
* Name: readWaterSensor
* Description: Function used to check water level. 
*/
int readWaterSensor() {
  
    digitalWrite(WSPower, HIGH);  // Turn the sensor ON
    delay(10); // wait 10 milliseconds
    Val = analogRead(WSPin); // Read the analog value form sensor
    digitalWrite(WSPower, LOW); // Turn the sensor OFF
    return Val; // send current reading
}

/* 
* Name: WaterPump
* Description: Function used to switch water pump on or off.  
*/
void WaterPump() {
    
    int level = readWaterSensor(); //get the reading from the function readSensor()
    if(level > 430) {

        WPSpeed = 255;
        analogWrite(WPSpeedPin, WPSpeed); // writes motor to run at what speed
     }else {
      
        WPSpeed = 0;
        analogWrite(WPSpeedPin, WPSpeed); // writes motor to run at what speed
    }
}

/*
* Name: sendData
* Description: Function used to send data to esp.
* Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
* Returns: The response from the esp (if there is a reponse)
*/
String sendData(String command, const int timeout, boolean debug) {
    
    String response = "";
    int dataSize = command.length();
    char data[dataSize];
    command.toCharArray(data,dataSize);
           
    esp.write(data,dataSize); // send the read character to the esp
    if(debug) {
      
        Serial.println("\r\n====== HTTP Response From Arduino ======");
        Serial.write(data,dataSize);
        Serial.println("\r\n========================================");
    }
    long int time = millis();
    
    while( (time+timeout) > millis()) {
      
        while(esp.available()) {
        
            // The esp has data so display its output to the serial window 
            char c = esp.read(); // read the next character.
            response+=c;
        }  
    }
    if(debug)
    
        Serial.print(response);
    
    return response;
}
 
/*
* Name: sendHTTPResponse
* Description: Function that sends HTTP 200, HTML UTF-8 response
*/
void sendHTTPResponse(int connectionId, String content) {
     
    // build HTTP response
    String httpResponse;
    String httpHeader;
    // HTTP Header
    httpHeader = "HTTP/2.0 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\n"; 
    httpHeader += "Content-Length: ";
    httpHeader += content.length();
    httpHeader += "\r\n";
    httpHeader +="Connection: close\r\n\r\n";
    httpResponse = httpHeader + content + " "; // There is a bug in this code: the last character of "content" is not sent, I cheated by adding this extra space
    sendCIPData(connectionId,httpResponse);
}
 
/*
* Name: sendCIPDATA
* Description: sends a CIPSEND=<connectionId>,<data> command
*/
void sendCIPData(int connectionId, String data) {
  
    String cipSend = "AT+CIPSEND=";
    cipSend += connectionId;
    cipSend += ",";
    cipSend +=data.length();
    cipSend +="\r\n";
    sendCommand(cipSend,1000,DEBUG);
    sendData(data,500,DEBUG);
}
 
/*
* Name: sendCommand
* Description: Function used to send data to esp.
* Params: command - the data/command to send; timeout - the time to wait for a response; debug - print to Serial window?(true = yes, false = no)
* Returns: The response from the esp (if there is a reponse)
*/
String sendCommand(String command, const int timeout, boolean debug) {
    
    String response = "";
    
    esp.print(command); // send the read character to the esp
    long int time = millis();
    
    while( (time+timeout) > millis()) {
      
        while(esp.available()) {
        
            //The esp has data so display its output to the serial window 
            char c = esp.read(); // read the next character.
            response+=c;
        
        } 
    }
    if(debug)
    
        Serial.print(response);
       
    return response;  
}
