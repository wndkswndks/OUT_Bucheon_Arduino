/*
  Wemos D1 mini, photocell, water level sensor, 2 relays
  light sensor to pin GPIO A0 (A0)
  water level sensor to pin GPIO 14 (D5)
  pump relay to pin GPIO 12 (D6)
  light relay to pin GPIO 13 (D7)
  Server IP Address: "192.168.0.1"
  last revised 2021.5.29 2021.3.15 2021.2.24  2020.11.2////
*/
#include <FS.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>////////---
#include <time.h>
String wifissid = "";
String wifipassword = "";
char wifissid2[20];
char wifipassword2[20];
int startSec;
int currentSec;/////wndks
int hour = 12;
int minute = 00;
int onHour = 7;
int offHour = 21;
int adjTimeMinute = hour * 60 + minute - int(millis() / 1000 / 60);
int virTimeMinute = int(millis() / 1000 / 60) + adjTimeMinute;
int remainingSec = 0;
int intervalTime = 120;  //120 sec
int pumpOpTime = 15;  //15 sec
int lastWaterFullSec = 0;
int lightStat = 1;
int lightValuePin = A0;  //A0
int lightOnValue = 600;  //600, 0 dark ~ 1023 bright
int lightValue = 0;
int levelSensor = 14; //D5
int pumpRelay = 12; //D6
int lightRelay = 13; //D7
IPAddress local_ip(192, 168, 0, 1);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);
String webPage = "";
String macaddress = "";
String macaddress2 = "";
String idString = "";
String pwString = "";
String pwString2 = "";
String line = "";
String remoteip = "";
char id[20];
char pw[20];
ESP8266WebServer server(80);
WiFiClient client;

void setup() {
  Serial.begin(9600);
  Serial.println();
  Serial.println("System started ");
  WiFi.mode(WIFI_AP_STA);
  pinMode(lightValuePin, INPUT);
  pinMode(levelSensor, INPUT);
  pinMode(pumpRelay, OUTPUT);
  pinMode(lightRelay, OUTPUT);
  digitalWrite(pumpRelay, HIGH); //low trigger
  digitalWrite(lightRelay, HIGH); //low trigger
  SPIFFS.begin();
  File f = SPIFFS.open("/interval.txt", "r");
  while (f.available()) {
    line = f.readStringUntil('\n');
    //Serial.println(line);
    intervalTime = line.toInt();
  }
  f.close();
  f = SPIFFS.open("/pump.txt", "r");
  while (f.available()) {
    line = f.readStringUntil('\n');
    //Serial.println(line);
    pumpOpTime = line.toInt();
  }
  f.close();
  f = SPIFFS.open("/light.txt", "r");
  while (f.available()) {
    line = f.readStringUntil('\n');
    //Serial.println(line);
    lightStat = line.toInt();
  }
  f.close();
  f = SPIFFS.open("/lightOn.txt", "r");
  while (f.available()) {
    line = f.readStringUntil('\n');
    //Serial.println(line);
    lightOnValue = line.toInt();
  }
  f.close();
  f = SPIFFS.open("/ssid.txt", "r");
  while (f.available()) {
    wifissid = f.readStringUntil('\n');
  }
  f.close();
  f = SPIFFS.open("/password.txt", "r");
  while (f.available()) {
    wifipassword = f.readStringUntil('\n');
  }
  f.close();
  f = SPIFFS.open("/pwString.txt", "r");
  while (f.available()) {
    pwString = f.readStringUntil('\n');
  }
  f.close();
  f = SPIFFS.open("/adjTimeMinute.txt", "r");
  while (f.available()) {
    line = f.readStringUntil('\n');
    adjTimeMinute = line.toInt();
  }

  f.close();
  f = SPIFFS.open("/onHour.txt", "r");
  while (f.available()) {
    line = f.readStringUntil('\n');
    onHour = line.toInt();
  }
  f.close();
  f = SPIFFS.open("/offHour.txt", "r");
  while (f.available()) {
    line = f.readStringUntil('\n');
    offHour = line.toInt();
  }
  f.close();
  f = SPIFFS.open("/remoteIP.txt", "r");
  while (f.available()) {
    line = f.readStringUntil('\n');
    remoteip = line.toInt();
  }
  f.close();
  
  if (wifissid.length() > 1) {
    wifissid.toCharArray(wifissid2, wifissid.length());
    wifipassword.toCharArray(wifipassword2, wifipassword.length());
    WiFi.begin(wifissid2, wifipassword2);
    delay(5000);
    configTime(9 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    delay(10000);
    if (time(nullptr)) {
      time_t now = time(nullptr);
      hour = atoi(ctime(&now) + 11);
      minute = atoi(ctime(&now) + 14);
      adjTimeMinute = hour * 60 + minute - int(millis() / 1000 / 60);
    }
    //Serial.print("SSID=");
    //Serial.println(wifissid2);
    //Serial.print("PASSWORD=");
    //Serial.println(wifipassword2);
    //Serial.print("Client mode started ");
    //IPAddress ip = WiFi.localIP();
    //Serial.print("IP=");
    //Serial.println(ip);
  }

  startSec = int(millis() / 1000);

  server.on("/", []() {
    page() ;
    server.send(200, "text/html", webPage);
    remoteip = server.client().remoteIP().toString();
    //Serial.println(addy);
    File f = SPIFFS.open("/remoteIP.txt", "w");
       if (f) f.println(remoteip);
       if (!f) Serial.println("file open failed");
       f.close();
    delay(10);
  });

  server.on("/interval+10", []() {
    intervalTime = intervalTime + 60;
    if (intervalTime > 1800) intervalTime = 1800;
    page() ;
    server.send(200, "text/html", webPage);
    writeInitial();
    startSec = int(millis() / 1000);
    delay(10);
  });

  server.on("/interval-10", []() {
    intervalTime = intervalTime - 60;
    if (intervalTime < 120) intervalTime = 120;
    page() ;
    server.send(200, "text/html", webPage);
    writeInitial();
    startSec = int(millis() / 1000);
    delay(10);
  });

  server.on("/pump+10", []() {
    pumpOpTime = pumpOpTime + 5;
    if (pumpOpTime > 20) pumpOpTime = 20;
    page() ;
    server.send(200, "text/html", webPage);
    writeInitial();
    startSec = int(millis() / 1000);
    delay(10);
  });

  server.on("/pump-10", []() {
    pumpOpTime = pumpOpTime - 5;
    if (pumpOpTime < 0) pumpOpTime = 0;
    page() ;
    server.send(200, "text/html", webPage);
    writeInitial();
    startSec = int(millis() / 1000);
    delay(10);
  });

  server.on("/lightOn", []() {
    lightStat = 1;
    page() ;
    server.send(200, "text/html", webPage);
    writeInitial();
    delay(10);
  });

  server.on("/lightOff", []() {
    lightStat = 0;
    page() ;
    server.send(200, "text/html", webPage);
    writeInitial();
    delay(10);
  });

  server.on("/lightAuto", []() {
    lightStat = 2;
    lightValue = (analogRead(lightValuePin));
    page() ;
    server.send(200, "text/html", webPage);
    writeInitial();
    delay(10);
  });

  server.on("/light+10", []() {
    lightOnValue = lightOnValue + 10;
    if (lightOnValue > 1000) lightOnValue = 1000;
    page() ;
    server.send(200, "text/html", webPage);
    writeInitial();
    delay(10);
  });

  server.on("/light-10", []() {
    lightOnValue = lightOnValue - 10;
    if (lightOnValue < 0) lightOnValue = 0;
    page() ;
    server.send(200, "text/html", webPage);
    writeInitial();
    delay(10);
  });

  server.on("/lightTimer", []() {
    lightStat = 3;
    virTimeMinute = int(millis() / 1000 / 60) + adjTimeMinute;
    page() ;
    server.send(200, "text/html", webPage);
    writeInitial();
    delay(10);
  });

  server.on("/setTime", []() {
    String s = "<!DOCTYPE html> <html>\n";
    s += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\" meta charset=\"UTF-8\">\n";
    s += "<title>easyGrow";
    s += macaddress2 ;
    s += "&nbsp;</title>\n";
    s += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto;}\n";
    s += "body{margin-top:30px;} h1 {color:DimGray;margin:30px auto 30px;} h3 {color:SlateBlue;margin-bottom:14px;}\n";
    s += "p {font-size: 16px;color:Navy;margin-bottom: 12px;}\n";
    s += "</style>\n";
    s += "</head>\n";
    s += "<body>\n";
    s += "<h1 style='text-align:center;'>easyGrow";
    s += macaddress2 ;
    s += "&nbsp;</h1>\n";
    s += "<h3 style='text-align:center;'>시각 설정</h3>\n" ;
    s += "<p style='text-align:center;'>현재 시각: " ;
    virTimeMinute = int(millis() / 1000 / 60) + adjTimeMinute;
    s += int((virTimeMinute / 60) % 24) ;
    s += " 시 " ;
    s += int(virTimeMinute % 60) ;
    s += " 분</p>\n" ;
    s += "<form action='/setTime2'><p style='text-align:left;'>시(0~23사이): <input type='number' name='hour'><br>";
    s += "분(0~59사이): <input type='number' name='minute'></p>\n";
    s += "<h3 style='text-align:center;'>LED 점등 Schedule 설정</h3>\n" ;
    s += "<p style='text-align:center;'>켜지는 시각: ";
    s += onHour;
    s += " <br>꺼지는 시각: ";
    s += offHour;
    s += " </p>\n" ;
    s += "<p style='text-align:left;'>켜지는 시각(0~24사이): <input type='number' name='onHour'><br>";
    s += "꺼지는 시각(0~24사이): <input type='number' name='offHour'><br></p>\n";
    s += "<p style='text-align:center;'><input type='submit' value='Submit'></p></form>\n";
    s += "</body></html>\r\n\r\n";
    server.send(200, "text/html", s);
    delay(10);
  });

  server.on("/setTime2", []() {
    if (server.hasArg("hour") && server.hasArg("minute") && server.hasArg("onHour") && server.hasArg("offHour")) {
      hour = server.arg("hour").toInt();
      minute = server.arg("minute").toInt();
      onHour = server.arg("onHour").toInt();
      offHour = server.arg("offHour").toInt();
      //Serial.print("hour: ");
      //Serial.print(hour);
      //Serial.print(" minute: ");
      //Serial.println(minute);
      adjTimeMinute = hour * 60 + minute - int(millis() / 1000 / 60);
      virTimeMinute = int(millis() / 1000 / 60) + adjTimeMinute;
      //Serial.println(adjTimeMinute);
      writeInitial4();
      delay(10);
    }
    page() ;
    server.send(200, "text/html", webPage);
    delay(10);
  });

  server.on("/selectNetwork", []() {
    int Tnetwork = 0, i = 0, len = 0;
    String st = "", s = "";
    Tnetwork = WiFi.scanNetworks();
    st = "<ul>";
    for (int i = 0; i < Tnetwork; ++i)  {
      // Print SSID and RSSI for each network found
      st += "<li>";
      st += i + 1;
      st += ": ";
      st += WiFi.SSID(i);
      st += " (";
      st += WiFi.RSSI(i);
      st += ")";
      st += (WiFi.encryptionType(i) == ENC_TYPE_NONE) ? " " : "*";
      st += "</li>";
    }
    st += "</ul>";
    IPAddress ip = WiFi.softAPIP();
    //String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
    s = "<!DOCTYPE html> <html>\n";
    s += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\"meta charset=\"UTF-8\">\n";
    s += "<title>easyGrow";
    s += macaddress2 ;
    s += "&nbsp;</title>\n";
    s += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto;}\n";
    s += "body{margin-top:30px;} h1 {color:DimGray;margin:30px auto 30px;} h3 {color:SlateBlue;margin-bottom:14px;}\n";
    s += ".button {display: inline-block;width: 80px;background-color:#1abc9c;border:none;color:white;padding: 10px 10px;text-align: center;text-decoration: none;font-size: 18px;margin: 0px auto 10px;cursor: pointer;border-radius: 4px;}\n";
    s += "p {font-size: 12px;color:Navy;margin-bottom: 6px;}\n";
    s += "</style>\n";
    s += "</head>\n";
    s += "<body>\n";
    s += "<h1 style='text-align:center;'>easyGrow";
    s += macaddress2 ;
    s += "&nbsp;</h1>\n";
    s += "<h3 style='text-align:center;'>Choose SSID</h3> ";
    //s += ipStr;
    s += "<p style='text-align:left;'>";
    s += st;
    s += "</p>";
    s += "<h3 style='text-align:center;'><form method='get' action='saveNetwork'><p><label>SSID입력: </label><input name='ssid' length=32><br>";
    s += "<label>Password입력: </label><input name='pass' length=64></h3><h3 style='text-align:center;'><input type='submit'></form></h3>";
    s += "<h3 style='text-align:center;'>";
    s += "모두 입력하시고 Submit 버튼을 눌러주세요</h3>";
    s += "</body></html>\r\n\r\n";
    server.send( 200, "text/html", s);
    delay(10);
  });

  server.on("/saveNetwork", []() {
    if (server.hasArg("ssid") && server.hasArg("pass")) {
      wifissid = server.arg("ssid"); //Get SSID
      wifipassword = server.arg("pass"); //Get Password
      writeInitial2();
      File f = SPIFFS.open("/ssid.txt", "r");
      while (f.available()) {
        wifissid = f.readStringUntil('\n');
      }
      f.close();

      f = SPIFFS.open("/password.txt", "r");
      while (f.available()) {
        wifipassword = f.readStringUntil('\n');
      }
      f.close();

      wifissid.toCharArray(wifissid2, wifissid.length());
      wifipassword.toCharArray(wifipassword2, wifipassword.length());
      WiFi.begin(wifissid2, wifipassword2);
      delay(5000);
    }
    String s = "<!DOCTYPE html> <html>\n";
    s += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\"meta charset=\"UTF-8\">\n";
    s += "<title>easyGrow";
    s += macaddress2 ;
    s += "&nbsp;</title>\n";
    s += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align:center;}\n";
    s += "body{margin-top:30px;} h1 {color:DimGray;margin:30px auto 30px;} h3 {color:SlateBlue;margin-bottom:14px;}\n";
    s += ".button {display: inline-block;width: 120px;background-color:#1abc9c;border:none;color:white;padding: 10px 10px;text-align: center;text-decoration: none;font-size: 18px;margin: 0px auto 10px;cursor: pointer;border-radius: 4px;}\n";
    s += "p {font-size: 12px;color:Navy;margin-bottom: 6px;}\n";
    s += "</style>\n";
    s += "</head>\n";
    s += "<body>\n";
    s += "<h1>easyGrow";
    s += macaddress2 ;
    s += "&nbsp;</h1>\n";
    s += "<h3>공유기가 부여한 IP 주소는 <br>";
    s += WiFi.localIP().toString();
    s += "입니다<br>이제부터는 공유기의 WiFi와 접속하시고<br>웹브라우저 주소창에<br>";
    s += WiFi.localIP().toString();
    s += "를 입력하세요</h3><p>위에서 IP주소가 IP unset이라고 표시된 경우는<br>페이지 고침을 하여 주세요<br><a class=\"button\" href=\"/\">메인페이지로 돌아가기</a></p></body></html>\r\n\r\n";
    server.send(200, "text/html", s);
  });

  server.on("/network", []() {
    String s = "<!DOCTYPE html> <html>\n";
    s += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\"meta charset=\"UTF-8\">\n";
    s += "<title>easyGrow";
    s += macaddress2 ;
    s += "&nbsp;</title>\n";
    s += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align:center;}\n";
    s += "body{margin-top:30px;} h1 {color:DimGray;margin:30px auto 30px;} h3 {color:SlateBlue;margin-bottom:14px;}\n";
    s += ".button {display: inline-block;width: 120px;background-color:#1abc9c;border:none;color:white;padding: 10px 10px;text-align: center;text-decoration: none;font-size: 18px;margin: 0px auto 10px;cursor: pointer;border-radius: 4px;}\n";
    s += "p {font-size: 12px;color:Navy;margin-bottom: 6px;}\n";
    s += "</style>\n";
    s += "</head>\n";
    s += "<body>\n";
    s += "<h1>easyGrow";
    s += macaddress2 ;
    s += "&nbsp;</h1>\n";
    s += "<h3>공유기가 부여한 IP 주소는 <br>";
    s += WiFi.localIP().toString();
    s += "입니다<br>이제부터는 공유기의 WiFi와 접속하시고<br>웹브라우저 주소창에<br>";
    s += WiFi.localIP().toString();
    s += "를 입력하세요</h3><p><a class=\"button\" href=\"/\">메인페이지로 돌아가기</a></p></body></html>\r\n\r\n";
    server.send(200, "text/html", s);
  });

  server.on("/changePW", []() {
    String s = "<!DOCTYPE html> <html>\n";
    s += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\"meta charset=\"UTF-8\">\n";
    s += "<title>easyGrow";
    s += macaddress2 ;
    s += "&nbsp;</title>\n";
    s += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align:center;}\n";
    s += "body{margin-top:30px;} h1 {color:DimGray;margin:30px auto 30px;} h3 {color:SlateBlue;margin-bottom:14px;}\n";
    s += "p {font-size: 12px;color:Navy;margin-bottom:6px;}\n";
    s += "</style>\n";
    s += "</head>\n";
    s += "<body>\n";
    s += "<h1>easyGrow";
    s += macaddress2 ;
    s += "&nbsp;</h1>\n";
    s += "<h3>Change Password</h3>\n" ;
    s += "<p>서버모드로 접속할 때의 비밀번호를 변경합니다<br>공유기와 접속할 때의 비밀번호와 관계가 없습니다<br>비밀번호는 8자리 이상으로 입력하세요</p>\n" ;
    //s += "<h3><p><form action='/changePW2'><input type='password' name='pwString'><br>";
    //s += "<input type='submit' value='Submit'></form></p></h3>";
    s += "<h3><p><form method='get' action='changePW2'><label>새로운 비밀번호: </label><input name='pwString2' length=32></p>";
    s += "<input type='submit'></form></p></h3></body></html>\r\n\r\n";
    server.send(200, "text/html", s);
    delay(10);
  });

  server.on("/changePW2", []() {
    if (server.hasArg("pwString2")) {
      pwString = server.arg("pwString2");
      if (pwString.length() >= 8) {
        writeInitial3();
        Serial.print("pwString=");
        Serial.println(pwString);
        //pw = pwString.c_str();
        //pwString.toCharArray(pw, pwString.length());
        //WiFi.softAPdisconnect();
        //delay(500);
        //WiFi.softAP(id, pw);
        //WiFi.softAPConfig(local_ip,gateway,subnet);
        String s = "<!DOCTYPE html> <html>\n";
        s += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\"meta charset=\"UTF-8\">\n";
        s += "<title>easyGrow";
        s += macaddress2 ;
        s += "&nbsp;</title>\n";
        s += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align:center;}\n";
        s += "body{margin-top:30px;} h1 {color:DimGray;margin:30px auto 30px;} h3 {color:SlateBlue;margin-bottom:14px;}\n";
        s += ".button {display: inline-block;width: 80px;background-color:#1abc9c;border:none;color:white;padding: 10px 10px;text-align: center;text-decoration: none;font-size: 12px;margin: 0px auto 10px;cursor: pointer;border-radius: 4px;}\n";
        s += "p {font-size: 12px;color:Navy;margin-bottom:6px;}\n";
        s += "</style>\n";
        s += "</head>\n";
        s += "<body>\n";
        s += "<h1>easyGrow";
        s += macaddress2 ;
        s += "&nbsp;</h1>\n";
        s += "<h3>비밀번호가 변경되어<br>시스템을 다시 시작합니다<br>서버모드로 접속하신 경우<br>WiFi 접속을 새로운 비밀번호로<br>다시 하여 주세요</h3>\n" ;
        s += "</body></html>\r\n\r\n";
        server.send(200, "text/html", s);
        delay(5000);
        WiFi.mode(WIFI_OFF);
        ESP.restart();
      }
      else {
        String s = "<!DOCTYPE html> <html>\n";
        s += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\"meta charset=\"UTF-8\">\n";
        s += "<title>easyGrow";
        s += macaddress2 ;
        s += "&nbsp;</title>\n";
        s += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align:center;}\n";
        s += "body{margin-top:30px;} h1 {color:DimGray;margin:30px auto 30px;} h3 {color:SlateBlue;margin-bottom:14px;}\n";
        s += ".button {display: inline-block;width: 80px;background-color:#1abc9c;border:none;color:white;padding: 10px 10px;text-align: center;text-decoration: none;font-size: 12px;margin: 0px auto 10px;cursor: pointer;border-radius: 4px;}\n";
        s += "p {font-size: 12px;color:Navy;margin-bottom:6px;}\n";
        s += "</style>\n";
        s += "</head>\n";
        s += "<body>\n";
        s += "<h1>easyGrow";
        s += macaddress2 ;
        s += "&nbsp;</h1>\n";
        s += "<p>비밀번호를 8자리 이상으로 다시 입력하여 주세요<br>\n" ;
        s += "<a class=\"button\" href=\"/changePW\">비밀번호변경 페이지로 돌아가기</a></p></body></html>\r\n\r\n";
        s += "</body></html>\r\n\r\n";
        server.send(200, "text/html", s);
      }
    }

  });

  //server.begin();
  Serial.println();
  Serial.println("HTTP server started");
  //Serial.print("SSID=");
  //Serial.println(wifissid);
  //Serial.print("PASSWORD=");
  //Serial.println(wifipassword);
  //Serial.println();
  macaddress = WiFi.macAddress();
  macaddress2 = macaddress.substring(12, 14) + macaddress.substring(15, 17);
  idString = ("easyGrow" + macaddress2);
  Serial.print(idString);
  Serial.println("started");
  idString.toCharArray(id, idString.length() + 1);
  //id = idString.c_str();
  if (pwString.length() <= 1) {
    pwString = idString;
    pwString.toLowerCase();;
    pwString.toCharArray(pw, pwString.length() + 1);
  }
  else {
    pwString.toCharArray(pw, pwString.length());
  }
  //pw = pwString.c_str();

  WiFi.softAPConfig(local_ip, gateway, subnet);
  WiFi.softAP(id, pw);
  server.begin();
  Serial.print("SSID=");
  Serial.println(id);
  Serial.print("PASSWORD=");
  Serial.print(pw);
  Serial.println("");

}

void loop() {
  delay(10);
  server.handleClient();
  //if (WiFi.status() != WL_CONNECTED) toGetIDnPass();
  currentSec = int(millis() / 1000);
  remainingSec = intervalTime - currentSec + startSec - pumpOpTime;
  //Serial.print(remainingSec);
  //Serial.println(remainingSec%5);
  if (currentSec > 3600 * 24 * 30) {
    adjTimeMinute = hour * 60 + minute;
    writeInitial4();
    ESP.restart();
  }
  if (remainingSec <= -1 * pumpOpTime )  {
    startSec = int(millis() / 1000);
  }
  if (digitalRead(levelSensor) == LOW ) { //LOW is out of water **
    if ((currentSec - lastWaterFullSec) > 1) {
      if (remainingSec % 5 == 0) {
        digitalWrite (lightRelay, LOW);  //low trigger
      }
      else {
        digitalWrite (lightRelay, HIGH);  //low trigger
      }

      digitalWrite(pumpRelay, HIGH); //low trigger

    }
  }
  else {
    lastWaterFullSec = int(millis() / 1000);                                 //**
    if (remainingSec > 0)  {
      digitalWrite(pumpRelay, HIGH); //low trigger
    }
    if (remainingSec <= 0 && remainingSec > -1 * pumpOpTime )  {
      digitalWrite(pumpRelay, LOW); //low trigger
    }
    if (lightStat == 0) digitalWrite(lightRelay, LOW);  //low trigger
    if (lightStat == 1) digitalWrite(lightRelay, HIGH);  //low trigger
    if (lightStat == 2) {
      //Serial.println(lightOnValue);
      if (remainingSec % 2 == 0) {
        lightValue = (analogRead(lightValuePin));                            //***
      }
      if (lightValue <= lightOnValue) { //0 dark ~ 1023 bright
        digitalWrite(lightRelay, HIGH);  //low trigger
        delay(1000);
      }
      else {
        digitalWrite(lightRelay, LOW);  //low trigger
        delay(1000);
      }
    }
    if (lightStat == 3) {
      virTimeMinute = int(millis() / 1000 / 60) + adjTimeMinute;
      hour = (virTimeMinute / 60) % 24;
      //Serial.print("current time: ");
      //Serial.println(hour);
      if (onHour < offHour) {
        if (hour < offHour && hour >= onHour) { //8AM-20PM
          digitalWrite(lightRelay, LOW);  //low trigger
        }
        else digitalWrite(lightRelay, HIGH);  //low trigger
      }
      if (onHour > offHour) {
        if (hour < offHour || hour >= onHour) { //20AM-8PM
          digitalWrite(lightRelay, LOW);  //low trigger
        }
        else digitalWrite(lightRelay, HIGH);  //low trigger
      }
      if (onHour == offHour) digitalWrite(lightRelay, HIGH);
    }
  }
}
void writeInitial() {
  File f = SPIFFS.open("/interval.txt", "w");
  if (f) f.println(String(intervalTime));
  if (!f) Serial.println("file open failed");
  f.close();
  f = SPIFFS.open("/pump.txt", "w");
  if (f) f.println(String(pumpOpTime));
  if (!f) Serial.println("file open failed");
  f.close();
  f = SPIFFS.open("/light.txt", "w");
  if (f) f.println(String(lightStat));
  if (!f) Serial.println("file open failed");
  f.close();
  f = SPIFFS.open("/lightOn.txt", "w");
  if (f) f.println(String(lightOnValue));
  if (!f) Serial.println("file open failed");
  f.close();
}

void writeInitial2() {
  File f = SPIFFS.open("/ssid.txt", "w");
  if (f) f.println(wifissid);
  if (!f) Serial.println("file open failed");
  f.close();
  f = SPIFFS.open("/password.txt", "w");
  if (f) f.println(wifipassword);
  if (!f) Serial.println("file open failed");
  f.close();
}

void writeInitial3() {
  File f = SPIFFS.open("/pwString.txt", "w");
  if (f) f.println(pwString);
  if (!f) Serial.println("file open failed");
  f.close();
}

void writeInitial4() {
  File f = SPIFFS.open("/adjTimeMinute.txt", "w");
  if (f) f.println(adjTimeMinute);
  if (!f) Serial.println("file open failed");
  f.close();
  f = SPIFFS.open("/onHour.txt", "w");
  if (f) f.println(onHour);
  if (!f) Serial.println("file open failed");
  f.close();
  f = SPIFFS.open("/offHour.txt", "w");
  if (f) f.println(offHour);
  if (!f) Serial.println("file open failed");
  f.close();
}

void page() {
  webPage = "<!DOCTYPE html> <html>\n";
  webPage += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\" meta charset=\"UTF-8\">\n";
  webPage += "<title>easyGrow";
  webPage += macaddress2 ;
  webPage += "&nbsp;</title>\n";
  webPage += "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align:center;}\n";
  webPage += "body{margin-top:30px;} h1 {color:DimGray ;margin:30px auto 30px;} h3 {color:SlateBlue;margin-bottom:14px;}\n";
  webPage += ".button {display: inline-block;width: 80px;background-color:#1abc9c;border:none;color:white;padding: 10px 10px;text-align: center;text-decoration: none;font-size: 18px;margin: 0px auto 10px;cursor: pointer;border-radius: 4px;}\n";
  webPage += ".button-on {background-color:Orange;}\n";
  webPage += ".button-on:active {background-color:Orange;}\n";
  webPage += ".button-off {background-color: #2980B9;}\n";
  webPage += ".button-off:active {background-color: #2980B9;}\n";
  webPage += ".button-auto {background-color:MediumSeaGreen;font-size: 14px;}\n";
  webPage += ".button-auto:active {background-color:MediumSeaGreen;font-size: 14px;}\n";
  webPage += ".button-timer {background-color:Crimson;font-size: 14px;}\n";
  webPage += ".button-timer:active {background-color:Crimson;font-size: 14px;}\n";
  webPage += ".button-plus {background-color:Tomato;}\n";
  webPage += ".button-plus:active {background-color:Tomato;}\n";
  webPage += ".button-plus2 {background-color:Tomato;font-size: 14px;}\n";
  webPage += ".button-plus2:active {background-color:Tomato;font-size: 14px;;}\n";
  webPage += ".button-minus{background-color: #4CAF50;}\n";
  webPage += ".button-minus:active {background-color: #4CAF50;}\n";
  webPage += ".button-minus2{background-color: #4CAF50;font-size: 14px;}\n";
  webPage += ".button-minus2:active {background-color: #4CAF50;font-size: 14px;}\n";
  webPage += ".button-set{background-color:LightSkyBlue;font-size: 14px;}\n";
  webPage += ".button-set:active {background-color:LightSkyBlue;font-size: 14px;}\n";
  webPage += ".button-changePW{background-color:LightSlateGray;font-size: 14px;}\n";
  webPage += ".button-changePW:active {background-color:LightSlateGray;font-size: 14px;;}\n";
  webPage += ".button-network{background-color:PeachPuff ;font-size: 14px;}\n";
  webPage += ".button-network:active {background-color:PeachPuff ;font-size: 14px;;}\n";
  webPage += "p {font-size: 12px;color:Navy;margin-bottom: 6px;}\n";
  webPage += "</style>\n";
  webPage += "</head>\n";
  webPage += "<body>\n";
  webPage += "<h1>easyGrow";
  webPage += macaddress2 ;
  webPage += "&nbsp;</h1>\n";
  webPage += "<h3>LED 제어: ";
  if (lightStat == 0) {
    webPage += "꺼짐</h3>\n";
  }
  if (lightStat == 1) {
    webPage += "켜짐</h3>\n";
  }
  if (lightStat == 2) {////팬 제어로 변경 
    webPage += "빛감지모드</h3>\n";
  }
  if (lightStat == 3) {
    webPage += "시간별제어</h3>\n";
  }
  webPage += "<p><a class=\"button button-on\" href=\"/lightOn\">ON</a>\n";
  webPage += "<a class=\"button button-off\" href=\"/lightOff\">OFF</a></p>\n";
  webPage += "<p><a class=\"button button-auto\" href=\"/lightAuto\">빛감지모드</a>\n";//// 이거 지우고 시간별제어 가운데로 오게하기
  webPage += "<a class=\"button button-timer\" href=\"/lightTimer\">김주안제어</a></p>\n";
  if (lightStat == 2) {//// 이자리에 그대로 넣어도 되나?? 아니면 밑으로 가게할까?
    webPage += "<h3>LED 점등감도: " ;
    webPage += lightOnValue ;
    webPage += "</h3>\n" ;
    webPage += "<p>현재 빛감도: " ;
    webPage += lightValue ;
    webPage += "</p>\n" ;
    webPage += "<p><a class=\"button button-minus2\" href=\"/light-10\">감도어둡게</a>\n";
    webPage += "<a class=\"button button-plus2\" href=\"/light+10\">감도밝게</a></p>\n";
  }
  if (lightStat == 3) {
    webPage += "<h3>현재시각: " ;
    virTimeMinute = int(millis() / 1000 / 60) + adjTimeMinute;
    webPage += int((virTimeMinute / 60) % 24) ;
    webPage += " 시 " ;
    webPage += int(virTimeMinute % 60) ;
    webPage += " 분</h3>\n" ;
    webPage += "<p>켜지는 시각: ";
    webPage += onHour;
    webPage += "시, 꺼지는 시각: ";
    webPage += offHour;
    webPage += "시</p>\n" ;
    webPage += "<p><a class=\"button button-set\" href=\"/setTime\">시각설정</a></p>\n";
  }
  webPage += "<h3>펌프가동 Interval: 매 " ;
  webPage += int(intervalTime / 60) ;
  webPage += "분 마다</h3>\n" ;
  webPage += "<p><a class=\"button button-minus\" href=\"/interval-10\">-1분</a>\n";
  webPage += "<a class=\"button button-plus\" href=\"/interval+10\">+1분</a></p>\n";
  webPage += "<h3>펌프작동시간: " ;
  webPage += pumpOpTime ;
  webPage += "초 가동</h3>\n" ;
  webPage += "<p><a class=\"button button-minus\" href=\"/pump-10\">-5초</a>\n";
  webPage += "<a class=\"button button-plus\" href=\"/pump+10\">+5초</a></p>\n";
  ////여기에 팬가동 , 팬 작동시간 넣기/FAN-10,FAN+10,INTERVAL-10,INTERVAL+10 생성
  ////그및에 펌프작동시작 설정 버튼 넣기
  webPage += "<h3>Password 변경 & Network 설정</h3>\n" ;
  webPage += "<p>서버모드 비밀번호 변경 및 클라이언트모드 접속 설정</p>\n" ;
  webPage += "<p><a class=\"button button-changePW\" href=\"/changePW\">비밀번호변경</a>\n";
  webPage += "<a class=\"button button-network\" href=\"/selectNetwork\">네트워크설정</a></p>\n";
  webPage += "<p>클라이언트모드 WiFi접속: ";
  webPage += wifissid;
  webPage += "<br>클라이언트모드 IP주소: ";
  webPage += WiFi.localIP().toString();
  webPage += "<br>The Last Remote IP주소: ";
  webPage += remoteip;
  webPage += "</p></body>\n";
  webPage += "</html>\n";
}

/*
  void toGetIDnPass () {
   IPAddress local_ip(192,168,0,1);
   IPAddress gateway(192,168,0,1);
   IPAddress subnet(255,255,255,0);
   macaddress = WiFi.macAddress();
   macaddress2 = macaddress.substring(12,14)+macaddress.substring(15,17);
   idString = ("easyGrow"+macaddress2);
   id = idString.c_str();
   pwString = idString;
   pwString.toLowerCase();;
   pw = pwString.c_str();
   WiFi.softAP(id, pw);
   WiFi.softAPConfig(local_ip,gateway,subnet);
   //Serial.println("Configuring Network...");
   //Serial.print("Connect to: ");
   //Serial.println(idString);
   //Serial.print("Password: ");
   //Serial.println(pwString);
  }
*/
