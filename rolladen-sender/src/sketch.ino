
/*
Serial Data:
Stellen 0-1: Befehl
Stellen 2-3: Rolladen
Stelle 4: Hoch/Runter (1 Hoch; 2 Runter)
Stellen 5-6: Zeit in Sekunden
------------
5 Stellen = 5 Bytes
*/

#include<mcp_can.h>
#include<SPI.h>
#define BUFLEN 7

const int SPI_CS_PIN = 9;
MCP_CAN CAN(SPI_CS_PIN);	// CS Pin 9

int decbytetoint(byte in){
	if(in >= 48 && in <= 57){
		return in-48;
	}
	else{
		return -1;
	}
}

bool inputval(byte *arr){
	bool out = 1;
	for(int i = 0; i < BUFLEN; i++){
		if(decbytetoint(*(arr+i)) == -1){
			out = 0;
			//Serial.println("Abbruch");
			break;
		}
	}
	return out;
}



void setup(){
	Serial.begin(115200);

	while(CAN_OK != CAN.begin(CAN_125KBPS,MCP_8MHz)){
		Serial.println("CAN BUS init fail");
		Serial.println("Init CAN BUS again...");
		delay(1000);
	}
	Serial.println("CAN BUS init ok");
}

void loop(){
	byte buffer[BUFLEN];
	byte *ptr = buffer;
	byte befehlNr = 0;
	int rolladenNr = 0;
	int richtung = 0;
	int zeit = 0;

	byte stmp[4];

	if(Serial.available() > 0){
		Serial.readBytes(buffer, BUFLEN);

		if(inputval(ptr)){
			befehlNr = decbytetoint(buffer[0])*10;
			befehlNr += decbytetoint(buffer[1]);
			Serial.print("Befehl Nr: ");
			Serial.println(befehlNr, DEC);

			rolladenNr = decbytetoint(buffer[2])*10;
			rolladenNr += decbytetoint(buffer[3]);

			Serial.print("Rolladen Nr: ");
			Serial.println(rolladenNr, DEC);

			richtung = decbytetoint(buffer[4]);
			Serial.print("Richtung: ");
			Serial.println(richtung, DEC);

			zeit = decbytetoint(buffer[5])*10;
			zeit += decbytetoint(buffer[6]);
			Serial.print("Zeit: ");
			Serial.println(zeit, DEC);

			stmp[0] = befehlNr;	// Typ: Rolladen
			stmp[1] = rolladenNr;
			stmp[2] = richtung;
			stmp[3] = zeit;

			Serial.println("Sende...");
			CAN.sendMsgBuf(0x01, 0, 4, stmp);
		}
	}
}

