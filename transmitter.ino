#include <SPI.h>
#include <DW1000.h>

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =====================================================
// ESP32 UWB PRO V1.3
// TX = DEVICE 1
//
// DEVICE 1 = TRANSMITTER
// DEVICE 2 = RX1
// DEVICE 3 = RX2
// DEVICE 4 = RX3
//
// RESPONSE SLOTS:
// DEVICE 2 = 0 ms
// DEVICE 3 = 15 ms
// DEVICE 4 = 30 ms
//
// TX RESPONSE WINDOW = 300 ms
// =====================================================
struct CmdWord {
  int deviceID;
  float distance;
  bool responseReceived;
};
CmdWord device1;

// =====================================================
// SPI
// =====================================================

#define SPI_SCK   18
#define SPI_MISO  19
#define SPI_MOSI  23


// =====================================================
// DW1000
// =====================================================

#define UWB_RST   27
#define UWB_IRQ   34
#define UWB_SS    21


// =====================================================
// OLED
// =====================================================

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


// =====================================================
// UWB SETTINGS
// =====================================================

#define DEVICE_ADDRESS  1
#define NETWORK_ID      10
#define UWB_CHANNEL      1

#define RESPONSE_TIMEOUT 1000


// =====================================================
// FLAGS
// =====================================================

volatile bool sentAck = false;
volatile bool received = false;
volatile bool errorFlag = false;


// =====================================================
// RESPONSE STATUS
// =====================================================

bool device2Received = false;
bool device3Received = false;
bool device4Received = false;


// =====================================================
// PACKET NUMBER
// =====================================================

uint32_t packetNumber = 0;


// =====================================================
// CALLBACKS
// =====================================================

void handleSent()
{
  sentAck = true;
}

void handleReceived()
{
  received = true;
}

void handleError()
{
  errorFlag = true;
}


// =====================================================
// OLED
// =====================================================

void showOLED(
  String line1,
  String line2,
  String line3,
  String line4,
  String line5 = ""
)
{
  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  display.setCursor(0, 0);
  display.println(line1);

  display.setCursor(0, 12);
  display.println(line2);

  display.setCursor(0, 24);
  display.println(line3);

  display.setCursor(0, 36);
  display.println(line4);

  display.setCursor(0, 48);
  display.println(line5);

  display.display();
}


// =====================================================
// DW1000 CONFIGURATION
// =====================================================

void configureDW1000()
{
  DW1000.newConfiguration();

  DW1000.setDefaults();

  DW1000.setDeviceAddress(DEVICE_ADDRESS);

  DW1000.setNetworkId(NETWORK_ID);

  DW1000.setChannel(UWB_CHANNEL);
  

  DW1000.enableMode(
    DW1000.MODE_LONGDATA_RANGE_LOWPOWER
  );

  DW1000.commitConfiguration();
}


// =====================================================
// START RECEIVE
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
// SEND POLL
// =====================================================

void sendPoll()
{
  packetNumber++;

  device2Received = false;
  device3Received = false;
  device4Received = false;


  String message =
    "POLL," + String(packetNumber);


  Serial.println();
  Serial.println("================================");
  Serial.println("BROADCAST TRANSMISSION");
  Serial.println("================================");

  Serial.println("SOURCE      : DEVICE 1");
  Serial.println("CHANNEL     : 1");

  Serial.print("PACKET      : ");
  Serial.println(packetNumber);

  Serial.print("COMMAND     : ");
  Serial.println(message);

  Serial.println("DESTINATION : ALL DEVICES");

  Serial.println("================================");


  showOLED(
    "BROADCAST",
    "DEVICE 1",
    "PACKET " + String(packetNumber),
    "Sending POLL...",
    ""
  );


  // ===================================================
  // TRANSMIT POLL
  // ===================================================

  sentAck = false;

  DW1000.newTransmit();

  DW1000.setDefaults();

  DW1000.setData(message);

  DW1000.startTransmit();


  // ===================================================
  // WAIT FOR TX COMPLETE
  // ===================================================

  unsigned long txStart = millis();

  while (
    !sentAck &&
    millis() - txStart < 1000
  )
  {
    delay(1);
  }


  if (!sentAck)
  {
    Serial.println("POLL TRANSMISSION FAILED");

    startReceive();

    return;
  }


  Serial.println("POLL SENT");


  // ===================================================
  // IMMEDIATELY ENTER RECEIVE MODE
  // ===================================================

  startReceive();


  // ===================================================
  // LISTEN FOR ALL THREE DEVICES
  // ===================================================

  unsigned long receiveStart = millis();

  while (
    millis() - receiveStart <
    RESPONSE_TIMEOUT
  )
  {
    if (received)
    {
      received = false;

      String response;

      DW1000.getData(response);


      Serial.print("RX: ");
      Serial.println(response);


      // ===============================================
      // DEVICE 2
      // ===============================================

      if (
        response.indexOf("DEVICE 2") >= 0
      )
      {
        device2Received = true;

        Serial.println(
          "DEVICE 2 RESPONSE RECEIVED"
        );
      }


      // ===============================================
      // DEVICE 3
      // ===============================================

      else if (
        response.indexOf("DEVICE 3") >= 0
      )
      {
        device3Received = true;

        Serial.println(
          "DEVICE 3 RESPONSE RECEIVED"
        );
      }
      


      // ===============================================
      // DEVICE 4
      // ===============================================

      else if (
        response.indexOf("DEVICE 4") >= 0
      )
      {
        device4Received = true;

        Serial.println(
          "DEVICE 4 RESPONSE RECEIVED"
        );
      }


      // ===============================================
      // CONTINUE LISTENING
      // ===============================================

      startReceive();
    }


    if (errorFlag)
    {
      errorFlag = false;

      startReceive();
    }


    delay(1);
  }


  // ===================================================
  // FINAL RESULT
  // ===================================================

  DW1000.idle();


  Serial.println();
  Serial.println("================================");
  Serial.println("RESPONSE SUMMARY");
  Serial.println("================================");

  Serial.print("DEVICE 2 : ");

  if (device2Received)
    Serial.println("RECEIVED");
  else
    Serial.println("NO RESPONSE");


  Serial.print("DEVICE 3 : ");

  if (device3Received)
    Serial.println("RECEIVED");
  else
    Serial.println("NO RESPONSE");


  Serial.print("DEVICE 4 : ");

  if (device4Received)
    Serial.println("RECEIVED");
  else
    Serial.println("NO RESPONSE");


  Serial.println("================================");


  // ===================================================
  // OLED RESULT
  // ===================================================

  String d2 =
    device2Received ? "D2: OK" : "D2: --";

  String d3 =
    device3Received ? "D3: OK" : "D3: --";

  String d4 =
    device4Received ? "D4: OK" : "D4: --";


  showOLED(
    "PACKET " + String(packetNumber),
    d2,
    d3,
    d4,
    "DONE"
  );
}


// =====================================================
// SETUP
// =====================================================

void setup()
{
  Serial.begin(115200);

  delay(1000);


  // OLED

  Wire.begin(
    I2C_SDA,
    I2C_SCL
  );


  if (
    !display.begin(
      SSD1306_SWITCHCAPVCC,
      0x3C
    )
  )
  {
    while (1)
    {
      delay(1000);
    }
  }


  showOLED(
    "ESP32 UWB PRO",
    "TX",
    "DEVICE 1",
    "Starting...",
    ""
  );


  delay(1000);


  // SPI

  SPI.begin(
    SPI_SCK,
    SPI_MISO,
    SPI_MOSI
  );


  // DW1000

  DW1000.begin(
    UWB_IRQ,
    UWB_RST
  );

  DW1000.select(
    UWB_SS
  );


  configureDW1000();


  // CALLBACKS

  DW1000.attachSentHandler(
    handleSent
  );

  DW1000.attachReceivedHandler(
    handleReceived
  );

  DW1000.attachReceiveFailedHandler(
    handleError
  );

  DW1000.attachErrorHandler(
    handleError
  );


  Serial.println();
  Serial.println("DEVICE 1 READY");
  Serial.println("SLOTS: 0 / 15 / 30 ms");


  showOLED(
    "UWB READY",
    "DEVICE 1",
    "CH: 1",
    "SLOTS 0/15/30",
    "Type POLL"
  );
}


// =====================================================
// LOOP
// =====================================================

void loop()
{
  if (
    Serial.available()
  )
  {
    String command =
      Serial.readStringUntil('\n');

    command.trim();

    command.toLowerCase();


    if (
      command == "poll" ||
      command == "p"
    )
    {
      sendPoll();
    }
  }
}
