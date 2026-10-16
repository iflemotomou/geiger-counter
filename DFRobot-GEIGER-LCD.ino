#include <DFRobot_Geiger.h>
#include <DFRobot_RGBLCD1602.h>

#define detect_pin 3
const int colorR = 255;
const int colorG = 0;
const int colorB = 0;
DFRobot_Geiger geiger(detect_pin);
DFRobot_RGBLCD1602 lcd(/*RGBAddr*/0x60 ,/*lcdCols*/16,/*lcdRows*/2); 

void setup()
{
  Serial.begin(115200);

  lcd.init();
    lcd.setRGB(colorR, colorG, colorB);
    lcd.print("Geiger (Sv/h)");
  geiger.start();
}

void loop()
{
  delay(3000);

  int cpm = geiger.getCPM();
  int nsv = geiger.getnSvh();
  float usv = geiger.getuSvh();

  Serial.print("CPM: "); Serial.println(cpm);
  Serial.print("nSv/h: "); Serial.println(nsv);
  Serial.print("uSv/h: "); Serial.println(usv);

  lcd.setCursor(0, 1);
  lcd.print("C:");
  lcd.print(cpm);

  lcd.print(" n:");
  lcd.print(nsv);

  lcd.print(" u:");
  lcd.print(usv,2);
}