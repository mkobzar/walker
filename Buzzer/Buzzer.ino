#include <avr/sleep.h>

  const byte
    LED = LED_BUILTIN,
    zuzzerPin = 1,
    pinFront = A0,
    pinBack = A1,
    maxMissedStepsAllowed = 5, // after 5 steps will ring() 
    pauseBetweenMeasurments = 100,
    minTimespanBetweenSteps = 200,
    minBackPressureRequiredFraction = 10, // == 10%
    minFrontPressureRequiredFraction = 2; // ==50%
  const int powerSaveTimeoutMs = 10000;
  const float highLearingRatio = 0.8;
  int
    missedSteps = 0,
    frontMaxValue = 20,
    backMaxValue = 20,
    backPreviousHighTime = 0,
    nonMissedStepsCount,
    nonMissedStepsTimeStamp,
    lastStepTimeStamp;
  bool pressingDown, backPressed, lightOn;

void setup(){
  pinMode(zuzzerPin, OUTPUT);
  pinMode(LED, OUTPUT);
  delay(10000);
}

void zuzz(){
  digitalWrite(zuzzerPin, HIGH);
  delay(250);
  digitalWrite(zuzzerPin, LOW);
}

void loop(){
  int timeNow = millis();
  digitalWrite(LED, lightOn);
  lightOn = !lightOn;
  int frontPressure = analogRead(pinFront);
  int  backPressure = analogRead(pinBack);
  if (frontPressure * minFrontPressureRequiredFraction > frontMaxValue){
    // front is on the flore,
    if (!pressingDown){
      lastStepTimeStamp = timeNow;
      //  and this is beginning of new step
      nonMissedStepsCount++;
      if (!backPressed){
        if (timeNow - backPreviousHighTime >= minTimespanBetweenSteps && missedSteps++ >= maxMissedStepsAllowed){
          zuzz();
          missedSteps = 0;
          nonMissedStepsTimeStamp = timeNow;
          nonMissedStepsCount = 0;
        }
      }
      // reset backPressed && backPreviousHighTime
      backPressed = false;
      backPreviousHighTime = timeNow;
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
  if (timeNow - lastStepTimeStamp > powerSaveTimeoutMs){
    sleep();
    lastStepTimeStamp = millis();
  }
  else {
    delay(pauseBetweenMeasurments);
  }
}



void wake()
{
  sleep_disable();
  detachInterrupt(0);
  pinMode(zuzzerPin, OUTPUT);
  pinMode(LED, OUTPUT);
}

void sleep(){
  byte adcsra = ADCSRA;
  ADCSRA = 0;
  pinMode(zuzzerPin, INPUT);
  pinMode(LED, INPUT);
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  noInterrupts();
  attachInterrupt(0, wake, FALLING);
  EIFR = bit(INTF0);
#ifdef BODSE
  MCUCR = bit(BODS) | bit(BODSE);
  MCUCR = bit(BODS);
#endif
  interrupts();
  sleep_cpu();
  ADCSRA = adcsra;
}

