#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <DHT_U.h>
#include <BlynkSimpleEsp8266.h>
#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Fuzzy.h>


#define BLYNK_PRINT Serial
#define BLYNK_TEMPLATE_ID "TMPL6Ph3aLENR"
#define BLYNK_TEMPLATE_NAME "Monitoring Ruangan"
#define BLYNK_AUTH_TOKEN "t4oj2x-sMryDItY3kIB9c2XiqeIrzjhx"
#define DHTPIN 13
#define DHTTYPE DHT11
#define ONE_WIRE_BUS 14
#define relay 12

const char* ssid = "kedaiKOi.2016";
const char* pass = "ngopangopi";       

LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
WidgetLCD vlcd(V0);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds(&oneWire);
Fuzzy *fuzzy = new Fuzzy();


// FuzzyInput Suhu
FuzzySet *dingin = new FuzzySet(-10, 5, 5, 20);
FuzzySet *sedang = new FuzzySet(15, 30, 30, 45);
FuzzySet *panas = new FuzzySet(35, 50, 50, 65);

// FuzzyInput Kelembaban
FuzzySet *kering = new FuzzySet(0, 30, 30, 60);
FuzzySet *lembab = new FuzzySet(50, 70, 70, 90);
FuzzySet *basah = new FuzzySet(70, 100, 100, 150);

// FuzzyOutput
FuzzySet *off = new FuzzySet(0, 0, 0, 0);
FuzzySet *on = new FuzzySet(1, 1, 1, 1);

void setup() {
  Serial.begin(9600);
  pinMode(relay, OUTPUT); digitalWrite(relay, HIGH);
  dht.begin(); ds.begin(); konfigFuzzy();
  lcd.begin(16, 2); lcd.init(); lcd.backlight();
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    lcd.setCursor(0, 0); lcd.print("Menghubungkan....");
  }
  Serial.println("Connected to WiFi");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  lcd.setCursor(0, 1); lcd.print("terhubung");
}

void loop() {
  Blynk.run(); lcd.clear();

  float suhu = bacaSuhu(); fuzzy->setInput(1, suhu);
  float kelembapan = bacaKelembapan(); fuzzy->setInput(2, kelembapan);
  fuzzy->fuzzify(); 
  float output = fuzzy->defuzzify(1);
  Serial.print("Suhu : "); Serial.print(suhu); statusSuhu();
  Serial.print("\tKelembapan : "); Serial.print(kelembapan); statusKelembapan();
  Serial.print("\tOutput : "); Serial.println(output);

  lcd.clear(); lcd.setCursor(0, 0); lcd.print("S:"); lcd.print(suhu); lcd.setCursor(9, 0); lcd.print("O:"); lcd.print(output);
  lcd.setCursor(0, 1); lcd.print("K:"); lcd.print(kelembapan);

  Blynk.virtualWrite(V1, suhu); 
  Blynk.virtualWrite(V2, kelembapan); 
  Blynk.virtualWrite(V3, output);
  lcd.setCursor(9, 1); 
  if(output >=0.9){
    digitalWrite(relay, LOW);
    Blynk.setProperty(V0, "color", "#FF0000");
    lcd.print("On"); vlcd.print(0, 1, "Kipas Aktif");
  } else{
    Blynk.setProperty(V0, "color", "#00FF00");
    lcd.print("Off"); vlcd.print(0, 1, "Kipas Nonaktif");
  }
  delay(1000);
}

float bacaSuhu(){
  ds.requestTemperatures();
  return ds.getTempCByIndex(0);
}

float bacaKelembapan(){
  return isnan(dht.readHumidity()) ? 0 : dht.readHumidity();
}

void konfigFuzzy(){
  FuzzyInput *suhu = new FuzzyInput(1);
    suhu->addFuzzySet(dingin);
    suhu->addFuzzySet(sedang);
    suhu->addFuzzySet(panas);
  fuzzy->addFuzzyInput(suhu);

  FuzzyInput *kelembapan = new FuzzyInput(2);
    kelembapan->addFuzzySet(kering);
    kelembapan->addFuzzySet(lembab);
    kelembapan->addFuzzySet(basah);
  fuzzy->addFuzzyInput(kelembapan);

  FuzzyOutput *fan = new FuzzyOutput(1);
    fan->addFuzzySet(off);
    fan->addFuzzySet(on);
  fuzzy->addFuzzyOutput(fan);

  FuzzyRuleAntecedent *ifDinginAndKering = new FuzzyRuleAntecedent();
  ifDinginAndKering->joinWithAND(dingin, kering);
  FuzzyRuleConsequent *thenFanOff1 = new FuzzyRuleConsequent();
  thenFanOff1->addOutput(off);
  FuzzyRule *fuzzyRule01 = new FuzzyRule(1, ifDinginAndKering, thenFanOff1);
  fuzzy->addFuzzyRule(fuzzyRule01);

  FuzzyRuleAntecedent *ifDinginAndLembab = new FuzzyRuleAntecedent();
  ifDinginAndLembab->joinWithAND(dingin, lembab);
  FuzzyRuleConsequent *thenFanOff2 = new FuzzyRuleConsequent();
  thenFanOff2->addOutput(off);
  FuzzyRule *fuzzyRule02 = new FuzzyRule(2, ifDinginAndLembab, thenFanOff2);
  fuzzy->addFuzzyRule(fuzzyRule02);

  FuzzyRuleAntecedent *ifDinginAndBasah = new FuzzyRuleAntecedent();
  ifDinginAndBasah->joinWithAND(dingin, basah);
  FuzzyRuleConsequent *thenFanOff3 = new FuzzyRuleConsequent();
  thenFanOff3->addOutput(off);
  FuzzyRule *fuzzyRule03 = new FuzzyRule(3, ifDinginAndBasah, thenFanOff3);
  fuzzy->addFuzzyRule(fuzzyRule03); 
  
  FuzzyRuleAntecedent *ifSedangAndKering = new FuzzyRuleAntecedent();
  ifSedangAndKering->joinWithAND(sedang, kering);
  FuzzyRuleConsequent *thenFanOn1 = new FuzzyRuleConsequent();
  thenFanOn1->addOutput(off);
  FuzzyRule *fuzzyRule04 = new FuzzyRule(4, ifSedangAndKering, thenFanOn1);
  fuzzy->addFuzzyRule(fuzzyRule04);

  FuzzyRuleAntecedent *ifSedangAndLembab = new FuzzyRuleAntecedent();
  ifSedangAndLembab->joinWithAND(sedang, lembab);
  FuzzyRuleConsequent *thenFanOn2 = new FuzzyRuleConsequent();
  thenFanOn2->addOutput(on);
  FuzzyRule *fuzzyRule05 = new FuzzyRule(5, ifSedangAndLembab, thenFanOn2);
  fuzzy->addFuzzyRule(fuzzyRule05);

  FuzzyRuleAntecedent *ifSedangAndBasah = new FuzzyRuleAntecedent();
  ifSedangAndBasah->joinWithAND(sedang, basah);
  FuzzyRuleConsequent *thenFanOff4 = new FuzzyRuleConsequent();
  thenFanOff4->addOutput(off);
  FuzzyRule *fuzzyRule06 = new FuzzyRule(6, ifSedangAndBasah, thenFanOff4);
  fuzzy->addFuzzyRule(fuzzyRule06);

  FuzzyRuleAntecedent *ifPanasAndKering = new FuzzyRuleAntecedent();
  ifPanasAndKering->joinWithAND(panas, kering);
  FuzzyRuleConsequent *thenFanOn3 = new FuzzyRuleConsequent();
  thenFanOn3->addOutput(on);
  FuzzyRule *fuzzyRule07 = new FuzzyRule(7, ifPanasAndKering, thenFanOn3);
  fuzzy->addFuzzyRule(fuzzyRule07);

  FuzzyRuleAntecedent *ifPanasLembab = new FuzzyRuleAntecedent();
  ifPanasLembab->joinWithAND(panas, lembab);
  FuzzyRuleConsequent *thenFanOn4 = new FuzzyRuleConsequent();
  thenFanOn4->addOutput(on);
  FuzzyRule *fuzzyRule08 = new FuzzyRule(8, ifPanasLembab, thenFanOn4);
  fuzzy->addFuzzyRule(fuzzyRule08);
  
  FuzzyRuleAntecedent *ifPanasBasah = new FuzzyRuleAntecedent();
  ifPanasBasah->joinWithAND(panas, basah);
  FuzzyRuleConsequent *thenFanOn5 = new FuzzyRuleConsequent();
  thenFanOn5->addOutput(on);
  FuzzyRule *fuzzyRule09 = new FuzzyRule(9, ifPanasBasah, thenFanOn5);
  fuzzy->addFuzzyRule(fuzzyRule09);
}

void statusSuhu(){
  float derajatSedang = sedang->getPertinence();
  float derajatDingin = dingin->getPertinence();
  String statusSuhu = "Panas";
  if (derajatSedang> 0.5) {
    statusSuhu = "Sedang";
  } else if (derajatDingin> 0.5) {
    statusSuhu = "Dingin";
  }
  Serial.print("\t"); Serial.print(statusSuhu); vlcd.print(0, 0, statusSuhu);
}

void statusKelembapan(){
  float derajatLembab = lembab->getPertinence();
  float derajatBasah = basah->getPertinence();
  String statusKelembapan = "Kering";

  if (derajatLembab> 0.5) {
    statusKelembapan = "Lembab";
  } else if (derajatBasah> 0.5) {
    statusKelembapan = "Basah";
  }
  Serial.print("\t"); Serial.print(statusKelembapan); vlcd.print(7, 0, statusKelembapan);
}