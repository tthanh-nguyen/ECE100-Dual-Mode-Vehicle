#include <SPI.h>
#include <RF24.h>

/* =================================
   RF24 TRANSMITTER CONFIGURATION
================================= */
RF24 radio(7, 8);                 
const byte address[6] = "12345"; 

/* =================================
   GLOBAL VARIABLES
================================= */
char command = 0;             
bool isManual = false;        
bool isModeSelected = false; 

void setup() 
{
  /* =================================
     SERIAL & RF INITIALIZATION
  ================================= */
  Serial.begin(9600);

  if (!radio.begin()) {
    Serial.println("radio hardware not responding!");
    while (1);
  }

  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_MIN);
  radio.setChannel(80);
  radio.setDataRate(RF24_250KBPS);
  radio.stopListening();   // Set RF as TRANSMITTER

  /* =================================
     USER MODE SELECTION DISPLAY
  ================================= */
  Serial.println("Select a mode for the car");
  Serial.println("Manual = 1 || Auto = 2");
  Serial.println("");
}

void loop() 
{
  if (Serial.available())
  {
    command = toupper(Serial.read()); //  USER INPUT - RECEIVE FROM SERIAL
    if (command == '\n' || command == '\r') return;
    //       MODE SELECTION VALIDATION
    if (!isModeSelected && (command < '1' || command > '2'))
    {
      Serial.println("You must select a mode first! (1 or 2)");
      Serial.println("");
      return;
    }
      // MODE COMMAND PROCESSING (RECEIVE MODE FROM USER)
    if (command == '1')
    {
      isManual = true;
      isModeSelected = true;
      Serial.println("Manual mode activated");
      Serial.println("W = Forward | X = Backward | A = Left | D = Right | S = Stop");
      // TRANSMIT MODE TO SLAVE (RF)
      radio.write(&command, sizeof(command));
      Serial.println("A sent to B: 1");
      Serial.println("");
      return;
    }
    else if (command == '2')
    {
      isManual = false;
      isModeSelected = true;
      Serial.println("Auto mode activated");
      Serial.println("Manual commands are locked");
      // TRANSMIT MODE TO SLAVE (RF)
      radio.write(&command, sizeof(command));
      Serial.println("A sent to B: 2");
      Serial.println("");
      return;
    }
    // AUTO MODE - COMMAND LOCK
    if (!isManual && isModeSelected)
    {
      Serial.println("Cannot control in AUTO mode");
      Serial.println("Press 1 to return to Manual mode");
      Serial.println("");
      return;
    }
    // MANUAL CONTROL COMMAND PROCESSING (RECEIVE MOVEMENT FROM USER)
    switch (command)
    {
      case 'W': Serial.println("Forward"); break;
      case 'X': Serial.println("Backward"); break;
      case 'A': Serial.println("Turn Left"); break;
      case 'D': Serial.println("Turn Right"); break;
      case 'S': Serial.println("Stop"); break;

      default:
        Serial.println("Invalid command (W A S D X only)");
        Serial.println("");
        return;
    }

    /* =================================
       TRANSMIT CONTROL COMMAND TO SLAVE
    ================================= */
    radio.write(&command, sizeof(command));
    Serial.print("A sent to B: ");
    Serial.println(command);
    Serial.println("");
  }
}
