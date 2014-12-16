
#include <SoftwareSerial.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Fat16.h>
#include <Fat16util.h> 

#if defined(ARDUINO) && ARDUINO >= 100
#define printByte(args)  write(args);
#else
#define printByte(args)  print(args,BYTE);
#endif

uint8_t huhn[8]  = {
  0x0,0xc,0x1d,0xf,0xf,0x6,0x0};
        
byte a;  
byte i = 0;
boolean buttonState1 = 0;            
boolean buttonState2 = 0;   
boolean buttonState3 = 0; 
byte programm = NULL;   
boolean programmende = true;
byte chosen1 = 0;
byte chickenNumber = NULL;      
unsigned long start;
unsigned long zeit;
unsigned long timewarp = 0;
byte pick;
byte futter;                  
byte myArray [10]; 
byte erfolge = 0; 
int xAxis;
int yAxis;
byte RD;
byte checksum;
byte zeichen;  // testweise byte statt int
byte rest;
char sdZeichen;
String sdWort;
byte picksRD = 0;
unsigned long picksRDmillis = 0;
unsigned long pickPause = 0;
byte frikasse[20][6];

Servo myServo;              
SoftwareSerial mySerial(8, 9, true); 
LiquidCrystal_I2C lcd(0x27,16,2);  
SdCard card;
Fat16 file;
#define error(s) error_P(PSTR(s))

void setup()
{
  pinMode(6, OUTPUT);
  pinMode(7, OUTPUT);
  pinMode(4, INPUT);
  pinMode(2, INPUT);
  pinMode(4, INPUT);

  digitalWrite (7, HIGH);
  delay(1000);
  digitalWrite (7, LOW);

  lcd.init(); 
  lcd.backlight();
  lcd.createChar(1, huhn);  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("ChickenBox v3.3");
  lcd.setCursor(0,1);
  lcd.print("(c)2014 O. Krohn");
  delay(3500);

  Serial.begin(9600);
  mySerial.begin(9600); 

  // SD-Karte
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Initialisierung");
  lcd.setCursor(0,1);
  if (!card.init()) error("card.init");
  if (!Fat16::init(&card)) error("Fat16::init");
  if (file.open("memory.TXT", O_READ)) {
  } 
  else {
    error("file.open");
  }
  int16_t c;
  for (a=0; a<20; a++){
    for (i=0; i<6; i++){
      do {
        sdZeichen = file.read();
        sdWort = String(sdWort+sdZeichen);
      } 
      while (sdZeichen != ',');
      frikasse[a][i] = sdWort.toInt();
      sdWort="0";
    }
  }
  lcd.print("lade Datei... ");
  file.close();
}

void loop()
{
  if (chosen1 == 0){
    menu1();
  }
  if (chosen1 == 1){
    menu2();
    futter = 1;
  }
  if (chosen1 == 2){
    if (programmende == true){
      programmende = false;
      chosen1 = 0;
      timewarp = 0;   // ebenso
      pick = 0;
      for (a=0; a<45;a++){  
        rest=mySerial.read();
      }
    }
    lcd.clear();
    lcd.setCursor(0,0);
    switch (programm-1){
      
    case 0:
      digitalWrite (7, HIGH);
      lcd.setCursor(0,0);
      lcd.print("Shaping         ");    
      counter();
      while (programmende == false){
        lcd.setCursor(0,1);
        lcd.print("Picks: ");
        lcd.print(erfolge); 
        lcd.print("  ");    
        readData();  
        switch (pick) {
        case 1:
          pick=0;
          chickenLogger();
        }
        buttonRead();
        if (buttonState1 == 1) {
          digitalWrite (7, LOW);
          programmende = true;
          frikasse[chickenNumber-1][programm-1] = frikasse[chickenNumber-1][programm-1]++;
          sdwrite();
          erfolge = 0;
          delay(500); // testweise
        }
      }
      break;      

    case 1:
      start = millis();
      zeit = 0; 
      timewarp = 0;  
      RD = 0;
      lcd.setCursor(0,0);
      lcd.print("Auto 1 ");       
      programmHaupt();
      break;  

    case 2:
      start = millis();
      zeit = 0; 
      timewarp = 0;  
      RD = 2500;
      lcd.setCursor(0,0);
      lcd.print("Auto 2 ");       
      programmHaupt();
      break;  

    case 3:
      start = millis();
      zeit = 0; 
      timewarp = 0;  
      RD = 5000;
      lcd.setCursor(0,0);
      lcd.print("Auto 3 ");
      programmHaupt();
      break;  

    case 4:  // Diskr.+
      start = millis();
      zeit = 0; // Diagnose?
      timewarp = 0;  
      RD = 5000;
      lcd.setCursor(0,0);
      lcd.print("Dis. + ");     
      programmHaupt();
      break;
      
    case 5:    // Dis-
      start = millis();
      zeit = 0; // Diagnose?
      timewarp = 0;  
      RD = 5000;
      lcd.setCursor(0,0);
      lcd.print("Dis. - ");      
      programmHaupt();
      break;
    }
  } 
}

void buttonRead(){
  // Liest den Status der Taster

  buttonState1 = 0;               // Variable fuer den grossen Taster
  buttonState2 = 0;               // Variable fuer den kleinen Taster links
  buttonState3 = 0;               // Variable fuer den kleinen Taster rechts

  buttonState1 = digitalRead(4);
  buttonState2 = digitalRead(2);
  buttonState3 = digitalRead(3);
  delay(150);
}

void readData(){
  // Einlesen des Datenpakets
  zeichen = mySerial.available();
  if (zeichen > 0) {
    if ((programm-1) < 5) {          // eingefuegt, weil Koordinaten nur im Shaping-modus interessant sind
      myArray[0,1,2,3,4,5,6,7,8,9] = 0,0,0,0,0,0,0,0,0,0;
      myArray[0]=mySerial.read();
      if (myArray[0] == 0x55){
        myArray[1]=mySerial.read();
        if (myArray[1] == 0x54){
          myArray[2]=mySerial.read();
          if (myArray[2]==1 || myArray[2]==2 || myArray[2]==4){
            for (a=3; a<10;a++){
              myArray[a]=mySerial.read();
            }
            checksum = myArray[1]+myArray[2]+myArray[3]+myArray[4]+myArray[5]+myArray[6]+myArray[7]+myArray[8]-1;
            if (myArray[9] == checksum && myArray[8] == 0 && myArray[4] <= 0x0F && myArray[6]<=0x0F){
              xAxis = word(myArray[4],myArray[3])+(-480); 
              yAxis = word(myArray[6],myArray[5])+(-610);  
              i++;
            }
            else {
              delay(20); // vorher 30, danach 5
            }
            if (i>1) { 
              erfolge++;
              pick = 1;
              chickenLogger();
              for (a=0; a<45;a++){ 
                rest=mySerial.read();
              }
              i=0;
            }
          }
        }
        else {
          delay(20); 
        }
      }
    }
  }
}


void menu1(){   
  do {   
    buttonRead();
    if ((buttonState1 == 1) && (buttonState2 == 1)){
      lcd.clear();
      lcd.setCursor(0,0); 
      lcd.print("Speicher nullen?");
      lcd.setCursor(0,1); 
      lcd.print("(rechts) druecken");
      delay(1000);
      buttonRead();
      if (buttonState3 == 1){
        for (a=0; a<20; a++){
          for (i=0; i<6; i++){
            frikasse[a][i] = 0;
          }
        }
        sdwrite();
      }


      break;
    }    
    switch (buttonState1){
    case 1: 
      digitalWrite(6, HIGH);
      chosen1 = 1;
      delay(100);
      return;
      break;
    case 0: 
      digitalWrite(6, LOW);
    }
    switch (buttonState2){
    case 1:
      programm--;
      delay(10);
      break;
    case 0: 
      break;
    }
    switch (buttonState3){
    case 1:
      programm++;
      delay(10);
      break;
    case 0:
      break;
    }


    if (programm < 1){
      programm=6;
    }
    if (programm > 6){
      programm=1;
    }
    lcd.setCursor(0,0);
    lcd.print("Modusauswahl:   ");
    lcd.setCursor(0,1);
    switch (programm-1){
    case 0:
      lcd.print("Shaping         ");
      break;      
    case 1:
      lcd.print("Autoshaping I   ");
      break;
    case 2:
      lcd.print("Autoshaping II  ");
      break;    
    case 3:
      lcd.print("Autoshaping III ");
      break;
    case 4:
      lcd.print("Diskr. +        ");
      break;
    case 5:
      lcd.print("Diskr. -        "); 
    }
  }
  while (chosen1 = 0);
}

void menu2(){
  delay(200);
  do {   
    buttonRead();
    if ((buttonState2 == 1) && (buttonState3 == 1)){
      chosen1 = 0;
      break;
    }
    switch (buttonState1){
    case 1:                         // Taster gedrueckt, und nu?
      digitalWrite(6, HIGH);
      chosen1 = 2;
      delay(100); // test hilfs delay
      return;
      digitalWrite(6, LOW);
      break;
    case 0:                         // Taster nicht gedrueckt
      digitalWrite(6, LOW);   
    }
    switch (buttonState2){
    case 1:                         // Taster gedrueckt
      chickenNumber--;
      delay(10);
      break;
    case 0:                         // Taster nicht gedrueckt
      break;
    }
    switch (buttonState3){
    case 1:          // Taster gedrueckt
      chickenNumber++;
      delay(10);
      break;
    case 0:                         // Taster nicht gedrueckt
      break;
    }
    if (chickenNumber < 1){
      chickenNumber=20;
    }
    if (chickenNumber > 20){
      chickenNumber=1;
    }
    lcd.setCursor(0,0);
    lcd.printByte(1);
    lcd.print(":       ");
    lcd.setCursor(3,0); 
    lcd.print(chickenNumber);
    lcd.setCursor(9,0); 
    switch (programm-1){
    case 0:
      lcd.print("Shaping");
      break;      
    case 1:
      lcd.print("Auto 1 ");
      break;
    case 2:
      lcd.print("Auto 2 ");
      break;
    case 3:
      lcd.print("Auto 3 ");
      break;
    case 4:
      lcd.print("Dis.+  ");
    case 5:
      lcd.print("Dis.-  ");
    } 
    lcd.setCursor(0,1);
    lcd.print(frikasse[chickenNumber-1][0]);
    lcd.print(",");
    lcd.print(frikasse[chickenNumber-1][1]);
    lcd.print(",");
    lcd.print(frikasse[chickenNumber-1][2]);
    lcd.print(",");
    lcd.print(frikasse[chickenNumber-1][3]);
    lcd.print(",");
    lcd.print(frikasse[chickenNumber-1][4]);
    lcd.print(",");
    lcd.print(frikasse[chickenNumber-1][5]);
    lcd.print("     ");
  }
  while (chosen1 = 1);
}

void chickenLogger(){
  Serial.print("#S|CHICKENLOG|[");
  Serial.print(chickenNumber);
  Serial.print(";");
  switch (programm-1){
  case 0: 
    Serial.print("Shaping"); 
    break;    
  case 1: 
    Serial.print("Auto1"); 
    break;
  case 2: 
    Serial.print("Auto2"); 
    break;
  case 3: 
    Serial.print("Auto3"); 
    break;
  case 4: 
    Serial.print("Dis+"); 
    break;
  case 5: 
    Serial.print("Dis-"); 
    break;
  }
  Serial.print(";");  
  Serial.print(frikasse[chickenNumber-1][programm-1]+1);  // öhm programm durch programm-1 ersetzt 
  Serial.print(";");
  Serial.print(erfolge);
  Serial.print(";");  
  Serial.print(xAxis);
  Serial.print(";");
  Serial.print(yAxis);
  Serial.print(";");
  Serial.print(zeit);
  Serial.print(";");  
  Serial.print(timewarp); 
  Serial.print(";");  
  Serial.print(picksRD); 
  Serial.print(";");  
  Serial.print(picksRDmillis); 
  Serial.print(";");  
  Serial.print(pickPause); 
  Serial.println("]#");
}

void chickenFeeder(){
  myServo.attach(5);
  myServo.write(1);
  delay(500);  
  myServo.write(90);
  delay(500);
  myServo.write(1);
  delay(500);
  myServo.detach();
}

void counter(){
  lcd.setCursor(7,0);
  lcd.printByte(1);
  lcd.print(":");
  lcd.print(chickenNumber);
  lcd.setCursor(12,0);
  lcd.print("D:");
  lcd.print(frikasse[chickenNumber-1][programm-1]+1); 
}

void error_P(const char*  str) {
  PgmPrint("error: ");
  SerialPrintln_P(str);
  if (card.errorCode) {
    PgmPrint("SD error: ");
  }
  while(1);
}

void writeNumber(uint32_t n) {
  uint8_t buf[10];
  uint8_t i = 0;
  do {
    i++;
    buf[sizeof(buf) - i] = n%10 + '0';
    n /= 10;
  } 
  while (n);
  file.write(&buf[sizeof(buf) - i], i); 
}

void sdwrite() { 
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Speicherung");
  if (!card.init()) error("card.init");
  if (!Fat16::init(&card)) error("Fat16::init");
  char name[] = "memory.TXT";
  lcd.setCursor(0,1);
  lcd.print(name);
  for (uint8_t i = 0; i < 100; i++) {
    if (file.open(name, O_CREAT | O_WRITE)) break;
  }
  if (!file.isOpen()) error ("file.open");

  for (a=0; a<20; a++){
    for (i=0; i<6; i++){
      file.print(frikasse[a][i]);
      file.write(",");
    }
  }
  file.close();
  lcd.print("...ok!");
  delay(500); 
}

void programmHaupt(){
        while ((zeit + timewarp) < 40000) {  
          programmPauseStart();
          programmReinforcementDelay();
          programmStimulus();        
        }
        programmFutter();
        programmPauseSchluss();
}      

void programmPauseStart() {
        while ((zeit + timewarp) < 30000) {
          readData(); 
          switch (pick) {
          case 1:
            pickPause = zeit;
            timewarp = 30000 - zeit; 
            pick = 0;
          }
          digitalWrite (7, LOW);
          counter();
          lcd.setCursor(0,1);
          lcd.print("Pause ");
          lcd.print(30-((zeit + timewarp)/1000));
          lcd.print(" s  ");
          lcd.setCursor(10,1);
          lcd.print("      ");
          zeit = millis() - start;      
        }
}

void programmReinforcementDelay(){
        while ((zeit + timewarp) > 30000 && (zeit + timewarp) < (30000 + RD) ) {
          pickPause = 0; 
          readData();
          switch (pick) {
          case 1:
            picksRD++;
            picksRDmillis = zeit;
            pick = 0; 
          }  
          digitalWrite (7, HIGH);   
          lcd.setCursor(0,1);
          lcd.print("RD       ");
          lcd.print(40-((zeit + timewarp)/1000));
          lcd.print(" s  ");
          lcd.setCursor(13,1);
          lcd.print("      ");
          zeit = millis()-start;      
        }   
}

void programmStimulus(){
        while ((zeit + timewarp) > (30000 + RD) && (zeit + timewarp) < 40000 ) {
          picksRD = 0;
          picksRDmillis = 0;
          readData();
          switch (pick) {
          case 1:
            timewarp = timewarp + ( 10000 - (( zeit + timewarp) - 30000 )); 
            pick = 0;
          }
          digitalWrite (7, HIGH);   
          lcd.setCursor(0,1);
          lcd.print("Stimulus ");
          lcd.print(40-((zeit + timewarp)/1000));
          lcd.print(" s  ");
          lcd.setCursor(13,1);
          lcd.print("      ");
          zeit = millis()-start;      
        }
}

void programmFutter(){
       while ((zeit + timewarp) > 40000 && (zeit + timewarp) < 45000 ) {
        digitalWrite (7, HIGH);
        if ((futter == 1) && ((programm-1) >= 1) && ((programm-1) <= 4)) {   // moment ... stimmt das  ??????
          pick = 0; 
          futter = 0;
          chickenFeeder();
        }
        lcd.setCursor(0,1);
        lcd.print("Futter ");
        lcd.print(45-((zeit + timewarp)/1000));
        lcd.print(" s  ");
        lcd.setCursor(11,1);
        lcd.print("      ");
        zeit = millis()-start;      
      } 
}

void programmPauseSchluss(){
        while ((zeit + timewarp) > 45000 && (zeit + timewarp) < 75000 ) {
        digitalWrite (7, LOW);
        lcd.setCursor(0,1);
        lcd.print("Pause ");
        lcd.print(75-((zeit + timewarp)/1000));
        lcd.print(" s  ");
        lcd.setCursor(10,1);
        lcd.print("      ");   
        zeit = millis()-start;              
      }
      if (((zeit + timewarp) > 74500) && ((zeit + timewarp) < 76000)) {  
        programmende = true;
        timewarp = 0;
        pick = 0; 
        erfolge = 0;
        chickenLogger();
        frikasse[chickenNumber-1][programm-1] = frikasse[chickenNumber-1][programm-1]++;
        sdwrite();
      }      
    }

