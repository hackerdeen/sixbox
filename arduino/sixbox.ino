#include "IPv6EtherShield.h"

// sixbox
//
// (C) Iain Learmonth 2013.
//
// The program implements a simple IPv6 http-server
// outputs the current values of the analog input pins.
// It will also respond to various HTTP headers by
// doing things with LEDs and a buzzer.
//
// Note: Works on an Arduino Uno with an ATMega328 and 
// an ENC28J60 Ethershield
//
// Please modify the following line. The MAC-address has to be unique
// in your local area network.

static uint8_t mMAC[6] = {0x00,0x22,0x15,0x24,0x02,0x04};

IPv6EtherShield ipv6ES = IPv6EtherShield();

#define WAITING_FOR_REQUEST 0
#define HTTP_OK_SENT        1
#define HEADER_SENT         2
#define DATA_SENT           3

#define GREEN_PIN           2
#define BUZZER_PIN          3
#define YELLOW_PIN          4

char* greenOnExpect = " X-Green: On";
char* greenOffExpect = " X-Green: Off";
char* yellowOnExpect = " X-Yellow: On";
char* yellowOffExpect = " X-Yellow: Off";
char* buzzExpect = " X-Buzz: Yes";

char* jsonTrue = "true";
char* jsonFalse = "false";

char httpState = WAITING_FOR_REQUEST;
char sendingDataLine;

int buzzTime, greenTime, yellowTime;

bool green, yellow;

void setup() {
  // init electronics
  
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(YELLOW_PIN, OUTPUT);
  
  Serial.begin(9600);
  
  // init network-device
  ipv6ES.initENC28J60(mMAC);
  ipv6ES.initTCPIP(mMAC, processIncomingData);  
  // add "Link Local Unicast" Address
  // for testing under Linux: ping6 -I eth0 fe80::1234
  ipv6ES.addAddress(0xfe80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1234);
  // add "Global Unicast Address"
  // for testing under Linux: ping6 2a00:eb0:100:15::1234
  ipv6ES.addAddress(0x2a00, 0xeb0, 0x100, 0x15, 0x00, 0x00, 0x00, 0x1234); 
  // telnet listen
  ipv6ES.tcpListen(80);    
}

bool checkHeader(char* data, char* expect) {

  int i;
  
  for ( i = 1 ; i < strlen(expect) ; ++i ) {
    
    if ( data[i] != expect[i] ) {
      return false;
    }
  }
  
  return true;
  
}

void processIncomingData() { 
  char* replyString;
  char* newData;  
  
  switch (httpState) {
    case WAITING_FOR_REQUEST:
      if (ipv6ES.newDataAvailable()) {
        newData = ipv6ES.getNewData();
        // if we got to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so we can send a reply
        boolean currentLineIsBlank = true;
        while (*newData != 0) {
          if (*newData == '\n' && currentLineIsBlank) {      
            replyString = "HTTP/1.0 200 OK\r\nContent-Type: application/json\r\n\r\n";
            ipv6ES.sendData(replyString, strlen(replyString)); 
            httpState++;
            break;
          }          
          if (*newData == '\n') {
            // we're starting a new line
            
            // check for green header
            if ( checkHeader(newData, greenOnExpect) ) {
              green = true;
            }
            
            if ( checkHeader(newData, greenOffExpect ) ) {
              green = false;
            }
            
            if ( checkHeader(newData, yellowOnExpect) ) {
              yellow = true;
            }
            
            if ( checkHeader(newData, yellowOffExpect ) ) {
              yellow = false;
            }
            
            if ( checkHeader(newData, buzzExpect) ) {
              buzzTime = 1000;
            }
            
            currentLineIsBlank = true;
          } else if (*newData != '\r') {
            // we've gotten a character on the current line
            currentLineIsBlank = false;
          }
          newData++;
        }
      }
      break;
      
    case HTTP_OK_SENT:
      char reply[60];
      sprintf(reply, "{\r\n  \"greenLED\": %s,\r\n  \"yellowLED\": %s,\r\n",
         ( green ) ? jsonTrue : jsonFalse,
         ( yellow ) ? jsonTrue : jsonFalse);
      ipv6ES.sendData(reply, strlen(reply));
      httpState++;
      sendingDataLine = 0;
      break;
      
    case HEADER_SENT:
      // send the values of all analog pins
      char data[30];
      char data2[40];
      sprintf(data, "  \"analog%i\": %i", 
              sendingDataLine, analogRead(sendingDataLine));
      if (++sendingDataLine == 6) {
        sprintf(data2, "%s\r\n}\r\n", data);
        ipv6ES.sendData(data2, strlen(data2));
        httpState++;
      } else {
        sprintf(data2, "%s,\r\n", data);
        ipv6ES.sendData(data2, strlen(data2));
      }
  
      break;
      
    default: 
      // let http-client time to process data
      delay(1);
      httpState = WAITING_FOR_REQUEST;
      ipv6ES.closeConnection();
      break;
  }  
}

void loop() {
  ipv6ES.receivePacket();
  if (ipv6ES.newDataLength() != 0) {
    if (ipv6ES.isIPv6Packet()) {
      ipv6ES.processTCPIP();
    }
  }  
  ipv6ES.pollTimers();
  
  if ( buzzTime > 0 ) {
    digitalWrite(BUZZER_PIN, HIGH);
    --buzzTime;
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  if ( greenTime > 0 || green ) {
    digitalWrite(GREEN_PIN, HIGH); 
    if ( greenTime > 0 ) {
      --greenTime;
    }
  } else {
    digitalWrite(GREEN_PIN, LOW);
  }
  
  if ( yellowTime > 0 || yellow ) {
    digitalWrite(YELLOW_PIN, HIGH); 
    if ( yellowTime > 0 ) {
      --yellowTime;
    }
  } else {
    digitalWrite(YELLOW_PIN, LOW);
  }
  
}
