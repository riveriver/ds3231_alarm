#include "RealTimeClock.h"
#include <Arduino.h>
#include <ESP32Time.h>
RTC_DS3231 rtc;
ESP32Time Inrtc;

void RealTimeClock::printDateTime(const DateTime& dt)
{
    char str[20];
    snprintf_P(str, 
        sizeof(str),
        PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
        dt.month(),
        dt.day(),
        dt.year(),
        dt.hour(),
        dt.minute(),
        dt.second() );
    Serial.print(str);
    Serial.println("");
}

void RealTimeClock::Initialize(TwoWire *wire,int UpdateDelay)
{
  // HACK UpdateFrequency
  UpdateFrequency = (UpdateDelay < 1000) ? 1000 / UpdateDelay : 0;
  // couldn't find rtc,use system_time;
  // find rtc,if lost power,use compile_time;
  // 
  if (!rtc.begin(wire))
  {
    Serial.print("[ERROR] couldn't find RTC! use system time: ");
    DateTime system_time(__DATE__,__TIME__);
    printDateTime(system_time);
    Serial.print("now:");
    now = DateTime(CompileTime.unixtime() + millis() / 1000);

    printDateTime(now);
    RtcBegin = false;
  }
  else
  {
    if (rtc.lostPower())
    {
      RtcLostPower = true;
      Serial.print("RTC lost power, reset the time!compile_time: ");
      DateTime compile_time(__DATE__,__TIME__);
      printDateTime(compile_time);
      rtc.adjust(CompileTime);
    }

    // rtc start,update time
    for (int i = 0; i < 10; i++)
    {
      if (rtc.now().isValid())
      {
        now = rtc.now();
        Serial.println("RTC Start, now = ");
        printDateTime(now);
        CompileTime = DateTime(now.unixtime() - millis() / 1000);
        Inrtc.setTime(now.unixtime()); // Set esp internal rtc time (For SDCard modified time)
        return;
      }
    }
    RtcBegin = false;
    now = DateTime(CompileTime.unixtime() + millis() / 1000);
    Inrtc.setTime(now.unixtime());
    Serial.println("[ERROR]can't load RTC time, try to reset the RTC!now: ");
    printDateTime(now);
  };
}

void RealTimeClock::update()
{
  /*If using now = rtc.now() directly in date-time string generating function,
  the library may consume large amount of ROM and have high possibility to crash.
  Update the date-time information in individual loop cycle will be more stable.*/

  // If RTC Begin faild in Initialize, assume no RTC install and use esp build in rtc.
  if (!RtcBegin)
  {
    now = DateTime(CompileTime.unixtime() + millis() / 1000);
    printDateTime(now);
    return;
  }

  if (tInvalid > UpdateFrequency)
  { // if reading RTC keep failling over 1 second, try to restart the RTC every 5 sec.
    if (tBeginFalse % (5 * UpdateFrequency) != 0)
    { // used build in rtc between checking rtc.begin()
      tBeginFalse++;
      now = DateTime(CompileTime.unixtime() + millis() / 1000);
      return;
    }
    else
    {
      if (!rtc.begin())
      {
        if (tBeginFalse == 0)
        { // Print error once.
          Serial.println("[Clock] RTC.begin() Return False");
        }
        tBeginFalse++;
        now = DateTime(CompileTime.unixtime() + millis() / 1000);
        return;
      }
      else
      {
        tBeginFalse = 0;
        tInvalid = 0;
        Serial.println("[Clock] RTC.begin() Return True");
      }
    }
  } // end rtc.begin() check

  DateTime NewTime = rtc.now();
  if (!NewTime.isValid())
  {
    tInvalid++;
    Serial.println("[Clock] RTC.now() Data Invalid");
    now = DateTime(CompileTime.unixtime() + millis() / 1000);
  } // end Check valid
  else if (NewTime == LastUpdate)
  {
    if (millis() - tLastUpdate > 1200)
    {
      if (!LostRTC)
      {
        LostRTC = true;
        CompileTime = DateTime(now.unixtime() - tLastUpdate);
        Serial.println("[Clock] RTC.now() Not Update");
      }
      tInvalid++;
      now = DateTime(CompileTime.unixtime() + millis() / 1000);
    }
  }
  else
  {
    if (tInvalid != 0)
    {
      Serial.println("[Clock] RTC OK");
    }
    now = NewTime;
    LastUpdate = NewTime;
    LostRTC = false;
    tLastUpdate = millis();
    tInvalid = 0;
  }
}

int RealTimeClock::NowSec()
{
  return now.unixtime();
}

String RealTimeClock::TimeStamp(String NowSet, String str)
{
  NowSet.toUpperCase();
  DateTime T;
  if (NowSet == "NOW")
  {
    T = now;
  }
  else if (NowSet == "SET")
  {
    T = UserSetTimeBuffer;
  }
  else
  {
    Serial.println("[Clock] TimeStamp print Error. T = now");
  }

  String Now = "";

  if (T.hour() < 10)
  {
    Now += "0";
  }
  Now += String(T.hour()) + str;
  if (T.minute() < 10)
  {
    Now += "0";
  }
  Now += String(T.minute()) + str;
  if (T.second() < 10)
  {
    Now += "0";
  }
  Now += String(T.second());

  return Now;
}

String RealTimeClock::DateStamp(String NowSet, String YMD, String str, int YearDigit)
{
  NowSet.toUpperCase();
  DateTime T;
  if (NowSet == "NOW")
  {
    T = now;
  }
  else if (NowSet == "SET")
  {
    T = UserSetTimeBuffer;
  }
  else
  {
    Serial.println("[Clock] TimeStamp print Error. T = now");
  }

  String Today = "";
  if (YMD == "DMY")
  {
    if (T.day() < 10)
    {
      Today += "0";
    }
    Today += String(T.day()) + str;
    if (T.month() < 10)
    {
      Today += "0";
    }
    Today += String(T.month());
    if (YearDigit != 0)
    {
      Today += str + String(T.year()).substring(min(4 - YearDigit, 4));
    }
  }
  else
  {
    Today = (YearDigit == 0) ? "" : String(T.year()).substring(min(4 - YearDigit, 4)) + str;
    if (T.month() < 10)
    {
      Today += "0";
    }
    Today += String(T.month()) + str;
    if (T.day() < 10)
    {
      Today += "0";
    }
    Today += String(T.day());
  }
  return Today;
}

String RealTimeClock::TimeStamp()
{
  return TimeStamp("Now", ":");
}

String RealTimeClock::DateStamp(String str, int YearDigit)
{
  return DateStamp("now", "YMD", str, YearDigit);
}

String RealTimeClock::DateStamp(String NowSet, String Format)
{
  NowSet.toUpperCase();
  DateTime T;
  if (NowSet == "NOW")
  {
    T = now;
  }
  else if (NowSet == "SET")
  {
    T = UserSetTimeBuffer;
  }
  else
  {
    Serial.println("[Clock] TimeStamp print Error. T = now");
  }

  String Str = Format;
  int Y4 = Format.indexOf("YYYY");
  int Y2 = Format.indexOf("YY");
  int M3 = Format.indexOf("MMM");
  int M2 = Format.indexOf("MM");
  int D2 = Format.indexOf("DD");
  int W3 = Format.indexOf("WWW");
  int W6 = Format.indexOf("WWWWWW");

  char S[5] = {"X"};
  String SS;
  if (Y4 != -1)
  {
    dtostrf(T.year(), 4, 0, S);
    for (int i = 0; i < 4; i++)
      Str.setCharAt(Y4 + i, S[i]);
  }
  else if (Y2 != -1)
  {
    dtostrf(T.year(), 4, 0, S);
    for (int i = 0; i < 2; i++)
      Str.setCharAt(Y2 + i, S[i + 2]);
  }

  if (D2 != -1)
  {
    dtostrf(T.day(), 2, 0, S);
    if (T.day() < 10)
      S[0] = '0';
    for (int i = 0; i < 2; i++)
      Str.setCharAt(D2 + i, S[i]);
  }
  if (W6 != -1)
  {
    switch (T.dayOfTheWeek())
    {
    case 1:
      SS = "Monday";
      break;
    case 2:
      SS = "Tuesday";
      break;
    case 3:
      SS = "Wednesday";
      break;
    case 4:
      SS = "Thursday";
      break;
    case 5:
      SS = "Friday";
      break;
    case 6:
      SS = "Saturday";
      break;
    case 0:
      SS = "Sunday";
      break;
    }
    Str = Str.substring(0, W6) + SS + Str.substring(W6 + 6);
  }
  else if (W3 != -1)
  {
    switch (T.dayOfTheWeek())
    {
    case 1:
      memcpy(S, "Mon", sizeof(S));
      break;
    case 2:
      memcpy(S, "Tue", sizeof(S));
      break;
    case 3:
      memcpy(S, "Wed", sizeof(S));
      break;
    case 4:
      memcpy(S, "Thu", sizeof(S));
      break;
    case 5:
      memcpy(S, "Fri", sizeof(S));
      break;
    case 6:
      memcpy(S, "Sat", sizeof(S));
      break;
    case 7:
      memcpy(S, "Sun", sizeof(S));
      break;
    }
    for (int i = 0; i < 3; i++)
      Str.setCharAt(W3 + i, S[i]);
  }
  if (M3 != -1)
  {
    switch (T.month())
    {
    case 1:
      memcpy(S, "JAN", sizeof(S));
      break;
    case 2:
      memcpy(S, "FEB", sizeof(S));
      break;
    case 3:
      memcpy(S, "MAR", sizeof(S));
      break;
    case 4:
      memcpy(S, "APR", sizeof(S));
      break;
    case 5:
      memcpy(S, "MAY", sizeof(S));
      break;
    case 6:
      memcpy(S, "JUN", sizeof(S));
      break;
    case 7:
      memcpy(S, "JUL", sizeof(S));
      break;
    case 8:
      memcpy(S, "AUG", sizeof(S));
      break;
    case 9:
      memcpy(S, "SEP", sizeof(S));
      break;
    case 10:
      memcpy(S, "OCT", sizeof(S));
      break;
    case 11:
      memcpy(S, "NOV", sizeof(S));
      break;
    case 12:
      memcpy(S, "DEC", sizeof(S));
      break;
    }
    for (int i = 0; i < 3; i++)
      Str.setCharAt(M3 + i, S[i]);
  }
  else if (M2 != -1)
  {
    dtostrf(T.month(), 2, 0, S);
    if (T.month() < 10)
      S[0] = '0';
    for (int i = 0; i < 2; i++)
      Str.setCharAt(M2 + i, S[i]);
  }
  return Str;
}

String RealTimeClock::DateTimeStamp()
{
  return (DateStamp("now", "YMD", "/", 4) + " " + TimeStamp());
}

String RealTimeClock::DateTimeStamp(String con)
{
  return (DateStamp("now", "YMD", "/", 4) + con + TimeStamp());
}

char RealTimeClock::RTC_State()
{
  if (!RtcBegin || tBeginFalse > UpdateFrequency * 300)
  {
    return '*';
  }
  else if (RtcLostPower || tInvalid > UpdateFrequency)
  {
    return '!';
  }
  else
  {
    return ' ';
  }
}

void RealTimeClock::ResetUserSetTimeBuffer()
{
  UserSetTimeBuffer = now;
  Cursor = 0;
}

void RealTimeClock::UserSetTime(int Do)
{
  if (Do == -1)
  {
    return;
  }
  switch (Cursor)
  {
  case 2:
    switch (Do)
    {
    case 0:
      UserSetTimeBuffer = UserSetTimeBuffer + TimeSpan(1, 0, 0, 0);
      break;
    case 1:
      Cursor++;
      break;
    case 2:
      UserSetTimeBuffer = UserSetTimeBuffer - TimeSpan(1, 0, 0, 0);
      break;
    }
    break;
  case 1:
    switch (Do)
    {
    case 0:
      UserSetTimeBuffer = TimeSpanYearMonth(UserSetTimeBuffer, 0, 1);
      break;
    case 1:
      Cursor++;
      break;
    case 2:
      UserSetTimeBuffer = TimeSpanYearMonth(UserSetTimeBuffer, 0, -1);
      break;
    }
    break;
  case 0:
    switch (Do)
    {
    case 0:
      UserSetTimeBuffer = TimeSpanYearMonth(UserSetTimeBuffer, 1, 0);
      break;
    case 1:
      Cursor++;
      break;
    case 2:
      UserSetTimeBuffer = TimeSpanYearMonth(UserSetTimeBuffer, -1, 0);
      break;
    }
    break;
  case 3:
    switch (Do)
    {
    case 0:
      UserSetTimeBuffer = UserSetTimeBuffer + TimeSpan(0, 1, 0, 0);
      break;
    case 1:
      Cursor++;
      break;
    case 2:
      UserSetTimeBuffer = UserSetTimeBuffer - TimeSpan(0, 1, 0, 0);
      break;
    }
    break;
  case 4:
    switch (Do)
    {
    case 0:
      UserSetTimeBuffer = UserSetTimeBuffer + TimeSpan(0, 0, 1, 0);
      break;
    case 1:
      Cursor = 0;
      break;
    case 2:
      UserSetTimeBuffer = UserSetTimeBuffer - TimeSpan(0, 0, 1, 0);
      break;
    }
    break;
  case 5:
    switch (Do)
    {
    case 0:
      UserSetTimeBuffer = UserSetTimeBuffer + TimeSpan(0, 0, 0, 1);
      break;
    case 1:
      Cursor++;
      break;
    case 2:
      UserSetTimeBuffer = UserSetTimeBuffer - TimeSpan(0, 0, 0, 1);
      break;
    }
    break;
  case 6:
    switch (Do)
    {
    case 0:
      Cursor = 0;
      break;
    case 2:
      Cursor = 3;
      break;
    case 1:
      if (RtcBegin)
      {
        rtc.adjust(UserSetTimeBuffer);
      }
      else
      {
        CompileTime = DateTime(UserSetTimeBuffer.unixtime() - millis() / 1000);
      }
      Cursor = 0;
      update();
      Inrtc.setTime(now.unixtime());
      Serial.println("[Clock] Adjust Time. now = " + DateTimeStamp());
      break;
    }
    break;
  }
}

DateTime RealTimeClock::TimeSpanYearMonth(DateTime T, int addYear, int addMonth)
{
  int MonthOperator = (T.month() + addMonth) % 12;
  int YearOperator = T.year() + addYear + (T.month() + addMonth - MonthOperator) / 12;
  if (MonthOperator == 0)
  {
    YearOperator--;
    MonthOperator = 12;
  }
  return DateTime(YearOperator, MonthOperator, T.day(), T.hour(), T.minute(), T.second());
}

int RealTimeClock::CheckTimeDifference(int Year, int Month, int Day, int hour, int minute, int second)
{
  DateTime D(Year, Month, Day, hour, minute, second);
  return D.unixtime() - now.unixtime();
}

void RealTimeClock::SetTime(int Year, int Month, int Day, int hour, int minute, int second)
{
  DateTime D(Year, Month, Day, hour, minute, second);
  int TimeDifference = D.unixtime() - now.unixtime();
  if (RtcBegin)
  {
    rtc.adjust(D);
  }
  else
  {
    CompileTime = DateTime(D.unixtime() - millis() / 1000);
  }
  Inrtc.setTime(now.unixtime());
  now = D;
  Serial.println("[Clock] Auto Adjust Time. Offset = " + String(TimeDifference) + " s. now = " + DateTimeStamp());
}

void RealTimeClock::SetTime(int unix_epoch)
{
  int TimeDifference = unix_epoch - now.unixtime();
  if (RtcBegin)
  {
    rtc.adjust(unix_epoch);
  }
  else
  {
    CompileTime = DateTime(unix_epoch - millis() / 1000);
  }
  Inrtc.setTime(unix_epoch);
  now = DateTime(unix_epoch);
  Serial.println("[Clock] Auto Adjust Time. Offset = " + String(TimeDifference) + " s. now = " + DateTimeStamp());
}