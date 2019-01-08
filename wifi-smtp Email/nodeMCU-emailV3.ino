
//Needs to be flashed 2 times. First time uncomment clearEEPROM(); line in setup function. This will write
//default data to EEPROM, before reading from it. Now comment or delete this line from sketch and reflash nodeMCU board.
//This is simple example of using web page as user interface to configure wifi parameters for nodeMCU board to connect to 
//existing wifi network. Example shows static IP configuration, but you can easily change it to DHCP mode. Upon first boot 
//board will start in AP mode with both SSID and password set to nodemcuwifi. Default IP is 192.168.4.1. When you enter IP 
//in web browser you should arrive to simple settings webpage where you can enter SSID, password, IP address, gateway IP and 
//subnet mask, all standard data for static IP configuration. When you click SAVE and restart your board, it should connect to 
//existing wifi network in client mode. 
//All data is stored in EEPROM as node_config object. When board is ON and you press FLASH button it will clear all saved data 
//in EEPROM and set board to boot in AP mode next time you restart it.

/*  
 * ESP Mailer - TestMail.ino
 * ESP Mailer is a PHPMailer like Library for your ESP.
 * Check out https://github.com/ArduinoHannover/ESPMailer
 * And for Pushover Notifications: https://github.com/ArduinoHannover/Pushover
 */

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "ESPMailer.h"

bool isActivated = false; // flag for AP mode, if isActivate = true, board connects as wifi client

int resetButton = 0; // pin number with RESET button when pressed clears stored information and activates AP mode
int LEDpin = 2; // nodeMCU onboard LED

uint addr = sizeof(isActivated); // EEPROM address of node_config object

// Configuration object
struct node_Configuration
{
  char wifi_ssid[32];     // SSID of wifi network
  char wifi_password[64]; // password for wifi network
  IPAddress wifi_ip;      // Static IP for board
  IPAddress wifi_gateway; // IP address of gateway (wifi router)
  IPAddress wifi_subnet;  // Local subnet mask
  
  char smtp_host[64]; // SMTP server hostname     
  uint smtp_port; // SMTP server port
  char smtp_username[64]; // SMTP server username    
  char smtp_password[64];  // SMTP server password
  char smtp_fromEmail[64];  // FROM email address
  char smtp_fromName[32]; // FROM email NAME
  char smtp_toEmail[64]; // TO EMAIL ADDRESS - all email messages go to this email address
} node_config;

ESP8266WebServer server(80);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200); // Serial for debugging SMTP email send process
  pinMode(resetButton, INPUT_PULLUP); // FLASH button on nodeMCU board, used to clear saved user values, and set board to boot in Access Point mode
  pinMode(LEDpin, OUTPUT); // onboard LED
  digitalWrite(LEDpin, HIGH); // turn off onboard LED
  
  EEPROM.begin(512); // start EEPROM functions

  // clearEEPROM(); // uncomment for first flash, then comment again and reflash
  
  EEPROM.get(0, isActivated); // get first value stored in EEPROM and assign it to isActivated variable which indicates if board will boot in client or AP wireless mode

  // If board is not activated start in AP mode, default ip is 192.168.4.1
  if (isActivated != true)
  { 
    clearEEPROM();
    WiFi.mode(WIFI_AP);
    WiFi.softAP("nodemcuwifi", "nodemcuwifi");
    delay(1000);
  }
  else // board is activated, get wifi values from EEPROM and connect as client
  {
    EEPROM.get(addr, node_config);

    // WiFi configuration
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    WiFi.config(node_config.wifi_ip, node_config.wifi_gateway, node_config.wifi_subnet, node_config.wifi_gateway);
    WiFi.begin(node_config.wifi_ssid, node_config.wifi_password);

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
    }
  }

  // Starting Webserver
  server.begin();

  // Webserver handles
  server.on("/", handleRoot); // handles root webpage - settings web page
  server.on("/saveEEPROM", handleEEPROM); // handles SAVE button on settings webpage, called as FORM ACTION in html
  server.on("/emailTest", handleEmailTest); // handles TEST button on settings webpage, sends one test email to check SMTP config

  delay(2000);
}

void loop() {
    // FLASH button on board sets default values and puts board into AP mode
  if (digitalRead(resetButton) == LOW)
  {
    clearEEPROM();
    digitalWrite(LEDpin, LOW); // turn ON onboard LED indicating that EEPROM is cleared
  }
  server.handleClient();
  delay(50);
}

// WebServer handles:
void handleRoot() {
  server.send(200, "text/html", getDefaultPage());
}
// handles SAVE button, called via FORM ACTION in html
void handleEEPROM()
{
  // If the POST request doesn't have username and password data
  if (!server.hasArg("wifi_ssid") || !server.hasArg("wifi_password") || server.arg("wifi_ssid") == NULL || server.arg("wifi_password") == NULL)
  {                                                         
    server.send(400, "text/plain", "400: Invalid Request"); // The request is invalid, so send HTTP status 400
    return;
  }
  String usr_ssid = server.arg("wifi_ssid");
  String usr_pass = server.arg("wifi_password");

  IPAddress usr_ip(server.arg("wifi_ip1").toInt(), server.arg("wifi_ip2").toInt(), server.arg("wifi_ip3").toInt(), server.arg("wifi_ip4").toInt());
  IPAddress usr_gw(server.arg("wifi_gw1").toInt(), server.arg("wifi_gw2").toInt(), server.arg("wifi_gw3").toInt(), server.arg("wifi_gw4").toInt());
  IPAddress usr_sub(server.arg("wifi_sub1").toInt(), server.arg("wifi_sub2").toInt(), server.arg("wifi_sub3").toInt(), server.arg("wifi_sub4").toInt());

  usr_ssid.toCharArray(node_config.wifi_ssid, 32);
  usr_pass.toCharArray(node_config.wifi_password, 64);
  node_config.wifi_ip = usr_ip;
  node_config.wifi_gateway = usr_gw;
  node_config.wifi_subnet = usr_sub;

  String usr_smtp_host = server.arg("smtp_host");
  String usr_smtp_port = server.arg("smtp_port");
  String usr_smtp_username = server.arg("smtp_username");
  String usr_smtp_password = server.arg("smtp_password");
  String usr_smtp_fromEmail = server.arg("smtp_fromEmail");
  String usr_smtp_fromName = server.arg("smtp_fromName");
  String usr_smtp_toEmail = server.arg("smtp_toEmail");

  usr_smtp_host.toCharArray(node_config.smtp_host, 64);
  node_config.smtp_port = usr_smtp_port.toInt();
  usr_smtp_username.toCharArray(node_config.smtp_username, 64);
  usr_smtp_password.toCharArray(node_config.smtp_password, 64);
  usr_smtp_fromEmail.toCharArray(node_config.smtp_fromEmail, 64);
  usr_smtp_fromName.toCharArray(node_config.smtp_fromName, 32);
  usr_smtp_toEmail.toCharArray(node_config.smtp_toEmail, 64);

  isActivated = true;
  
  EEPROM.put(0, isActivated);
  EEPROM.put(addr, node_config);
  EEPROM.commit();

  server.sendHeader("Location", String("/"), true);
  server.send(302, "text/plain", "");
}
void handleEmailTest() {
  sendEmail("Test Subject", "Email sent from nodeMCU"); // sends email, first argument is subject, second is text (body);
  
  server.sendHeader("Location", String("/"), true);
  server.send(302, "text/plain", "");
}

// webServer handles END
// getTemp page
String getDefaultPage() {

  // Settings webpage
  String page = "<!DOCTYPE html>\n<html>\n<head>\n <title>WIFI-EMAIL SETTINGS</title>\n <meta charset=\"utf-8\" />\n <meta name=\"viewport\" content=\"width=600, user-scalable=no\" />\n\n <style>\n body {\n background-color: #fff;\n margin: auto;\n font-family: sans-serif;\n font-weight: bold;\n text-decoration: none !important;\n text-align: center;\n }\n\n a {\n text-decoration: none;\n }\n\n .container {\n display: block;\n width: 600px;\n clear: both;\n margin: auto;\n }\n\n /* ROWS */\n .row {\n display: block;\n height: 80px;\n clear: both;\n vertical-align: middle;\n line-height: 80px;\n border-bottom: 2px solid #1a1a1a;\n overflow: hidden;\n font-size: 30px;\n }\n .row-xs {\n height: 40px;\n line-height: 40px;\n font-size: 14px;\n } \n\n .row-sm {\n height: 60px;\n line-height: 60px;\n font-size: 24px;\n }\n\n .row-lg {\n height: 120px;\n line-height: 120px;\n font-size: 36px;\n }\n\n /* CELLS */\n\n .cell-lg {\n width: 600px;\n float: left;\n }\n\n .cell-md {\n width: 400px;\n float: left;\n }\n\n .cell-sm {\n width: 200px;\n float: left;\n text-align: center;\n }\n\n .indicator {\n display: block;\n width: 60px;\n float: left;\n height: 100%;\n text-align: center;\n }\n\n .btn {\n width: 180px;\n height: 100%;\n float: left;\n font-size: 24px;\n font-weight: bold;\n border-left: 10px solid #000;\n border-right: 10px solid #000;\n background-color: #0094ff;\n color: white;\n }\n\n /*colors and backrounds*/\n .bg-info {\n background-color: #e6e6e6;\n color: #1a1a1a;\n }\n\n .bg-data {\n background-color: #0c336e;\n color: white;\n }\n\n .bg-title {\n background-color: #4e4e4e;\n color: white;\n }\n\n /* indicator styles */\n .on {\n background-color: #59e430;\n color: #59e430;\n }\n\n .off {\n color: #ff0000;\n background-color: #ff0000;\n }\n /* Empty Cell */\n .empty {\n background-color: #fff;\n color: #fff;\n }\n .big {\n font-size: 72px;\n font-weight: bold;\n }\n .text-left {\n text-align: left;\n padding-left: 12px;\n }\n .textbox {\n text-align: center;\n font-size: 32px;\n width: 80%;\n }\n .textbox-ip {\n text-align: center;\n font-size: 32px;\n width: 65px;\n }\n .btn-in {\n width: 200px;\n height: 120px;\n float: left;\n font-size: 32px;\n font-weight: bold;\n }\n .active {\n background-color: darkorange;\n }\n </style>\n\n</head>\n<body>\n <div class=\"container\">\n\n <div class=\"row bg-title\">SETTINGS</div>\n\n <form action=\"/saveEEPROM\" method=\"POST\"> \n \n <div class=\"row row-sm text-left\">WiFi Configuration</div>\n <div class=\"row row-sm bg-info\">\n <div class=\"cell-sm\">SSID</div>\n <div class=\"cell-md\"><input type=\"text\" class=\"textbox\" name=\"wifi_ssid\" value=\"##wifi_ssid##\"></div>\n </div>\n <div class=\"row row-sm bg-info\">\n <div class=\"cell-sm\">WiFi Pass</div>\n <div class=\"cell-md\"><input type=\"password\" class=\"textbox\" name=\"wifi_password\" value=\"##wifi_password##\"></div>\n </div>\n\n <div class=\"row row-sm bg-info\">\n <div class=\"cell-sm\">IP address</div>\n <div class=\"cell-md\">\n <input type=\"text\" class=\"textbox-ip\" name=\"wifi_ip1\" value=\"##wifi_ip1##\">.\n <input type=\"text\" class=\"textbox-ip\" name=\"wifi_ip2\" value=\"##wifi_ip2##\">.\n <input type=\"text\" class=\"textbox-ip\" name=\"wifi_ip3\" value=\"##wifi_ip3##\">.\n <input type=\"text\" class=\"textbox-ip\" name=\"wifi_ip4\" value=\"##wifi_ip4##\">\n </div>\n </div>\n\n <div class=\"row row-sm bg-info\">\n <div class=\"cell-sm\">Gateway</div>\n <div class=\"cell-md\">\n <input type=\"text\" class=\"textbox-ip\" name=\"wifi_gw1\" value=\"##wifi_gw1##\">.\n <input type=\"text\" class=\"textbox-ip\" name=\"wifi_gw2\" value=\"##wifi_gw2##\">.\n <input type=\"text\" class=\"textbox-ip\" name=\"wifi_gw3\" value=\"##wifi_gw3##\">.\n <input type=\"text\" class=\"textbox-ip\" name=\"wifi_gw4\" value=\"##wifi_gw4##\">\n </div>\n </div>\n\n <div class=\"row row-sm bg-info\">\n <div class=\"cell-sm\">Subnet</div>\n <div class=\"cell-md\">\n <input type=\"text\" class=\"textbox-ip\" name=\"wifi_sub1\" value=\"##wifi_sub1##\">.\n <input type=\"text\" class=\"textbox-ip\" name=\"wifi_sub2\" value=\"##wifi_sub2##\">.\n <input type=\"text\" class=\"textbox-ip\" name=\"wifi_sub3\" value=\"##wifi_sub3##\">.\n <input type=\"text\" class=\"textbox-ip\" name=\"wifi_sub4\" value=\"##wifi_sub4##\">\n </div>\n </div>\n\n <div class=\"row row-sm text-left\">SMTP Email Configuration</div>\n <div class=\"row row-sm bg-info\">\n <div class=\"cell-sm\">Hostname</div>\n <div class=\"cell-md\"><input type=\"text\" class=\"textbox\" name=\"smtp_host\" value=\"##smtp_host##\"></div>\n </div>\n <div class=\"row row-sm bg-info\">\n <div class=\"cell-sm\">Port</div>\n <div class=\"cell-md\"><input type=\"text\" class=\"textbox\" name=\"smtp_port\" value=\"##smtp_port##\"></div>\n </div>\n <div class=\"row row-sm bg-info\">\n <div class=\"cell-sm\">Username</div>\n <div class=\"cell-md\"><input type=\"text\" class=\"textbox\" name=\"smtp_username\" value=\"##smtp_username##\"></div>\n </div>\n <div class=\"row row-sm bg-info\">\n <div class=\"cell-sm\">Password</div>\n <div class=\"cell-md\"><input type=\"password\" class=\"textbox\" name=\"smtp_password\" value=\"##smtp_password##\"></div>\n </div>\n <div class=\"row row-sm bg-info\">\n <div class=\"cell-sm\">From Email</div>\n <div class=\"cell-md\"><input type=\"text\" class=\"textbox\" name=\"smtp_fromEmail\" value=\"##smtp_fromEmail##\"></div>\n </div>\n <div class=\"row row-sm bg-info\">\n <div class=\"cell-sm\">From Name</div>\n <div class=\"cell-md\"><input type=\"text\" class=\"textbox\" name=\"smtp_fromName\" value=\"##smtp_fromName##\"></div>\n </div>\n <div class=\"row row-sm bg-info\">\n <div class=\"cell-sm\">TO Email</div>\n <div class=\"cell-md\"><input type=\"text\" class=\"textbox\" name=\"smtp_toEmail\" value=\"##smtp_toEmail##\"></div>\n </div>\n <br>\n\n <div class=\"row row-lg\">\n <div class=\"cell-sm empty\">.</div>\n <div class=\"cell-sm\"><input type=\"submit\" class=\"btn-in bg-info\" value=\"SAVE\"></div>\n <a href=\"/emailTest\" class=\"cell-sm btn-in bg-info\">Test</a>\n </div>\n <br>\n <br>\n </form>\n\n</body>\n</html>";
  
  EEPROM.get(addr, node_config);

  page.replace("##wifi_ssid##", (String)node_config.wifi_ssid);
  page.replace("##wifi_password##", (String)node_config.wifi_password);
  // ip
  page.replace("##wifi_ip1##", (String)node_config.wifi_ip[0]);
  page.replace("##wifi_ip2##", (String)node_config.wifi_ip[1]);
  page.replace("##wifi_ip3##", (String)node_config.wifi_ip[2]);
  page.replace("##wifi_ip4##", (String)node_config.wifi_ip[3]);
  // gateway
  page.replace("##wifi_gw1##", (String)node_config.wifi_gateway[0]);
  page.replace("##wifi_gw2##", (String)node_config.wifi_gateway[1]);
  page.replace("##wifi_gw3##", (String)node_config.wifi_gateway[2]);
  page.replace("##wifi_gw4##", (String)node_config.wifi_gateway[3]);
  // subnet
  page.replace("##wifi_sub1##", (String)node_config.wifi_subnet[0]);
  page.replace("##wifi_sub2##", (String)node_config.wifi_subnet[1]);
  page.replace("##wifi_sub3##", (String)node_config.wifi_subnet[2]);
  page.replace("##wifi_sub4##", (String)node_config.wifi_subnet[3]);

  // SMTP Configuration 
  page.replace("##smtp_host##", (String)node_config.smtp_host);
  page.replace("##smtp_port##", (String)node_config.smtp_port);
  page.replace("##smtp_username##", (String)node_config.smtp_username);
  page.replace("##smtp_password##", (String)node_config.smtp_password);
  page.replace("##smtp_fromEmail##", (String)node_config.smtp_fromEmail);
  page.replace("##smtp_fromName##", (String)node_config.smtp_fromName);
  page.replace("##smtp_toEmail##", (String)node_config.smtp_toEmail);

  return page;
}
// End Default page

void clearEEPROM()
{
  isActivated = false;
  EEPROM.put(0, isActivated);

  node_config.wifi_ssid[0] = '\0';
  node_config.wifi_password[0] = '\0';
  node_config.wifi_ip = IPAddress(0, 0, 0, 0);
  node_config.wifi_gateway = IPAddress(0, 0, 0, 0);
  node_config.wifi_subnet = IPAddress(255, 255, 255, 0);

  node_config.smtp_host[0] = '\0';     
  node_config.smtp_port = 25; 
  node_config.smtp_username[0] = '\0';    
  node_config.smtp_password[0] = '\0';
  node_config.smtp_fromEmail[0] = '\0';
  node_config.smtp_fromName[0] = '\0';
  node_config.smtp_toEmail[0] = '\0';

  EEPROM.put(addr, node_config);
  EEPROM.commit();
}

void sendEmail(String mailSubject, String mailBody) {

  ESPMailer* mail = new ESPMailer();
	
  /*
	 * -1 = muted
	 *  0 = only errors
	 *  1 = Client -> Server
	 *  2 = Server -> Client
	 *  3 = both
	 */
	mail->setDebugLevel(1);
	mail->Host = node_config.smtp_host;
  mail->Port = node_config.smtp_port;
	mail->SMTPAuth = true;
	mail->AuthType = LOGIN;
	mail->Username = node_config.smtp_username;
	mail->Password = node_config.smtp_password;
	mail->setFrom(node_config.smtp_fromEmail,node_config.smtp_fromName);
	mail->setTimezone(1); //defaults to UTC
	mail->addAddress(node_config.smtp_toEmail);
	//mail->addBCC("bcc@example.org");
	mail->Subject = mailSubject;
	mail->isHTML(false);
	mail->Body = mailBody;
	mail->AltBody = mailBody;
	if (mail->send()) {
		Serial.println("Mail sent sucessfully!");
  }
}