#include "m5_gps.h"

//******************************
// セットアップ
//******************************
void setup()
{
    M5.begin(true, true, true, false, kMBusModeInput);               // M5stack通信開始
    M5.Axp.SetSpkEnable(true);                                       // スピーカー有効化
    WiFi.mode(WIFI_OFF);                                             // Wi-Fi OFF
    Serial2.begin(SERIAL_SPEED, SERIAL_8N1, GPS_TX_PIN, GPS_RX_PIN); // GPSモジュール通信開始
    lcd.init();                                                      // 画面初期化
    sprite.setColorDepth(COLOR_DEPTH);                               // カラーモード
    sprite.setTextColor(ORANGE, BLACK);                              // フォントカラー設定
    sprite.setTextSize(1);                                           // テキストサイズ設定
    sprite.createSprite(lcd.width(), lcd.height());                  // スプライトエリア作成

    // MP3再生タスク起動
    xTaskCreatePinnedToCore(playVoiceTask, MP3_TASK_NAME, MP3_STACK_SIZE,
                            NULL, MP3_TASK_PRIORITY, NULL, MP3_CORE_ID);
    // GPS処理タスク起動
    xTaskCreatePinnedToCore(gpsTask, GPS_TASK_NAME, GPS_STACK_SIZE,
                            NULL, GPS_TASK_PRIORITY, NULL, GPS_CORE_ID);
}

//******************************
// 音声再生タスク
//******************************
void playVoiceTask(void *arg)
{
    AudioGeneratorMP3 *mp3;
    AudioFileSourceSD *file;
    AudioOutputI2S *out;
    AudioFileSourceID3 *id3;

    // ハード側 MP3再生設定
    out = new AudioOutputI2S(I2S_NUM_0, EXTERNAL_I2S); // 外部D/Aコンバータ設定
    out->SetPinout(BLCK_PIN, WLCK_PIN, DOUT_PIN);      // 出力ピン設定
    out->SetOutputModeMono(true);                      // スピーカーをモノラルモードに設定
    out->SetGain((float)OUTPUT_GAIN);                  // 音量設定
                                                       // MP3再生クラスのインスタンス生成
    file = new AudioFileSourceSD("/voice/hello.mp3");
    id3 = new AudioFileSourceID3(file);
    mp3 = new AudioGeneratorMP3();

    if (helloFlag == ON)
    {
        mp3->begin(id3, out);
        helloFlag = OFF;
    }

    while (true)
    {

        if (mp3->isRunning()) // 再生中か
        {
            if (!mp3->loop()) // 再生が終了するまで待ち
            {
                vTaskDelay(MP3_DELAY);
            }
        }
        vTaskDelay(MP3_DELAY);
    }
}

//******************************
// GPS処理タスク
//******************************
void gpsTask(void *arg)
{
    while (true)
    {
        readGPSInfo(GPS_DELAY); // GPS情報読み込み
        if (initFlag == ON)     // 初回起動か
        {
            initGPS(); // GPSモジュールのイニシャル待ち
        }
        else // 初回起動でない
        {
            displayInfo(); // GPS情報画面表示
        }
        vTaskDelay(GPS_DELAY);
    }
}

//******************************
// メインループ
//******************************
void loop()
{
    vTaskDelay(GPS_DELAY); // メイン処理ディレイ
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
    float batVoltage;               //バッテリー電圧
    float batPercentage;            // バッテリー充電率

    // バッテリー電圧取得
    batVoltage = M5.Axp.GetBatVoltage();
    // バッテリー充電率取得
    batPercentage = (batVoltage < LOW_LIMIT_VOLTAGE) ? 0 : (batVoltage - LOW_LIMIT_VOLTAGE) * 100;

    M5.Rtc.GetDate(&RTC_DateStruct); // 日付取得
    M5.Rtc.GetTime(&RTC_TimeStruct); // 時刻取得

    sprite.clear();          // 画面クリア
    sprite.setCursor(0, 0);  // カーソル移動
    sprite.setFont(FONT_12); // フォント設定
    // 起動メッセージ表示
    sprite.printf("Talking GPS Logger\n");
    sprite.printf("Version:%s\n", VERSION_NO);
    sprite.printf("Product:abtroG 2022\n");
    sprite.printf("Voice:Reina Shirakaba\n");
    sprite.printf("Date:%04d/%02d/%02d\n",
                  RTC_DateStruct.Year, RTC_DateStruct.Month, RTC_DateStruct.Date);
    sprite.printf("RTC:%02d:%02d:%02d\n",
                  RTC_TimeStruct.Hours, RTC_TimeStruct.Minutes, RTC_TimeStruct.Seconds);
    sprite.printf("Battery:%d%%\n", (int)batPercentage);
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
        vTaskDelay(INIT_DELAY);  // イニシャル表示ディレイ
    }
    sprite.pushSprite(0, 0); // バッファエリアをディスプレイに描画
    vTaskDelay(GPS_DELAY);
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
    unsigned long recTime;          // 時刻計測用
    float batVoltage;               //バッテリー電圧
    float batPercentage;            // バッテリー充電率

    // バッテリー電圧取得
    batVoltage = M5.Axp.GetBatVoltage();
    // バッテリー充電率取得
    batPercentage = (batVoltage < LOW_LIMIT_VOLTAGE) ? 0 : (batVoltage - LOW_LIMIT_VOLTAGE) * 100;

    M5.Rtc.GetDate(&RTC_DateStruct); // 時刻取得
    M5.Rtc.GetTime(&RTC_TimeStruct); // 日付取得

    if (gps.location.isValid()) // 測位有効
    {
        sprite.clear();         // 画面クリア
        sprite.setCursor(0, 0); // カーソル移動
        // 時刻表示
        sprite.setFont(FONT_9); // フォント設定(9pt)
        // 時刻表示
        sprite.printf("%04d/%02d/%02d %02d:%02d:%02d BAT:%02d%%\n\n",
                      RTC_DateStruct.Year, RTC_DateStruct.Month, RTC_DateStruct.Date,
                      RTC_TimeStruct.Hours, RTC_TimeStruct.Minutes, RTC_TimeStruct.Seconds,
                      (int)batPercentage);

        if (gps.satellites.value() > 0) // 受信できる衛星あり
        {
            sprite.setFont(FONT_18); // フォント設定(18pt)
            sprite.printf("ALT:\n");
            if (gps.satellites.value() >= ALT_MINIMUM_SAT) // 高度が取得可能なら
            {
                sprite.setFont(FONT_24); // フォント設定(18pt)
                // 高度表示
                sprite.printf("%9d m\n", (int)gps.altitude.meters());
            }
            else // 高度異常
            {
                sprite.setFont(FONT_24); // フォント設定
                // 無効データ表示
                sprite.printf("    ----- m\n");
            }
            sprite.setFont(FONT_18); // フォント設定
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
            sprite.setFont(FONT_12); // フォント設定
            sprite.printf("\n");
            sprite.printf("LAT:%3.6f\n", gps.location.lat());
            sprite.printf("LNG:%3.6f\n", gps.location.lng());
            // 記録フラグONなら
            if (recFlag == ON)
            {
                // ログをSDカードに書き込み
                recodingGPSInfo();
                // 記録時刻更新
                recTime = millis();
                recFlag = OFF;
            }
            // 指定時間経過したか
            if ((millis() - recTime) > REC_INTERVAL)
            {
                // 記録フラグON
                recFlag = ON;
            }
        }
        else // 受信できる衛星なし
        {
            sprite.setFont(FONT_18);             // フォント設定
            sprite.printf("\n\nGPS OFF LINE\n"); // メッセージ表示
        }
    }
    else // 測位無効
    {
        sprite.setFont(FONT_18);             // フォント設定
        sprite.printf("\n\nGPS OFF LINE\n"); // メッセージ表示
    }

    sprite.pushSprite(0, 0); // バッファエリアをディスプレイに描画
    vTaskDelay(GPS_DELAY);   // ディレイ
}

//******************************
// GPS情報 記録
//******************************
static void recodingGPSInfo()
{
    File recFile;                   // ファイルポインタ
    char logFilename[MAX_STR];      //  ファイル名
    RTC_DateTypeDef RTC_DateStruct; // Date
    RTC_TimeTypeDef RTC_TimeStruct; // Time

    // ログファイル名初期化
    memset(logFilename, 0x00, sizeof(logFilename));

    M5.Rtc.GetDate(&RTC_DateStruct); // 日付取得
    M5.Rtc.GetTime(&RTC_TimeStruct); // 時刻取得

    // ファイル名生成
    sprintf(logFilename, "/%04d%02d%02d.log",
            RTC_DateStruct.Year,
            RTC_DateStruct.Month,
            RTC_DateStruct.Date);

    // ログファイルが存在するか確認し、なければ新規作成する
    if (SD.exists(logFilename) == true) // ログファイルが存在する
    {
        // SDカードのファイルオープン(追記モード)
        recFile = SD.open(logFilename, FILE_APPEND);
    }
    else // ファイルが存在しない
    {
        // SDカードのファイルオープン(新規作成)
        recFile = SD.open(logFilename, FILE_WRITE);
    }

    // SDカードへログを書き込み
    // フォーマットは以下
    // YYYY/MM/DD,hh:mm:ss,高度,速度,緯度,経度
    recFile.printf("%d/%02d/%02d,%02d:%02d:%02d,%d,%d,%lf,%lf\n",
                   RTC_DateStruct.Year,
                   RTC_DateStruct.Month,
                   RTC_DateStruct.Date,
                   RTC_TimeStruct.Hours,
                   RTC_TimeStruct.Minutes,
                   RTC_TimeStruct.Seconds,
                   (int)gps.altitude.meters(),
                   (int)gps.speed.kmph(),
                   gps.location.lat(),
                   gps.location.lng());
    // ファイルクローズ
    recFile.close();
}
