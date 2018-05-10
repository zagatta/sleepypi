//
// Simple example showing how to set the RTC alarm pin to wake up the Arduino.
// This is a different mode to the alarm clock, which wakes at a particular time.
// This mode is a repeating periodic time, waking the Arduino at fixed intervals.
// Note: this example doesn't wake up the RPi. For that add:
//
//     SleepyPi.enablePiPower(true);
//
// after arduino wakeup. For a clearer picture of how to do this see the
// eaxmple WakePiPeriodically which wakes the Rpi at fixed intervals.
//
// To test on the RPi without power cycling and using the Arduino IDE
// to view the debug messages, either fit the Power Jumper or enable
// self-power. http://spellfoundry.com/sleepy-pi/programming-arduino-ide/
//

  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////DO NOT EDIT FROM HERE....///////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////

// **** INCLUDES *****
#include "SleepyPi2.h"
#include <Time.h>
#include <LowPower.h>
#include <PCF8523.h>
#include <Wire.h>

const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

const int LED_PIN = 13;

// Globals
// ++++++++++++++++++++ Change me ++++++++++++++++++
// .. Setup the Periodic Timer
// .. use either eTB_SECOND or eTB_MINUTE or eTB_HOUR
eTIMER_TIMEBASE  PeriodicTimer_Timebase     = eTB_MINUTE;   // e.g. Timebase set to seconds. Other options: eTB_MINUTE, eTB_HOUR
uint8_t          PeriodicTimer_Value        = 1;           // Timer Interval in units of Timebase e.g 10 seconds
// ++++++++++++++++++++ End Change me ++++++++++++++++++

tmElements_t tm;


  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////...TO HERE /////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////

//
//Global Variables
//
const int rebootHour = 4;
const int rebootMinute = 0;
boolean scheduledRebootInit = false;
boolean scheduledRebootDone = false;
DateTime rebootTime;


  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////DO NOT EDIT FROM HERE....///////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////

void alarm_isr()
{
    // Just a handler for the alarm interrupt.
    // You could do something here

}

void setup()
{


  // Configure "Standard" LED pin
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN,LOW);		// Switch off LED

  // initialize serial communication: In Arduino IDE use "Serial Monitor"
  Serial.begin(9600);
  Serial.println("Starting, but I'm going to go to sleep for a while...");
  delay(50);

  SleepyPi.rtcInit(true);

  // Default the clock to the time this was compiled.
  // Comment out if the clock is set by other means
  // ...get the date and time the compiler was run
  if (getDate(__DATE__) && getTime(__TIME__)) {
      // and configure the RTC with this info
      SleepyPi.setTime(DateTime(F(__DATE__), F(__TIME__)));
  }

  printTimeNow();

  //Set current time as last reboot
  rebootTime = SleepyPi.readTime();

  Serial.print("Periodic Interval Set for: ");
  Serial.print(PeriodicTimer_Value);
  switch(PeriodicTimer_Timebase)
  {
    case eTB_SECOND:
      Serial.println(" seconds");
      break;
    case eTB_MINUTE:
      Serial.println(" minutes");
      break;
    case eTB_HOUR:
      Serial.println(" hours");
    default:
        Serial.println(" unknown timebase");
        break;
  }
}

void loop()
{
    SleepyPi.rtcClearInterrupts();

    // Allow wake up alarm to trigger interrupt on falling edge.
    attachInterrupt(0, alarm_isr, FALLING);    // Alarm pin

    // Set the Periodic Timer
    SleepyPi.setTimer1(PeriodicTimer_Timebase, PeriodicTimer_Value);

    delay(500);

    // Enter power down state with ADC and BOD module disabled.
    // Wake up when wake up pin is low.
    SleepyPi.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);

    // Disable external pin interrupt on wake up pin.
    detachInterrupt(0);

    SleepyPi.ackTimer1();
    Serial.println("I've Just woken up on a Periodic Timer!");
    printTimeNow();

    digitalWrite(LED_PIN,HIGH);
    delay(100);
    digitalWrite(LED_PIN,LOW);


  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////...TO HERE /////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////


   //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////ACTUAL CODE.... ////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////

    //Check if Pi is offline, without cutting power when he is NOT (bcuz we dont wanna interrup the boot)
    if (SleepyPi.checkPiStatus(false) == false) //offline
    {
      Serial.println("Pi is offline");

      //Try feeding him the juice to start him up
      delay(100);
      digitalWrite(LED_PIN,HIGH);
      SleepyPi.enablePiPower(true);
      delay(100);
      digitalWrite(LED_PIN,LOW);


      if (scheduledRebootInit == true) //Pi seems to have been offline because of reboot sheduling
      {
        if (scheduledRebootDone == false)
        {
          // Power to da Pi (set flag for pi not to start reboot sequence again, actual powerOn happens above, everytime the pi is offline)
          scheduledRebootDone = true;
        }
        else //Pi was powered, but did not turn on
        {
          //Check how long it was
          int threshold = SleepyPi.readTime().minute() - rebootTime.minute();
          if (threshold > 2) // 2 minutes should normally be enough to boot
          {
            //Cut Power and Wait for next cycle
            SleepyPi.enablePiPower(false);
          }
        }

      }
      else // Pi is offline but was not sheduled to be offline
      {
         SleepyPi.enablePiPower(false);
         delay(500);
         SleepyPi.enablePiPower(true);
         scheduledRebootInit = true;
      }
    }
    else  //Pi is online
    {
      Serial.println("Pi is running");
     //Check if it is time to reboot -> 0H und <10M
     DateTime now = SleepyPi.readTime();
     if (now.hour() == rebootHour && abs(now.minute()-rebootMinute)<=5)
     {
         Serial.println("It's reboot time");
         //Check if reboot was initiated
         if (scheduledRebootDone == false) //No reboot done yet
         {
             Serial.println("Initiating reboot");
             if (scheduledRebootInit == false) //And not even tried yet
             {
               Serial.println("Shutting down");
               //Shutting Pi Down
               SleepyPi.piShutdown();
               rebootTime = SleepyPi.readTime();
               scheduledRebootInit = true;
             }
             else //shutdown tried before
             {
               Serial.println("Why am i still alive?");
                int threshold = SleepyPi.readTime().minute() - rebootTime.minute();
                if (threshold >= 1) // and it was more than 1 minute ago
                {
                   Serial.println("Harakiri");
                   SleepyPi.enablePiPower(false); //Kill that MoFo
                }
             }
         }
         else
         {
           Serial.println("But i already did that");
         }
     }
     else //Reset Reboot Flag
     {
       // Print out the time
       Serial.print("Ok, RebootTime = ");
       print2digits(rebootHour);
       Serial.write(':');
       print2digits(rebootMinute);
       Serial.println();
       Serial.println("Never touch a running system, resetting reboot flag");
       scheduledRebootDone = false;
       scheduledRebootInit = false;
     }


    }
}




   //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////....END OF ACTUAL CODE /////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////


  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////DO NOT EDIT FROM HERE....///////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////


// **********************************************************************
//
//  - Helper routines
//
// **********************************************************************
void printTimeNow()
{
    // Read the time
    DateTime now = SleepyPi.readTime();

    // Print out the time
    Serial.print("Ok, Time = ");
    print2digits(now.hour());
    Serial.write(':');
    print2digits(now.minute());
    Serial.write(':');
    print2digits(now.second());
    Serial.print(", Date (D/M/Y) = ");
    Serial.print(now.day());
    Serial.write('/');
    Serial.print(now.month());
    Serial.write('/');
    Serial.print(now.year(), DEC);
    Serial.println();

    return;
}
bool getTime(const char *str)
{
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getDate(const char *str)
{
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}


  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////...TO HERE /////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////
