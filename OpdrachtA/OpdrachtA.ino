// Include files.
#include <SPI.h>                  // Ethernet shield uses SPI-interface
#include <Ethernet.h>             // Ethernet library (use Ethernet2.h for new ethernet shield v2)

// Set Ethernet Shield MAC address  (check yours)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Ethernet adapter shield S. Oosterhaven
int ethPort = 11250;                                  // Take a free port (check your router)

int buttonPin = 2;
boolean beenPressed = false;

int trigPin = 3; //Pin wehere the trigger point on the ultrasonic sensor is connected
int echoPin = 4; //Pin where the echo point of the ultrasonic sensor is connected

EthernetServer server(ethPort);              // EthernetServer instance (listening on port <ethPort>)

void setup()
{
  Serial.begin(9600);
  pinMode(buttonPin, INPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

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
  // Listen for incomming connection (app)
  EthernetClient ethernetClient = server.available();
  if (!ethernetClient) {
    return; // wait for connection and blink LED
  }

  Serial.println("Application connected");

  // Do what needs to be done while the socket is connected.
  while (ethernetClient.connected()) 
  {
    if (ringBell()){
      doorBellPress();
    }

    // Execute command hen byte is received.
    while (ethernetClient.available())
    {
      char inByte = ethernetClient.read();
      executeCommand(inByte);
      inByte = NULL;
    } 
  }
  
  Serial.println("Application disonnected");
}

// Implementation of (simple) protocol between app and Arduino
// Response (to app) is 4 chars and ends with \n (not all commands demand a response)
void executeCommand(char cmd)
{     
  //*  Command explanation 
  //*  R:  Received bell
  //*      Confirmation from the app, it received the bell press
  //*    Returns: nothing
  //*  L:  Lock package box
  //*      Call the function that will close and lock the package box
  //*    Returns: nothing
  //*  U:  Unlock the package box
  //*      Calls the function that will open the package box.
  //*    Returns: nothing
  //*  S: Status of package box
  //*    Returns: CLS if box is closed and locked, returns OPN if box is unlocked
  //*  P: Status of package, present or not
  //*    Returns: YES if box contains package, NO if box does not contain package
  
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  // !!!!!!!!!!!!!  IMPORTANT  !!!!!!!!!!!!!
  // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  
  // Responses need to be 4 bytes (see below, see Connection class in app) and need to end with with "\n"
  // Syntax to respond: server.write(buf, len). Note "len": you need to specify the length of the buffer (4). Don't forget!
  
  char buf[4] = {'\0', '\0', '\0', '\0'}; 
  
  // Command protocol
  Serial.print("["); Serial.print(cmd); Serial.print("] -> ");
  switch(cmd)
  {
    case 'r':
      beenPressed = false;
      break;
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
      if(HasPackage(trigPin, echoPin, 20)){ //Limit set to 20, change according to the box size
        server.write("YES\n", 4);
      }
      else {
        server.write(" NO\n", 4);
      }
  }
}

boolean ringBell() {
  if (beenPressed == false)
    if (digitalRead(buttonPin))
      return true;

  return false;
}

void doorBellPress() 
{
  Serial.println("\nBell has been pressed");
  beenPressed = true;
  server.write("BEL\n");
}

//Close and lock the package box
void closePackageBox()
{

}

//Open the package box, start to check if the weightsensor value has changed
void openPackageBox() 
{

}

//Check the status of the box
// False = open / unlocked
// True = Closed / locked
bool checkBoxStatus() 
{
  
  
  return false;
}

//Checks if the box contains an object using the ultrasonic sensor
//  trig = pin where trigger point is connected (use trigPin)
//  echo = pin where echo point is connected (use echoPin)
//  limit = maximum distance required to return true (I recommend the length of the box - 5)
bool HasPackage(int trig, int echo, int limit){
  float v = 0.0343; // Speed of sound at 20 degrees Celsius in centimeters/microsecond;
  long t; // Time
  int s; // Distance

  // Sends out a soundwave
  digitalWrite(trig, HIGH);
  delay(10);
  digitalWrite(trig, LOW);

  t = pulseIn(echo, HIGH); //Returns the time between start and pulse recieved in microseconds

  s = v * (0.5 * t); //Calculates the distance to an object in centimeters

  if(s < limit){
    return true;
  }
  else{
    return false;
  }
}
