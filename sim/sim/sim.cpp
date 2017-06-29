#include "stdafx.h"
#include <windows.h>  
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <ctime>
#include <iostream>
#pragma warning(disable: 4244)

#define MOTOR
//#define SOUND
#define VS
#define POWERSAVING 
#define DATA
#define DEBUG



int analogRead(int pin);
void delay(int ms);
void loop();
int millis();
void happySound();
void ring();
void resetValues();
void printStatus();
void digitalWrite(int i, bool b);

#ifdef VS
using namespace std;
string str = "";
unsigned int  tiks = 0, lineCount = 0, pinValueA, pinValueB, LED = 0;
#endif

struct values
{
	unsigned short int sineMaxValue = 0, sineMinValue = 0;
	unsigned short int value = 0;
	unsigned short int pin = 0;
	bool isDown;
};
bool readBackReader();
void readFrontReader();

bool readReader(values* thisValues, bool canReset);

#if defined(DEBUG) || defined(DATA) || defined(VS) 
unsigned short int currentStatus = 0;
#endif
unsigned int stepTimeStamp = 0;
unsigned short int nonMissedStepsCount = 0;// , sineMinValue = 10, sineMaxValue = 200;
values frontValues;
values backValues;
const float
sineMinPercent = 0.2f,
sineMaxPercent = 0.6f;

const short int
maxStandTimeMs = 5000,
pauseBetweenMeasurments = 100,
maxMissedStepsAllowed = 5,
frontPin = 4,
backPin = 5,
buzzerPin = 0,
powerPin = 1,
buzzerToneHigh = 3200,
buzzerToneLow = 2000,
buzzerDuration = 30,
powerSaveTimeoutMs = 30000;

//const unsigned int restartTimeOut = 1800000;
bool lightOn = false;
unsigned long timeNow = 0;
unsigned short int c[20];

int _tmain()
{
#ifdef VS
	const long double timeStart = time(0);
	string buf; // Have oldValue buffer string
	string line;
	ifstream myfile("data3b.txt");
	//ifstream myfile("data3.txt");
	fstream myfileO;
	myfileO.open("data_with_motor_Marks.csv", ios::out);
	//ifstream myfileO("D:\\m\\iot\\vsSimulator\\data3_trimmed_From_89_To_127b.txt");
	if (!myfile.is_open())
		return 1;
	auto headers = "front,back,Fdown,Bdown,code";
	myfileO << headers << endl;
	frontValues.pin = 0;
	backValues.pin = 1;
	//ifstream myfile("data3.txt");
	if (myfile.is_open())
	{
		// getline(myfile, line);
		while (!myfile.eof())
		{
			//tiks += pauseBetweenMeasurments;
			lineCount++;
			getline(myfile, line);
			stringstream ss(line); // Insert the string into oldValue stream
			vector<string> tokens; // Create vector to hold our words
			while (ss >> buf)
				tokens.push_back(buf);
			if (tokens.size() > 2) {
				//sound = atoi(tokens[0].c_str());
				pinValueA = atoi(tokens[1].c_str());
				pinValueB = atoi(tokens[2].c_str());
				//frontValues.value = atoi(tokens[1].c_str());
				//backValues.value = atoi(tokens[2].c_str());
				loop();
				myfileO << str << endl;
			}
		}
		myfile.close();
		myfileO.close();
	}
#elif
	loop();
#endif
	const long double timeEnd = time(0);
	auto dur = timeEnd - timeStart;
	return 0;
}

void ring()
{
	resetValues();
}


void loop() {
	digitalWrite(LED, lightOn);
	lightOn = !lightOn;
	timeNow = millis();
	auto backIsDown = readBackReader();
#if defined(DEBUG) || defined(DATA)
	currentStatus = 0;
#endif
	if (!backIsDown) readFrontReader();
#if defined(DEBUG)
	printStatus();
#endif
	delay(pauseBetweenMeasurments);
}


bool readBackReader()
{
	backValues.value = analogRead(backValues.pin);
	if (!backValues.isDown) {
		if (backValues.value > backValues.sineMaxValue)
		{
			backValues.isDown = true;
			if (backValues.value * sineMaxPercent > backValues.sineMaxValue)
			{
				backValues.sineMaxValue = backValues.value* sineMaxPercent;
				backValues.sineMinValue = backValues.value* sineMinPercent;
			}
			resetValues();
			return true;
		}
		return false;
	}
	if (backValues.value < backValues.sineMinValue) {
		backValues.isDown = false;
		return false;
	}
	resetValues();
	return true;
}

void readFrontReader()
{
	frontValues.value = analogRead(frontValues.pin);
	if (!frontValues.isDown) {
		if (frontValues.value > frontValues.sineMaxValue)
		{
			frontValues.isDown = true;
			// adjust highest value
			if (frontValues.value * sineMaxPercent > frontValues.sineMaxValue)
			{
				frontValues.sineMaxValue = frontValues.value* sineMaxPercent;
				frontValues.sineMinValue = frontValues.value* sineMinPercent;
			}
			nonMissedStepsCount++;
			stepTimeStamp = timeNow;
			// unwanted walk
			if (nonMissedStepsCount > maxMissedStepsAllowed)
			{
#if defined(DEBUG) || defined(DATA)  || defined(VS) 
				currentStatus = 1;
#endif
				ring();
			}
			return;
		}
		if (timeNow - stepTimeStamp > powerSaveTimeoutMs)
		{
#if defined(DEBUG) || defined(DATA) || defined(VS) 
			currentStatus = 6;
#endif
			resetValues();
		}
	}
	else
	{
		if (frontValues.value < frontValues.sineMinValue) {
			frontValues.isDown = false;
			return;
		}

		// too long stand on front
		if (timeNow - stepTimeStamp > maxStandTimeMs)
		{
#if defined(DEBUG) || defined(DATA)  || defined(VS) 
			currentStatus = 2;
#endif
			ring();
		}
	}
}


void resetValues()
{
	nonMissedStepsCount = 0;
	stepTimeStamp = timeNow;
}


#ifdef DEBUG
void printStatus()
{
#if defined (VS)
	auto stat = currentStatus == 0 ? "" : to_string(currentStatus);
	auto front = frontValues.isDown ? "-19," : ",";
	auto back = backValues.isDown ? "-20," : ",";
	str = to_string(frontValues.value) + "," + to_string(backValues.value) + "," + front + back + stat;
	// +"," + to_string(lineCount);
#else
	if (currentStatus != 0)
		Serial.println(currentStatus);
#endif
}
#endif

#ifdef SOUND 
void happySound() {
	resetValues();
#ifndef VS
	int happyNotes[] = { 383, 60, 383, 60, 255, 60, 255, 60, 227, 60, 227, 60, 255, 120, 286, 60, 286, 60, 304, 60, 304, 60, 340, 60, 340, 60, 383, 120 };
	for (unsigned int i = 0; i < sizeof(happyNotes) / 4; i++) {
		auto duration = happyNotes[i * 2 + 1];
		auto nota = happyNotes[i * 2];
		for (long j = 0; j < duration * 1000L; j += nota * 2) {
			digitalWrite(buzzerPin, HIGH);
			delayMicroseconds(nota);
			digitalWrite(buzzerPin, LOW);
			delayMicroseconds(nota);
		}
		delay(duration / 2);
	}
#endif
}
#endif
void delay(int ms) {
	tiks += ms;
}
int analogRead(int pin)
{
	if (pin == 0)
		return pinValueA;
	return pinValueB;
}
int millis() {
	return tiks;
}

void digitalWrite(int i, bool b) {}