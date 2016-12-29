#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <FS.h>
#include <ESP8266WebServer.h>

#define VERSION 6

//by how many do we adjust the brightness during dimming or brightening?
#define BRI_INC 20

/////////////////////////////////////////////////////////////////////

#define IBRIDGE_PIN_COL0 16
#define IBRIDGE_PIN_COL1 5
#define IBRIDGE_PIN_COL2 4
#define IBRIDGE_PIN_ROW0 0
#define IBRIDGE_PIN_ROW1 2
#define IBRIDGE_PIN_ROW2 14

void IBridge_GPIO_Config(void);

HTTPClient http;
ESP8266WebServer httpd(80);

String token = "";
String bridgeIP = "";
int lastSelectorButtonPressed = 0;
int lastButtonPressed = 0;
boolean actionDone = true;
int targetGroup[6+1];
const char * header = R"(<!DOCTYPE html>
<html>
<head>
<title>huepad</title>
<link rel="stylesheet" href="/style.css">
<meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no" />
</head>
<body>
<div id="top">
        <span id="title">huepad</span>
        <a href="/">Configuration</a>
        <!-- <a href="/debug">Debug</a> -->
</div>
)";

void IBridge_GPIO_Config()
{
	pinMode(IBRIDGE_PIN_COL0, OUTPUT);
	pinMode(IBRIDGE_PIN_COL1, OUTPUT);
	pinMode(IBRIDGE_PIN_COL2, OUTPUT);

	pinMode(IBRIDGE_PIN_ROW0, INPUT);
	pinMode(IBRIDGE_PIN_ROW1, INPUT);
	pinMode(IBRIDGE_PIN_ROW2, INPUT);
}

uint8_t IBridge_Read_Key()
{
	//unsigned char i = 10;

	//Serial.println("Column 0 scan");

	
	digitalWrite(IBRIDGE_PIN_COL0, HIGH);
	digitalWrite(IBRIDGE_PIN_COL1, HIGH);
	digitalWrite(IBRIDGE_PIN_COL2, LOW);

	if(!(digitalRead(IBRIDGE_PIN_ROW0)) &&	(digitalRead(IBRIDGE_PIN_ROW1)) && (digitalRead(IBRIDGE_PIN_ROW2)))
		return (7);
	if((digitalRead(IBRIDGE_PIN_ROW0)) && !(digitalRead(IBRIDGE_PIN_ROW1)) && (digitalRead(IBRIDGE_PIN_ROW2)))
		return (8);
	if((digitalRead(IBRIDGE_PIN_ROW0)) && (digitalRead(IBRIDGE_PIN_ROW1)) && !(digitalRead(IBRIDGE_PIN_ROW2)))
		return (9);

	//Serial.println("Column 1 scan");
		
	digitalWrite(IBRIDGE_PIN_COL0, HIGH);
	digitalWrite(IBRIDGE_PIN_COL1, LOW);
	digitalWrite(IBRIDGE_PIN_COL2, HIGH);

	if(!(digitalRead(IBRIDGE_PIN_ROW0)) &&	(digitalRead(IBRIDGE_PIN_ROW1)) && (digitalRead(IBRIDGE_PIN_ROW2)))
		return (4);
	if((digitalRead(IBRIDGE_PIN_ROW0)) && !(digitalRead(IBRIDGE_PIN_ROW1)) && (digitalRead(IBRIDGE_PIN_ROW2)))
		return (5);
	if((digitalRead(IBRIDGE_PIN_ROW0)) && (digitalRead(IBRIDGE_PIN_ROW1)) && !(digitalRead(IBRIDGE_PIN_ROW2)))
		return (6);

	//Serial.println("Column 2 scan");
		
	digitalWrite(IBRIDGE_PIN_COL0, LOW);
	digitalWrite(IBRIDGE_PIN_COL1, HIGH);
	digitalWrite(IBRIDGE_PIN_COL2, HIGH);

	if(!(digitalRead(IBRIDGE_PIN_ROW0)) &&	(digitalRead(IBRIDGE_PIN_ROW1)) && (digitalRead(IBRIDGE_PIN_ROW2)))
		return (1);
	if((digitalRead(IBRIDGE_PIN_ROW0)) && !(digitalRead(IBRIDGE_PIN_ROW1)) && (digitalRead(IBRIDGE_PIN_ROW2)))
		return (2);
	if((digitalRead(IBRIDGE_PIN_ROW0)) && (digitalRead(IBRIDGE_PIN_ROW1)) && !(digitalRead(IBRIDGE_PIN_ROW2)))
		return (3);	
		
	return(0);

}

uint8_t getKeyWithDebounce(int ms) {
	uint8_t x = IBridge_Read_Key();
	delay(ms);
	uint8_t y = IBridge_Read_Key();
	if ( x == y ) {
		return x;
	}
}

void spiffsWrite(String path, String contents) {
	File f = SPIFFS.open(path, "w");
	f.println(contents);
	f.close();
}
String spiffsRead(String path) {
	File f = SPIFFS.open(path, "r");
	String x = f.readStringUntil('\n');
	f.close();
	return x;
}

int loadTargetGroupForButton(int button) {
	String path = String("/buttons/") + button;
	Serial.print("path="); Serial.println(path);
	if ( SPIFFS.exists(path) ) {
		String temp = spiffsRead(path);
		Serial.print("button "); Serial.print(button); Serial.print(" controls group "); Serial.println(temp);
		return temp.toInt();
	} else {
		Serial.print("Returning default for button "); Serial.println(button);
		return button; //default to the same group number as the button pressed.
	}
}
void loadTargetGroupForButtons() {
	for ( int i=1; i<=6; i++ ) {
		targetGroup[i] = loadTargetGroupForButton(i);
		Serial.print("button "); Serial.print(i); Serial.print("="); Serial.println(targetGroup[i]);
	}
}
void setTargetGroupForButton(int button, String target) {
	Serial.print("save: button "); Serial.print(button); Serial.print(" controls group "); Serial.println(target);
	spiffsWrite(String("/buttons/") + button, target);
}

String jsonPseudoDecode(String json, String key) {
	//I HATE MYSELF
	int offset = json.indexOf(key)+key.length()+2;
	if ( json.charAt(offset) == '"' ) {
		offset++; //it's a string in the json, so we increment the offset.
	}
	String result = json.substring(offset);
    return result.substring(0,result.indexOf('"'));
}


boolean hueIsPaired() {
	Serial.print("checking pairing to bridge at "); Serial.println(bridgeIP); //Serial.print(": ");
	String url = String("http://") + bridgeIP + "/api/" + token + "/lights";
	//Serial.println(url);

	http.begin(url);
	int code = http.GET();
	Serial.print(code);
	String result = http.getString();
	http.end();

	if ( result.indexOf("error") != -1 ) {
		Serial.println(" - we're not paired to the Hue bridge");
		return false;
	}
	Serial.println(" - We are paired to the bridge");
	return true;
}
void huePair() {
	http.begin(bridgeIP, 80, String("/api"));
	
	//get MAC - thanks https://github.com/esp8266/Arduino/issues/313
	byte mac[6];
	WiFi.macAddress(mac);
	String cMac = "";
	for (int i = 0; i < 6; ++i) {
		cMac += String(mac[i],HEX);
		//if(i<5) cMac += "-";
	}

	String post = String("{\"devicetype\":\"huepad#") + cMac + "\"}";
	Serial.print("post: "); Serial.println(post);
	int code = http.POST(post);
	Serial.print("pairing request got "); Serial.print(code); Serial.print(": ");
	String result = http.getString();
	Serial.println(result);
	if ( result.indexOf("success") != -1 ) {
		//we were paired.
		token = jsonPseudoDecode(result, "username");
		Serial.print("paired - got token "); Serial.println(token);
		spiffsWrite("/token", token);
		Serial.println("Wrote token to spiffs.");
	}
	http.end();
}
boolean hueGetGroupState(int number) {
	http.begin(bridgeIP, 80, String("/api/") + token + "/groups/" + String(number));
	int code = http.GET();
	String result = http.getString();
	http.end();
	Serial.print("checking group "); Serial.print(number); Serial.print(" - "); Serial.println(result);
	result = jsonPseudoDecode(result, "any_on");
	return result.startsWith("true");
}
void hueSetGroupState(int number, boolean desiredState) {
	http.begin(bridgeIP, 80, String("/api/") + token + "/groups/" + String(number) + "/action");
	String data = String("{\"on\":") + (desiredState ? "true" : "false") + ", \"effect\":\"none\"}";
	int code = http.PUT(data);
	Serial.print("setting group "); Serial.print(number); Serial.print(" - "); Serial.print(data); Serial.print(" - result: ");
	String result = http.getString();
	Serial.println(result);
	http.end();
}
void hueSetGroupEffect(int number) {
	http.begin(bridgeIP, 80, String("/api/") + token + "/groups/" + String(number) + "/action");
	String data = "{\"effect\":\"colorloop\"}";
	int code = http.PUT(data);
	Serial.print("setting group "); Serial.print(number); Serial.print(" - "); Serial.print(data); Serial.print(" - result: ");
	String result = http.getString();
	Serial.println(result);
	http.end();
}

void hueToggleGroupState(int number) {
	hueSetGroupState(number, !hueGetGroupState(number));
}
void hueIncrementGroupBrightness(int number, int inc) {
	http.begin(bridgeIP, 80, String("/api/") + token + "/groups/" + String(number) + "/action");
	String data = String("{\"bri_inc\":") + inc + ", \"transitiontime\":2}";
	int code = http.PUT(data);
	Serial.print("setting group "); Serial.print(number); Serial.print(" - "); Serial.print(data); Serial.print(" - result: ");
	String result = http.getString();
	Serial.println(result);
	http.end();
	delay(200); //bridge cooldown
}

void setup()
{
	Serial.begin(115200);
	Serial.println();
	delay(200);
	Serial.print("huepad v"); Serial.println(VERSION);

	Serial.println("Starting keypad");
	IBridge_GPIO_Config();

	if ( getKeyWithDebounce(1000) == 5 ) {
		Serial.print("Button 5 was held. Forgetting everything - this could take a while");
		WiFi.disconnect(); Serial.print(".");
		SPIFFS.format(); Serial.print(".");
		Serial.println(" done. Reloading");
		ESP.restart();
	}

	Serial.print("Mounting disk... ");
	FSInfo fs_info;
	SPIFFS.begin();
	if ( ! SPIFFS.info(fs_info) ) {
		//the FS info was not retrieved correctly. it's probably not formatted
		Serial.print("failed. Formatting disk... ");
		SPIFFS.format();
		Serial.print("done. Mounting disk... ");
		SPIFFS.begin();
		SPIFFS.info(fs_info);
	}
	Serial.print("done. ");
	Serial.print(fs_info.usedBytes); Serial.print("/"); Serial.print(fs_info.totalBytes); Serial.println(" bytes used");
	
	Serial.println("Loading buttons");
	loadTargetGroupForButtons();

	Serial.println("Checking version");
	String lastVerString = spiffsRead("/version");
	if ( lastVerString.toInt() != VERSION ) {
		//we just got upgrayedded or downgrayedded
		Serial.print("We just moved from v"); Serial.println(lastVerString);
		spiffsWrite("/version", String(VERSION));
		Serial.print("Welcome to v"); Serial.println(VERSION);
	}

	Serial.println("Starting wireless.");
	WiFi.hostname("huepad");
	WiFiManager wifiManager; //Load the Wi-Fi Manager library.
	wifiManager.setTimeout(300); //Give up with the AP if no users gives us configuration in this many secs.
	if(!wifiManager.autoConnect("huepad Setup")) {
		Serial.println("failed to connect and hit timeout");
		delay(3000);
		ESP.restart();
	}
	Serial.print("WiFi connected: ");
	Serial.println(WiFi.localIP());

	Serial.println("Starting Web server.");
	httpd.on("/buttons.json", HTTP_POST, [&](){
		Serial.println("Saving button configs.");
		for ( int i=1; i<=6; i++ ) {
			String value = httpd.arg(String(i));
			Serial.print(i); Serial.print("="); Serial.println(value);
			//spiffsWrite(String("/buttons/") + i, value);
			setTargetGroupForButton(i, value);
		}
		Serial.println("done. Reloading button assignments.");
		loadTargetGroupForButtons();
		httpd.sendHeader("Location", "/?saved");
		httpd.send(302, "application/json", "");
		httpd.client().stop();
	});
	httpd.on("/style.css", [&](){
		httpd.send(200, "text/css",R"(
			html {
				font-family:sans-serif;
				background-color:black;
				color: #e0e0e0;
			}
			div {
				background-color: #202020;
			}
			h1,h2,h3,h4,h5 {
				color: #2020e0;
			}
			a {
				color: #50f050;
			}
			form * {
				display:block;
				border: 1px solid #000;
				font-size: 14px;
				color: #fff;
				background: #004;
				padding: 5px;
			}
		)");
		httpd.client().stop();
	});
	httpd.on("/", [&](){
		httpd.setContentLength(CONTENT_LENGTH_UNKNOWN);
		httpd.send(200, "text/html", header);
		httpd.sendContent(R"(<h1>Configure buttons</h1><form method="POST" action="/buttons.json">)");
		for ( int i=1; i<=6; i++ ) {
			httpd.sendContent(String("Button ") + i + ": <select name=\"" + i + "\">");
			for ( int j=0; j<=16; j++ ) {
				httpd.sendContent(String("<option value=\"") + j + "\">" + (j==0 ? "All Lights" : (String("Group ") + j) ) + "</option>");
			}
			httpd.sendContent("</select>");
		}
		httpd.sendContent("<button type=\"submit\">Save</button></form>");
		httpd.client().stop();
	});
	httpd.begin();

	Serial.println("Asking for your Hue bridges using N-UPnP hack... ");
	WiFiClientSecure client;
	boolean status = client.connect("www.meethue.com",443);
	if ( ! status ) {
		Serial.println("Sorry, but I could not connect to Philips. I am unable to go any further.");
		delay(60000);
		Serial.println("Restarting to try again.");
		ESP.restart();
		return;
	}
	client.print(R"(GET /api/nupnp HTTP/1.1
Host: www.meethue.com
User-Agent: huepad
Connection: close

)");
	Serial.print("Receiving headers");
	while ( client.readStringUntil('\n') != "\r" ) {
		Serial.print(".");
	}
	Serial.println(" receiving data... ");
	String lineStr = client.readStringUntil('\n');
	//Serial.println(lineStr);

	//oh boy
	int temp = lineStr.indexOf("internalipaddress");
	if ( temp < 0 ) {
		Serial.println("Sorry, but Philips says you don't have any bridges. I am unable to go any further.");
		delay(60000);
		Serial.println("Restarting to try again.");
		ESP.restart();
		return;
	}
	bridgeIP = lineStr.substring(temp+17+3);
	bridgeIP = bridgeIP.substring(0,bridgeIP.indexOf('"'));
	Serial.print("Your first bridge is at "); Serial.println(bridgeIP);
	client.stop();

	Serial.print("Loading token from disk... ");
	if ( SPIFFS.exists("/token") ) {
		token = spiffsRead("/token");
		Serial.println(token);
	} else {
		Serial.println("Not found");
	}

	http.setReuse(true);
	Serial.print("Talking to Hue bridge... ");
	while ( !hueIsPaired() ) {
		Serial.println("bridge is not paired. Starting pairing");
		huePair();
		Serial.println("Waiting for bridge to settle.");
		delay(10000);
	}


	Serial.println("Startup complete - helo frien");
}

void loop()
{
	
	httpd.handleClient();

	uint8_t button = getKeyWithDebounce(20);
	
	if ( button > 0 && button <= 6 ) {
		if ( lastButtonPressed == 0 ) {
			//This is a new button press, so make it clear that we haven't done anything for this press yet.
			actionDone = false;
		}
		lastSelectorButtonPressed = button;
	}
	if ( button == 0 && !actionDone ) {
		//the user pressed a button, then released it
		hueToggleGroupState(targetGroup[lastSelectorButtonPressed]);
		actionDone = true;
	} 
	if ( button == 7 ) {
		//dim that group
		hueIncrementGroupBrightness(targetGroup[lastSelectorButtonPressed], -BRI_INC);
		actionDone = true;
	}
	if ( button == 8 ) {
		//party-mode that group
		hueSetGroupEffect(targetGroup[lastSelectorButtonPressed]);
		actionDone = true;
	}
	if ( button == 9 ) {
		//brighten that group
		hueIncrementGroupBrightness(targetGroup[lastSelectorButtonPressed], BRI_INC);
		actionDone = true;
	}

	lastButtonPressed = button;
}



