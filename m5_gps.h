
//********** define **********
#define SERIAL_SPEED 9600 // GPSモジュール通信速度
#define GPS_TX_PIN 13     // M5stack 送信ピンNo.
#define GPS_RX_PIN 14     // M5stack 受信ピンNo.
#define MAIN_DELAY 100    // メインループディレイ(ms)
#define MAX_STRING 256    // 文字列最大長
#define COLOR_DEPTH 8     // カラーモード(24bit)
#define RED_COMP 0xc2     // 赤
#define GREEN_COMP 0x89   // 緑
#define BLUE_COMP 0x4b    // 青
#define INIT_DELAY 2000   // イニシャル表示待ち

//********** プロトタイプ宣言 **********
static void readGPSInfo(unsigned long ms);
static void displayInfo();
static void initDisp();