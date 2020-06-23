#ifndef __HEADER_H__
#define __HEADER_H__


#include <Arduino.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <TimeAlarms.h>
#include <RBD_Button.h>
#include <RTClib.h>
#include <SoftwareSerial.h>
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <ESP8266mDNS.h>


char* string2char(String command);
void connectToWifi(String ssid, String password);
void createAccessPoint();
void setupWifi();
void setWifi ();
void setRTCTime();
String padInteger(int pUnPadded);
String timeToString(time_t inputTime);
String dateToString(time_t inputTime);
time_t getSunTimes(int calculationType, time_t inputDate, int zenithType);
time_t syncProvider();
String getAlarmTime (int pAlarm, int pFormat, bool pUTC);
String getTime(int pFormat, bool pUTC);
String getSunriseTime(int pFormat, bool pUTC);
String getSunsetTime(int pFormat, bool pUTC);
void handleRoot();
void loadCSS();
void loadHeaderImage();
void loadDateImage();
void loadDoorImage();
void loadSunriseImage();
void loadSunsetImage();
void loadTimeImage();
void redirectHome(String message);
void setupServer();
void clearWifiCredentials();
int getOverRun();
void setOverRun();
void handleReset ();
void handleSettings();
void motorForward();
void motorReverse();
void motorStop();
void setDoorState(int pDoorState);
void openDoor();
void closeDoor();
void stopDoor(int stoppedState);
void stopDoorOpened();
void stopDoorClosed();
String getDoorState();
void checkDoorState();
void alterDoorState();
void checkManualOverideButton();
void setSunAlarms();

//Set up switch pins
const int MANUAL_OVERIDE_PIN = D6;
const int DOOR_OPEN_PIN = D4;
const int DOOR_CLOSED_PIN = D5;

// Set up motor pins
int MOTOR_INPUT_1 = D7;
int MOTOR_INPUT_2 = D8;

//SunCalc
const int SUNCALC_SUNRISE = 0;
const int SUNCALC_SUNSET = 1;
const int ZENITH_DEFAULT = 0;
const int ZENITH_CIVIL = 1;
const int ZENITH_NAUTICAL = 2;
const int ZENITH_ASTRONOMICAL = 3;

//Mortlake Long/Lat
const float LATITUDE = -38.07164;
const float LONGITUDE = 142.803125;

//Door State
const String DOOR_STATE_NAME[7] = { "Unknown", "Open", "Closed", "Opening", "Closing", "Stopped-Opening", "Stopped-Closing" };
const int DOOR_STATE_UNKNOWN = 0;
const int DOOR_STATE_OPEN = 1;
const int DOOR_STATE_CLOSED = 2;
const int DOOR_STATE_OPENING = 3;
const int DOOR_STATE_CLOSING = 4;
const int DOOR_STATE_STOPPED_OPENING = 5;
const int DOOR_STATE_STOPPED_CLOSING = 6;

//What time to update open/close alarms (UTC)
const int ALARM_UPDATE_HOUR = 15;
const int ALARM_UPDATE_MINUTE = 0;
const int ALARM_UPDATE_SECOND = 0;

//Alarm types
const int ALARM_OPEN = 1;
const int ALARM_CLOSE = 2;
const int ALARM_UPDATE = 3;

//Time Date formats for string output
const int GT_TIMEONLY = 1;
const int GT_DATEONLY = 2;
const int GT_DATETIME = 3;

struct wifiCredentials {
  char ssid[20];
  char pwd[20];
};

int doorState;
int overRun;

//Setup Web Server
ESP8266WebServer server(80);

//Set up buttons
RBD::Button manualOveride(MANUAL_OVERIDE_PIN);
RBD::Button doorOpenSwitch(DOOR_OPEN_PIN);
RBD::Button doorClosedSwitch(DOOR_CLOSED_PIN);

//AlarmIDs
AlarmID_t dailyAlarm;
AlarmID_t openAlarm;
AlarmID_t closeAlarm;

//Initialise RTC
RTC_DS1307 RTC;

//Mortlake DST settings
TimeChangeRule auEDT = { "AEDT", First, Sun, Oct, 2, 660 };    //UTC + 11 hours
TimeChangeRule auEST = { "AEST", First, Sun, Apr, 3, 600 };    //UTC + 10 hours
Timezone localTime(auEDT, auEST);


#endif // __HEADER_H__