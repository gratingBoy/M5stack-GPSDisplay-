//********************
// include
//********************
#include <M5Core2.h>
#include <TinyGPS++.h>
#include <LovyanGFX.h>
#include <time.h>
#include <WiFi.h>
#include <driver/i2s.h>
#include <SD.h>
#include "AudioFileSourceSD.h"
#include "AudioFileSourceID3.h"
#include "AudioGeneratorMP3.h"
#include "AudioOutputI2S.h"

//********************
//  define
//********************
#define SERIAL_SPEED 9600              // GPSモジュール通信速度
#define GPS_TX_PIN 13                  // M5stack 送信ピンNo.
#define GPS_RX_PIN 14                  // M5stack 受信ピンNo.
#define GPS_DELAY 100                  // GPSループディレイ(ms)
#define MAX_STRING 256                 // 文字列最大長
#define COLOR_DEPTH 8                  // カラーモード(24bit)
#define RED_COMP 0xc2                  // 赤
#define GREEN_COMP 0x89                // 緑
#define BLUE_COMP 0x4b                 // 青
#define INIT_DELAY 2000                // イニシャル表示待ち
#define ON 1                           // フラグON
#define OFF 0                          // フラグOFF
#define VERSION_NO "0.05"              // Version番号
#define MP3_DELAY 1                    // MP3再生待ちポーリング時間
#define OUTPUT_GAIN 0.5                // MP3再生音量
#define JST_OFFSET (9 * 3600)          // UTC->JSTのオフセット
#define I2S_NUM_0 0                    // 外部DAコンバータ設定
#define EXTERNAL_I2S 0                 // 外部DAコンバータ設定
#define BLCK_PIN 12                    // MP3 BLCK ピンNo.
#define WLCK_PIN 0                     // MP3 WLCK ピンNo.
#define DOUT_PIN 2                     // MP3出力ピンNo.
#define YEAR_OFFSET 1900               // 年設定時のオフセット
#define MONTH_OFFSET 1                 // 月設定時のオフセット
#define MAX_STR 32                     // 文字列最大長
#define REC_INTERVAL (60 * 1000)       // ログ書き込みのインターバル時間
#define ALT_MINIMUM_SAT 4              // 高度が取得可能な衛星数
#define LOW_LIMIT_VOLTAGE 3.2          // バッテリー電圧下限値
#define FONT_9 &fonts::FreeMono9pt7b   // フォント9pt
#define FONT_12 &fonts::FreeMono12pt7b // フォント12pt
#define FONT_18 &fonts::FreeMono18pt7b // フォント18pt
#define FONT_24 &fonts::FreeMono24pt7b // フォント24pt
//********** MP3再生タスク用 **********
#define MP3_TASK_NAME "PlayVoiceTask" // タスク名
#define MP3_STACK_SIZE 8192           // スタックエリアサイズ
#define MP3_TASK_PRIORITY 0           // タスク優先度
#define MP3_CORE_ID 0                 // マルチタスク コアID
//********** GSPタスク用 **********
#define GPS_TASK_NAME "gpsTask" // タスク名
#define GPS_STACK_SIZE 8192     // スタックエリアサイズ
#define GPS_TASK_PRIORITY 0     // タスク優先度
#define GPS_CORE_ID 1           // マルチタスク コアID

//********************
//  変数宣言
//********************
static TinyGPSPlus gps;          // GPS読み込み用
static LGFX lcd;                 // LCD描画インスタンス
static LGFX_Sprite sprite(&lcd); // スプライトエリアインスタンス
static int initFlag = ON;        // イニシャルフラグ
static int helloFlag = ON;       // あいさつ再生フラグ
static int recFlag = ON;         // 位置情報記録フラグ

//********************
//  プロトタイプ宣言
//********************
static void readGPSInfo(unsigned long ms);
static void displayInfo();
static void initGPS();
void playVoiceTask(void *arg);
void gpsTask(void *arg);
static void synchTime();
static void recodingGPSInfo();
static int getBatteryPersentage();
