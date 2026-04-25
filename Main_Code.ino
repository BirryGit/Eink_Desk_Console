#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include <time.h>
#include <SPI.h>
#include <ctype.h>
#include <GxEPD2_BW.h>
#include <Fonts/FreeSansBold18pt7b.h>

//Wifi Credentials
const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

//Time Server Details
const char* ntpServer = "pool.ntp.org";
const char* tzInfo = "CST6CDT,M3.2.0/2,M11.1.0/2";

//ISS API Information
const char* issUrl = "https://api.wheretheiss.at/v1/satellites/25544";

const char* geoapifyApiKey = "YOUR_GEOAPIFY_KEY";

//Weather API Information
const char* weatherApiKey = "YOUR_OPENWEATHER_KEY";
const char* lat = "YOUR_LATITUDE";
const char* lon = "YOUR_LONGITUDE";

//Home assistant API information
const char* homeAssistantBaseUrl = "YOUR_HOME_ASSISTANT_URL";
const char* homeAssistantToken = "YOUR_HOME_ASSISTANT_TOKEN";

const char* printerPowerEntityId = "sensor.3d_printer_current_consumption";
const char* powerStripPowerEntityId = "sensor.tp_link_power_strip_current_consumption";

//Google Sheets API Link

const char* googleAPI = "YOUR_GOOGLE_SCRIPT_URL?token=YOUR_KEY_IN_APP_SCRIPT";

//Default Weather Values
float outsideTempF = 0.0f;
bool outsideTempValid = false;

char weatherMain[24] = "";
char weatherDescription[48] = "";
bool weatherConditionValid = false;

// Home Assistant default cached values
float printerPowerWatts = 0.0f;
bool printerPowerValid = false;

float powerStripPowerWatts = 0.0f;
bool powerStripPowerValid = false;

// ISS Data structure
struct IssData {
  bool valid;
  float latitude;
  float longitude;
  float altitudeKm;
  float velocityKmh;
  long timestamp;
  char overCountry[40];
};

//Schedule Data Structure
struct ScheduleData {
  char name[80];
  char manualStatus[32];
  char autoStatus[32];
  int daysSinceStart;
  int daysTillTaskEnd;
  bool valid;
};

ScheduleData scheduleData = {
  "Loading...",
  "--",
  "--",
  0,
  0,
  false
};

IssData issData = {false, 0.0f, 0.0f, 0.0f, 0.0f, 0, "Unknown"};

unsigned long lastClockCheck = 0;
const unsigned long clockCheckInterval = 1000; // once per second

unsigned long lastWeatherUpdate = 0;
const unsigned long weatherUpdateInterval = 10UL * 60UL * 1000UL; // every 10 minutes

unsigned long lastIssUpdate = 0;
const unsigned long issUpdateInterval = 60UL * 1000UL; // every minute

unsigned long lastHomeAssistantUpdate = 0;
const unsigned long homeAssistantUpdateInterval = 60UL * 1000UL; // every minute

const unsigned long scheduleUpdateInterval = 15UL * 60UL * 1000UL; // every 15 minutes
unsigned long lastScheduleUpdate = 0;

//Data Setup To make sure things actually happen
int lastDisplayedMinute = -1;
bool dashboardDirty = true;

/* Wiring:

  VCC -> 3V3
  GND -> GND
  DIN -> D9
  CLK -> D7
  CS -> D3
  DC -> D1
  RST -> D0
  BUSY -> D2
  PWR -> D5
*/
static const int PIN_EPD_BUSY = D2;
static const int PIN_EPD_RST  = D0;
static const int PIN_EPD_DC   = D1;
static const int PIN_EPD_CS   = D3;
static const int PIN_EPD_SCK  = D7;
static const int PIN_EPD_MOSI = D9;
static const int PIN_EPD_PWR  = D5;

// For newer GxEPD2 versions, 7.5 inch BW V2 is GxEPD2_750_GDEY075T7.
GxEPD2_BW<GxEPD2_750_GDEY075T7, GxEPD2_750_GDEY075T7::HEIGHT> display(
  GxEPD2_750_GDEY075T7(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY)
);

void clearScreen(){
  //Make the whole screen white
  display.fillScreen(GxEPD_WHITE);
}

void drawWifiConnecting(){
  //Displays static black text "Wifi Connecting..."
  display.setTextColor(GxEPD_BLACK);
  display.setTextSize(5);
  display.setTextWrap(false);
  display.setFont(&FreeSansBold18pt7b);
  display.setCursor(231, 222);
  display.print("Wifi");

  display.setTextSize(3);
  display.setCursor(68, 388);
  display.print("Connecting...");
}

void drawTimeDate() {
  //Get the time info
  struct tm timeinfo;

  //Padding for correct alignment
  const int panelRight = 803;
  const int rightPadding = 20;

  const int timeRightEdge = panelRight - rightPadding;
  const int dateRightEdge = panelRight - rightPadding;

  const int timeBaselineY = 64;
  const int dateBaselineY = 110;

  //Set Text Info
  display.setFont(&FreeSansBold18pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setTextWrap(false);

  int16_t x1, y1;
  uint16_t w, h;

  //Is Everything Working?
  if (!getLocalTime(&timeinfo)) {
    display.setTextSize(2);
    display.getTextBounds("Time Unavailable", 0, 0, &x1, &y1, &w, &h);
    display.setCursor(timeRightEdge - w, timeBaselineY);
    display.print("Time Unavailable");

    display.setTextSize(1);
    display.getTextBounds("Date Unavailable", 0, 0, &x1, &y1, &w, &h);
    display.setCursor(dateRightEdge - w, dateBaselineY);
    display.print("Date Unavailable");
    return;
  }

  // Overkill memory managment
  char timeBuffer[16];
  char dateBuffer[40];

  //Format time
  int hour12 = timeinfo.tm_hour % 12;
  if (hour12 == 0) hour12 = 12;

  snprintf(
    timeBuffer,
    sizeof(timeBuffer),
    "%d:%02d %s",
    hour12,
    timeinfo.tm_min,
    (timeinfo.tm_hour >= 12) ? "PM" : "AM"
  );

  strftime(dateBuffer, sizeof(dateBuffer), "%A %d, %Y", &timeinfo);

  //Actually print data
  display.setTextSize(2);
  display.getTextBounds(timeBuffer, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(timeRightEdge - w, timeBaselineY);
  display.print(timeBuffer);

  display.setTextSize(1);
  display.getTextBounds(dateBuffer, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(dateRightEdge - w, dateBaselineY);
  display.print(dateBuffer);
}



void drawISSTracker(const IssData& data) {
  //Epic outline. (Graphics Design is my passion)
  display.drawRect(0, 0, 410, 207, GxEPD_BLACK);

  //Font Info
  display.setFont(&FreeSansBold18pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setTextWrap(false);

  //Label
  display.setTextSize(2);
  display.setCursor(4, 57);
  display.print("ISS Tracker");

  //Make Text Smaller for Values
  display.setTextSize(1);

  // Fixed layout anchors for alignment
  const int overLabelX = 10;
  const int overLabelY = 92;

  const int countryRightEdge = 395;
  const int countryBaselineY = 127;

  const int coordRightEdge = 395;
  const int coordBaselineY = 162;

  const int speedLeftX = 20;
  const int speedBaselineY = 197;

  const int altitudeRightEdge = 395;
  const int altitudeBaselineY = 197;

  int16_t x1, y1;
  uint16_t w, h;

  display.setCursor(overLabelX, overLabelY);
  display.print("Over:");

  //Memory Managment
  char countryBuffer[40];
  char coordBuffer[40];
  char altitudeBuffer[20];
  char speedBuffer[24];

  char latHemisphere = (data.latitude >= 0) ? 'N' : 'S';
  char lonHemisphere = (data.longitude >= 0) ? 'E' : 'W';

  //Make Sure Everything is Good and Actually Exists
  if (!data.valid) {
    strncpy(countryBuffer, "Unknown", sizeof(countryBuffer) - 1);
    countryBuffer[sizeof(countryBuffer) - 1] = '\0';

    strncpy(coordBuffer, "--.--, --.--", sizeof(coordBuffer) - 1);
    coordBuffer[sizeof(coordBuffer) - 1] = '\0';

    strncpy(altitudeBuffer, "-- km", sizeof(altitudeBuffer) - 1);
    altitudeBuffer[sizeof(altitudeBuffer) - 1] = '\0';

    strncpy(speedBuffer, "-- km/h", sizeof(speedBuffer) - 1);
    speedBuffer[sizeof(speedBuffer) - 1] = '\0';
  } else {
    snprintf(countryBuffer, sizeof(countryBuffer), "%s", data.overCountry);
    snprintf( coordBuffer, sizeof(coordBuffer), "%.2f %c, %.2f %c", fabs(data.latitude), latHemisphere, fabs(data.longitude), lonHemisphere);
    snprintf(altitudeBuffer, sizeof(altitudeBuffer), "%d km", (int)round(data.altitudeKm));
    snprintf(speedBuffer, sizeof(speedBuffer), "%d km/h", (int)round(data.velocityKmh));
  }
  
  //Alignment uses math and stuff to get into the right spot
  // Country: right-aligned
  display.getTextBounds(countryBuffer, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(countryRightEdge - w, countryBaselineY);
  display.print(countryBuffer);

  // Coordinates: right-aligned
  display.getTextBounds(coordBuffer, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(coordRightEdge - w, coordBaselineY);
  display.print(coordBuffer);

  // Speed: left-aligned on left side
  display.setCursor(speedLeftX, speedBaselineY);
  display.print(speedBuffer);

  // Altitude: right-aligned on right side
  display.getTextBounds(altitudeBuffer, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(altitudeRightEdge - w, altitudeBaselineY);
  display.print(altitudeBuffer);
}



void drawWeather(float outsideTempFValue, bool outsideTempIsValid, const char* conditionText, bool conditionIsValid){
  //Alignment Stuff
  const int panelRight = 803;
  const int rightPadding = 20;
  const int weatherRightEdge = panelRight - rightPadding;

  const int labelBaselineY = 249;
  const int dataBaselineY = 290;
  
  //Font Stuff
  display.setTextSize(1);
  display.setFont(&FreeSansBold18pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setTextWrap(false);

  int16_t x1, y1;
  uint16_t w, h;

  //Memory Managment
  char weatherLine[80];

  const char* safeCondition = conditionIsValid ? conditionText : "--";

  //Is Everything OK?
  if (outsideTempIsValid) {
    snprintf(
      weatherLine,
      sizeof(weatherLine),
      "%s %d F",
      safeCondition,
      (int)round(outsideTempFValue)
    );
  } else {
    snprintf(
      weatherLine,
      sizeof(weatherLine),
      "%s -- F",
      safeCondition
    );
  }

  //Print Text
  display.getTextBounds("Weather:", 0, 0, &x1, &y1, &w, &h);
  display.setCursor(weatherRightEdge - w, labelBaselineY);
  display.print("Weather:");

  display.getTextBounds(weatherLine, 0, 0, &x1, &y1, &w, &h);
  display.setCursor(weatherRightEdge - w, dataBaselineY);
  display.print(weatherLine);
}

void drawScheduleDisplay() {
  //Black Background
  display.fillRect(0, 328, 803, 152, GxEPD_BLACK);

  //Font Info
  display.setTextSize(1);
  display.setFont(&FreeSansBold18pt7b);
  display.setTextColor(GxEPD_WHITE);
  display.setTextWrap(false);

  //Current Task Label
  display.setCursor(12, 370);
  display.print("Current Task:");

  //Current Task
  display.setCursor(256, 370);
  if (scheduleData.valid) {
    display.print(scheduleData.name);
  } else {
    display.print("Schedule Unavailable");
  }

  //That is the same for the rest :D
  display.setCursor(12, 415);
  display.print("Days Elapsed:");

  display.setCursor(268, 415);
  if (scheduleData.valid) {
    display.print(scheduleData.daysSinceStart);
  } else {
    display.print("--");
  }

  display.setCursor(12, 462);
  display.print("Days Remaining:");

  display.setCursor(311, 462);
  if (scheduleData.valid) {
    display.print(scheduleData.daysTillTaskEnd);
  } else {
    display.print("--");
  }

  display.setCursor(470, 462);
  display.print("Status:");

  display.setCursor(595, 462);
  if (scheduleData.valid) {
    display.print(scheduleData.manualStatus);
  } else {
    display.print("--");
  }
}

void drawHomeAssistantData(float printerWatts, bool printerValid, float stripWatts, bool stripValid) {
  //Font Info
  display.setTextSize(1);
  display.setFont(&FreeSansBold18pt7b);
  display.setTextColor(GxEPD_BLACK);

  //Label
  display.setCursor(12, 244);
  display.print("Home Assistant:");

  //Printer Label and Value
  display.setCursor(14, 280);
  display.print("Printer");
  display.setCursor(139, 280);
  if (printerValid) {
    display.print((int)round(printerWatts));
    display.print(" W");
  } else {
    display.print("-- W");
  }

  //Power Strip Label and Value
  display.setCursor(14, 315);
  display.print("Strip");
  display.setCursor(116, 315);
  if (stripValid) {
    display.print((int)round(stripWatts));
    display.print(" W");
  } else {
    display.print("-- W");
  }
}

void drawDashboard() {
  //Draw Everything with the correct data
  display.firstPage();
  do {
    clearScreen();
    drawTimeDate();
    drawISSTracker(issData);
    drawWeather(outsideTempF, outsideTempValid, weatherDescription, weatherConditionValid);
    drawScheduleDisplay();
    drawHomeAssistantData(printerPowerWatts, printerPowerValid, powerStripPowerWatts, powerStripPowerValid);
  }
  while (display.nextPage());
}

//---API call updates---

void checkClockForDisplayUpdate() {
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    return;
  }

  if (timeinfo.tm_min != lastDisplayedMinute) {
    lastDisplayedMinute = timeinfo.tm_min;
    dashboardDirty = true;
  }
}

// Home assistant call function
bool fetchHomeAssistantWatts(const char* entityId, float& wattsOut, bool& validOut) {
  //Ensure Wifi not dead
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }
  
  // Get data from HA
  String url = String(homeAssistantBaseUrl) + "/api/states/" + entityId;

  HTTPClient http;
  http.begin(url);
  http.addHeader("Authorization", String("Bearer ") + homeAssistantToken);
  http.addHeader("Content-Type", "application/json");

  int httpCode = http.GET();
  if (httpCode != 200) {
    http.end();
    validOut = false;
    return false;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    validOut = false;
    return false;
  }

  //Grab Data and make sure its good
  const char* stateText = doc["state"] | "";
  if (strlen(stateText) == 0 ||
      strcmp(stateText, "unknown") == 0 ||
      strcmp(stateText, "unavailable") == 0 ||
      strcmp(stateText, "none") == 0) {
    validOut = false;
    return false;
  }

  float newWatts = atof(stateText);
  bool changed = (!validOut) || ((int)round(newWatts) != (int)round(wattsOut));

  wattsOut = newWatts;
  validOut = true;

  return changed;
}

//Calls Home Assistant API functions with params
bool updateHomeAssistantData() {
  bool printerChanged = fetchHomeAssistantWatts(
    printerPowerEntityId,
    printerPowerWatts,
    printerPowerValid
  );

  bool stripChanged = fetchHomeAssistantWatts(
    powerStripPowerEntityId,
    powerStripPowerWatts,
    powerStripPowerValid
  );

  return printerChanged || stripChanged;
}

bool updateWeatherData() {
  //Ensure Wifi connected
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  //Make API URL and Get the Data
  String url = String("https://api.openweathermap.org/data/2.5/weather?lat=") + lat + "&lon=" + lon + "&appid=" + weatherApiKey + "&units=imperial";

  HTTPClient http;
  http.begin(url);

  int httpCode = http.GET();
  if (httpCode != 200) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    return false;
  }

  //Extract Data and check if it is ok and changed
  float newTempF = doc["main"]["temp"] | NAN;
  const char* newMain = doc["weather"][0]["main"] | "";
  const char* newDescription = doc["weather"][0]["description"] | "";

  if (isnan(newTempF) || strlen(newMain) == 0) {
    return false;
  }

  bool tempChanged =
    (!outsideTempValid) ||
    ((int)round(newTempF) != (int)round(outsideTempF));

  bool conditionChanged =
    (!weatherConditionValid) ||
    (strcmp(weatherMain, newMain) != 0) ||
    (strcmp(weatherDescription, newDescription) != 0);

  //Set New Values
  outsideTempF = newTempF;
  outsideTempValid = true;

  strncpy(weatherMain, newMain, sizeof(weatherMain) - 1);
  weatherMain[sizeof(weatherMain) - 1] = '\0';

  strncpy(weatherDescription, newDescription, sizeof(weatherDescription) - 1);
  weatherDescription[sizeof(weatherDescription) - 1] = '\0';
  if (weatherDescription[0] != '\0') {
    weatherDescription[0] = toupper(weatherDescription[0]);
  }

  weatherConditionValid = true;

  return tempChanged || conditionChanged;
}

bool updateIssCountry(float latValue, float lonValue, char* countryOut, size_t countryOutSize) {
  //Ensure Connection
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  //Get the Data
  String url = String("https://api.geoapify.com/v1/geocode/reverse?lat=") + String(latValue, 6) + "&lon=" + String(lonValue, 6) + "&type=country&format=json&limit=1&lang=en&apiKey=" + geoapifyApiKey;

  HTTPClient http;
  http.begin(url);

  int httpCode = http.GET();
  if (httpCode != 200) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    return false;
  }

  //Extract information
  if (!doc["results"].is<JsonArray>() || doc["results"].size() == 0) {
    strncpy(countryOut, "Ocean", countryOutSize - 1);
    countryOut[countryOutSize - 1] = '\0';
    return true;
  }

  const char* fullCountryName = doc["results"][0]["country"] | "";
  if (strlen(fullCountryName) == 0) {
    strncpy(countryOut, "Unknown", countryOutSize - 1);
    countryOut[countryOutSize - 1] = '\0';
    return true;
  }

  strncpy(countryOut, fullCountryName, countryOutSize - 1);
  countryOut[countryOutSize - 1] = '\0';
  return true;
}

bool updateIssData() {
  //Ensure Wifi Connection
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  //Call Endpoint
  HTTPClient http;
  http.begin(issUrl);

  int httpCode = http.GET();
  if (httpCode != 200) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    return false;
  }

  //Extract Data
  float newLatitude = doc["latitude"] | NAN;
  float newLongitude = doc["longitude"] | NAN;
  float newAltitudeKm = doc["altitude"] | NAN;
  float newVelocityKmh = doc["velocity"] | NAN;
  long newTimestamp = doc["timestamp"] | 0;

  //Checks that values are numbers
  if (isnan(newLatitude) || isnan(newLongitude) || isnan(newAltitudeKm) || isnan(newVelocityKmh)) {
    return false;
  }

  //Update The country the ISS is over
  char newCountry[40];
  strncpy(newCountry, issData.overCountry, sizeof(newCountry) - 1);
  newCountry[sizeof(newCountry) - 1] = '\0';

  updateIssCountry(newLatitude, newLongitude, newCountry, sizeof(newCountry));

  //Checks if values have changed
  bool changed =
    (!issData.valid) ||
    ((int)round(newLatitude * 100) != (int)round(issData.latitude * 100)) ||
    ((int)round(newLongitude * 100) != (int)round(issData.longitude * 100)) ||
    ((int)round(newAltitudeKm) != (int)round(issData.altitudeKm)) ||
    ((int)round(newVelocityKmh) != (int)round(issData.velocityKmh)) ||
    (strcmp(issData.overCountry, newCountry) != 0);  
    //Updates Data into structure
    issData.latitude = newLatitude;
    issData.longitude = newLongitude;
    issData.altitudeKm = newAltitudeKm;
    issData.velocityKmh = newVelocityKmh;
    issData.timestamp = newTimestamp;
    issData.valid = true;

    strncpy(issData.overCountry, newCountry, sizeof(issData.overCountry) - 1);
    issData.overCountry[sizeof(issData.overCountry) - 1] = '\0';

    return changed;
}

bool updateScheduleData() {
  //Ensure Wifi Connection
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  }

  //Let Google Redirect Because it apperently is like that sometimes
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(googleAPI);

  //Get the Data
  int httpCode = http.GET();
  if (httpCode != 200) {
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, payload);
  if (error) {
    return false;
  }
  //Do the Data stuff
  const char* newName = doc["name"] | "";
  const char* newStatus = doc["status"] | "";
  int newDaysElapsed = doc["daysSinceStart"] | -1;
  int newDaysRemaining = doc["daysTillTaskEnd"] | -1;

  if (strlen(newName) == 0 || strlen(newStatus) == 0 ||
      newDaysElapsed < 0 || newDaysRemaining < 0) {
    return false;
  }

  bool changed =
    (!scheduleData.valid) ||
    (strcmp(scheduleData.name, newName) != 0) ||
    (strcmp(scheduleData.manualStatus, newStatus) != 0) ||
    (scheduleData.daysSinceStart != newDaysElapsed) ||
    (scheduleData.daysTillTaskEnd != newDaysRemaining);

  strncpy(scheduleData.name, newName, sizeof(scheduleData.name) - 1);
  scheduleData.name[sizeof(scheduleData.name) - 1] = '\0';

  strncpy(scheduleData.manualStatus, newStatus, sizeof(scheduleData.manualStatus) - 1);
  scheduleData.manualStatus[sizeof(scheduleData.manualStatus) - 1] = '\0';

  scheduleData.daysSinceStart = newDaysElapsed;
  scheduleData.daysTillTaskEnd = newDaysRemaining;
  scheduleData.valid = true;

  return changed;
}

void setup()
{
  //Begin Wifi and Serial
  WiFi.begin(ssid, password);
  Serial.begin(115200);
  delay(100);

  //Set up Screen things
  pinMode(PIN_EPD_PWR, OUTPUT);
  digitalWrite(PIN_EPD_PWR, HIGH);
  delay(10);

  SPI.begin(PIN_EPD_SCK, -1, PIN_EPD_MOSI, PIN_EPD_CS);
  display.init(115200, true, 2, false);

  display.setRotation(0);
  display.setFont(&FreeSansBold18pt7b);

  //Clear screen and Show Wifi Connecting screen first
  display.firstPage();
  do
  {
    clearScreen();
    drawWifiConnecting();
  }
  while (display.nextPage());

  // wait for Wifi to connect
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  //Configure and get time
  configTzTime(tzInfo, ntpServer);
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    delay(500);
    Serial.println("Waiting for time...");
  }

  //Make the APIs call quickly
  lastWeatherUpdate = millis() - weatherUpdateInterval;
  lastIssUpdate = millis() - issUpdateInterval;
  lastHomeAssistantUpdate = millis() - homeAssistantUpdateInterval;
  lastScheduleUpdate = millis() - scheduleUpdateInterval;

  lastDisplayedMinute = timeinfo.tm_min;
  dashboardDirty = true;
  drawDashboard();
}



void loop() {
  unsigned long now = millis();

  //Make Sure Clock Gets Updated
  if (now - lastClockCheck >= clockCheckInterval) {
    lastClockCheck = now;
    checkClockForDisplayUpdate();
  }

  //Weather Data Update
  if (now - lastWeatherUpdate >= weatherUpdateInterval) {
    lastWeatherUpdate = now;
    if (updateWeatherData()) {
      dashboardDirty = true;
    }
  }

  //ISS Data Update
  if (now - lastIssUpdate >= issUpdateInterval) {
    lastIssUpdate = now;
    if (updateIssData()) {
      dashboardDirty = true;
    }
  }

  //Home Assistant Data Update
  if (now - lastHomeAssistantUpdate >= homeAssistantUpdateInterval) {
    lastHomeAssistantUpdate = now;
    if (updateHomeAssistantData()) {
      dashboardDirty = true;
    }
  }

  //Google Sheets Schedule Update
  if (now - lastScheduleUpdate >= scheduleUpdateInterval) {
    lastScheduleUpdate = now;
    if (updateScheduleData()) {
      dashboardDirty = true;
    }
  }
  
  // Checks to see if any data has changed, and therefore needs to be changed.
  if (dashboardDirty) {
    dashboardDirty = false;
    drawDashboard();
  }

  //Delay for 50 milliseconds, so the microcontroller doesn't explode trying to do everything 200 times per second
  delay(50);
}
