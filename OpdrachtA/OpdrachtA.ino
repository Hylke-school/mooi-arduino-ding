// Arduino Domotica server with Klik-Aan-Klik-Uit-controller 
//
// By Sibbele Oosterhaven, Computer Science NHL, Leeuwarden
// V1.2, 16/12/2016, published on BB. Works with Xamarin (App: Domotica)
//
// Hardware: Arduino Uno, Ethernet shield W5100; RF transmitter on RFpin; debug LED for serverconnection on ledPin
// The Ethernet shield uses pin 10, 11, 12 and 13
// Use Ethernet2.h libary with the (new) Ethernet board, model 2
// IP address of server is based on DHCP. No fallback to static IP; use a wireless router
// Arduino server and smartphone should be in the same network segment (192.168.1.x)
// 
// Supported kaku-devices
// https://eeo.tweakblogs.net/blog/11058/action-klik-aan-klik-uit-modulen (model left)
// kaku Action device, old model (with dipswitches); system code = 31, device = 'A' 
// system code = 31, device = 'A' true/false
// system code = 31, device = 'B' true/false
//
// // https://eeo.tweakblogs.net/blog/11058/action-klik-aan-klik-uit-modulen (model right)
// Based on https://github.com/evothings/evothings-examples/blob/master/resources/arduino/arduinoethernet/arduinoethernet.ino.
// kaku, Action, new model, codes based on Arduino -> Voorbeelden -> RCsw-2-> ReceiveDemo_Simple
//   on      off       
// 1 2210415 2210414   replace with your own codes
// 2 2210413 2210412
// 3 2210411 2210410
// 4 2210407 2210406
//
// https://github.com/hjgode/homewatch/blob/master/arduino/libraries/NewRemoteSwitch/README.TXT
// kaku, Gamma, APA3, codes based on Arduino -> Voorbeelden -> NewRemoteSwitch -> ShowReceivedCode
// 1 Addr 21177114 unit 0 on/off, period: 270us   replace with your own code
// 2 Addr 21177114 unit 1 on/off, period: 270us
// 3 Addr 21177114 unit 2 on/off, period: 270us

// Supported KaKu devices -> find, download en install corresponding libraries
#define unitCodeApa3      21177114  // replace with your own code
#define unitCodeActionOld 31        // replace with your own code
#define unitCodeActionNew 2210406   // replace with your own code

// Include files.
#include <SPI.h>                  // Ethernet shield uses SPI-interface
#include <Ethernet.h>             // Ethernet library (use Ethernet2.h for new ethernet shield v2)
//#include <NewRemoteTransmitter.h> // Remote Control, Gamma, APA3
//#include <RemoteTransmitter.h>    // Remote Control, Action, old model
//#include <RCSwitch.h>           // Remote Control, Action, new model

// Set Ethernet Shield MAC address  (check yours)
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Ethernet adapter shield S. Oosterhaven
int ethPort = 11250;                                  // Take a free port (check your router)

#define RFPin        3  // output, pin to control the RF-sender (and Click-On Click-Off-device)
#define lowPin       5  // output, always LOW
#define highPin      6  // output, always HIGH
#define switchPin    7  // input, connected to some kind of inputswitch
#define ledPin       8  // output, led used for "connect state": blinking = searching; continuously = connected
#define infoPin      9  // output, more information
#define analogPin    0  // sensor value

EthernetServer server(ethPort);              // EthernetServer instance (listening on port <ethPort>).
//NewRemoteTransmitter apa3Transmitter(unitCodeApa3, RFPin, 260, 3);  // APA3 (Gamma) remote, use pin <RFPin> 
//ActionTransmitter actionTransmitter(RFPin);  // Remote Control, Action, old model (Impulse), use pin <RFPin>
//RCSwitch mySwitch = RCSwitch();            // Remote Control, Action, new model (on-off), use pin <RFPin>


// ==========================================
//                        __                          
//   ____  __ __  _______/  |_  ____   _____     
// _/ ___\|  |  \/  ___/\   __\/  _ \ /     \   
// \  \___|  |  /\___ \  |  | (  <_> )  Y Y  \ 
//  \___  >____//____  > |__|  \____/|__|_|  / 
//      \/           \/                    \/  
// Custom stuff. Things below this depend on what app we make or even what sensors are attached
// and thus might need to be changed for each individual person.

char actionDevice = 'A';                 // Variable to store Action Device id ('A', 'B', 'C')
bool pinState = false;                   // Variable to store actual pin state
bool pinChange = false;                  // Variable to store actual pin change
int  sensorValue = 0;                    // Variable to store actual sensor value
char criteriaMode = 'm';                 // 'm' = manual, only switch states after user input. 'a' = auto, change states depending on sensor value
int sensorCriteria = 50;                 // Variable to store which value the sensor needs to reach to turn switches ON if criteriaMode == 'm'

// Voor de afstandssensor
const int distanceTrigPin = 2;
const int distanceEchoPin = 4;

void setup()
{
   Serial.begin(9600);
   //while (!Serial) { ; }               // Wait for serial port to connect. Needed for Leonardo only.

   Serial.println("Domotica project, Arduino Domotica Server\n");
   
   //Init I/O-pins
   pinMode(switchPin, INPUT);            // hardware switch, for changing pin state
   pinMode(lowPin, OUTPUT);
   pinMode(highPin, OUTPUT);
   pinMode(RFPin, OUTPUT);
   pinMode(ledPin, OUTPUT);
   pinMode(infoPin, OUTPUT);
   
   //Default states
   digitalWrite(switchPin, HIGH);        // Activate pullup resistors (needed for input pin)
   digitalWrite(lowPin, LOW);
   digitalWrite(highPin, HIGH);
   digitalWrite(RFPin, LOW);
   digitalWrite(ledPin, LOW);
   digitalWrite(infoPin, LOW);

   // Voor de afstandssensor
   pinMode(distanceTrigPin, OUTPUT);
   pinMode(distanceEchoPin, INPUT);

   //Try to get an IP address from the DHCP server.
   if (Ethernet.begin(mac) == 0)
   {
      Serial.println("Could not obtain IP-address from DHCP -> do nothing");
      while (true){     // no point in carrying on, so do nothing forevermore; check your router
      }
   }
   
   Serial.print("LED (for connect-state and pin-state) on pin "); Serial.println(ledPin);
   Serial.print("Input switch on pin "); Serial.println(switchPin);
   Serial.println("Ethernetboard connected (pins 10, 11, 12, 13 and SPI)");
   Serial.println("Connect to DHCP source in local network (blinking led -> waiting for connection)");
   
   //Start the ethernet server.
   server.begin();

   // Print IP-address and led indication of server state
   Serial.print("Listening address: ");
   Serial.print(Ethernet.localIP());
   
   // for hardware debug: LED indication of server state: blinking = waiting for connection
   int IPnr = getIPComputerNumber(Ethernet.localIP());   // Get computernumber in local network 192.168.1.3 -> 3)
   Serial.print(" ["); Serial.print(IPnr); Serial.print("] "); 
   Serial.print("  [Testcase: telnet "); Serial.print(Ethernet.localIP()); Serial.print(" "); Serial.print(ethPort); Serial.println("]");
   signalNumber(ledPin, IPnr);
}

void loop()
{
   // Listen for incomming connection (app)
   EthernetClient ethernetClient = server.available();
   if (!ethernetClient) {
      blink(ledPin);
      return; // wait for connection and blink LED
   }

   Serial.println("Application connected");
   digitalWrite(ledPin, LOW);

   // Server messages are limited to 15 chars (14 if you include >)
   int maxCommandLength = 15;
   char inBuffer[maxCommandLength];
   int inCount = 0;

   // Do what needs to be done while the socket is connected.
   while (ethernetClient.connected()) 
   {
      // If criteria mode is auto, make pins go on when sensorCriteria is reached (or off it it goes below)
      if (criteriaMode == 'a') {
        if (getSensorValue() > sensorCriteria && pinState == false) {
          pinState = true;
          digitalWrite(ledPin, HIGH);
          Serial.println("Automatically turned pins ON!");
        }

        else if (getSensorValue() < sensorCriteria && pinState == true) {
          pinState = false;
          digitalWrite(ledPin, LOW);
          Serial.println("Automatically turned pins OFF!");
        }
      }
   
      // Execute command hen byte is received.
      while (ethernetClient.available())
      {
         char inByte = ethernetClient.read();   // Get byte from the client.

         // End of command
         if (inByte == '>') {
            
            // Get the command message. 
            // Since inBuffer has a fixed size, we need to loop through it and ignore every character after inCount index.
            char copiedBuffer[inCount];
            for (int i = 0; i < inCount; i++) {
              copiedBuffer[i] = inBuffer[i];
            }

            // Execute command
            executeCommand(String(copiedBuffer));
            
            // Reset the read byte
            inByte = NULL;                         
           
            // Reset the inBuffer
            inCount = 0;
            for (int i = 0; i < maxCommandLength; i++) {
              inBuffer[i] = "";
            }
         }
         
         // Keep reading the next byte
         else {
            if (inCount < maxCommandLength) {
              inBuffer[inCount] = inByte;
              inCount++;
             }
         }
         
      } 
   }
   Serial.println("Application disonnected");
}

// Implementation of (simple) protocol between app and Arduino
// Response (to app) is 4 chars and ends with \n (not all commands demand a response)
void executeCommand(String cmd)
{     
   //* GetSensorValue()     - "s" - Value
   //* GetPinState()        - "p" - "ON", "OFF"
   //* GetSensorCriteria()  - "c" - Sensor criteria number
   //* GetCriteriaMode()    - "m" - "00a" (auto) or "00m" (manual)
   //* 
   //* TogglePinState()     - "t" - "ON" or "OFF" (the new state)
   //* SetSensorCriteria()  - "s=X" where x is a number - "sOk"
   //* SetCriteriaMode()    - "ma" or "mm" (for Mode Automatic, or Mode Manual) - "00m" or "00a" (the NEW mode)

   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   // !!!!!!!!!!!!!  IMPORTANT  !!!!!!!!!!!!!
   // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   
   // Responses need to be 4 bytes (see below, see Connection class in app) and need to end with with "\n"
   // Syntax to respond: server.write(buf, len). Note "len": you need to specify the length of the buffer (4). Don't forget!
 
   char buf[4] = {'\0', '\0', '\0', '\0'}; 

   // Command protocol
   Serial.print("["); Serial.print(cmd); Serial.print("] -> ");

   // SetSensorCriteria()
   if (cmd.startsWith("s=")) {
      String newSensorCriteria = cmd.substring(2); // Start counting from index 2 (third character, after =)
      sensorCriteria = newSensorCriteria.toInt();
      server.write("sOk\n", 4);
      Serial.println("New censor criteria: " + newSensorCriteria);
   }

   // SetCriteriaMode()
   else if (cmd.startsWith("ma") || cmd.startsWith("mm")) {
       criteriaMode = cmd[1];
       // create "00m\n" or "000a\n", response needs to be 4 bytes...
       buf[0] = '0';   buf[1] = '0';   buf[2] = criteriaMode;  buf[3] = '\n'; 
       server.write(buf, 4);
       Serial.println("New criteria mode: " + criteriaMode);
   }

   // GetSensorValue()
   else if (cmd.startsWith("s")) {
       intToCharBuf(getSensorValue(), buf, 4);
       Serial.println("GetSensorValue()");
   }

   // GetPinState()
   else if (cmd.startsWith("p")) {
      server.write(pinState ? " ON\n" : "OFF\n", 4);
      Serial.println("GetPinState() " + pinState ? " ON\n" : "OFF\n");
   }

   // GetSensorCriteria()
   else if (cmd.startsWith("c")) {
      intToCharBuf(sensorCriteria, buf, 4);
      server.write(buf, 4);
      Serial.println("GetSensorCriteria(): " + sensorCriteria);
   }

   // GetCriteriaMode()
   else if (cmd.startsWith("m")) {
      // create "00m\n" or "00a\n", response needs to be 4 bytes...
      buf[0] = '0';   buf[1] = '0';   buf[2] = criteriaMode;  buf[3] = '\n'; 
      server.write(buf, 4);
      Serial.println("GetCriteriaMode(): " + criteriaMode);
   }

   // TogglePinState() 
   else if (cmd.startsWith("t")) {
      pinState = pinState ? false : true;
      digitalWrite(ledPin, pinState ? HIGH : LOW);
      server.write(pinState ? " ON\n" : "OFF\n", 4);
      Serial.println("TogglePinState(): new state is " + pinState  ? "ON" : "OFF");
   }
}

// Read value from pin pn, return value is mapped between 0 and mx-1
int readSensor(int pn, int mx)
{
  return map(analogRead(pn), 0, 1023, 0, mx-1);    
}

// Convert int <val> char buffer with length <len>
void intToCharBuf(int val, char buf[], int len)
{
   String s;
   s = String(val);                        // convert tot string
   if (s.length() == 1) s = "0" + s;       // prefix redundant "0" 
   if (s.length() == 2) s = "0" + s;  
   s = s + "\n";                           // add newline
   s.toCharArray(buf, len);                // convert string to char-buffer
}

// ============================================================================================
//                        __                                   __  .__               .___      
//   ____  __ __  _______/  |_  ____   _____     _____   _____/  |_|  |__   ____   __| _/______
// _/ ___\|  |  \/  ___/\   __\/  _ \ /     \   /     \_/ __ \   __\  |  \ /  _ \ / __ |/  ___/
// \  \___|  |  /\___ \  |  | (  <_> )  Y Y  \ |  Y Y  \  ___/|  | |   Y  (  <_> ) /_/ |\___ \ 
//  \___  >____//____  > |__|  \____/|__|_|  / |__|_|  /\___  >__| |___|  /\____/\____ /____  >
//      \/           \/                    \/        \/     \/          \/            \/    \/ 
// Custom methods. Things important for interaction with the app. Methods used for custom server responses. Very important.



// Use this function to return sensor value to the mobile App
// IMPORTANT: distance() is 255 cm at most, but analog sensors are range 0-1023 
// and need to be mapped to something < 1000 so it's 3 characters since Arduino can only send 3 characters at most
int getSensorValue() {
  //return map(analogRead(A0), 0, 1023, 0, 100);
  return distance();
}

// Returns the distance measured by the distance sensor, in cm. 
int distance() {
    long timeWaited, cm;
 
    // De sensor wordt getriggerd bij 10 us, geef eerst een lage puls om een schone hoge puls te krijgen
    digitalWrite(distanceTrigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(distanceTrigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(distanceTrigPin, LOW);
 
    // Wacht op een hoge puls en meet de tijd
    timeWaited = pulseIn(distanceEchoPin, HIGH);
    
    // De formule om tijd in afstand om te zetten
    cm = timeWaited / 29 / 2;
    return cm;
}

// ============================================================================================                          
//   ________________  ______  
// _/ ___\_  __ \__  \ \____ \ 
// \  \___|  | \// __ \|  |_> >
//  \___  >__|  (____  /   __/ 
//      \/           \/|__|    
// Crap. Things that aren't important to remember / don't require much changing / are not even used (old code from docent voorbeeld) and belong at the bottom of the file.


// Choose and switch your Kaku device, state is true/false (HIGH/LOW)
void switchDefault(bool state)
{   
   //apa3Transmitter.sendUnit(0, state);          // APA3 Kaku (Gamma)                
   //delay(100);
   //actionTransmitter.sendSignal(unitCodeActionOld, actionDevice, state);  // Action Kaku, old model
   //delay(100);
   //////mySwitch.send(2210410 + state, 24);  // tricky, false = 0, true = 1  // Action Kaku, new model
   //////delay(100);
}

// Check switch level and determine if an event has happend
// event: low -> high or high -> low
void checkEvent(int p, bool &state)
{
   static bool swLevel = false;       // Variable to store the switch level (Low or High)
   static bool prevswLevel = false;   // Variable to store the previous switch level

   swLevel = digitalRead(p);
   if (swLevel)
      if (prevswLevel) delay(1);
      else {               
         prevswLevel = true;   // Low -> High transition
         state = true;
         pinChange = true;
      } 
   else // swLevel == Low
      if (!prevswLevel) delay(1);
      else {
         prevswLevel = false;  // High -> Low transition
         state = false;
         pinChange = true;
      }
}

// blink led on pin <pn>
void blink(int pn)
{
  digitalWrite(pn, HIGH); 
  delay(100); 
  digitalWrite(pn, LOW); 
  delay(100);
}

// Visual feedback on pin, based on IP number, used for debug only
// Blink ledpin for a short burst, then blink N times, where N is (related to) IP-number
void signalNumber(int pin, int n)
{
   int i;
   for (i = 0; i < 30; i++)
       { digitalWrite(pin, HIGH); delay(20); digitalWrite(pin, LOW); delay(20); }
   delay(1000);
   for (i = 0; i < n; i++)
       { digitalWrite(pin, HIGH); delay(300); digitalWrite(pin, LOW); delay(300); }
    delay(1000);
}

// Convert IPAddress tot String (e.g. "192.168.1.105")
String IPAddressToString(IPAddress address)
{
    return String(address[0]) + "." + 
           String(address[1]) + "." + 
           String(address[2]) + "." + 
           String(address[3]);
}

// Returns B-class network-id: 192.168.1.3 -> 1)
int getIPClassB(IPAddress address)
{
    return address[2];
}

// Returns computernumber in local network: 192.168.1.3 -> 3)
int getIPComputerNumber(IPAddress address)
{
    return address[3];
}

// Returns computernumber in local network: 192.168.1.105 -> 5)
int getIPComputerNumberOffset(IPAddress address, int offset)
{
    return getIPComputerNumber(address) - offset;
}
