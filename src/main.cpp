#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

// -------------------- PIN MAP --------------------
// MPU6050 (I2C)
static const int I2C_SDA = 21;
static const int I2C_SCL = 22;

// TFT ILI9341 (SPI)
static const int TFT_CS  = 5;
static const int TFT_DC  = 2;
static const int TFT_RST = 4;

// ESP32 VSPI default pins
static const int SPI_SCK  = 18;
static const int SPI_MISO = 19;
static const int SPI_MOSI = 23;

// -------------------- OBJECTS --------------------
Adafruit_MPU6050 mpu;
Adafruit_ILI9341 tft(TFT_CS, TFT_DC, TFT_RST);

// -------------------- UI CONSTANTS --------------------
static const int HEADER_H = 40;
static const uint16_t COL_BG     = ILI9341_BLACK;
static const uint16_t COL_HEADER = ILI9341_DARKCYAN;

static const uint16_t COL_ROLL  = ILI9341_GREEN;
static const uint16_t COL_PITCH = ILI9341_CYAN;
static const uint16_t COL_TEMP  = ILI9341_RED;

// Value areas (so we can refresh without flicker)
static const int LINE_X_LABEL = 10;
static const int LINE_X_VAL   = 110;
static const int LINE_Y1      = HEADER_H + 30;
static const int LINE_Y2      = HEADER_H + 70;
static const int LINE_Y3      = HEADER_H + 110;

static const int VAL_W = 200;
static const int VAL_H = 24;

static void drawHeader()
{
  tft.fillRect(0, 0, tft.width(), HEADER_H, COL_HEADER);
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, COL_HEADER);
  tft.setCursor(10, 12);
  tft.print("ERESENSE - MPU6050");
}

static void drawStaticLabels()
{
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE, COL_BG);

  tft.setCursor(LINE_X_LABEL, LINE_Y1);
  tft.print("Roll:");

  tft.setCursor(LINE_X_LABEL, LINE_Y2);
  tft.print("Pitch:");

  tft.setCursor(LINE_X_LABEL, LINE_Y3);
  tft.print("Temp:");
}

static void clearValueBox(int y)
{
  tft.fillRect(LINE_X_VAL, y, VAL_W, VAL_H, COL_BG);
}

static void printValue(int y, float value, const char* unit, uint16_t color, int decimals)
{
  clearValueBox(y);

  tft.setTextSize(2);
  tft.setTextColor(color, COL_BG);
  tft.setCursor(LINE_X_VAL, y);

  // Print with fixed decimals
  tft.print(value, decimals);
  tft.print(" ");
  tft.print(unit);
}

void setup()
{
  Serial.begin(115200);
  delay(200);

  // I2C init
  Wire.begin(I2C_SDA, I2C_SCL);

  // MPU6050 init
  if (!mpu.begin())
  {
    Serial.println("ERROR: MPU6050 not found!");
    while (true) delay(100);
  }

  // Optional tuning (not required, but nice)
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  // SPI + TFT init
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI, TFT_CS);
  tft.begin();

  // Portrait mode
  // 0 and 2 are portrait; choose 0 (USB at bottom depending on part)
  tft.setRotation(0);

  tft.fillScreen(COL_BG);
  drawHeader();
  drawStaticLabels();

  Serial.println("System started. Reading MPU6050...");
}

void loop()
{
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Accelerometer values are in m/s^2 in Adafruit lib
  float ax = a.acceleration.x;
  float ay = a.acceleration.y;
  float az = a.acceleration.z;

  // Roll & Pitch (degrees) from accelerometer
  float roll  = atan2f(ay, az) * 180.0f / PI;
  float pitch = atan2f(-ax, sqrtf(ay * ay + az * az)) * 180.0f / PI;

  // Temperature in Celsius
  float tC = temp.temperature;

  // Serial debug (requested)
  Serial.printf("Ax: %.2f Ay: %.2f Az: %.2f | Roll: %.2f deg Pitch: %.2f deg | Temp: %.1f C\n",
                ax, ay, az, roll, pitch, tC);

  // TFT update (requested: anlik guncelleme)
  printValue(LINE_Y1, roll,  "deg", COL_ROLL,  1);
  printValue(LINE_Y2, pitch, "deg", COL_PITCH, 1);
  printValue(LINE_Y3, tC,    "C",   COL_TEMP,  1);  // 0.1 hassasiyet

  delay(100); // ~10 Hz refresh
}
