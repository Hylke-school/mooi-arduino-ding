// Include files.
#include <SPI.h>                  // Ethernet shield uses SPI-interface
#include <Ethernet.h>             // Ethernet library (use Ethernet2.h for new ethernet shield v2)

// Set Ethernet Shield MAC address  (check yours)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; 
int ethPort = 11250;

boolean isBoxClosed = false; // Bool that keeps track of the box status (open or closed)

#define TRIGGER_PIN_SENSOR1   2     // Pin where the trigger point 1 on the ultrasonic sensor is connected.
#define ECHO_PIN_SENSOR1      3     // Pin where the echo point 1 of the ultrasonic sensor is connected.
#define TRIGGER_PIN_SENSOR2   8     // Pin where the trigger point 2 on the ultrasonic sensor is connected.
#define ECHO_PIN_SENSOR2      9     // Pin where the echo point 2 of the ultrasonic sensor is connected.
#define SWITCH_PIN            7     // Pin where the switch of the box is connected.
#define LOCK_PIN              6     // Pin where the mechanical lock of the box is connected.

#define BELL_TIMEOUT 7000     // Timeout of 7 seconds for the bell (how long the AnalogRead will read 0 to notice if the bell has been pressed).
#define UNLOCK_TIMEOUT 10000 // Timout of 10 seconds for the lock, it takes 10 seconds and then the lock closes automatically.

unsigned long dingDongTimeout = millis() + dingDongTimeout;
unsigned long unlockTimeout = 0;

EthernetServer server(ethPort);     // EthernetServer instance (listening on port <ethPort>)
EthernetClient client;              // EthernetClient instance, for sending POST request

void setup()
{
  Serial.begin(115200);

  //pinMode(DOORBEL_PIN, INPUT);
  pinMode(TRIGGER_PIN_SENSOR1, OUTPUT);
  pinMode(ECHO_PIN_SENSOR1, INPUT);
  pinMode(TRIGGER_PIN_SENSOR2, OUTPUT);
  pinMode(ECHO_PIN_SENSOR2, INPUT);
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  pinMode(LOCK_PIN, OUTPUT);

  Serial.println("Domotica project, Arduino Domotica Server\n");

  //Try to get an IP address from the DHCP server.
  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("Could not obtain IP-address from DHCP -> do nothing");
    while (true){ }   // no point in carrying on, so do nothing forevermore; check your router
  }

  //Start the ethernet server.
  server.begin();
  Serial.print("Listening address: ");
  Serial.print(Ethernet.localIP());
  Serial.print("\t Listening port: ");
  Serial.println(ethPort);
}

void loop()
{
  doorBellPressed();
  unlockTimer();
  // Listen for incomming connection (app).
  EthernetClient ethernetClient = server.available();
  if (!ethernetClient) {
    return; // Wait for connection.
  }

  Serial.println("Application connected");

  // Do what needs to be done while the socket is connected.
  while (ethernetClient.connected()) 
  {
    // Execute command hen byte is received.
    while (ethernetClient.available())
    {
      char inByte = ethernetClient.read();
      executeCommand(inByte);
      inByte = NULL;
    } 
    doorBellPressed();
    unlockTimer();
  }
  
  Serial.println("Application disconnected");
}

// Implementation of (simple) protocol between app and Arduino
// Response (to app) is 4 chars and ends with \n (not all commands demand a response)
void executeCommand(char cmd)
{     
  //*  Command explanation 
  //*  L:  Lock package box.
  //*      Call the function that will close and lock the package box.
  //*    Returns: nothing.
  //*  U:  Unlock the package box
  //*      Calls the function that will open the package box.
  //*    Returns: nothing.
  //*  S: Status of package box.
  //*    Returns: CLS if box is closed, returns OPN if box is open.
  //*  P: Status of package, present or not.
  //*    Returns: YES if box contains package, NO if box does not contain package.
  
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // !!!!!!!!!!!!!  IMPORTANT  !!!!!!!!!!!!!
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  
  // Responses need to be 4 bytes (see below, see Connection class in app) and need to end with with "\n"
  // Syntax to respond: server.write(buf, len). Note "len": you need to specify the length of the buffer (4). Don't forget!
  
 // char buf[4] = {'\0', '\0', '\0', '\0'}; 
  
  // Command protocol
  Serial.print("["); Serial.print(cmd); Serial.println("] -> ");
  switch (cmd) {
    case 'l':
      closePackageBox();
      break;
    case 'u':
      openPackageBox();
      break;
    case 's':
      if (checkBoxStatus())
        server.write("CLS\n", 4);
      else 
        server.write("OPN\n", 4);
        break;
    case 'p':
      switchPressed();
      if (hasPackage(TRIGGER_PIN_SENSOR1, ECHO_PIN_SENSOR1, 25) || hasPackage(TRIGGER_PIN_SENSOR2, ECHO_PIN_SENSOR2, 25)) {
        server.write("YES\n", 4);
      }
      else {
        server.write(" NO\n", 4);
      }
    }
}

//Close and lock the package box
void closePackageBox()
{
  digitalWrite(LOCK_PIN, LOW);
}

//Open the package box, start to check if the weightsensor value has changed
void openPackageBox() 
{
  digitalWrite(LOCK_PIN, HIGH);
  unlockTimeout = millis();
}

//Check the status of the box
// False = open / unlocked
// True = Closed / locked
bool checkBoxStatus() 
{  
  return isBoxClosed;
}

//Checks if the box contains an object using the ultrasonic sensor
//  trig = pin where trigger point is connected (use trigPin)
//  echo = pin where echo point is connected (use echoPin)
//  limit = maximum distance required to return true (I recommend the length of the box - 5)
bool hasPackage(int trig, int echo, int limit){
  float v = 0.0343; // Speed of sound at 20 degrees Celsius in centimeters/microsecond;
  long t; // Time
  int s; // Distance
  
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  // Sends out a soundwave
  digitalWrite(trig, HIGH);
  delayMicroseconds(50);
  digitalWrite(trig, LOW);

  t = pulseIn(echo, HIGH, 12333); //Returns the time between start and pulse recieved in microseconds
  
  s = v * (0.5 * t); //Calculates the distance to an object in centimeters
  Serial.println(s);
  if (s == 0) {
    return false;
  }
  if(s < limit){
    return true;
  }
  else{
    return false;
  }

}
//Sends http request to custom PHP script, that sends HTTPS request to Firebase, to trigger notification in app
//  data = JSON code for notification
//  server = server address, not used
//  IMPORTANT, DNS does not work with this script, so you have to manually resolve the IP address and fill it in at client.connect!
void sendNotification(){
  
  String data = "{\"to\":\"/topics/notifications\",\"notification\": {\"body\": \"Someone rang your doorbell!\",\"title\" : \"Doorbell\"}}";
  char server[] = "http://mooi-deurbel-ding.000webhostapp.com/api.php";
  
  Serial.print("connecting to ");  Serial.print(server);  Serial.println("...");  Serial.print("With data:");  Serial.print(data);  Serial.println("...");

  if (client.connect("145.14.144.89", 80))  {
   Serial.println("Connected to the server..");   Serial.println(client.remoteIP());
   client.println("POST /api.php HTTP/1.1");
   client.println("Authorization: key=AAAAiDM7u40:APA91bGO0pMhKbF_eMvLoP-k4Mxz16pbLn3WP-zsyL_DISf1q1KJmohQIly3zHwnH4zyZkeRqZrC-2yJ42RSgJA4i9x2Y1L5Rtvc-4QsBaAYj0mQOcFWC4J3_cYL951GaoJ-b9reKs5c");
   client.println("Host: mooi-deurbel-ding.000webhostapp.com");
   client.println("Content-Type: application/json");
   client.print("Content-Length: ");
   client.println(data.length());
   client.print("\n");
   client.print(data);
  } else {
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

void doorBellPressed() {
  if (millis() - dingDongTimeout > BELL_TIMEOUT) {
      int val = analogRead(0);
      if (val > 300) {
          dingDongTimeout = millis();
          sendNotification();
      }
   }
}

void unlockTimer() {
  if (unlockTimeout == 0) return;  // Timer not active
  if (millis() - unlockTimeout > UNLOCK_TIMEOUT) {
    unlockTimeout = 0; //millis();
    closePackageBox();
  }
}

void switchPressed() {
  //read the pushbutton value into a variable
  int sensorValue = digitalRead(SWITCH_PIN);

  isBoxClosed = sensorValue == HIGH;

}
