/*
	Satelite

	EEPROM Layout:
	Byte 0: Anzahl Schalter
	Byte 1: Gruppennummer
	Byte 2-9: Schalternummer

*/

/*
Ablauf:
	auf Interrupt vom CAN BUS warten:
	pruefen, ob wir gemeint sind
		Aktion ausfuehren
		runterzaehlen
		1 Sekunde warten
		manuell pollen und pruefen
*/

// Todo:
// Emergency Open/Stop
//  - Muesste persistiert werden und eine eigene Funktion in Dauerschleife haben

#include<EEPROM.h>
#include "mcp_can.h"
#include<Wire.h>
#include<avr/sleep.h>
#define NARR 8

uint8_t nSchalter;
uint8_t gruppenNr;
byte schalterNr[NARR];


volatile unsigned int zaehler[NARR] = {0,0,0,0,0,0,0,0};		// wird nicht runtergezaehlt, solange warte[] > 0.
volatile byte aktion[NARR] = {0,0,0,0,0,0,0,0};		// default sind alle Rollaeden gestoppt; wird mit warten ueberstimmt, wenn warte[] > 0
volatile unsigned int warte[NARR] = {0,0,0,0,0,0,0,0};		// default keine Wartezeit; > 0 bedeutet stop und selbst runterzaehlen.
uint16_t localcopy = 0;

static int delaytime = 1000;		// 1000 = delaytime*multiplicator


/* CS pin 9 */
const int SPI_CS_PIN = 9;
MCP_CAN CAN(SPI_CS_PIN);
unsigned char flagRecv = 0;

unsigned char len = 0;
unsigned char buf[8];

/* EEPROM in Array einlesen */
void eeprom2array(){
	nSchalter = EEPROM.read(0);
	gruppenNr = EEPROM.read(1);
	if(nSchalter > NARR){
		Serial.println("fehler: nSchalter zu gross");
		delay(10000);
	}
	else{
		for(int i = 0; i < nSchalter ; i++){
			schalterNr[i] = EEPROM.read(i+2);
		}
	}
}

bool countdown(){
	bool out = 0;	// 1 wenn runtergezaehlt, 0 wenn nicht
	for(int i = 0; i < NARR; i++){
		if(*(warte+i) > 0){			// erstmal Wartezeit abarbeiten
			*(warte+i) = *(warte+i)-1;
			out = 1;
		}
		else{
			if(*(zaehler+i) > 0){
				*(zaehler+i) = *(zaehler+i)-1;
				out = 1;

//				Serial.print("dekrementiere Zaehler ");
//				Serial.print(i, DEC);
//				Serial.print("\t");
//				Serial.println(*(zaehler+i), DEC);
			}
			else if(*(aktion+i)!=0){
				*(aktion+i) = 0;
				out = 1;
//				Serial.println("setze Aktion auf 0");
//				Serial.println(i, DEC);
			}
		}
	}
	return out;
}

bool schalterMyDepartment(byte schalter){
	for(int i = 0; i < NARR; i++){
		if(schalterNr[i] == schalter){
//			Serial.println("bin zustaendig");
			return 1;
		}
	}
	Serial.println("Schalter not my Department");
	return 0;
}

bool befehlValide(){
	if(buf[0] == 1){
		if(schalterMyDepartment(buf[1])){
			if(buf[2] == 0 || buf[2] == 1 || buf[2] == 2){
//				Serial.println("Befehl valide");
				return 1;
			}
		}
	}
	Serial.println("Befehl invaid");
	return 0;
}

void befehlEintragen(){
	for(int i = 0; i < NARR; i++){
		if(buf[1] == schalterNr[i]){
			if((*(aktion+i) == 1 && buf[2] == 2)||		// Richtungsaenderung mit Zwangspause
				(*(aktion+i) == 2 && buf[2] == 1)){
				*(warte+i) = 5;						// = 1Sekunde
			}

			if(buf[3] == 0){				// Zwangspause nachm Stoppen
				*(warte+i) = 10;						// = 1Sekunde
			}

			*(aktion+i) = buf[2];
			*(zaehler+i) = buf[3];

			return;
		}
	}
}

void checkNewCommand(){
	while(CAN_MSGAVAIL == CAN.checkReceive()){
		CAN.readMsgBuf(&len, buf);

		switch (buf[0]){
			case 1:
				if(befehlValide()){
					Serial.println("Befehl eintragen...");
					befehlEintragen();
				}
				break;

			default:
				Serial.println("anderer Befehl");
				for(int i = 0; i<len; i++){
					Serial.print(buf[i]);
					Serial.print("\t");
				}
				Serial.println();
				break;
		}
	}
}

void printStatus(){
	for(int i = 0; i < NARR; i++){
		Serial.print(*(aktion+i),DEC);
		Serial.print(" ");
		Serial.print(*(zaehler+i),DEC);
		Serial.print(" ");
		Serial.print(*(warte+i),DEC);
		Serial.print("\t");
	}
	Serial.println(" ");
	Serial.println(localcopy,BIN);
}

void writetomcp(){
	uint16_t out = 0;
	uint8_t outa = 0;
	uint8_t outb = 0;
	uint16_t tmp = 0;
	Serial.print("vorher: ");
	Serial.println(localcopy, BIN);
	for(int i = 0; i < NARR; i++){
		Serial.print("i: ");
		Serial.println(i,DEC);
//		if(*(aktion+i) == 0){		// warten oder keine Aktion
//			tmp = 3;
//			Serial.println(tmp, BIN);
//			tmp << i*2;
//			Serial.println(tmp, BIN);
//			out &= ~tmp;		// out and not tmp
//			Serial.println(*localcopy, BIN);
//			Serial.println("foo!!");
//		}

		if(*(warte+i) == 0 && (*(aktion+i) == 1 || *(aktion+i) == 2)){
			tmp = *(aktion+i);
			tmp = tmp << (i*2);
			out |= tmp;
		}
	}

//	if(out != *localcopy){
		localcopy = out;
		outa = out;
		out >>=8;
		outb = out;
		Wire.beginTransmission(0x20);
		Wire.write(0x12);
		Wire.write(outa);
		Wire.endTransmission();

		Wire.beginTransmission(0x20);
		Wire.write(0x13);
		Wire.write(outb);
		Wire.endTransmission();

		Serial.print("nacher: ");
		Serial.println(out,BIN);
//	}
}


void MCP2515_ISR(){
	checkNewCommand();
	Serial.println("got command");
//	flagRecv = 1;
}

void setup(){
	Serial.begin(115200);
	eeprom2array();

	Wire.begin();
	Wire.beginTransmission(0x20);
	Wire.write(0x00);	// IODIRA register
	Wire.write(0x00);	// set all of port A to outputs
	Wire.endTransmission();

	Wire.beginTransmission(0x20);
	Wire.write(0x01);	// IODIRB register
	Wire.write(0x00); // set all of port B to outputs
	Wire.endTransmission();

	Serial.print("Anzahl Schalter: ");
	Serial.println(nSchalter, DEC);
	Serial.print("Gruppen Nr: ");
	Serial.println(gruppenNr, DEC);
	for(int i = 0; i < nSchalter; i++){
		Serial.println(schalterNr[i], DEC);
	}

	while(CAN_OK != CAN.begin(CAN_125KBPS,MCP_8MHz)){
		Serial.println("CAN BUS init fail");
		Serial.println("Init CAN BUS Shield again");
		delay(500);
	}
	attachInterrupt(0, MCP2515_ISR, FALLING); // start interrupt

	Serial.println("CAN BUS init ok");
}

void sleepNow(){
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();
	Serial.println("sleep1");
	sleep_mode();
	sleep_disable();
}

void loop(){

	if(countdown()){
		Serial.println("countdown");
		writetomcp();
		printStatus();
	}
	else{
		// clear
		//writetomcp();
		// sleep deep
		sleepNow();
	}
	digitalWrite(6,HIGH);
	delay(500);
	digitalWrite(6,LOW);
	delay(500);


//	if(flagRecv){
//		flagRecv = 0;
//
//		Serial.println("00\t01\t02\t03\t04\t05\t06\t07");
//		for(int i = 0; i < NARR; i++){
//			Serial.print(schalterNr[i],DEC);
//			Serial.print("\t");
//		}
//		Serial.println();
//		Serial.println("-------------------------------");
//
//		do{
//			printStatus(zaehler, aktion, warte, &localcopy);
//			checkNewCommand(zaehler, aktion, warte);
//			writetomcp(aktion, warte, &localcopy);
////			Serial.println("befehl abfragen");
//			delay(delaytime);
//		} while(countdown(zaehler, aktion, warte));
//
//		printStatus(zaehler, aktion, warte, &localcopy);
//
//	}
}

