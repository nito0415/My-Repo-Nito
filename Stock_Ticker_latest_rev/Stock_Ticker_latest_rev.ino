#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <WiFiClientSecureBearSSL.h>
#include <time.h>

TFT_eSPI tft = TFT_eSPI();

#define TFT_CS   PIN_D8  // Chip select control pin D8
#define TFT_DC   PIN_D3  // Data Command control pin
#define TFT_RST  PIN_D4  // Reset pin (could connect to NodeMCU RST, see next line)

#define TFT_GREY 0x5AEB
const char *ssid         = "CP-IoT-Secure"; //Your Wifi SSID
const char *password     = "sHyWEM6fiN"; //Your W ifi Password
String payload = "";

void connect() {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(70, 20, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("Connecting to Wifi ");
  WiFi.begin ( ssid, password );
  while ( WiFi.status() != WL_CONNECTED ) {
    tft.print(".");
    delay (500);
  }
  tft.print(" Connnected!");
  delay(700);


}

      // Function that gets current epoch time
// Function that gets current epoch time
unsigned long getCurrentTime() {
    time_t now;
    struct tm timeinfo;
    configTime(-8 * 3600, 0, "pool.ntp.org");
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return 0;
    }
    time(&now);
    return now;
}


void reset_sc() {
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(2);
}

void read_graph(int x_i, int y_i, String stock_name, String resolution, unsigned long start_time, unsigned long end_time) {
  
  float closePrice;
  float currentPrice;
  float highPrice;
  float lowPrice;
  float openPrice;
  String statusResponse;
  int timeStampCandle;
  int volumeTraded;
  float differenceInPrice;
  float previousClosePrice;
  String httpRequestAddress;
  
  BearSSL::WiFiClientSecure client;
  HTTPClient http;
  String httpGraphRequestAddress = "https://finnhub.io/api/v1/stock/candle?symbol=" + stock_name + "&resolution=" + String(resolution) + "&from=" + String(start_time) + "&to=" + String(end_time) + "&token=cffv541r01qgfjm42nn0cffv541r01qgfjm42nng";
  client.setInsecure();

  Serial.print("Making HTTP request to: ");
  Serial.println(httpGraphRequestAddress);

  http.begin(client, httpGraphRequestAddress);
  int httpCode = http.GET();
  Serial.println(httpCode);
  
  if (httpCode == 200) {
    String response = http.getString();
    Serial.println(response);
    const size_t capacity = JSON_OBJECT_SIZE(6) + 120;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, response);

    JsonArray results_ohlc = doc["o"];
    JsonArray results_high = doc["h"];
    JsonArray results_low = doc["l"];
    JsonArray results_close = doc["c"];
    JsonArray results_previous_close = doc["pc"];
    JsonArray results_timestamp = doc["t"];

    int length = results_ohlc.size();

    for (int i = 0; i < length; i++) {
      openPrice = results_ohlc[i];
      highPrice = results_high[i];
      lowPrice = results_low[i];
      closePrice = results_close[i];
      timeStampCandle = results_timestamp[i];
      int x = x_i + i * 2;
      int y = y_i - highPrice;
      int w = 2;
      int h = highPrice - lowPrice;
      if (y < 0) {
        h += y;
        y = 0;
      }
      if (y + h > tft.height()) {
        h = tft.height() - y;
      }
      if (h > 0 && w > 0) {
        tft.fillRect(x, y, w, h, TFT_WHITE);
      }

      currentPrice = (openPrice + highPrice + lowPrice + closePrice) / 4;


      tft.fillRect(x, y_i - closePrice, w, 1, TFT_RED);
      tft.fillRect(x, y_i - openPrice, w, 1, TFT_GREEN);

      if (i > 0) {
        tft.drawLine(x_i + (i-1)*2, y_i - previousClosePrice, x_i + i*2, y_i - currentPrice, TFT_GREEN);
      }
      previousClosePrice = currentPrice;
    }
  } else {
    Serial.print("HTTP request failed with error code: ");
    Serial.println(httpCode);
  }
  
  http.end(); //Close connection
}



void read_price(int x_i, int y_i, String stock_name) {
  
  float previousClosePrice;
  float currentPrice;
  float differenceInPrice;
  String  httpRequestAddress;
  BearSSL::WiFiClientSecure client;
  HTTPClient http; //Declare an object of class HTTPClient
  httpRequestAddress = "https://finnhub.io/api/v1/quote?symbol=" + stock_name + "&token=cffv541r01qgfjm42nn0cffv541r01qgfjm42nng";
  client.setInsecure();

  http.begin(client, httpRequestAddress);  // Specify request destination
  DynamicJsonDocument doc(1024);
  int httpCode = http.GET();  // Send the request
  if (httpCode > 0) {          // Check the returning code

    String payload = http.getString();  // Get the request response payload
    Serial.println(payload);            // Print the response payload
    deserializeJson(doc, payload);
    JsonObject obj = doc.as<JsonObject>();
    previousClosePrice = obj[String("pc")];
    currentPrice = obj[String("c")];

    float diff = currentPrice - previousClosePrice;
    differenceInPrice = ((currentPrice - previousClosePrice) / previousClosePrice) * 100.0;
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(25 + x_i, 10 + y_i, 4);
    tft.print(stock_name);


    if (differenceInPrice < 0.0) {
      tft.setTextColor(TFT_RED);
      tft.setCursor(25 + x_i, 45 + y_i, 4);
      tft.drawFloat(currentPrice, 2, 25 + x_i, 45 + y_i, 4);
      tft.setCursor(5 + x_i, 82 + y_i, 3);
      tft.fillTriangle(5 + x_i, 95 + y_i, 20 + x_i, 95 + y_i, 12 + x_i, 110 + y_i, TFT_RED);
      tft.drawFloat(differenceInPrice , 2, 25 + x_i, 82 + y_i, 2);
      tft.setCursor(65 + x_i, 82 + y_i, 2);
      tft.print("% (");
      tft.drawFloat(diff , 2, 90 + x_i, 82 + y_i, 2);
      tft.setCursor(135 + x_i, 82 + y_i, 2);
      tft.print(")");
    }
    else {
      tft.setTextColor(TFT_GREEN);
      tft.setCursor(25 + x_i, 45 + y_i, 4);
      tft.drawFloat(currentPrice, 2, 25 + x_i, 45 + y_i, 4);
      tft.setCursor(5 + x_i, 82 + y_i, 2);
      tft.fillTriangle(5 + x_i, 105 + y_i, 20 + x_i, 105 + y_i, 12 + x_i, 90 + y_i, TFT_GREEN);
      tft.drawFloat(differenceInPrice , 2, 25 + x_i, 82 + y_i, 2);
      tft.setCursor(65 + x_i, 82 + y_i, 2);
      tft.print("% (");
      tft.drawFloat(diff , 2, 90 + x_i, 82 + y_i, 2);
      tft.setCursor(135 + x_i, 82 + y_i, 2);
      tft.print(")");
    }

  }
  http.end(); //Close connection

}

void main_page() {
  tft.setCursor(20, 75, 4);
  tft.fillRect(0, 0, 320, 240, TFT_WHITE);
  tft.setTextColor(TFT_BLACK);
  tft.println("24x7 Stock Prices Tracker");
}

void stock_page(String s1, String s2) {
  unsigned long start_time = getCurrentTime() - (7 * 24 * 60 * 60); // 5 days ago
  unsigned long end_time = getCurrentTime();
  read_price(0, 0, s1);
  read_graph(160, 0, s1,"D", start_time, end_time);
  read_price(0, 120, s2);
  read_graph(160, 120, s2, "D", start_time, end_time);
  tft.fillTriangle(305, 100, 305, 140, 319, 120, TFT_WHITE);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  tft.init();         // Initialize TFT display with defined pins 
  tft.setSwapBytes(true);
  tft.setRotation(3); // horizontal display
  main_page();
  connect();          // Connect to WiFi network

}

void loop() {
  reset_sc();
  stock_page("AAPL", "AMZN");
  delay(6000);
  reset_sc();
  stock_page("CFSB", "WFC");
  delay(6000);
  reset_sc();
  stock_page("EBAY", "GME");
  delay(6000);
  reset_sc();
  stock_page("TSLA", "MSFT");
  delay(6000);
  reset_sc();
  stock_page("ALLY", "FRC");
  delay(6000);
  reset_sc();
  stock_page("AMC", "BBBY");
  delay(6000);
  reset_sc();
  stock_page("PFE", "TSM");
  delay(6000);
  reset_sc();
  stock_page("GM", "SONY");
  delay(6000);      
}
