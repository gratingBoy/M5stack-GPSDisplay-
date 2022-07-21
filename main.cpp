#include "m5_gps.h"

static TinyGPSPlus gps;          // GPS読み込み用
static LGFX lcd;                 // LCD描画インスタンス
static LGFX_Sprite sprite(&lcd); // スプライトエリアインスタンス
static int initFlag = ON;        // イニシャルフラグ
static int helloFlag = ON;       // あいさつ再生フラグ

AudioGeneratorMP3 *mp3;
AudioFileSourceSD *file;
AudioOutputI2S *out;
AudioFileSourceID3 *id3;

//******************************
// セットアップ
//******************************
void setup()
{
    M5.begin(true, true, true, false, kMBusModeInput);               // M5stack通信開始
    Serial2.begin(SERIAL_SPEED, SERIAL_8N1, GPS_TX_PIN, GPS_RX_PIN); // GPSモジュール通信開始
    lcd.init();                                                      // 画面初期化
    sprite.setColorDepth(COLOR_DEPTH);                               // カラーモード
    sprite.setTextColor(ORANGE, BLACK);                              // フォントカラー設定
    sprite.setTextSize(1);                                           // テキストサイズ設定
    sprite.createSprite(lcd.width(), lcd.height());                  // スプライトエリア作成
}

//******************************
// メインループ
//******************************
void loop()
{
    readGPSInfo(MAIN_DELAY); // GPS情報読み込み
    if (initFlag == ON)      // 初回起動か
    {
        initGPS(); // GPSモジュールのイニシャル待ち
    }
    else // 初回起動でない
    {
        displayInfo(); // GPS情報画面表示
    }
}

//******************************
// 時刻同期
//******************************
static void synchTime()
{
    RTC_DateTypeDef RTC_DateStruct; // Data
    RTC_TimeTypeDef RTC_TimeStruct; // Time
    time_t unixTime;                // 通算秒
    struct tm gps_time;             // GPS時刻取得用
    struct tm *set_time;            // 時刻変換用

    M5.Rtc.GetDate(&RTC_DateStruct); // RTCから日付取得
    M5.Rtc.GetTime(&RTC_TimeStruct); // RTCから時刻取得

    gps_time.tm_year = gps.date.year() - YEAR_OFFSET;  // GPSから年 取得(通算年へ変換)
    gps_time.tm_mon = gps.date.month() - MONTH_OFFSET; // GPSから月 取得(月は0相対のため-1)
    gps_time.tm_mday = gps.date.day();                 // GPSから日 取得
    gps_time.tm_hour = gps.time.hour();                // GPSから時 取得
    gps_time.tm_min = gps.time.minute();               // GPSから分 取得
    gps_time.tm_sec = gps.time.second();               // GPSから秒 取得

    unixTime = mktime(&gps_time);     // 通算秒へ変換
    unixTime = unixTime + JST_OFFSET; // UTC->JSTに変換
    set_time = localtime(&unixTime);  // localタイムに変換

    // RTC設定用構造体にデータセット
    RTC_DateStruct.Year = set_time->tm_year + YEAR_OFFSET;
    RTC_DateStruct.Month = set_time->tm_mon + MONTH_OFFSET;
    RTC_DateStruct.Date = set_time->tm_mday;
    RTC_TimeStruct.Hours = set_time->tm_hour;
    RTC_TimeStruct.Minutes = set_time->tm_min;
    RTC_TimeStruct.Seconds = set_time->tm_sec;

    M5.Rtc.SetDate(&RTC_DateStruct); // RTCに日付設定
    M5.Rtc.SetTime(&RTC_TimeStruct); // RTCに時刻設定
}

//******************************
// GPSモジュールイニシャル待ち
//******************************
static void initGPS()
{
    RTC_DateTypeDef RTC_DateStruct; // 日付取得用
    RTC_TimeTypeDef RTC_TimeStruct; // 時刻取得用

    M5.Rtc.GetDate(&RTC_DateStruct); // 日付取得
    M5.Rtc.GetTime(&RTC_TimeStruct); // 時刻取得

    sprite.clear();                         // 画面クリア
    sprite.setCursor(0, 0);                 // カーソル移動
    sprite.setFont(&fonts::FreeMono12pt7b); // フォント設定
    // 起動メッセージ表示
    sprite.printf("Talking GPS Logger\n");
    sprite.printf("Version:%s\n", VERSION_NO);
    sprite.printf("Product:abtroG 2022\n");
    sprite.printf("Voice:Reina Shirakaba\n");
    sprite.printf("Date:%04d/%02d/%02d\n",
                  RTC_DateStruct.Year, RTC_DateStruct.Month, RTC_DateStruct.Date);
    sprite.printf("RTC:%02d:%02d:%02d\n",
                  RTC_TimeStruct.Hours, RTC_TimeStruct.Minutes, RTC_TimeStruct.Seconds);
    sprite.printf("initializing...");

    if (gps.location.isValid()) // GPSの測位有効になったら
    {
        sprite.printf("OK\n");
        sprite.printf("RTC Set Date...");
        synchTime(); // 時刻合わせ
        sprite.printf("OK\n");
        initFlag = OFF; // イニシャルフラグOFF
        sprite.printf("Initial END\n");
        sprite.pushSprite(0, 0); // バッファエリアをディスプレイに描画
        delay(INIT_DELAY);       // イニシャル表示ディレイ
    }
    sprite.pushSprite(0, 0); // バッファエリアをディスプレイに描画
    delay(MAIN_DELAY);
}

//******************************
// GPS情報読み込み
//******************************
static void readGPSInfo(unsigned long ms)
{
    unsigned long start = millis(); // 実行時間(ms)取得
    do
    {
        while (Serial2.available() > 0) // GPSモジュールが通信可能の間
        {
            gps.encode(Serial2.read()); // GPSデータ取得
        }
    } while (millis() - start < ms); // 待ち
}

//******************************
// GPS情報 表示
//******************************
static void displayInfo()
{
    RTC_DateTypeDef RTC_DateStruct; // RTC 日付取得用
    RTC_TimeTypeDef RTC_TimeStruct; // RTC 時刻取得用

    M5.Rtc.GetDate(&RTC_DateStruct); // 時刻取得
    M5.Rtc.GetTime(&RTC_TimeStruct); // 日付取得

    if (gps.location.isValid()) // 測位有効
    {
        sprite.clear();         // 画面クリア
        sprite.setCursor(0, 0); // カーソル移動
        // 時刻表示
        sprite.setFont(&fonts::FreeMono9pt7b); // フォント設定(9pt)
        // 時刻表示
        sprite.printf("%04d/%02d/%02d %02d:%02d:%02d\n\n",
                      RTC_DateStruct.Year, RTC_DateStruct.Month, RTC_DateStruct.Date,
                      RTC_TimeStruct.Hours, RTC_TimeStruct.Minutes, RTC_TimeStruct.Seconds);

        sprite.setFont(&fonts::FreeMono18pt7b); // フォント設定(18pt)
        sprite.printf("ALT:");
        if (gps.altitude.isValid()) // 高度が正常なら
        {
            // 高度表示
            sprite.printf("%5d m\n", (int)gps.altitude.meters());
        }
        else // 高度異常
        {
            // 無効データ表示
            sprite.printf("----- m\n");
        }

        sprite.printf("SPD:");
        if (gps.speed.isValid()) // 速度が正常なら
        {
            // 速度表示
            sprite.printf("%5d km/h\n", (int)gps.speed.kmph());
        }
        else // 速度異常
        {
            // 無効データ表示
            sprite.printf("----- km/h\n");
        }

        // 位置情報表示
        sprite.setFont(&fonts::FreeMono12pt7b); // フォント設定
        sprite.printf("\n");
        sprite.printf("LAT:%3.6f\n", gps.location.lat());
        sprite.printf("LNG:%3.6f\n", gps.location.lng());
    }
    else // 測位無効
    {
        int x;                       // 文字横位置
        int y;                       // 文字縦位置
        char msg[] = "GPS OFF LINE"; // 表示メッセージ

        sprite.setFont(&fonts::FreeMono18pt7b);       // フォント設定
        x = lcd.width() / 2 - lcd.textWidth(msg) / 2; // 横センタリング位置
        y = lcd.height() / 2;                         // 縦センタリング位置
        sprite.setCursor(x, y);                       // カーソル移動
        sprite.printf("%s\n", msg);                   // メッセージ表示
    }

    sprite.pushSprite(0, 0); // バッファエリアをディスプレイに描画
    delay(MAIN_DELAY);       // ディレイ
}
