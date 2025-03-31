#include "openchessboard.h"

// LED Settings
#define NUM_LEDS 256
#define BRIGHTNESS 255
CRGB leds[NUM_LEDS];

// HW GPIO configuration
const uint8_t LED_DATA_PIN = D2;
const uint8_t HALL_SENSE = A3;

int HALL_OUT_S0 = 10; // D10
int HALL_OUT_S1 = 9;  // D9
int HALL_OUT_S2 = 8;  // D8

int HALL_ROW_S0 = A7; // A7/D21
int HALL_ROW_S1 = A6; // A6/D20
int HALL_ROW_S2 = A5; // A5/D19


#define SENSE_THRS_NEG 2300
#define SENSE_THRS_POS 3000
#define SENSE_THRS_NO_POWER 800

/* ---------------------------------------
 *  Function to initiate GPIOs.
 *  Defines GPIOs input and output states.
 *  Depends on Arduino HW (adapt HW GPIO configuration to match Arduino Board)
 *  @params[in] void
 *  @return void
 */
void initHW(void)
{
  DEBUG_SERIAL.println("init HW");

  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);

/*
  pinMode(HALL_OUT_S0, OUTPUT);
  pinMode(HALL_OUT_S1, OUTPUT);
  pinMode(HALL_OUT_S2, OUTPUT);

  pinMode(HALL_ROW_S0, OUTPUT);
  pinMode(HALL_ROW_S1, OUTPUT);
  pinMode(HALL_ROW_S2, OUTPUT);*/

  pinMode(HALL_SENSE, INPUT);
}

/* ---------------------------------------
 *  Function to write array to LED shift registers.
 *  Activates LEDs immediately.
 *  @params[in] byte array (max size 8 bytes)
 *  @return void
 */
void shiftOut(byte led_data_array[])
{
  FastLED.clear();

  for (int i = 0; i < 8; i++)
  {
    for (int k = 0; k < 8; k++)
    {
      if (led_data_array[i] & (1 << k))
      {
        setLEDs(i, k);
      }
    }
  }
  FastLED.show();
}

void setLEDs(int row, int col)
{
  leds[row * 32 + (col * 2)] = CRGB::Red;
  leds[row * 32 + (col * 2) + 1] = CRGB::Red;
  leds[((((row + 1) * 32 - 1) - (col * 2)))] = CRGB::Red;
  leds[((((row + 1) * 32 - 2) - (col * 2)))] = CRGB::Red;
}

/* ---------------------------------------
 *  Function to ready Hall sensors states to array.
 *  Multiplexing all sensors. Sets 0 or 1 n array if threshold is exceeded.
 *  @params[in] byte array (max size 8 bytes)
 *  @return void
 */
void readHall(byte read_hall_array[])
{

  int hall_val = 0;

  for (int k = 0; k < 8; k++)
  {
    read_hall_array[k] = 0x00;
  }

  for (int row_index = 0; row_index < 8; row_index++)
  {

    bool bit0 = ((byte)row_index & (1 << 0)) != 0;
    bool bit1 = ((byte)row_index & (1 << 1)) != 0;
    bool bit2 = ((byte)row_index & (1 << 2)) != 0;
    digitalWrite(HALL_ROW_S0, bit0);
    digitalWrite(HALL_ROW_S1, bit1);
    digitalWrite(HALL_ROW_S2, bit2);

    for (int col_index = 0; col_index < 8; col_index++)
    {

      bool bit0 = ((byte)col_index & (1 << 0)) != 0;
      bool bit1 = ((byte)col_index & (1 << 1)) != 0;
      bool bit2 = ((byte)col_index & (1 << 2)) != 0;
      digitalWrite(HALL_OUT_S0, bit0);
      digitalWrite(HALL_OUT_S1, bit1);
      digitalWrite(HALL_OUT_S2, bit2);

      delay(1);
      hall_val = analogRead(HALL_SENSE);


      if (hall_val > SENSE_THRS_NO_POWER && (hall_val < SENSE_THRS_NEG || hall_val > SENSE_THRS_POS))
      {
        read_hall_array[row_index] |= 1UL << (col_index);
      }
    }
  }
}

void rotate90CounterClockwise(uint8_t hallBoardState[8])
{
  uint8_t rotated[8] = {0}; // New rotated board

  for (int row = 0; row < 8; row++)
  {
    for (int col = 0; col < 8; col++)
    {
      // Extract the bit from (row, col)
      uint8_t bit = (hallBoardState[row] >> col) & 1;

      // Place it at the rotated position
      rotated[7 - col] |= (bit << row);
    }
  }

  // Copy back the rotated result
  for (int i = 0; i < 8; i++)
  {
    hallBoardState[i] = rotated[i];
  }
}

/* ---------------------------------------
 *  Function that waits for a move input.
 *  Waits for a move input (blocking, but can be exited by isr if game is set to be not running)
 *  and returns move string.
 *  Example move: e2e4(piece moves from e2 to e4)
 *  @params[in] void
 *  @return String move_input
 */

String getMoveInput(void)
{
  const char columns[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
  String mvInput;

#ifdef MANUAL_MOVE_INPUT
  DEBUG_SERIAL.println("Enter a move with keyboard:");
  // Wait here until data is available

  while (!Serial.available() && is_game_running)
  {
    delay(10); // Small delay to prevent the loop from consuming too much CPU time
  }

  // Read input when data is available and newline is entered
  mvInput = Serial.readStringUntil('\n');
  mvInput.trim(); // Removes any whitespace or newline characters at the start or end
#else
  byte hallBoardStateInit[8];
  byte hallBoardState1[8];
  byte hallBoardState2[8];
  byte hallBoardState3[8];
  byte ledBoardState[8];

  for (int k = 0; k < 8; k++)
  {
    hallBoardStateInit[k] = 0x00;
    hallBoardState1[k] = 0x00;
    hallBoardState2[k] = 0x00;
    hallBoardState3[k] = 0x00;
    ledBoardState[k] = 0x00;
  }

  bool mvStarted = false;
  bool mvFinished = false;

  // get inital position
  readHall(hallBoardStateInit);

  // wait for Start move event
  while (!mvStarted && is_game_running)
  {
    readHall(hallBoardState1);

    for (int row_index = 0; row_index < 8; row_index++)
    {
      for (int col_index = 0; col_index < 8; col_index++)
      {

        int state1 = bitRead(hallBoardStateInit[row_index], col_index);
        int state2 = bitRead(hallBoardState1[row_index], col_index);
        if (state1 != state2)
        {
          ledBoardState[7 - row_index] |= 1UL << (7 - col_index);
#ifdef PLUG_AT_TOP
          mvInput = mvInput + (String)columns[7 - col_index] + (String)(7 - row_index + 1);
#else
          mvInput = mvInput + (String)columns[7 - row_index] + (String)(col_index + 1);
#endif
          mvStarted = true;
          break;
        }
      }
    }
  }

  shiftOut(ledBoardState);

  // wait for end move event
  while (!mvFinished && is_game_running)
  {
    readHall(hallBoardState2);
    delay(100);
    readHall(hallBoardState3);
    delay(100);

    for (int row_index = 0; row_index < 8; row_index++)
    {
      for (int col_index = 0; col_index < 8; col_index++)
      {

        int state_prev = bitRead(hallBoardState1[row_index], col_index);

        int hallBoardState1 = bitRead(hallBoardState2[row_index], col_index);
        int hallBoardState2 = bitRead(hallBoardState3[row_index], col_index);

        if ((hallBoardState1 != state_prev) && (hallBoardState2 != state_prev))
        {
          if (hallBoardState1 == hallBoardState2)
          {
            mvFinished = true;
            ledBoardState[7 - row_index] |= 1UL << (7 - col_index);

#ifdef PLUG_AT_TOP
            mvInput = mvInput + (String)columns[7 - col_index] + (String)(7 - row_index + 1);
#else
            mvInput = mvInput + (String)columns[7 - row_index] + (String)(col_index + 1);
#endif
          }
        }
      }
    }
  }

  shiftOut(ledBoardState);

  if (dimLEDs)
  {
    // analogWrite(LED_OE_N_PIN , 150);
  }
  else
  {
    // digitalWrite(LED_OE_N_PIN , 0);
  }

  delay(300);
#endif

  return mvInput;
}

inline String getRow(const byte hallBoardState[], int row_index)
{
  String row{};
  int count = 0;
  for (int col_index = 0; col_index < 8; col_index++)
  {
    int state = bitRead(hallBoardState[row_index], col_index);
    if (state)
    {
      if (count > 0)
        row += String(count);
      row += '?';
      count = 0;
    }
    else
    {
      count++;
    }
  }
  if (count > 0)
    row += String(count);
  return row;
}

inline String getPiecesPlacement(const byte hallBoardState[])
{
  String piecesPlacement{};

  for (int row_index = 7; row_index > 0; row_index--)
  {
    piecesPlacement += getRow(hallBoardState, row_index);
    piecesPlacement += '/';
  }
  piecesPlacement += getRow(hallBoardState, 0);
  return piecesPlacement;
}

String getFen(void)
{
  byte hallBoardState[8];
  readHall(hallBoardState);
  rotate90CounterClockwise(hallBoardState);

  return getPiecesPlacement(hallBoardState);
}

bool areFensSame(const String &peripheralFen, const String &centralFen)
{
  static const String piecesNames = "prbnkqPRBNKQ";
  if (peripheralFen.length() > centralFen.length())
  {
    return false;
  }
  for (int i = 0; i < peripheralFen.length(); i++)
  {
    if (peripheralFen[i] == '?')
    {
      if (piecesNames.indexOf(centralFen[i]) == -1)
      {
        return false;
      }
    }
    else if (peripheralFen[i] != centralFen[i])
    {
      return false;
    }
  }
  return true;
}

/* ---------------------------------------
 *  Function that clears all LED states.
 * Writes 0 to shift registers for all LEDs.
 *  @params[in] void
 *  @return void
 */
void clearDisplay(void)
{
  byte led_test_array[8];

  for (int k = 0; k < 8; k++)
  {
    led_test_array[k] = 0x00;
  }
  shiftOut(led_test_array);
}

/* ---------------------------------------
 *  Function that displays connection animation.
 *  Writes to specific shift registers and flips states periodically by isr.
 *  @params[in] void
 *  @return void
 */
void displayConnectWait(void)
{
  byte connect_led_array[8] = {0};

  if (connect_flipstate)
  {
    connect_led_array[3] = 0x10;
    connect_led_array[4] = 0x08;
  }
  else
  {
    connect_led_array[4] = 0x10;
    connect_led_array[3] = 0x08;
  }

  shiftOut(connect_led_array);
  if (dimLEDs)
  {
    // analogWrite(LED_OE_N_PIN , 150);
  }
  else
  {
    // digitalWrite(LED_OE_N_PIN , 0);
  }
}

/* ---------------------------------------
 *  Function that transforms a move string to led array.
 *  Writes move input to led array.
 *  @params[in] byte array, string move
 *  @return void
 */
void setDisplayMove(byte led_data_array[], String move_string)
{

  const char columns[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
  const char rows[] = {'1', '2', '3', '4', '5', '6', '7', '8'};

  int row1 = 0;
  int col1 = 0;
  int row2 = 0;
  int col2 = 0;

  for (int k = 0; k < 8; k++)
  {
    if (columns[k] == move_string.charAt(0))
    {
      col1 = k;
    }
    if (columns[k] == move_string.charAt(2))
    {
      col2 = k;
    }
  }

  for (int k = 0; k < 8; k++)
  {
    if (rows[k] == move_string.charAt(1))
    {
      row1 = k;
    }
    if (rows[k] == move_string.charAt(3))
    {
      row2 = k;
    }
  }
#ifdef PLUG_AT_TOP
  led_data_array[row1] |= 1UL << col1;
  led_data_array[row2] |= 1UL << col2;
#else
  led_data_array[col1] |= 1UL << 7 - row1;
  led_data_array[col2] |= 1UL << 7 - row2;
#endif
}

/* ---------------------------------------
 *  Function that displays booting animation.
 *  Writes to specific shift registers and flips states periodically by isr.
 *  @params[in] void
 *  @return void
 */
void displayBootWait(void)
{
  byte boot_led_array[8] = {0};

  if (boot_flipstate)
  {
#ifdef PLUG_AT_TOP
    boot_led_array[3] = 0x01;
#else
    boot_led_array[0] = 0x10;
#endif
  }

  shiftOut(boot_led_array);
  DEBUG_SERIAL.println();
  if (dimLEDs)
  {
    // analogWrite(LED_OE_N_PIN, 150);
  }
  else
  {
    // digitalWrite(LED_OE_N_PIN, 0);
  }

  delay(100);
}

/* ---------------------------------------
 *  Function that displays move.
 *  Writes to specific shift registers and show start and end position of piece movement.
 *  @params[in] string move
 *  @return void
 */
void displayMove(String last_move)
{
  byte led_test_array[8] = {0};

  setDisplayMove(led_test_array, last_move);

  shiftOut(led_test_array);
  if (dimLEDs)
  {
    // analogWrite(LED_OE_N_PIN, 150);
  }
  else
  {
    // digitalWrite(LED_OE_N_PIN, 0);
  }
}

void displayArray(byte ledBoardState[])
{
  shiftOut(ledBoardState);
  if (dimLEDs)
  {
    // analogWrite(LED_OE_N_PIN, 150);
  }
  else
  {
    // digitalWrite(LED_OE_N_PIN, 0);
  }
}

void displayFrame(byte frame[8])
{
  shiftOut(frame);
  if (dimLEDs)
  {
    // analogWrite(LED_OE_N_PIN, 150);
  }
  else
  {
    // digitalWrite(LED_OE_N_PIN, 0);
  }

  delay(100);
}

void displayNewGame(void)
{
  byte step1[8] = {0b00000000,
                   0b00000000,
                   0b00000000,
                   0b00011000,
                   0b00011000,
                   0b00000000,
                   0b00000000,
                   0b00000000};

  byte step2[8] = {0b00000000,
                   0b00000000,
                   0b00000000,
                   0b00000000,
                   0b00000000,
                   0b00000000,
                   0b00000000,
                   0b00000000};

  displayFrame(step1);
  delay(80);
  displayFrame(step2);
  delay(80);
  displayFrame(step1);
  delay(80);
  displayFrame(step2);
  delay(80);
  clearDisplay();
}

void displayWaitForGame(void)
{
  byte step1[8] = {0b00000000,
                   0b00000000,
                   0b00000000,
                   0b00010000,
                   0b00011000,
                   0b00000000,
                   0b00000000,
                   0b00000000};

  byte step2[8] = {0b00000000,
                   0b00000000,
                   0b00000000,
                   0b00011000,
                   0b00010000,
                   0b00000000,
                   0b00000000,
                   0b00000000};

  byte step3[8] = {0b00000000,
                   0b00000000,
                   0b00000000,
                   0b00011000,
                   0b00001000,
                   0b00000000,
                   0b00000000,
                   0b00000000};

  byte step4[8] = {0b00000000,
                   0b00000000,
                   0b00000000,
                   0b00001000,
                   0b00011000,
                   0b00000000,
                   0b00000000,
                   0b00000000};

  displayFrame(step4);
  delay(80);
  displayFrame(step3);
  delay(80);
  displayFrame(step2);
  delay(80);
  displayFrame(step1);
  delay(80);
  clearDisplay();
}
