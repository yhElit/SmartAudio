#include <wiringPi.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//for PATH_MAX
#include <linux/limits.h>

#include <unistd.h>
#include "Communication.h"
#include <pthread.h>
#include "ini.h"
/*Debugging*
int M = 0;			//measurement
int LF = 0, LFold = 0;		//LoopFail
int I = 0, Iold = 0;		//In
int O = 0, Oold = 0;		//Out
int DF = 0, DFold = 0;		//DetectionFail
int DFA = 0;			//DetectionFail US-Sensor A
int DFB = 0;			//DetectionFAil US-Sensor B
int DA = 0;			//Distance A by Fail
int DB = 0;			//Distance B by FAil
/**/
typedef struct
{
	const char* IP;	//IP-Adresse des MQTT-Broker
	const char* ID;	//Sensor ID
} configuration;


void sigterm(int termsignal)
{
	cleanup();
}

static int iniHandler(void* user, const char* section, const char* name,
	const char* value) {
	configuration* pconfig = (configuration*)user;
	//#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0

	if (MATCH("connection", "broker")) {
		pconfig->IP = strdup(value);
	}
	else if (MATCH("connection", "sensorid")) {
		pconfig->ID = strdup(value);
	}
	else {
		return 0;
	}
	return 1;
}

long getDistance(int trigger, int echo) {
	long startBreak = 0;
	long startM = 0;
	long stopM = 0;
	double ellapse = 0;
	long dist = 0;
	//20ms nach trigern darf erneut gemessen werden
	long beginMeasuring = 0;
	long endMeasuring = 0;
	double waitTime = 0;
	/**if((DFold < DF) || (LFold < LF) || (Iold < I) || (Oold < O) || (M == 1800000)){
                        printf("\nMeasurements: %d      LoopFail:       %d      In:     %d      Out:    %d      DetectionFail:  %d      DFA:    %d      %d      DFB:    %d      %d \n", M, LF, I, O, DF, DFA, DA, DFB, DB);
                        DA = 0; DB = 0; DFold = DF; LFold = LF; Iold = I; Oold = O;
                }
                if(M == 1800000){
                        printf("\n%d Messungen\n", M);
                        getchar();
                }
	/**/
	//trigger kurz an und aus schalten
	digitalWrite(trigger, HIGH);
	delayMicroseconds(100);
	digitalWrite(trigger, LOW);

	beginMeasuring = clock();
	//Startzeiten
	startM = stopM = clock();

	//Startzeit speichern
	while (digitalRead(echo) == 0 && waitTime < 0.02) {
		endMeasuring = startM = clock();
		waitTime = (double)(endMeasuring-beginMeasuring)/CLOCKS_PER_SEC;
	//wenn in Schleife gefangen (normlerweise dauert das keine 2 millisiekunden)
		if(waitTime >= 0.02){
///DEBUG*/		LF++;
			break;
		}
	}
	while (digitalRead(echo) == 1){
		stopM = clock();
	}

	endMeasuring = clock();
	//jetzt noch warten bis Sensor für nächste  Messung bereit ist
	waitTime = (double)(endMeasuring-beginMeasuring)/CLOCKS_PER_SEC;
	while (waitTime < 0.02 ){
		endMeasuring = clock();
		waitTime = (double)(endMeasuring-beginMeasuring)/CLOCKS_PER_SEC;
	}
	//distanz berechnen
	ellapse = (double)(stopM - startM) / CLOCKS_PER_SEC;
	dist = (ellapse * 34320) / 2;
///DEBUG*/ 	M++;

	//negative Messungen gibts nicht und näher als 3 cm kann HC-SR04 nicht messen
	//also wird 1000cm zurückgegeben
	if(dist < 3) dist = 1000;
	return dist;
}

void mqttHandler(const char* message, const char* topic)
{	
	printf("handler active - message: %s\n", *message);
	printf("topic: %s", *topic);
	if(strcmp(message, "VOn") == 0)
	{
		printf("Turning on");
		system("snapclient -h volumiomaster.local -s 2");
	}
	if(strcmp(message, "VOff") == 0)
	{
		system("pkill -f snapclient*");
	}
}

int main() {
	atexit(cleanup);
	struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = sigterm;
    sigaction(SIGINT, &action, NULL);

	//Beinhaltet Hostnamen
	const int len = 30;
	char hostname[len];
	//buffer for current working directory
	char curWD[PATH_MAX] = {0};

	//initialize memory of hostname
	memset(hostname, 0, len);

	if(gethostname(hostname, 30) != 0)
	{
		fprintf(stdout, "Error fetching the hostname\n");
		exit(-1);
	}
	else
	{
		fprintf(stdout, "Hostname: %s\n", hostname);
	}

	//INITIALISIERUNG DER PINS
	//------------------------------------
	wiringPiSetup();

	int trigger1 = 1;	//GPIO 18
	int trigger2 = 0;	//GPIO 17
	int echo1 = 5;		//GPIO 24
	int echo2 = 6;		//GPIO 25

	pinMode(trigger1, OUTPUT);
	pinMode(trigger2, OUTPUT);
	pinMode(echo1, INPUT);
	pinMode(echo2, INPUT);

//------------------------------------
	//INITIALISIERUNG DER SERVERVERBINDUNG
//------------------------------------
	configuration config;

	//IP von MQTT-Broker und Sensor ID aus config.ini lesen

	fprintf(stdout, "Loading configuration\n");
	
	getcwd(curWD, PATH_MAX);
	strcat(curWD, "/config.ini");
	fprintf(stdout, "Path to config: %s\n", curWD);

	SAInitializeIni(curWD, iniHandler, &config);
	
	const char* topic = config.ID;
	const char* serverName = config.IP;

	//Verbindung mit SAInitialize initialisieren
	
	fprintf(stdout, "Initializing MQTT\n");
	SAInitializeMQTT(serverName, hostname, NULL);
	
	printf("MQTT Initialized\n");

	//Auf Verbindung warten
	printf("Wait -- Connection established\n");

	SASubscribeToTopic(topic);
	delayMicroseconds(300000);

	
	//Initialisierung der Variablen
	/* Bedeutungen:
	 * 		Detektion:		Das Ergebnis einer Distanzmessung EINES Sensors, wenn es unter dem vorgegebenen Wert minDistance liegt
	 * 		Durchgang:		Erneute Detektion des jeweils anderen Sensors, nachdem eine Detektion erfolgte.
	 * 		Polling:		Warten bis kein Hindernis mehr detektiert wird, nachdem ein Durchgang verzeichnet wurde
	 * */
	 
/*------Variablen für Detektion-------------------------*/
	long distance_a = 0;								//Distanz Sensor A
	long distance_b = 0;								//Distanz Sensor B
	long minDistance = 50; 								//Distanz die unterschritten werden muss, um als Detektion zu zählen
	
	/*------Variablen Durchgang nach Detektion--------------*/
	long init = minDistance + 1;						//initialisierung für liste
	long liste[5];										//Messungen nach Detektion
	long l[3];											//Messungen nach Detektion
	long addDistance = 0;								//neue Distanz nach Detektion wird in liste eingefügt
	unsigned int i = 0;									//counter in for schleifen
	
	int trigger = 0;									//nach einem Durchgang auf 1 gesetzt um Polling zu veranlassen
/*------Variablen für Polling nach Durchgang------------*/
	const int timesHigherMinTarget = 10;				//Anzahl an Messungen außerhalb von minDistance SOLL
	int timesHigherMin = 0;								//Anzahl an Messungen außerhalb von minDistance IST
	int bothDistLowerMin = 0;							//Boolean (DistALowerMin && DistBLowerMin)
	int DistALowerMin = 0;								//Boolean
	int DistBLowerMin = 0;								//Boolean
/*------------------------------------------------------*/
	

	int counter = 0;
	while (1){
		CheckSnapclientStatus();

		/*abwechselnde distanzmessung auf Sensor A und B*/
		distance_a = getDistance(trigger1, echo1);
		distance_b = getDistance(trigger2, echo2);
		
		/*wenn Sensor a eine Person detektiert hat, so wird Sensor b 5 mal abgefragt.
		Wenn mindestens 1 von den 5 Messungen kleiner als minDistance ist,
		so wird eine Nachricht mit "in" in unser MQTT-Topic gesendet*/
		if (distance_a < minDistance) {
			///DEBUG*/ 			DA = (int) distance_a;
			counter = 0;
			//Fehlmessungen ausschließen indem a nochmals abgefragt wird
			for(i = 0; i <(sizeof(l) / sizeof(*l)); i++){
				l[i] = init;
			}
			for(i = 0; i <(sizeof(l) / sizeof(*l)); i++){
                                addDistance = getDistance(trigger1, echo1);
				l[i] = addDistance;
                        }
			for(i = 0; i <(sizeof(l) / sizeof(*l)); i++){
                                if(l[i] < minDistance) counter++;
                        }
			if(counter >=2){
				/*liste initialisieren*/
				for (i = 0; i <(sizeof(liste) / sizeof(*liste)); i++){
					liste[i] = init;
				}
				for (i = 0; i < (sizeof(liste) / sizeof(*liste)); i++) {
					addDistance = getDistance(trigger2, echo2);
					liste[i] = addDistance;
				}
				for (i = 0; i < (sizeof(liste) / sizeof(*liste)); i++) {
					if (liste[i] < minDistance) {
//						printf("\nin\n");
///DEBUG*/ 						Iold = I++;
						trigger = 1;
						SASendMessage("in", topic);
						break;
					}//ENDIF
				}//ENDFOR
			}//ENDIF
///DEBUG*/ 			else{DF++;DFA++;}
		}//ENDIF
		/*Sensor B analog zu 
		 * -Sensor a, nur wird "out" gesendet*/
		else if (distance_b < minDistance) {
			///DEBUG*/ 			DB = (int) distance_b;
			//Fehlmessungen ausschließen indem b nochmals abgefragt wird
			counter = 0;
                        for(i = 0; i <(sizeof(l)/sizeof(*l));i++){
                                l[i] = init;
                        }
                        for(i = 0; i <(sizeof(l)/sizeof(*l));i++){
                                addDistance = getDistance(trigger2, echo2);
                                l[i] = addDistance;
                        }
                        for(i = 0; i <(sizeof(l)/sizeof(*l));i++){
                                if(l[i] < minDistance) counter++;
                        }
                        if(counter >=2){
				for (i = 0; i <(sizeof(liste) / sizeof(*liste)); i++){
					liste[i] = init;
				}
				for (i = 0; i < (sizeof(liste) / sizeof(*liste)); i++) {
					addDistance = getDistance(trigger1, echo1);
					liste[i] = addDistance;
				}
				for (i = 0; i < (sizeof(liste) / sizeof(*liste)); i++) {
					if (liste[i] < minDistance) {
//						printf("\nout\n");
///DEBUG*/ 						Oold = O++;
						trigger = 1;
						SASendMessage("out", topic);
						break;
					}//ENDIF
				}//ENDFOR
			}//ENDIF
///DEBUG*/ 			else{DF++;DFB++;}
		}//ENDIF
		

/*-------------------POLLING----------------------------*/
		if(trigger == 1){
			/*DEBUGGING*
			int c = 1;
			printf("\n	locked");
			/*ENDDEBUGGING*/
			timesHigherMin = 0;
			bothDistLowerMin = 0;
			DistALowerMin = 0;
			DistBLowerMin = 0;
			
			while(timesHigherMin < timesHigherMinTarget) {
				distance_a = getDistance(trigger1, echo1);
				distance_b = getDistance(trigger2, echo2);
				DistALowerMin = (distance_a <= minDistance);
				DistBLowerMin = (distance_b <= minDistance);
				bothDistLowerMin = DistALowerMin && DistBLowerMin;
				
				if(!bothDistLowerMin) timesHigherMin++;
				else timesHigherMin = 0;
				
				/*DEBUGGING*
				printf("\n 	Messung %i:\n", c);
				printf("		Distance a: %ld	min: %ld	isLower: %i\n", distance_a, minDistance, DistALowerMin);
				printf("		Distance b: %ld	min: %ld	isLower: %i\n", distance_b, minDistance, DistBLowerMin);
				printf("		bothDistLowerMin:		 %ld\n", bothDistLowerMin);
				printf("		timesLowerZero: 			 %ld\n", timesHigherMin);
				c++;
				/*ENDDEBUGGING*/
			}//ENDWHILE
			
			/*DEBUGGING*
			printf("	unlocked durch:\n");
			printf("\n 	Messung %i:\n", c);
			printf("		Distance a: %ld	min: %ld	isLower: %i\n", distance_a, minDistance, DistALowerMin);
			printf("		Distance b: %ld	min: %ld	isLower: %i\n", distance_b, minDistance, DistBLowerMin);
			printf("		bothDistLowerMin:		 %ld\n", bothDistLowerMin);
			printf("		timesLowerZero:			 %ld\n", timesHigherMin);
			/*ENDDEBUGGING*/
			trigger = 0;
		}
	}//ENDWHILE
	return 0;
}
