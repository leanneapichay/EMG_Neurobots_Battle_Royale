////////////////////////////////////////////////////////////////////////
//            OpenBCI_GUI to Arduino via Serial: EMG                  //
//                                                                    //
//   Tested 6/16/2019 using iMac, Arduino UNO R3, OpenBCI_GUI 4.1.3   //
////////////////////////////////////////////////////////////////////////

#define NUM_CHAN 8 // if using Ganglion set to (4) / if using Cyton set to (8) / if using Cyton+Daisy set to (16)
#define BAUD_RATE 57600 //Tested with 57600 and 115200

#define THRESHOLD 0.25

const byte BufferSize = 96;
char Buffer[BufferSize + 1];
boolean newData = false;
float emgData[NUM_CHAN];
int pins[] = {3, 4, 5, 6, 7};

/* FIRE = pins[0]
   FORWARD = pins[1]
   LEFT = pins[2]
   RIGHT = pins[3]
   BACKWARD = pins[4]
*/

unsigned long delayTime = 500;
unsigned long timeOfLastFire = 0;
unsigned long timeOfLastForward = 0;
unsigned long timeOfLastRight = 0;
unsigned long timeOfLastLeft = 0;
unsigned long timeOfLastBackward = 0;

unsigned long loopLimit = 100;
unsigned long loopTime = 0;

void setup() {
  Serial.begin(BAUD_RATE);
  pinMode(LED_BUILTIN, OUTPUT);

  //set all controller pins to be able to write HIGH/LOW and default to HIGH
  for (int i = 0; i < 5; i++) {
    pinMode(pins[i], OUTPUT);
    digitalWrite(pins[i], HIGH);
  }

}

void loop() {
  // Data Format 4CH
  // 0.999,0.001,0.002,0.003\n
  // Data Format 16CH
  // 0.999,0.001,0.002,0.003,0.004,0.005,0.006,0.007,0.008,0.009,0.010,0.011,0.012,0.013,0.014,0.015\n
  receiveMoreThan64Chars();
  if (newData) {
    parseData(",", Buffer);
    newData = false;
  }

  /* FIRE = pins[0]
     FORWARD = pins[1]
     LEFT = pins[2]
     RIGHT = pins[3]
     BACKWARD = pins[4]
  */

  //only allow FIRE if it's been at least 100ms since ACTIVATE_RIGHT
  if (millis() - loopTime > loopLimit) {
    //if fire is above threshold, deiactivate turn right, then fire ... otherwise, deactive fire
    if (emgData[0] >= THRESHOLD) {    // if fire is above threshold
      digitalWrite(pins[3], HIGH);    // deactivate turn right
      if (millis() - timeOfLastRight > delayTime && millis() - timeOfLastLeft > delayTime && millis() - timeOfLastForward > delayTime && delayTime && millis() - timeOfLastBackward) { //only allow FIRE if it's been at least 100ms since ACTIVATE_RIGHT, ACTIVATE_LEFT, or ACTIVATE_FORWARD
        digitalWrite(pins[0], LOW);     // then FIRE
        timeOfLastFire = millis();
      }
    } else {
      digitalWrite(pins[0], HIGH);    // deactivate fire
    }

    if (millis() - timeOfLastFire > delayTime) { // only check for LEFT, RIGHT, FORWARD, BACK, if it's been a little bit since you've fired
      
      //if forward or backward are above threshold, deactivate the lower of the two, and activate the higher ... otherwise, deactivate both
      if (emgData[1] >= THRESHOLD || emgData[4] >= THRESHOLD) {    // if forward or backward are above threshold
        if (emgData[1] >= emgData[4]) {        // if forward is greater than backward (giving priority to forward in case of a tie)
          digitalWrite(pins[4], HIGH);    // deactivate backward
          digitalWrite(pins[1], LOW);     // activate forward
          timeOfLastForward = millis();
        } else {
          digitalWrite(pins[1], HIGH);    // deactivate forward
          digitalWrite(pins[4], LOW);     // activate backward
          timeOfLastBackward = 0;
        }
      } else { //if neither are above threshold
        digitalWrite(pins[4], HIGH);    // deactivate backward
        digitalWrite(pins[1], HIGH);    // deactivate forward
      }


      //if left or right are above threshold, deactivate the lower of the two, and activate the higher ... otherwise, deactivate both
      if ((emgData[2] >= THRESHOLD || emgData[3] >= THRESHOLD) && millis() - timeOfLastForward > delayTime && millis() - timeOfLastBackward > delayTime) {    // if left or right are above threshold && it's been a little bit since last back or forward
        if (emgData[2] >= emgData[3]) {        // if left is greater than right (giving priority to left in case of a tie)
          digitalWrite(pins[3], HIGH);    // deactivate RIGHT
          if (millis() - timeOfLastRight > delayTime) {     //only allow ACTIVATE_LEFT if it's been at least 100ms since ACTIVATE_RIGHT
            digitalWrite(pins[2], LOW); // activate LEFT
            timeOfLastLeft = millis();
          }
        } else {
          digitalWrite(pins[2], HIGH);    // deactivate LEFT
          if (millis() - timeOfLastLeft > delayTime) {    //only allow ACTIVATE_RIGHT if it's been at least 100ms since ACTIVATE_LEFT
            digitalWrite(pins[3], LOW); // activate RIGHT
            timeOfLastRight = millis();
          }
        }
      } else { //if neither are above threshold
        digitalWrite(pins[2], HIGH);    // deactivate left
        digitalWrite(pins[3], HIGH);    // deactivate right
      }
    }

    loopTime = millis();
    
  } //end of check

}

void receiveMoreThan64Chars() {
  if (Serial.available() > 0) {

    // Clear Buffer content
    memset(Buffer, 0, BufferSize + 1);

    while (Serial.available() > 0) {
      // "readBytes" terminates if the determined length has been read, or it
      // times out. It fills "Buffer" with 1 to 96 bytes of data. To change the
      // timeout use: Serial.setTimeout() in setup(). Default timeout is 1000ms.
      Serial.readBytes(Buffer, BufferSize);
    }

    // Print out buffer contents
    //Serial.println(Buffer);

    // You can use Serial.peek() to check if more than 96 chars
    // were in the serial buffer and if Buffer has truncated data.
    // This should never happen because you know what the max length of
    // the incoming data is and you have adequately sized your input buffer.
    if (Serial.peek() != -1) {
      Serial.println("96 byte Buffer Overflowed. ");
    }
    Buffer[BufferSize] = '\0'; //overwrite the \n char with \0 to terminate the string
    newData = true;
  }
}

void parseData(char* delimiter, char* str) {
  char* pch;
  pch = strtok (str, delimiter);
  int i = 0;
  while (pch != NULL) {
    emgData[i] = atof(pch);
    pch = strtok (NULL, ",");
    i++;
  }
}
