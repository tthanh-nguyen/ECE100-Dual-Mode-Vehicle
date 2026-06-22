#include <SPI.h>                 // Include SPI library for RF24 module communication
#include <RF24.h>                // Include RF24 library to control nRF24L01 module
#include <SoftwareSerial.h>      // Include SoftwareSerial library for UART communication with Arduino C

RF24 radio(7, 8);               // Create RF24 object with CE pin = 7, CSN pin = 8

const byte address[6] = "12345"; // Address for RF communication

SoftwareSerial toC(2, 3);       // Create SoftwareSerial on pins 2 (RX) and 3 (TX) to communicate with Arduino C

char command;                    // Variable to store received command

void setup() 
{
  Serial.begin(9600);            
  toC.begin(9600);               

  // Initialize RF24 module
  if (!radio.begin()) 
  {
    Serial.println("radio hardware not responding!"); 
    while (1);               
  }

  Serial.println("RF24 module initialized successfully!");

  // Configure RF24 module for receiving
  radio.openReadingPipe(0, address); 
  radio.setPALevel(RF24_PA_MIN);    
  radio.setChannel(80);              
  radio.setDataRate(RF24_250KBPS);   

  radio.startListening();           
  Serial.println("RF starts listening!");
}

void loop() 
{
  // ===== 1) RF receives command from Arduino A =====
  if (radio.available())            // Check if any data has been received
  {
    radio.read(&command, sizeof(command)); // Read the received command into 'command'

    Serial.print("B received from A: "); 
    Serial.println(command);       // Display received command on Serial Monitor

    // ===== 2) Send the received command to Arduino C via UART =====
    toC.write(command);             // Send the single character command over UART

    Serial.println("B sent to C"); // Confirm sending to Arduino C
  }
}
