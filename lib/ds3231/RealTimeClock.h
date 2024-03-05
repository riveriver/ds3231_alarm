#ifndef RealTimeClock_h
#define RealTimeClock_h
#include <Arduino.h>
#include <RTClib.h>

class RealTimeClock
{
public:
  void Initialize(TwoWire *wire,int UpdateDelay);
  String TimeStamp(String NowSet, String str);
  String TimeStamp();
  String DateStamp(String NowSet, String YMD, String str, int YearDigit);
  String DateStamp(String str, int YearDigit);
  String DateStamp(String NowSet, String Format);
  String DateTimeStamp();
  String DateTimeStamp(String con);
  char RTC_State(); 
  bool RtcBegin = true;
  bool RtcLostPower = false;
  bool LostRTC = false;
  // return * : RTC Failed, used built in. ! : RTC Lost Power
  void update();
  int NowSec();
  int tInvalid = 0; // Read Data invalid count
  DateTime now;
  void printDateTime(const DateTime& dt);
  void ResetUserSetTimeBuffer();
  void UserSetTime(int Do);
  void SetTime(int Year, int Month, int Day, int hour, int minute, int second);
  void SetTime(int inux_epoch);
  int CheckTimeDifference(int Year, int Month, int Day, int hour, int minute, int second);
  DateTime TimeSpanYearMonth(DateTime T, int addYear, int addMonth);
  int Cursor = -1;

private:
  DateTime CompileTime = DateTime(F(__DATE__), F(__TIME__));
  DateTime UserSetTimeBuffer;
  DateTime LastUpdate;
  
  int tLastUpdate = 0;
  int tBeginFalse = 0;
  int UpdateFrequency = 10;
};
#endif
