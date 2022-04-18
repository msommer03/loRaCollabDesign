#include <radio_config_Si4460.h>
#include <RHGenericDriver.h>
#include <RHHardwareSPI.h>
#include <RHGenericSPI.h>
#include <RadioHead.h>
#include <RHSPIDriver.h>

#include <RH_RF95.h>
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 7
#define RF95_FREQ 915.0
RH_RF95 rf95(RFM95_CS, RFM95_INT); 


const int  buttonPin = 11;    // the pin that the pushbutton is attached to
//const int tonepin = 12; //the pin that the speaker is attached to NOT IN USE
const int ledPin = 12; //the pin that the LED is attached to
const int onboardLED = 13; //the pin that connects to the onboard LED

int buttonState = 0;     // current state of the button
int lastButtonState = 0; // previous state of the button
int startPressed = 0;    // the moment the button was pressed
int endPressed = 0;      // the moment the button was released
int holdTime = 0;        // how long the button was hold
int idleTime = 0;        // how long the button was idle
int idleArray[100];     // array to record button idle times
int holdArray[100];   // array to record button hold times
int i; 
int j;
int k;
int sendArray[200];   // array to store idle/hold times to be sent
uint8_t buf[251];   //Lora message buffer
int recvArray[124];   
int pressed = 0;

//setup
void setup() {
  //sets up radio pin as an output pin
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  pinMode(ledPin, OUTPUT); //sets up LED pin

  Serial.begin(9600);
  
 // while (!Serial) {
 //   delay(1);
 // }

  //this connects the two boards
  delay(100);
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
  //intialization of the radio frequency
  while (!rf95.init()) {
    //Serial.println("LoRa radio init failed");
    //SoS(); // beeps SoS if radio fails to start
    while (1);
  }
  if (!rf95.setFrequency(RF95_FREQ)) {
    //Serial.println("setFrequency failed");
    //SoS();
    while (1);
  }
  //Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
  
  pinMode(buttonPin, INPUT); // initialize the button pin as a input, so can
  //rf95.setTxPower(23, false);

}

//this loops
void loop() {
  buttonState = digitalRead(buttonPin); // read the button input (maybe change to analog)[no]
  updateState();
  /******************************Transmit******************************/
  if(idleTime > 5000 && pressed == 1){ //Only transmit if it's been 5 seconds since last button press
  pressed = 0; //reset button press
  k = 0;
  idleArray[0] = 0; //set first delay to zero so the message doesn't start with delay
  for(i=0;i< sizeof recvArray;){
      sendArray[i]=idleArray[k]; //Put idle times in even index of sendArray
      i++; //incrementing position in send array by 1
      sendArray[i]=holdArray[k]; //Put hold times in odd index of sendArray
      i++; k++; //incrementing position of send array by 1 and of arrays that transcribe into send array by 1
  }
    memcpy(buf,sendArray,sizeof(sendArray)); //copy send array into buffer array
    //for(i=0;i<200;i++){ //[FOR DEBUG]
    //  Serial.println((int)x[i]);
    //}
    rf95.send(buf, sizeof(buf)); // Sends buffer
    rf95.waitPacketSent(); 
    startPressed = millis(); //resets counter 
    memset(idleArray,0,sizeof(idleArray)); //clears arrays
    memset(holdArray,0,sizeof(holdArray));
    i = 0; 
    j = 0;
    //tone(tonepin,NOTE_C6); //plays tone to indicate message sent [UNUSED, REPLACED WITH LED]
    delay(100);
    //noTone(tonepin); //2nd part to tone code, also unused
  }
  delay(2);
  if(lastButtonState != buttonState && buttonState == 1){ //if the button was not pressed and now is pressed
    idleArray[i] = idleTime; //Record how long the button was not pressed into array
    //Serial.print(idleArray[i]); //prints the time idle at current index, for debug
    //Serial.print(" ");
    i++;
  }

  if(lastButtonState != buttonState && buttonState == 0){ //if the button was pressed and now is not pressed
    holdArray[j] = holdTime; //records how long the button was pressed into array
    //Serial.println(holdArray[j]);//prints the hold time at current index, for debug
    j++;
  }
  lastButtonState = buttonState; //resets button state to last recorded button state, so it is kept to compare to for next loop
  /******************************Recieve******************************/
   //if the radio frequency is available
   if (rf95.available())
  {
    uint8_t len = sizeof(buf);
    //if it recieves buffer
    if (rf95.recv(buf, &len))
    {
      //prints the buffer
      RH_RF95::printBuffer("Received: ",buf, len);
      Serial.println(*(int*)buf);//changes buffer to int so it can print w better formatting
      Serial.print("RSSI: "); //formatting
      Serial.println(rf95.lastRssi(), DEC);
      //
      memcpy(recvArray,buf,sizeof(buf)); //copy recieved buffer into our reciving array
      /*
      for(i = 0; i < 100; i++){
        //Serial.println(x[i]);
        Serial.print(idleIn[i]);
        Serial.print(" ");
        Serial.println(holdIn[i]);
      }*/
     for(i = 0; i < sizeof recvArray - 1; i++){ //play the message through the onboard and attached LEDs
        digitalWrite(ledPin, LOW);//turns off attached LED
        digitalWrite(onboardLED, LOW);//turns off onboard LED
        delay(recvArray[i]); //holds for delay where button was not pressed
        digitalWrite(ledPin, HIGH);//turns on attached LED
        digitalWrite(onboardLED, HIGH);//turns on onboard LED
        Serial.println("Recieved");//prints "recieved" message to Serial Monitor [FOR DEBUG]
        i++; //increments variable i to get to place in array where time button is pressed is measured
        delay(recvArray[i]); //holds for delay where button is pressed
        digitalWrite(ledPin, LOW);//turns off attached LED so no message leaks
        digitalWrite(onboardLED, LOW);//turns off onboard LED once again so no message leaks
      }
      i = 0; //resets variable i for debug purposes
    }
  }
  
}

//function to update state of button
void updateState() {
  //if button is pressed:
  if (buttonState == HIGH) {
      startPressed = millis();
      holdTime = startPressed - endPressed; //measures time button was held down
      //tone(tonepin,NOTE_C5);
      if(idleTime > 5000){
        idleTime = 0; //if button was pressed after more than 5 seconds, restart 5 second timer
      }
      pressed = 1; //button is pressed
  } 
  //if button is not pressed:
  if(buttonState == LOW){
    //noTone(tonepin);
      endPressed = millis(); 
      idleTime = endPressed - startPressed; //measures time button was idle
    }
}
