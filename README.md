# mooi-arduino-ding

Alle arduino code voor het IoT project

# Opdracht A

**Server responses** zijn altijd 4 bytes, 4 characters, waar de laatste "\n" is, want zo was de voorbeeld code...

Op dit moment kan hij een LED lamp aan / uit laten doen. Of ervoor zorgen dat hij automatisch aan / uit gaat als criteriaMode op "a" (auto) staat. 

## !!!!!!! HEEL BELANGRIJK !!!!!!!

Lees alles hieronder voordat je begint met testen:

![1](https://i.imgur.com/K1Lou9i.png)

Op dit moment gebruikt de server gewoon digitalWrite(ledPin, HIGH/LOW). Zorg er dus voor dat je ledPin op de juiste actuator hebt ingestelt. Als je geen LED gebruikt maar iets anders, zoals een Servo motor, dan moet je waarschijnlijk ook een paar commands aanpassen. 

De rest van al die pins is nog van de docenten voorbeeld code en heb ik daar gelaten zodat niks kapot kon gaan.

![2](https://i.imgur.com/zsmYD5B.png) 
![3](https://i.imgur.com/1EDVxE5.png) 

Ik heb zelf een afstandssensor gebruikt als sensor. Als je zelf iets anders gebruikt zoals een temperatuursensor of potentiometer moet je waarschijnlijk die pinModes daar uit de setup weghalen. 

![4](https://i.imgur.com/dJFNI75.png)

De server gebruikt getSensorValue() om de sensor waarde naar de app te sturen. Bij het testen gebruikte ik een afstandssensor dus staat daar distance(), een functie die ik daarvoor heb gemaakt. Distance() returnt maximaal 255 centimeer ongeveer. Als je een andere sensor gebruik kan het zijn dat de waarde tussen 0-1023 ligt. Pas dan de functie aan en gebruik dan map() om het 3 characters ipv 4 te maken (1 + 0 + 2 + 3 = 4 characters). Bijvoorbeeld:

```
int getSensorValue() {
  return map(analogRead(sensorPin), 0, 1023, 0, 100);
}
```
