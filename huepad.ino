#include <WiFiManager.h>
#include <ESP8266HTTPClient.h>
#include <FS.h>

#define IBRIDGE_PIN_COL0 16
#define IBRIDGE_PIN_COL1 5
#define IBRIDGE_PIN_COL2 4
#define IBRIDGE_PIN_ROW0 0
#define IBRIDGE_PIN_ROW1 2
#define IBRIDGE_PIN_ROW2 14

//by how many do we adjust the brightness during dimming or brightening?
#define BRI_INC 5

void IBridge_GPIO_Config(void);

HTTPClient http;

String token = "";
String bridgeIP = "";

int lastSelectorButtonPressed = 0;
int lastButtonPressed = 0;
boolean actionDone = true;

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
	String data = String("{\"bri_inc\":") + inc + "}";
	int code = http.PUT(data);
	Serial.print("setting group "); Serial.print(number); Serial.print(" - "); Serial.print(data); Serial.print(" - result: ");
	String result = http.getString();
	Serial.println(result);
	http.end();
}

void setup()
{
	Serial.begin(115200);
	Serial.println();
	delay(200);
	Serial.println("huepad");

	Serial.println("Starting keypad");
	IBridge_GPIO_Config();

	if ( getKeyWithDebounce(1000) == 5 ) {
		Serial.println("Button 5 was held. Forgetting everything - this could take a while.");
		WiFi.disconnect();
		SPIFFS.format();
	}

	Serial.println("Starting wireless.");
	WiFiManager wifiManager; //Load the Wi-Fi Manager library.
	wifiManager.setTimeout(300); //Give up with the AP if no users gives us configuration in this many secs.
	if(!wifiManager.autoConnect()) {
		Serial.println("failed to connect and hit timeout");
		delay(3000);
		ESP.restart();
	}
	Serial.print("WiFi connected: ");
	Serial.println(WiFi.localIP());

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
		hueToggleGroupState(lastSelectorButtonPressed-1);
		actionDone = true;
	} 
	if ( button == 7 ) {
		//dim that group
		hueIncrementGroupBrightness(lastSelectorButtonPressed-1, -BRI_INC);
		actionDone = true;
	}
	if ( button == 8 ) {
		//party-mode that group
		hueSetGroupEffect(lastSelectorButtonPressed-1);
		actionDone = true;
	}
	if ( button == 9 ) {
		//brighten that group
		hueIncrementGroupBrightness(lastSelectorButtonPressed-1, BRI_INC);
		actionDone = true;
	}

	lastButtonPressed = button;
}

