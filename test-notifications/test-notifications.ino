/*
  Web client

 This sketch connects to a website (http://www.google.com)
 using an Arduino Wiznet Ethernet shield.

 Circuit:
 * Ethernet shield attached to pins 10, 11, 12, 13

 created 18 Dec 2009
 by David A. Mellis
 modified 9 Apr 2012
 by Tom Igoe, based on work by Adrian McEwen

 */

#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = { 0x40, 0x6c, 0x8f, 0x36, 0x74, 0x8a };


// Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 0, 177);
IPAddress myDns(192, 168, 0, 1);

// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
EthernetClient client;


void sendDataToFirebase() {

//  String data = "{" ;
//  data = data + "\"to\": \"/topics/notifications\"," ;
//  data = data + "\"notification\": {" ;
//  data = data + "\"body\":" + "\"Motion detected\"," ;
//  data = data + "\"title\" : \"Alarm\" " ; data = data + "} }" ;
  String data = "{\"to\":\"/topics/notifications\",\"notification\": {\"body\": \"test message\",\"title\" : \"Title\"}}";
  char server[] = "http://mooi-deurbel-ding.000webhostapp.com/api.php";
  
  Serial.print("connecting to ");
  Serial.print(server);
  Serial.println("...");
  Serial.print("With data:");
  Serial.print(data);
  Serial.println("...");

  if (client.connect("145.14.144.89", 80))  {
   Serial.println("Connected to the server..");
   Serial.println(client.remoteIP());
   client.println("POST /api.php HTTP/1.1");
   client.println("Authorization: key=AAAAiDM7u40:APA91bGO0pMhKbF_eMvLoP-k4Mxz16pbLn3WP-zsyL_DISf1q1KJmohQIly3zHwnH4zyZkeRqZrC-2yJ42RSgJA4i9x2Y1L5Rtvc-4QsBaAYj0mQOcFWC4J3_cYL951GaoJ-b9reKs5c");
   client.println("Host: mooi-deurbel-ding.000webhostapp.com");
   client.println("Content-Type: application/json");
   client.print("Content-Length: ");
   client.println(data.length());
   client.print("\n");
   client.print(data);
  }
  else{
    Serial.println("connection failed");
  }

  Serial.println("Data sent...Reading response..");
  while (client.available())  {
   char c = client.read();
   Serial.print(c);
  }

  Serial.println("Finished!");
  client.flush();
  client.stop();
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // start the Ethernet connection:
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip, myDns);
  } else {
    Serial.print("  DHCP assigned IP ");
    Serial.println(Ethernet.localIP());
  }
  // give the Ethernet shield a second to initialize:
  delay(3000);
  sendDataToFirebase();
}

void loop() {
}
