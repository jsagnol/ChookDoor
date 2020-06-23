#include <header.h>


//Getting it all sorted
void setup() {

  //Begin Serial
  Serial.begin(9600);

  //Begin EEPROM
  EEPROM.begin(512);

  //Clear wifi credentials from EEPROM if override button is pressed at startup
  if (manualOveride.onPressed()) {
    clearWifiCredentials();
  }

  setupWifi ();

  // Set up mDNS responder:
  while (!MDNS.begin("casadelpollo")) {
    delay(1000);
  }
  // Add service to MDNS-SD
  MDNS.addService("http", "tcp", 80);

  //Setup request handlers
  setupServer();

  //Begin SPIFFS
  SPIFFS.begin();

  //Begin Real Time Clock
  RTC.begin();

  //Set system clock (time) to sync with RTC
  setSyncProvider(syncProvider);

  //Setup alarms to open/close door
  setSunAlarms();

  //setup daily alarm to set open/close door times
  dailyAlarm = Alarm.alarmRepeat(ALARM_UPDATE_HOUR, ALARM_UPDATE_MINUTE, 0, setSunAlarms);

  EEPROM.get(0, doorState);
  EEPROM.get(45, overRun);

  //Button/Switch debounce
  manualOveride.setDebounceTimeout(150);
  doorOpenSwitch.setDebounceTimeout(50);
  doorClosedSwitch.setDebounceTimeout(50);

  //Setup Motor Pins
  pinMode(MOTOR_INPUT_1, OUTPUT);
  pinMode(MOTOR_INPUT_2, OUTPUT);

  //Button/Switch debounce
  manualOveride.setDebounceTimeout(150);
  doorOpenSwitch.setDebounceTimeout(50);
  doorClosedSwitch.setDebounceTimeout(50);

}

//Just keeps on going
void loop() {
  server.handleClient();
  checkDoorState();
  checkManualOverideButton();
  Alarm.delay(0);
}

//Check and action manual overide button
void checkManualOverideButton() {
  if (manualOveride.onPressed()) {
    alterDoorState();
  }
}

void motorForward() {
  digitalWrite(MOTOR_INPUT_1, LOW);
  digitalWrite(MOTOR_INPUT_2, HIGH);
}

void motorReverse() {
  digitalWrite(MOTOR_INPUT_1, HIGH);
  digitalWrite(MOTOR_INPUT_2, LOW);
}

void motorStop() {
  digitalWrite(MOTOR_INPUT_1, LOW);
  digitalWrite(MOTOR_INPUT_2, LOW);
}

void setDoorState(int pDoorState) {
  doorState = pDoorState;
  EEPROM.put(0, doorState);
  EEPROM.commit();
}

String getDoorState() {
  return DOOR_STATE_NAME[doorState];
}

//Check if door is open/closed/in between
void checkDoorState() {
  switch (doorState) {
    case DOOR_STATE_OPENING:
      if (doorOpenSwitch.isPressed()) {
        delay(overRun);
        stopDoor(DOOR_STATE_OPEN);
      }
      break;
    case DOOR_STATE_CLOSING:
      if (doorClosedSwitch.isPressed()) {
        delay(overRun);
        stopDoor(DOOR_STATE_CLOSED);
      }
      break;
    case DOOR_STATE_UNKNOWN:
      if (doorOpenSwitch.isReleased()) {
        openDoor();
      }
      else {
        setDoorState(DOOR_STATE_OPEN);
      }
      break;
    default:
      break;
  }
}

void alterDoorState() {
  switch (doorState) {
    case DOOR_STATE_OPEN:
      closeDoor();
      break;
    case DOOR_STATE_CLOSED:
      openDoor();
      break;
    case DOOR_STATE_OPENING:
      stopDoor(DOOR_STATE_STOPPED_OPENING);
      break;
    case DOOR_STATE_CLOSING:
      stopDoor(DOOR_STATE_STOPPED_CLOSING);
      break;
    case DOOR_STATE_STOPPED_OPENING:
      closeDoor();
      break;
    case DOOR_STATE_STOPPED_CLOSING:
      openDoor();
      break;
    case DOOR_STATE_UNKNOWN:
      openDoor();
      break;
    default:
      break;
  }
  redirectHome("Function: AlterDoorState");
}

void openDoor() {
  setDoorState(DOOR_STATE_OPENING);
  if (doorOpenSwitch.isReleased()) {
    motorForward();
  }
  else {
    setDoorState(DOOR_STATE_OPEN);
    motorStop();
  }
  redirectHome("Function: OpenDoor");
}

void closeDoor() {
  if (doorClosedSwitch.isReleased()) {
    setDoorState(DOOR_STATE_CLOSING);
    motorReverse();
  }
  else {
    setDoorState(DOOR_STATE_CLOSED);
    motorStop();
  }
  redirectHome("Function: CloseDoor");
}

void stopDoor(int stoppedState) {
  setDoorState(stoppedState);
  motorStop();
}

void stopDoorOpened() {
  setDoorState(DOOR_STATE_OPEN);
  motorStop();
  redirectHome("Function: StopDoorOpened");
}

void stopDoorClosed() {
  setDoorState(DOOR_STATE_CLOSED);
  motorStop();
  redirectHome("Function: StopDoorClosed");
}

//Set alarms to trigger door actions at sunrise/sunset
void setSunAlarms() {
  time_t sunrise;
  time_t sunset;
  TimeElements sunsetElements;
  TimeElements sunriseElements;
  sunrise = getSunTimes(SUNCALC_SUNRISE, localTime.toLocal(now()), ZENITH_DEFAULT);
  sunset = getSunTimes(SUNCALC_SUNSET, localTime.toLocal(now()), ZENITH_NAUTICAL);
  breakTime(sunset, sunsetElements);
  breakTime(sunrise, sunriseElements);
  closeAlarm = Alarm.alarmOnce(sunsetElements.Hour, sunsetElements.Minute, sunsetElements.Second, closeDoor);
  openAlarm =  Alarm.alarmOnce(sunriseElements.Hour, sunriseElements.Minute, sunriseElements.Second, openDoor);
}

//Pad time elements with a leading zero for display
String padInteger(int pUnPadded) {
  String unPadded;
  String padded;
  if (pUnPadded < 10) {
    unPadded = (String)pUnPadded;
    padded = "0" + unPadded;
    return padded;
  }
  unPadded = (String)pUnPadded;
  padded = unPadded;
  return padded;
}

//Convert H,M,S to readable string. UTC: LocalTime = false
String timeToString(time_t inputTime) {
  TimeElements inputTimeElements;
  String timeString;
  breakTime(inputTime, inputTimeElements);
  timeString = padInteger(inputTimeElements.Hour) + ":" + padInteger(inputTimeElements.Minute) + ":" + padInteger(inputTimeElements.Second);
  return timeString;
}

//Convert Y,M,D to readable string
String dateToString(time_t inputTime) {
  String dateString;
  TimeElements inputTimeElements;
  breakTime(inputTime, inputTimeElements);
  dateString = padInteger(inputTimeElements.Day) + "/" + padInteger(inputTimeElements.Month) + "/" + (inputTimeElements.Year + 1970);
  return dateString;
}

//Calculate sunrise and suset
//void CalcSun(int calcType, int year, int month, int day, int zenithType, int *ptrHour, int *ptrMinute)
time_t getSunTimes(int calculationType, time_t inputDate, int zenithType) {
  TimeElements inputDateElements;
  TimeElements outputTimeElements;
  breakTime(inputDate, inputDateElements);
  int inputDateYear = inputDateElements.Year + 1970;
  int inputDateMonth = inputDateElements.Month;
  int inputDateDay = inputDateElements.Day;

  //Used to convert degrees to radians and vice versa
  double D2R = 3.141592653 / 180;
  double R2D = 180 / 3.141592653;

  //Set zenith for different measures of twilight
  float zenith;
  switch (zenithType) {
    case ZENITH_DEFAULT:
      zenith = 90.83;  //Trial and error adjusted from 90 degrees to correspond more closely to results from online tools. (Google, SunCalc)
      break;
    case ZENITH_CIVIL:
      zenith = 96;
      break;
    case ZENITH_NAUTICAL:
      zenith = 102;
      break;
    case ZENITH_ASTRONOMICAL:
      zenith = 108;
      break;
    default:
      zenith = 90.83;
      break;
  }

  //Nitty gritty
  int N1 = floor(275 * inputDateMonth / 9);
  int N2 = floor((inputDateMonth + 9) / 12);
  int N3 = (1 + floor((inputDateYear - 4 * floor(inputDateYear / 4) + 2) / 3));
  int N = N1 - (N2 * N3) + inputDateDay - 30;
  double lngHour = LONGITUDE / 15;
  double t;
  if (calculationType == SUNCALC_SUNRISE) {
    t = N + ((6 - lngHour) / 24);
  }
  else {
    t = N + ((18 - lngHour) / 24);
  }
  double M = (0.9856 * t) - 3.289;
  double L = M + (1.916 * sin(M * D2R)) + (0.020 * sin(2 * M * D2R)) + 282.634;
  if (L > 360) {
    L = L - 360;
  }
  else if (L < 0) {
    L = L + 360;
  }
  double RA = R2D * atan(0.91764 * tan(L * D2R));
  if (RA < 0) {
    RA = RA + 360;
  }
  else {
    if (RA > 360) {
      RA = RA - 360;
    }
  }
  double LQuadrant = (floor(L / 90)) * 90;
  double RAQuadrant = (floor(RA / 90)) * 90;
  RA = RA + (LQuadrant - RAQuadrant);
  RA = RA / 15;
  double sinDec = 0.39782 * sin(L * D2R);
  double cosDec = cos(asin(sinDec));
  double cosH = (cos(zenith * D2R) - (sinDec * sin(LATITUDE * D2R))) / (cosDec * cos(LATITUDE * D2R));
  double H;
  if (calculationType == SUNCALC_SUNRISE) {
    H = 360 - R2D * acos(cosH);
  }
  else {
    H = R2D * acos(cosH);
  }
  H = H / 15;
  double T = H + RA - (0.06571 * t) - 6.622;
  double UT = T - lngHour;
  if (UT > 24) {
    UT = UT - 24;
  }
  else if (UT < 0) {
    UT = UT + 24;
  }

  //Calculate hour and minutes from UT (milliseconds)
  UT = UT * 3600 * 1000;
  long calcHour = floor(UT / 3600000);
  long calcMinute = floor(UT / 60000);
  long calcSecond = floor(UT / 1000);
  calcMinute = (calcMinute % 60);
  calcSecond = (calcSecond % 60);
  if (calcSecond >= 30) {
    if (calcMinute < 59) {
      calcMinute++;
    }
    else {
      calcMinute = 0;
      if (calcHour < 24) {
        calcHour++;
      }
      else {
        calcHour = 0;
      }
    }
  }
  outputTimeElements.Hour = (int)calcHour;
  outputTimeElements.Minute = (int)calcMinute;
  outputTimeElements.Day = inputDateElements.Day;
  outputTimeElements.Month = inputDateElements.Month;
  outputTimeElements.Year = inputDateElements.Year;

  return makeTime(outputTimeElements);
}

//Sync system time with RTC
time_t syncProvider() {
  return RTC.now().unixtime();
}

String getAlarmTime (int pAlarm, int pFormat, bool pUTC) {
  time_t alarmTime;
  time_t rtcTime;
  time_t fullAlarmTime;
  TimeElements currentDateElements;
  TimeElements alarmElements;
  String sOutput = "";

  switch (pAlarm) {
    case ALARM_OPEN:
      alarmTime = Alarm.read(openAlarm);
      break;
    case ALARM_CLOSE:
      alarmTime = Alarm.read(closeAlarm);
      break;
    case ALARM_UPDATE:
      alarmTime = Alarm.read(dailyAlarm);
      break;
    default:
      break;
  }

  //Alarm times don't store dates. Add date from RTC
  rtcTime = now();
  breakTime(rtcTime, currentDateElements);
  breakTime(alarmTime, alarmElements);
  alarmElements.Year = currentDateElements.Year;
  alarmElements.Month = currentDateElements.Month;
  alarmElements.Day = currentDateElements.Day;
  fullAlarmTime = makeTime(alarmElements);

  if (!pUTC) {
    fullAlarmTime = localTime.toLocal(fullAlarmTime);
  }
  switch (pFormat) {
    case GT_TIMEONLY:
      sOutput = timeToString(fullAlarmTime);
      break;
    case GT_DATEONLY:
      sOutput = dateToString(fullAlarmTime);
      break;
    case GT_DATETIME:
      sOutput = dateToString(fullAlarmTime);
      sOutput.concat(" ");
      sOutput.concat(timeToString(fullAlarmTime));
      break;
    default:
      break;
  }
  return sOutput;
}

String getTime(int pFormat, bool pUTC) {
  time_t rtcTime;
  String sOutput = "";
  if (pUTC) {
    rtcTime = now();
  } else {
    rtcTime = localTime.toLocal(now());
  }
  switch (pFormat) {
    case GT_TIMEONLY:
      sOutput = timeToString(rtcTime);
      break;
    case GT_DATEONLY:
      sOutput = dateToString(rtcTime);
      break;
    case GT_DATETIME:
      sOutput = dateToString(rtcTime);
      sOutput.concat(" ");
      sOutput.concat(timeToString(rtcTime));
      break;
    default:
      break;
  }
  return sOutput;
}

String getSunriseTime(int pFormat, bool pUTC) {
  time_t sunriseTime;
  String sOutput = "";
  sunriseTime = getSunTimes(SUNCALC_SUNRISE, localTime.toLocal(now()), ZENITH_DEFAULT);
  if (!pUTC) {
    sunriseTime = localTime.toLocal(sunriseTime);
  }
  switch (pFormat) {
    case GT_TIMEONLY:
      sOutput = timeToString(sunriseTime);
      break;
    case GT_DATEONLY:
      sOutput = dateToString(sunriseTime);
      break;
    case GT_DATETIME:
      sOutput = dateToString(sunriseTime);
      sOutput.concat(" ");
      sOutput.concat(timeToString(sunriseTime));
      break;
    default:
      break;
  }
  return sOutput;
}

String getSunsetTime(int pFormat, bool pUTC) {
  time_t sunsetTime;
  String sOutput = "";
  sunsetTime = getSunTimes(SUNCALC_SUNSET, localTime.toLocal(now()), ZENITH_CIVIL);
  if (!pUTC) {
    sunsetTime = localTime.toLocal(sunsetTime);
  }
  switch (pFormat) {
    case GT_TIMEONLY:
      sOutput = timeToString(sunsetTime);
      break;
    case GT_DATEONLY:
      sOutput = dateToString(sunsetTime);
      break;
    case GT_DATETIME:
      sOutput = dateToString(sunsetTime);
      sOutput.concat(" ");
      sOutput.concat(timeToString(sunsetTime));
      break;
    default:
      break;
  }
  return sOutput;
}

void setupWifi() {
  String eepromSSID = "";
  String eepromPWD = "";
  int i;
  wifiCredentials creds;
  EEPROM.get(4, creds);
  for (i = 0; creds.ssid[i] != 0; i++) {
    eepromSSID.concat(creds.ssid[i]);
  }
  for (i = 0; creds.pwd[i] != 0; i++) {
    eepromPWD.concat(creds.pwd[i]);
  }
  if (eepromSSID.length() == 0) {
    createAccessPoint();
  } else {
    connectToWifi(eepromSSID,eepromPWD);
  }
}

void connectToWifi(String ssid, String password) {
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.begin(string2char(ssid), string2char(password));
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void createAccessPoint() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("CasaDelPollo", "foxesgohome");
  //IPAddress accessIP = WiFi.softAPIP();
  /* Go to http://192.168.4.1 in a web browser
   * connected to this access point to see it.
   */
}

char* string2char(String command){
    if(command.length()!=0){
        char *p = const_cast<char*>(command.c_str());
        return p;
    }
}

void setupServer() {
  //Setup request handling
  server.on("/", handleRoot);
  server.on("/open", openDoor);
  server.on("/close", closeDoor);
  server.on("/override", alterDoorState);
  server.on("/stopopened", stopDoorOpened);
  server.on("/stopclosed", stopDoorClosed);
  server.on("/header.png", loadHeaderImage);
  server.on("/date.png", loadDateImage);
  server.on("/door.png", loadDoorImage);
  server.on("/sunrise.png", loadSunriseImage);
  server.on("/sunset.png", loadSunsetImage);
  server.on("/time.png", loadTimeImage);
  server.on("/pollo.css", loadCSS);
  server.on("/setwifi", setWifi);
  server.on("/clearwifi", clearWifiCredentials);
  server.on("/settime", setRTCTime);
  server.on("/setoverrun", setOverRun);
  server.on("/settings", handleSettings);
  server.on("/reset", handleReset);
  server.serveStatic("/",  SPIFFS, "/" ,"max-age=86400");
  server.begin();
}

void handleRoot() {
  String htmlString;
  htmlString="";
    htmlString.concat("<html>");
    htmlString.concat("<head>");
      htmlString.concat("<title>Casa del Pollo</title>");
      htmlString.concat("<META name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0' />");
      htmlString.concat("<META name='format-detection' content='telephone=no' />");
      htmlString.concat("<link rel='stylesheet' href='/pollo.css'>");
    htmlString.concat("</head>");
    htmlString.concat("<body>");
      htmlString.concat("<div class='content'>");
        htmlString.concat("<div class='header'>");
        htmlString.concat("</div>");
        if (server.arg("message") != "" ) {
          htmlString.concat("<div class='message'>");
          htmlString.concat(server.arg("message"));
          htmlString.concat("</div>");
        }
        htmlString.concat("<div class='info'>");
          htmlString.concat("<table>");
            htmlString.concat("<tr>");
              htmlString.concat("<td>");
                htmlString.concat("<div class='date'>");
              htmlString.concat("</td>");
              htmlString.concat("<td class='data'>");
                htmlString.concat(getTime(GT_DATEONLY, false));
              htmlString.concat("</td>");
            htmlString.concat("</tr>");
            htmlString.concat("<tr>");
              htmlString.concat("<td>");
                htmlString.concat("<div class='time'>");
              htmlString.concat("</td>");
              htmlString.concat("<td class='data'>");
                htmlString.concat(getTime(GT_TIMEONLY, false));
              htmlString.concat("</td>");
            htmlString.concat("</tr>");
            htmlString.concat("<tr>");
              htmlString.concat("<td>");
                htmlString.concat("<div class='sunrise'>");
              htmlString.concat("</td>");
              htmlString.concat("<td class='data'>");
                htmlString.concat(getSunriseTime(GT_TIMEONLY, false));
              htmlString.concat("</td>");
            htmlString.concat("</tr>");
            htmlString.concat("<tr>");
              htmlString.concat("<td>");
                htmlString.concat("<div class='sunset'>");
              htmlString.concat("</td>");
              htmlString.concat("<td class='data'>");
                htmlString.concat(getSunsetTime(GT_TIMEONLY, false));
              htmlString.concat("</td>");
            htmlString.concat("</tr>");
            htmlString.concat("<tr>");
              htmlString.concat("<td>");
                htmlString.concat("<div class='doorstate'>");
              htmlString.concat("</td>");
              htmlString.concat("<td class='data'>");
                htmlString.concat(getDoorState());
              htmlString.concat("</td>");
            htmlString.concat("</tr>");
          htmlString.concat("</table>");
        htmlString.concat("</div>");
        htmlString.concat("<div id='buttons'>");
          htmlString.concat("<table>");
            htmlString.concat("<tr>");
              htmlString.concat("<td>");
                htmlString.concat("<a href='/' class='button'>Refresh</a>");
              htmlString.concat("</td>");
              htmlString.concat("<td>");
                htmlString.concat("<a href='stopopened' class='buttonOpen'>Set Open</a>");
              htmlString.concat("</td>");
            htmlString.concat("</tr>");
            htmlString.concat("<tr>");
              htmlString.concat("<td>");
                htmlString.concat("<a href='override' class='buttonOverride'>Override</a>");
              htmlString.concat("</td>");
              htmlString.concat("<td>");
                htmlString.concat("<a href='stopclosed' class='buttonClosed'>Set Closed</a>");
              htmlString.concat("</td>");
            htmlString.concat("</tr>");
            htmlString.concat("<tr>");
              htmlString.concat("<td>");
              htmlString.concat("</td>");
              htmlString.concat("<td>");
                 htmlString.concat("<a href='settings' class='buttonSettings'>Settings</a>");
              htmlString.concat("</td>");
            htmlString.concat("</tr>");
          htmlString.concat("</table>");
        htmlString.concat("</div>"); 
      htmlString.concat("</div>");
    htmlString.concat("</body>");
  htmlString.concat("</html>");
  server.send(200, "text/html", htmlString);
}

void loadCSS(){
  File file = SPIFFS.open("/pollo.css", "r");
  server.streamFile(file, "text/css");
  file.close();
}

void loadHeaderImage(){
  File file = SPIFFS.open("/header.png", "r");
  server.streamFile(file, "image/png");
  file.close();
}

void loadDateImage() {
  File file = SPIFFS.open("/date.png", "r");
  server.streamFile(file, "image/png");
  file.close();
}

void loadDoorImage() {
  File file = SPIFFS.open("/door.png", "r");
  server.streamFile(file, "image/png");
  file.close();
}

void loadSunriseImage() {
  File file = SPIFFS.open("/sunrise.png", "r");
  server.streamFile(file, "image/png");
  file.close();
}

void loadSunsetImage() {
  File file = SPIFFS.open("/sunset.png", "r");
 server.streamFile(file, "image/png");
  file.close();
}

void loadTimeImage() {
  File file = SPIFFS.open("/time.png", "r");
  server.streamFile(file, "image/png");
  file.close();
}

void redirectHome(String message) {
  String homeURL = "/";
  if (message.length() > 0) {
    homeURL.concat("?message=");
    homeURL.concat(message);
  }
  server.sendHeader("Location", homeURL, true);
  server.send ( 302, "text/plain", "");
}

void setWifi () {
  String message;
  int i;
  String pSSID;
  String pPassword;
  char chSSID[20];
  char chPWD[20];
  wifiCredentials creds;

  if (server.arg("ssid")==""){
    return;
  } else {
    if (server.arg("password")=="") {
      return;
    } else {
      pSSID = server.arg("ssid");
      pPassword = server.arg("password");
    }
  }
  pSSID.toCharArray(chSSID,20);
  pPassword.toCharArray(chPWD,20);
  
  for ( i=0; i< 20; i++) {
    creds.ssid[i] = chSSID[i];
    creds.pwd[i] = chPWD[i];
  }
  EEPROM.put(4,creds);
  EEPROM.commit();
  message = "Wifi Credentials Set";
  message.concat("<br><b>SSID</b>: ");
  message.concat(pSSID);
  message.concat("<br><b>Password</b>: ");
  message.concat(pPassword);
  redirectHome(message);
}

void setRTCTime() {
  String message;
  TimeElements newTimeElements;
  time_t newTime;
  time_t newTimeUTC;
  //Get Time Elements from querystring
  String argYear = server.arg("year");
  String argMonth = server.arg("month");
  String argHour = server.arg("hour");
  String argMinute = server.arg("minute");
  String argSecond = server.arg("second");
  String argDay = server.arg("day");
  //build newTime from TimeElements
  newTimeElements.Year = argYear.toInt()-1970;
  newTimeElements.Month = argMonth.toInt();
  newTimeElements.Day = argDay.toInt();
  newTimeElements.Hour = argHour.toInt();
  newTimeElements.Minute = argMinute.toInt();
  newTimeElements.Second = argSecond.toInt(); 
  newTime = makeTime(newTimeElements);
  //Internal times use UTC. Convert to UTC
  newTimeUTC = localTime.toUTC(newTime);
  //RTC.adjust(DateTime(2017, 8, 20, 9, 13, 30));
  RTC.adjust(DateTime(year(newTimeUTC), month(newTimeUTC), day(newTimeUTC), hour(newTimeUTC), minute(newTimeUTC), second(newTimeUTC)));
  setSyncProvider(syncProvider);
  //Setup alarms to open/close door
  setSunAlarms();
  message = "RTC Time Set: ";
  message.concat(getTime(GT_DATETIME,false));
  redirectHome(message);
}

void clearWifiCredentials () {
  EEPROM.put(4,0);
  EEPROM.commit();
  redirectHome("Wifi Credentials Cleared");
}

int getOverRun() {
  int overRunTime;
  EEPROM.get(45,overRunTime);
  return overRunTime;
}

void setOverRun() {
  String message;
  int overRunTime;
  if (server.arg("overrun") == ""){
    return;
  } else {
    overRunTime = server.arg("overrun").toInt();
    EEPROM.put(45,overRunTime);
    EEPROM.commit();
  }
  overRun = overRunTime;
  message = "Overrun Set:";
  message.concat(overRunTime);
  message.concat(" milliseconds");
  redirectHome(message);
}

void handleReset () {
  ESP.restart();
}

void handleSettings(){
	int i;
  time_t rtcTime;
  String eepromSSID = "";
  String eepromPWD = "";
  String htmlString="";
  wifiCredentials creds;
  String MONTH_NAME[13] = { "Unknown", "January", "February", "March", "April", "May", "June","July","August","September","October","November","December"};
	
  rtcTime = localTime.toLocal(now());
  TimeElements rtcTimeElements;
  breakTime(rtcTime, rtcTimeElements);
  
  htmlString.concat("<html>");
    htmlString.concat("<head>");
      htmlString.concat("<title>Casa del Pollo</title>");
      htmlString.concat("<META name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0' />");
      htmlString.concat("<META name='format-detection' content='telephone=no' />");
      htmlString.concat("<link rel='stylesheet' href='/pollo.css'>");
    htmlString.concat("</head>");
    htmlString.concat("<body>");
    	htmlString.concat("<form action='settime' method='get'>");
    	htmlString.concat("<h4>Date/Time</h4>");
      
    	htmlString.concat("<select name='day'>");
    	  for(i=1;i<32;i++){
    	  	htmlString.concat("<option value='");
    	  	htmlString.concat(i);
          if (i == rtcTimeElements.Day) {
            htmlString.concat("' selected='selected'>");
          } else {
    	  	  htmlString.concat("'>");
          }
    	  	htmlString.concat(i);
    	  	htmlString.concat("</option>");
    	  }
    	htmlString.concat("</select>");
    	htmlString.concat("<select name='month'>");
    	  for(i=1;i<13;i++){
    	  	htmlString.concat("<option value='");
    	  	htmlString.concat(i);
    	  	if (i == rtcTimeElements.Month) {
            htmlString.concat("' selected='selected'>");
          } else {
            htmlString.concat("'>");
          }
    	  	htmlString.concat(MONTH_NAME[i]);
    	  	htmlString.concat("</option>");
    	  }
    	htmlString.concat("</select>");
    	htmlString.concat("<select name='year'>");
    	  for(i=2017;i<2040;i++){
    	  	htmlString.concat("<option value='");
    	  	htmlString.concat(i);
    	  	if (i == rtcTimeElements.Year) {
            htmlString.concat("' selected='selected'>");
          } else {
            htmlString.concat("'>");
          };
    	  	htmlString.concat(i);
    	  	htmlString.concat("</option>");
    	  }
    	htmlString.concat("</select>");
    	htmlString.concat("<select name='hour'>");
    	 for(i=0;i<24;i++){
    	  	htmlString.concat("<option value='");
    	  	htmlString.concat(i);
    	  	if (i == rtcTimeElements.Hour) {
            htmlString.concat("' selected='selected'>");
          } else {
            htmlString.concat("'>");
          }
    	  	htmlString.concat(padInteger(i));
    	  	htmlString.concat("</option>");
    	  }
    	htmlString.concat("</select>");
    	htmlString.concat("<select name='minute'>");
    	 for(i=0;i<60;i++){
    	  	htmlString.concat("<option value='");
    	  	htmlString.concat(i);
    	  	if (i == rtcTimeElements.Minute) {
            htmlString.concat("' selected='selected'>");
          } else {
            htmlString.concat("'>");
          }
    	  	htmlString.concat(padInteger(i));
    	  	htmlString.concat("</option>");
    	  }
    	htmlString.concat("</select>");
    	htmlString.concat("<input type='hidden' name='second' value=0>");
    	htmlString.concat("<input type='submit' value='Set Date'>");
    	htmlString.concat("</form>");
    
    	htmlString.concat("<form action='setoverrun' method='get'>");
    	htmlString.concat("<h4>Overrun time</h4>");
    	htmlString.concat("<input type='text' name='overrun' ");
      htmlString.concat("value='");
    	htmlString.concat(overRun);
    	htmlString.concat("'>");
    	htmlString.concat("<input type='submit' value='Set Overrun'>");
    	htmlString.concat("</form>");
      EEPROM.get(4, creds);
      for (i = 0; creds.ssid[i] != 0; i++) {
        eepromSSID.concat(creds.ssid[i]);
      }
      for (i = 0; creds.pwd[i] != 0; i++) {
        eepromPWD.concat(creds.pwd[i]);
      }
    	htmlString.concat("<form action='setwifi' method='get'>");
    	htmlString.concat("<h4>Wifi Credentials</h4>");
    	htmlString.concat("<table>");
    	htmlString.concat("<tr><td>SSID:</td><td><input type='text' name='ssid' value='");
    	htmlString.concat(eepromSSID);
    	htmlString.concat("'></td></tr>");
    	htmlString.concat("<tr><td>Password:</td><td><input type='password' name='password' value='");
      htmlString.concat(eepromPWD);
      htmlString.concat("'></td></tr>");
    	htmlString.concat("</table>");
    	htmlString.concat("<input type='submit' value='Set Credentials'>");
    	htmlString.concat("</form>");

      htmlString.concat("<form action='reset' method='get'>");
      htmlString.concat("<h4>Restart</h4>");
      htmlString.concat("<input type='submit' value='Restart'>");
      htmlString.concat("</form>");
      
     htmlString.concat("</body>");
  htmlString.concat("</html>");
  server.send(200, "text/html", htmlString);
}
