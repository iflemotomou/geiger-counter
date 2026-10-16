#include <DFRobot_RGBLCD1602.h>
// Ψηφιακή έξοδος του αισθητήρα Geiger
#define DETECT_PIN 3

// Χρονικό παράθυρο άμεσης μέτρησης: 10 δευτερόλεπτα
const unsigned long INSTANT_WINDOW_MS = 10000UL;

// Συνολική διάρκεια κύριας μέτρησης: 2 λεπτά
const unsigned long TWO_MINUTES_MS =
  2UL * 60UL * 1000UL;

// Οθόνη LCD 16x2
DFRobot_RGBLCD1602 lcd(0x60, 16, 2);

// Συνολικά counts από την έναρξη της μέτρησης
volatile unsigned long totalPulseCount = 0;

// Χρονικές μεταβλητές
unsigned long measurementStart;
unsigned long instantWindowStart;
unsigned long lastDisplayUpdate = 0;

// Συνολικά counts στην αρχή του προηγούμενου παραθύρου των 10 δευτερολέπτων
unsigned long previousPulseCount = 0;

// Counts του τελευταίου ολοκληρωμένου παραθύρου 10 s
unsigned long lastWindowCounts = 0;

// Άμεσο CPM ως ακέραιος
unsigned long instantCPM = 0;

// Τελικά αποτελέσματα δίλεπτης μέτρησης
unsigned long finalTwoMinuteCounts = 0;
unsigned long finalTwoMinuteCPM = 0;

// Κατάσταση λειτουργίας
bool instantValueAvailable = false;
bool twoMinuteMeasurementFinished = false;


// Εκτελείται κάθε φορά που φτάνει παλμός από το Geiger
void countPulse()
{
  totalPulseCount++;
}


// Ασφαλής ανάγνωση του μετρητή παλμών
unsigned long readPulseCount()
{
  noInterrupts();
  unsigned long countCopy = totalPulseCount;
  interrupts();

  return countCopy;
}


// Καθαρισμός μιας γραμμής της LCD
void clearLCDLine(byte row)
{
  lcd.setCursor(0, row);
  lcd.print("                ");
  lcd.setCursor(0, row);
}


// Πρώτη γραμμή: counts στα τελευταία 10 s και άμεσο CPM
void displayInstantMeasurement()
{
  clearLCDLine(0);

  if (!instantValueAvailable)
  {
    lcd.print("C10s:- EstCPM:-");
  }
  else
  {
    lcd.print("C10s:");
    lcd.print(lastWindowCounts);

    lcd.print(" EstCPM:");
    lcd.print(instantCPM);
  }
}


// Δεύτερη γραμμή: συνολικά counts και χρόνος που απομένει
void displayTwoMinuteMeasurement(
  unsigned long currentTime,
  unsigned long currentPulseCount)
{
  clearLCDLine(1);

  // Τελική εμφάνιση μετά τα 2 λεπτά
  if (twoMinuteMeasurementFinished)
  {
    lcd.print("T:");
    lcd.print(finalTwoMinuteCounts);

    lcd.print(" CPM:");
    lcd.print(finalTwoMinuteCPM);

    return;
  }

  unsigned long elapsedTime =
    currentTime - measurementStart;

  unsigned long remainingTime;

  if (elapsedTime < TWO_MINUTES_MS)
  {
    remainingTime =
      TWO_MINUTES_MS - elapsedTime;
  }
  else
  {
    remainingTime = 0;
  }

  // Στρογγυλοποίηση προς τα πάνω, ώστε να ξεκινά από 02:00
  unsigned long remainingSeconds =
    (remainingTime + 999UL) / 1000UL;

  unsigned int minutes =
    remainingSeconds / 60UL;

  unsigned int seconds =
    remainingSeconds % 60UL;

  // CTot = συνολικά counts από την αρχή
  lcd.print("CTot:");
  lcd.print(currentPulseCount);
  lcd.print(" ");

  if (minutes < 10)
  {
    lcd.print("0");
  }

  lcd.print(minutes);
  lcd.print(":");

  if (seconds < 10)
  {
    lcd.print("0");
  }

  lcd.print(seconds);
}


void setup()
{
  Serial.begin(115200);

  lcd.init();
  lcd.setRGB(255, 0, 0);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Geiger Counter");

  lcd.setCursor(0, 1);
  lcd.print("Starting...");

  pinMode(DETECT_PIN, INPUT);

  delay(1500);

  // Μηδενισμός πριν από την έναρξη
  noInterrupts();
  totalPulseCount = 0;
  interrupts();

  measurementStart = millis();
  instantWindowStart = measurementStart;
  previousPulseCount = 0;

  // Καταγραφή κάθε καθοδικού παλμού
  attachInterrupt(
    digitalPinToInterrupt(DETECT_PIN),
    countPulse,
    FALLING
  );

  lcd.clear();

  displayInstantMeasurement();

  lcd.setCursor(0, 1);
  lcd.print("T:0 02:00");
}


void loop()
{
  unsigned long currentTime = millis();

  unsigned long currentPulseCount =
    readPulseCount();

  /*
   * ΑΜΕΣΗ ΜΕΤΡΗΣΗ ΑΝΑ 10 ΔΕΥΤΕΡΟΛΕΠΤΑ
   */
  unsigned long instantElapsedTime =
    currentTime - instantWindowStart;

  if (instantElapsedTime >= INSTANT_WINDOW_MS)
  {
    // Counts στο τελευταίο παράθυρο 10 s
    lastWindowCounts =
      currentPulseCount - previousPulseCount;

    /*
     * Αναγωγή σε CPM και στρογγυλοποίηση
     * στον πλησιέστερο ακέραιο.
     *
     * CPM = counts × 60000 / χρόνος σε ms
     */
    instantCPM =
      (lastWindowCounts * 60000UL +
       instantElapsedTime / 2UL) /
      instantElapsedTime;

    // Προετοιμασία επόμενου παραθύρου
    previousPulseCount = currentPulseCount;
    instantWindowStart = currentTime;

    instantValueAvailable = true;

    displayInstantMeasurement();

    Serial.print("Counts in last 10 s: ");
    Serial.println(lastWindowCounts);

    Serial.print("Immediate CPM: ");
    Serial.println(instantCPM);
  }

  /*
   * ΟΛΟΚΛΗΡΩΣΗ ΔΙΛΕΠΤΗΣ ΜΕΤΡΗΣΗΣ
   */
  if (!twoMinuteMeasurementFinished)
  {
    unsigned long twoMinuteElapsed =
      currentTime - measurementStart;

    if (twoMinuteElapsed >= TWO_MINUTES_MS)
    {
      finalTwoMinuteCounts =
        readPulseCount();

      /*
       * CPM = συνολικά counts / 2.
       *
       * Η πρόσθεση του 1 πριν από την ακέραια
       * διαίρεση δίνει στρογγυλοποίηση στον
       * πλησιέστερο ακέραιο.
       */
      finalTwoMinuteCPM =
        (finalTwoMinuteCounts + 1UL) / 2UL;

      twoMinuteMeasurementFinished = true;

      displayTwoMinuteMeasurement(
        currentTime,
        finalTwoMinuteCounts
      );

      Serial.println();
      Serial.println("----- 2 MINUTE RESULT -----");

      Serial.print("Total counts: ");
      Serial.println(finalTwoMinuteCounts);

      Serial.print("Two-minute CPM: ");
      Serial.println(finalTwoMinuteCPM);
    }
  }

  /*
   * ΑΝΑΝΕΩΣΗ ΔΕΥΤΕΡΗΣ ΓΡΑΜΜΗΣ ΑΝΑ 1 s
   */
  if (currentTime - lastDisplayUpdate >= 1000UL)
  {
    lastDisplayUpdate = currentTime;

    currentPulseCount = readPulseCount();

    displayTwoMinuteMeasurement(
      currentTime,
      currentPulseCount
    );

    if (!twoMinuteMeasurementFinished)
    {
      Serial.print("Total counts: ");
      Serial.println(currentPulseCount);
    }
  }
}
