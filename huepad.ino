int IBRIDGE_PIN_COL0 = 16;
int IBRIDGE_PIN_COL1 = 5;
int IBRIDGE_PIN_COL2 = 4;
int IBRIDGE_PIN_ROW0 = 0;
int IBRIDGE_PIN_ROW1 = 2;
int IBRIDGE_PIN_ROW2 = 14;

void IBridge_GPIO_Config(void);

void IBridge_GPIO_Config()
{
	pinMode(IBRIDGE_PIN_COL0, OUTPUT);
	pinMode(IBRIDGE_PIN_COL1, OUTPUT);
	pinMode(IBRIDGE_PIN_COL2, OUTPUT);
	//pinMode(IBRIDGE_PIN_COL3, OUTPUT);

	pinMode(IBRIDGE_PIN_ROW0, INPUT);
	pinMode(IBRIDGE_PIN_ROW1, INPUT);
	pinMode(IBRIDGE_PIN_ROW2, INPUT);
 // pinMode(IBRIDGE_PIN_ROW3, INPUT);
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

void setup()
{
	Serial.begin(115200);
	IBridge_GPIO_Config();
	
}

void loop()
{
	uint8_t temp = getKeyWithDebounce(20);
	
	if ( temp > 0 ) {
		Serial.print(temp);
	}	
}

