#include <M5StickC.h>

RTC_TimeTypeDef RTC_TimeStruct;
RTC_DateTypeDef RTC_DateStruct;


uint8_t led_count = 15;
long brightnessTime, tiltTime = 0, tiltTime2 = 0;
float b, c = 0;
int battery = 0;

float accX = 0;
float accY = 0;
float accZ = 0;


#define TFT_GREY 0x5AEB

float sx = 0, sy = 1, mx = 1, my = 0, hx = -1, hy = 0;    // Saved H, M, S x & y multipliers
float sdeg = 0, mdeg = 0, hdeg = 0;
uint16_t osx = 120, osy = 120, omx = 120, omy = 120, ohx = 120, ohy = 120; // Saved H, M, S x & y coords
uint16_t x0 = 0, x1 = 0, yy0 = 0, yy1 = 0;
uint32_t targetTime = 0;                    // for next 1 second timeout

static uint8_t conv2d(const char* p); // Forward declaration needed for IDE 1.6.x

uint8_t hh, mm, ss, YY, MM, DD, dd;

boolean initial = 1;

void setup() {
  //COMMENT next line after first upload

  M5.begin();
  Wire.begin(32, 33);
  pinMode(M5_BUTTON_HOME, INPUT);
  pinMode(M5_BUTTON_RST, INPUT);
  pinMode(M5_BUTTON_HOME, INPUT_PULLUP);

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_37, 0); //1 = High, 0 = Low

  //COMMENT next 12 lines after first upload
  
  hh = conv2d(__TIME__), mm = conv2d(__TIME__ + 3), ss = conv2d(__TIME__ + 6); // Get H, M, S from compile time
  RTC_TimeTypeDef TimeStruct;
  TimeStruct.Hours   = hh;
  TimeStruct.Minutes = mm;
  TimeStruct.Seconds = ss;
  M5.Rtc.SetTime(&TimeStruct);
  RTC_DateTypeDef DateStruct;
  DateStruct.WeekDay = 1;
  DateStruct.Month = 9;
  DateStruct.Date = 22;
  DateStruct.Year = 2019;
  M5.Rtc.SetData(&DateStruct);
  
  //COMMENT UPTO HERE

  M5.Rtc.GetTime(&RTC_TimeStruct);
  hh = RTC_TimeStruct.Hours;
  mm = RTC_TimeStruct.Minutes;
  ss = RTC_TimeStruct.Seconds;

  M5.Rtc.GetData(&RTC_DateStruct);
  YY = RTC_DateStruct.Year;
  MM = RTC_DateStruct.Month;
  DD = RTC_DateStruct.Date;
  dd = RTC_DateStruct.WeekDay;

  M5.Lcd.setRotation(3);
  M5.Lcd.fillScreen(TFT_GREY);

  M5.Lcd.setTextColor(TFT_WHITE, TFT_GREY);  // Adding a background colour erases previous text automatically

  // Draw clock face
  M5.Lcd.fillCircle(40, 40, 40, TFT_BLACK);
  M5.Lcd.fillCircle(40, 40, 36, TFT_BLACK);

  // Draw 12 lines
  for (int i = 0; i < 360; i += 30) {
    sx = cos((i - 90) * 0.0174532925);
    sy = sin((i - 90) * 0.0174532925);
    x0 = sx * 38 + 40;
    yy0 = sy * 38 + 40;
    x1 = sx * 32 + 40;
    yy1 = sy * 32 + 40;

    M5.Lcd.drawLine(x0, yy0, x1, yy1, TFT_WHITE);
  }

  // Draw 60 dots
  for (int i = 0; i < 360; i += 6) {
    sx = cos((i - 90) * 0.0174532925);
    sy = sin((i - 90) * 0.0174532925);
    x0 = sx * 34 + 40;
    yy0 = sy * 34 + 40;
    // Draw minute markers
    M5.Lcd.drawPixel(x0, yy0, TFT_WHITE);

    // Draw main quadrant dots
    if (i == 0 || i == 180) M5.Lcd.fillCircle(x0, yy0, 2, TFT_WHITE);
    if (i == 90 || i == 270) M5.Lcd.fillCircle(x0, yy0, 2, TFT_WHITE);
  }

  M5.Lcd.fillCircle(40, 40, 2, TFT_WHITE);
  M5.Lcd.drawCentreString("WAyKEy", 120, 260, 4);

  targetTime = millis() + 1000;

  M5.Axp.ScreenBreath(9);
  M5.MPU6886.Init();

}
void brightnessT() {
  M5.MPU6886.getAccelData(&accX, &accY, &accZ);
  accX *= 1000;
  accY *= 1000;

  if ((accX < (0 - 200) && (accX > -900)) && ((accY > 0 - 300) && (accY < 300)))
  {
    if (millis() > (tiltTime + 500)) {
      M5.Axp.ScreenBreath(12);
      brightnessTime = millis();
      //    while (accX < (0 - 500) && ((accY > 0) && (accY < 100)));
    }
  }
  else
    tiltTime = millis();

  if (brightnessTime < millis() - 2000)
  {
    M5.Axp.ScreenBreath(7);
    brightnessTime = 0;
  }
}
void wristWatch() {
  if (targetTime < millis()) {
    targetTime += 1000;
    ss++;              // Advance second
    if (ss == 60) {
      ss = 0;
      mm++;            // Advance minute
      if (mm > 59) {
        mm = 0;
        hh++;          // Advance hour
        if (hh > 23) {
          hh = 0;
        }
      }
    }

    // Pre-compute hand degrees, x & y coords for a fast screen update
    sdeg = ss * 6;                // 0-59 -> 0-354
    mdeg = mm * 6 + sdeg * 0.01666667; // 0-59 -> 0-360 - includes seconds
    hdeg = hh * 30 + mdeg * 0.0833333; // 0-11 -> 0-360 - includes minutes and seconds
    hx = cos((hdeg - 90) * 0.0174532925);
    hy = sin((hdeg - 90) * 0.0174532925);
    mx = cos((mdeg - 90) * 0.0174532925);
    my = sin((mdeg - 90) * 0.0174532925);
    sx = cos((sdeg - 90) * 0.0174532925);
    sy = sin((sdeg - 90) * 0.0174532925);

    if (ss == 0 || initial) {
      initial = 0;
      // Erase hour and minute hand positions every minute
      M5.Lcd.drawLine(ohx, ohy, 40, 40, TFT_BLACK);
      ohx = hx * 15 + 40;
      ohy = hy * 15 + 40;
      M5.Lcd.drawLine(omx, omy, 40, 40, TFT_BLACK);
      omx = mx * 20 + 40;
      omy = my * 20 + 40;
    }

    // Redraw new hand positions, hour and minute hands not erased here to avoid flicker
    M5.Lcd.drawLine(osx, osy, 40, 40, TFT_BLACK);
    osx = sx * 25 + 40;
    osy = sy * 25 + 40;
    M5.Lcd.drawLine(osx, osy, 40, 40, TFT_RED);
    M5.Lcd.drawLine(ohx, ohy, 40, 40, TFT_WHITE);
    M5.Lcd.drawLine(omx, omy, 40, 40, TFT_WHITE);
    M5.Lcd.drawLine(osx, osy, 40, 40, TFT_RED);

    M5.Lcd.fillCircle(40, 40, 2, TFT_RED);
  }
  dd = RTC_DateStruct.WeekDay;
  weekDay();
  Date();
}

void weekDay() {
  M5.Lcd.setCursor(110, 12, 2);
  M5.Lcd.setTextColor(WHITE, TFT_GREY);

  //  M5.Lcd.print(dd);
  switch (dd) {
    case 1:
      M5.Lcd.print("SUN");
      break;
    case 2:
      M5.Lcd.print("MON");
      break;
    case 3:
      M5.Lcd.print("TUE");
      break;
    case 4:
      M5.Lcd.print("WED");
      break;
    case 5:
      M5.Lcd.print("THU");
      break;
    case 6:
      M5.Lcd.print("FRI");
      break;
    case 0:
      M5.Lcd.print("SAT");
      break;
  }
}

void Date() {
  M5.Lcd.setCursor(110, 26, 2);
  M5.Lcd.setTextColor(WHITE, TFT_GREY);
  M5.Lcd.print(DD);
  M5.Lcd.print('/');
  M5.Lcd.print(MM);
}

void batteryLevel() {
  M5.Lcd.setCursor(110, 3, 1);
  c = M5.Axp.GetVapsData() * 1.4 / 1000;
  b = M5.Axp.GetVbatData() * 1.1 / 1000;
  //  M5.Lcd.print(b);
  battery = ((b - 3.0) / 1.2) * 100;

  if (c >= 4.5) {
    M5.Lcd.setTextColor(TFT_YELLOW, TFT_GREY);
    M5.Lcd.print("CHG:");
  }
  else {
    M5.Lcd.setTextColor(TFT_GREEN, TFT_GREY);
    M5.Lcd.print("BAT:");
  }

  if (battery > 100)
    battery = 100;
  else if (battery < 100 && battery > 9)
    M5.Lcd.print(" ");
  else if (battery < 9)
    M5.Lcd.print("  ");
  if (battery < 10)
    M5.Axp.DeepSleep();

  //  if (digitalRead(M5_BUTTON_HOME) == LOW) {
  //    while (digitalRead(M5_BUTTON_HOME) == LOW);
  //    M5.Axp.DeepSleep(SLEEP_SEC(1));
  //  }
  M5.Lcd.print(battery);
  M5.Lcd.print("%");
}

void batterySaver() {
  M5.MPU6886.getAccelData(&accX, &accY, &accZ);
  accX *= 1000;
  accY *= 1000;

  if (!((accX < (0 - 200) && (accX > -900)) && ((accY > 0 - 300) && (accY < 300))))
  {
    if (millis() > (tiltTime2 + 10000)) {
      M5.Axp.DeepSleep();
      //    while (accX < (0 - 500) && ((accY > 0) && (accY < 100)));
    }
  }
  else
    tiltTime2 = millis();
}
void loop() {

  brightnessT();


  wristWatch();


  batteryLevel();


  batterySaver();
}

static uint8_t conv2d(const char* p) {
  uint8_t v = 0;
  if ('0' <= *p && *p <= '9')
    v = *p - '0';
  return 10 * v + *++p - '0';
}
