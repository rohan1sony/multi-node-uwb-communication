#include <SPI.h>
#include <DW1000.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================================================
// RX1 = DEVICE 2
// RESPONSE SLOT = 0 ms
// =====================================================

#define SPI_SCK   18
#define SPI_MISO  19
#define SPI_MOSI  23

#define UWB_RST   27
#define UWB_IRQ   34
#define UWB_SS    21

#define I2C_SDA   4
#define I2C_SCL   5

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

#define DEVICE_ADDRESS  2
#define NETWORK_ID      10
#define UWB_CHANNEL      1

#define RESPONSE_DELAY   0

volatile bool received = false;
volatile bool sentAck = false;
volatile bool errorFlag = false;

uint32_t lastPacketNumber = 0;


// =====================================================
// CALLBACKS
// =====================================================

void handleReceived()
{
  received = true;
}

void handleSent()
{
  sentAck = true;
}

void handleError()
{
  errorFlag = true;
}


// =====================================================
// OLED
// =====================================================

void showOLED(
  String a,
  String b,
  String c,
  String d,
  String e = ""
)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println(a);

  display.setCursor(0, 12);
  display.println(b);

  display.setCursor(0, 24);
  display.println(c);

  display.setCursor(0, 36);
  display.println(d);

  display.setCursor(0, 48);
  display.println(e);

  display.display();
}


// =====================================================
// CONFIGURATION
// =====================================================

void configureDW1000()
{
  DW1000.newConfiguration();

  DW1000.setDefaults();

  DW1000.setDeviceAddress(
    DEVICE_ADDRESS
  );

  DW1000.setNetworkId(
    NETWORK_ID
  );

  DW1000.setChannel(
    UWB_CHANNEL
  );

  DW1000.enableMode(
    DW1000.MODE_LONGDATA_RANGE_LOWPOWER
  );
  DW1000.TRX_RATE_110KBPS;


  DW1000.commitConfiguration();
}


// =====================================================
// RECEIVE
// =====================================================

void startReceive()
{
  received = false;
  errorFlag = false;

  DW1000.newReceive();

  DW1000.setDefaults();

  DW1000.receivePermanently(true);

  DW1000.startReceive();

}


// =====================================================
// RESPONSE
// =====================================================

void sendResponse()
{
  String response =
    "HAI I AM DEVICE 2";


  // 0 ms slot

  if (RESPONSE_DELAY > 0)
  {
    delay(RESPONSE_DELAY);
  }


  sentAck = false;

  DW1000.newTransmit();

  DW1000.setDefaults();

  DW1000.setData(response);

  DW1000.startTransmit();
  


  unsigned long start =
    millis();


  while (
    !sentAck &&
    millis() - start < 1000
  )
  {
    delay(1);
  }
  showOLED("RX1","DEVICE 2","CHANNEL 1","REPLY SENT"," ");
  


  // Return to receive

  delay(5);

  startReceive();
}


// =====================================================
// PROCESS PACKET
// =====================================================

void processPacket()
{
  String message;

  DW1000.getData(message);


  if (
    message.startsWith("POLL,")
  )
  {
    int comma =
      message.indexOf(',');


    uint32_t number =
      message.substring(
        comma + 1
      ).toInt();
    


    // Ignore duplicate

    if (
      number == lastPacketNumber
    )
    {
      startReceive();
      return;
    }


    lastPacketNumber =
      number;


    sendResponse();
    showOLED("RX1","DEVICE 2","CHANNEL 1","MESSAGE RECEIVED","");
  }
  else
  {
    startReceive();
  }
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);


  Wire.begin(
    I2C_SDA,
    I2C_SCL
  );


  display.begin(
    SSD1306_SWITCHCAPVCC,
    0x3C
  );


  SPI.begin(
    SPI_SCK,
    SPI_MISO,
    SPI_MOSI
  );


  DW1000.begin(
    UWB_IRQ,
    UWB_RST
  );

  DW1000.select(
    UWB_SS
  );


  configureDW1000();


  DW1000.attachReceivedHandler(
    handleReceived
  );

  DW1000.attachSentHandler(
    handleSent
  );

  DW1000.attachReceiveFailedHandler(
    handleError
  );

  DW1000.attachErrorHandler(
    handleError
  );


  Serial.println("DEVICE 2");


  showOLED(
    "UWB READY",
    "DEVICE 2",
    "RX1",
    "SLOT: 0 ms",
    "WAITING"
  );


  startReceive();
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
  if (received)
  {
    received = false;

    processPacket();
    
  }


  if (errorFlag)
  {
    errorFlag = false;

    startReceive();
  }


  delay(1);
}
