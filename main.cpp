#include <M5Core2.h>
#include <TinyGPS++.h>
#include <LovyanGFX.h>
#include "m5_gps.h"

static TinyGPSPlus gps; // GPS読み込み用
static LGFX lcd;
static LGFX_Sprite sprite(&lcd);

//******************************
// セットアップ
//******************************
void setup()
{
  M5.begin(true, true, true, false, kMBusModeInput);               // M5stack通信開始
  Serial2.begin(SERIAL_SPEED, SERIAL_8N1, GPS_TX_PIN, GPS_RX_PIN); // GPSモジュール通信開始
  lcd.init();

  sprite.setColorDepth(COLOR_DEPTH);              // カラーモード
                                                  //   sprite.setTextColor(lcd.color565(RED_COMP, GREEN_COMP, BLUE_COMP)); // フォントカラー設定
  sprite.setTextColor(GREEN, BLACK);              // フォントカラー設定
  sprite.setTextSize(1);                          // テキストサイズ設定
  sprite.createSprite(lcd.width(), lcd.height()); // スプライトエリア作成
}

//******************************
// メインループ
//******************************
void loop()
{
  displayInfo();
  readGPSInfo(MAIN_DELAY);
}

//******************************
// GPS情報読み込み
//******************************
static void readGPSInfo(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    while (Serial2.available() > 0)
    {
      gps.encode(Serial2.read());
    }
  } while (millis() - start < ms);
}

//******************************
// GPS情報 表示
//******************************
static void displayInfo()
{
  char buff[MAX_STRING];

  sprite.setFont(&fonts::FreeMono18pt7b); // フォント設定
  sprite.setCursor(0, 40);
  sprite.printf("ALT:");

  // 高度表示
  if (gps.altitude.isValid())
  {
    sprite.printf("%5d m\n", (int)gps.altitude.meters());
  }
  else
  {
    sprite.printf("----- m\n");
  }

  sprite.printf("SPD:");
  // 速度表示
  if (gps.speed.isValid())
  {
    sprite.printf("%5d km/h\n", (int)gps.speed.kmph());
  }
  else
  {
    sprite.printf("----- km/h\n");
  }

  sprite.setFont(&fonts::FreeMono12pt7b); // フォント設定
  sprite.printf("\n");
  sprite.printf("LAT:%lf\n", gps.location.lat());
  sprite.printf("LNG:%lf\n", gps.location.lng());

  sprite.pushSprite(0, 0); // バッファエリアをディスプレイに描画
  sprite.clear();

  delay(MAIN_DELAY);

#if 0
    M5.Lcd.println();
    M5.Lcd.setTextSize(10);
    if (gps.date.isValid())
    {
        sprintf(buff, "%04d/%02d/%02d\n",
                gps.date.year(),
                gps.date.month(),
                gps.date.day());
    }
    else
    {
        sprintf(buff, "--/--/--\n");
    }
    M5.Lcd.print(buff);

    if (gps.time.isValid())
    {
        sprintf(buff, "%02d:%02d:%02d.%02d\n",
                gps.time.hour(),
                gps.time.minute(),
                gps.time.second(),
                gps.time.centisecond());
    }
    else
    {
        sprintf(buff,"--:--:--.--\n");
    }
    M5.Lcd.print(buff);
#endif
}