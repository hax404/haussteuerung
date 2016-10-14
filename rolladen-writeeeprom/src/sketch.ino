#include<EEPROM.h>

void setup(){
	Serial.begin(115200);
	/* Gruppe 1 */
	uint8_t nSchalter = 8;
	byte schalterNr[] = {0,1,4,5,6,7,8,9};
	uint8_t gruppenNr = 1;

	Serial.println("Schreibe:");

	Serial.print("nSchalter = ");
	Serial.println(nSchalter, DEC);
	EEPROM.update(0, nSchalter);

	Serial.print("gruppenNr = ");
	Serial.println(gruppenNr, DEC);
	EEPROM.update(1, gruppenNr);

	for(int i = 0; i < nSchalter; i++){
		Serial.print("Ausgang ");
		Serial.print(i, DEC);
		Serial.print(": ");
		Serial.println(schalterNr[i], DEC);
		EEPROM.update(i+2, schalterNr[i]);
	}

	Serial.println("Schreiben abgeschlossen");

	Serial.println("Ruecklesen:");
	Serial.print("Anzahl Rolladen: ");
	Serial.println(EEPROM.read(0), DEC);

	Serial.print("Gruppen Nr: ");
	Serial.println(EEPROM.read(1), DEC);

	for(int i = 0; i < nSchalter; i++){
		Serial.print("Ausgang ");
		Serial.print(i, DEC);
		Serial.print(": ");
		Serial.println(EEPROM.read(i+2), DEC);
	}

	Serial.println("Beschreibung und Pruefung fertig");
}

void loop(){
	delay(60000);
}

