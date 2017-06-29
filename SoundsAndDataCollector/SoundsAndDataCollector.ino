#define SOUND
#include <SPI.h>
#include <SdFat.h>

  uint8_t chunkCounter = 0,
    maxChunksCounter;

  const byte
    LED = LED_BUILTIN,
    maxRecordsPerChunk = 100,
    pinFront = A4,
    pinBack = A5,
    zuzzerPin = 10,
    chipSelect = 4,
    maxMissedStepsAllowed = 5,  
    pauseBetweenMeasurements = 100,
    minTimespanBetweenSteps = 200,
    minBackPressureRequiredFraction = 20, // == 5%
    minFrontPressureRequiredFraction = 2, // ==50%
    minNonMissedStepsCount = 100;

  int missedSteps = 0,
    frontMaxValue = 20,
    backMaxValue = 20,
    backPreviousHighTime = 0,
    nonMissedStepsCount,
    nonMissedStepsTimeStamp,
    maxChunksPerFile = 100;

  const int zuzzerToneHigh = 3200,
    zuzzerToneLow = 2000,
    zuzzerDuration = 30,
    minNonMissedStepsTimeMs = 180000;  
  const float highLearingRatio = 0.8;
  bool pressingDown, backPressed, lightOn;


SdFat sd;
SdFile file;
#define FILE_BASE_NAME "Data"
#define error(msg) sd.errorHalt(F(msg))
char fileName[13] = FILE_BASE_NAME "01.csv";
String str = "";
const byte BASE_NAME_SIZE = sizeof(FILE_BASE_NAME) - 1;

void setup(){
  pinMode(zuzzerPin, OUTPUT);
  pinMode(chipSelect, OUTPUT);
  pinMode(LED, OUTPUT);
  delay(2000);
  //  if (!sd.begin(chipSelect, SPI_HALF_SPEED)) {
  if (!sd.begin(chipSelect, SPI_FULL_SPEED)) {
    for (int i = 0; i<10; i++){
      tone(zuzzerPin, zuzzerToneHigh, zuzzerDuration);
      delay(1000);
    }
    sd.initErrorHalt();
  }
  if (BASE_NAME_SIZE > 6) {
    error("FILE_BASE_NAME too long");
  }
  makeNewFile();
  tone(zuzzerPin, zuzzerToneLow, zuzzerDuration);
}

void makeNewFile() {
  maxChunksCounter = maxChunksPerFile;
  while (sd.exists(fileName)) {
    if (fileName[BASE_NAME_SIZE + 1] != '9') {
      fileName[BASE_NAME_SIZE + 1]++;
    }
    else if (fileName[BASE_NAME_SIZE] != '9') {
      fileName[BASE_NAME_SIZE + 1] = '0';
      fileName[BASE_NAME_SIZE]++;
    }
    else {
      error("Can't create file name");
    }
  }
  if (!file.open(fileName, O_CREAT | O_WRITE | O_EXCL)) {
    error("file.open");
  }
  file.println(F("sound,front,back"));
}
void loop(){
  digitalWrite(LED, lightOn);
  lightOn = !lightOn;
  int frontPressure = analogRead(pinFront);
  int  backPressure = analogRead(pinBack);
  chunkCounter++;
  int alert = 0; // 0 = no sound, 1 = ring (missed steps), 2 = happy sound
  if (frontPressure * minFrontPressureRequiredFraction > frontMaxValue){
    // front is on the flore,
    if (!pressingDown){
      //  and this is beginning of new step
      nonMissedStepsCount++;
      if (!backPressed){
        if (millis() - backPreviousHighTime >= minTimespanBetweenSteps && missedSteps++ >= maxMissedStepsAllowed){
          // tone
#ifdef SOUND
          ring();
#endif
          alert = 1;
          missedSteps = 0;
          nonMissedStepsTimeStamp = millis();
          nonMissedStepsCount = 0;
        }
      }
      // reset backPressed && backPreviousHighTime
      backPressed = false;
      backPreviousHighTime = millis();
    }

    pressingDown = true;
    if (!backPressed){
      if (backPressure * minBackPressureRequiredFraction > backMaxValue){
        backPressed = true;
        missedSteps = 0;
        if (backPressure * highLearingRatio > backMaxValue){
          backMaxValue = backPressure * highLearingRatio;
        }
      }
    }
    if (frontPressure * highLearingRatio > frontMaxValue){
      frontMaxValue = frontPressure * highLearingRatio;
    }
  }
  else {
    // front is in the air
    pressingDown = false;
  }
  // check for happyTone
  alert += happyToneCheck();
  String a = (String) alert;

  str += a + "," + frontPressure + "," + backPressure + "\n";
  if (chunkCounter >= maxRecordsPerChunk){
    chunkCounter = 0;
    file.print(str);
    file.flush();
    str = "";
    if (maxChunksCounter-- <= 1){
      file.close();
      makeNewFile();
    }
  }
  //  LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF);
  delay(pauseBetweenMeasurements);
}

int happyToneCheck(){
  if (millis() - nonMissedStepsTimeStamp > minNonMissedStepsTimeMs && nonMissedStepsCount > minNonMissedStepsCount){
#ifdef SOUND
    happyTone();
#endif
    nonMissedStepsTimeStamp = millis();
    nonMissedStepsCount = 0;
    return 2;
  }
  return 0;
}
#ifdef SOUND


void happyTone() {
  int happyNotes [] = { 383, 60, 383, 60, 255, 60, 255, 60, 227, 60, 227, 60, 255, 120, 286, 60, 286, 60, 304, 60, 304, 60, 340, 60, 340, 60, 383, 120 };
  for (int i = 0; i < sizeof(happyNotes) / 4; i++){
    int duration = happyNotes[i * 2 + 1];
    int nota = happyNotes[i * 2];
    for (long i = 0; i < duration * 1000L; i += nota * 2) {
      digitalWrite(zuzzerPin, HIGH);
      delayMicroseconds(nota);
      digitalWrite(zuzzerPin, LOW);
      delayMicroseconds(nota);
    }
    delay(duration / 2);
  }

}

void ring(){
  for (int i = 0; i < 2; i++){
    tone(zuzzerPin, zuzzerToneLow, zuzzerDuration);
    delay(zuzzerDuration);
    tone(zuzzerPin, zuzzerToneHigh, zuzzerDuration);
    delay(zuzzerDuration);
  }
}
#endif
