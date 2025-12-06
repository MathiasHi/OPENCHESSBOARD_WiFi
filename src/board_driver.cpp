#include "openchessboard.h"

// LED Settings
#define NUM_LEDS 256
#define BRIGHTNESS 50

#define NUM_ROW 8
#define NUM_COL 8
#define NUM_LEDS_PER_FIELD 4

uint32_t leds[NUM_LEDS];

// HW GPIO configuration
const uint8_t LED_DATA_PIN = D2;

const uint8_t HALL_SENSE_ROW_1 = A0;
const uint8_t HALL_SENSE_ROW_2 = A1;
const uint8_t HALL_SENSE_ROW_3 = A2;
const uint8_t HALL_SENSE_ROW_4 = A3;
const uint8_t HALL_SENSE_ROW_5 = A4;
const uint8_t HALL_SENSE_ROW_6 = A5;
const uint8_t HALL_SENSE_ROW_7 = A6;
const uint8_t HALL_SENSE_ROW_8 = A7;

const uint8_t HALL_COL_1 = D3;
const uint8_t HALL_COL_2 = D4;
const uint8_t HALL_COL_3 = D5;
const uint8_t HALL_COL_4 = D6;
const uint8_t HALL_COL_5 = D7;
const uint8_t HALL_COL_6 = D8;
const uint8_t HALL_COL_7 = D9;
const uint8_t HALL_COL_8 = D10;

Adafruit_NeoPixel strip(NUM_LEDS, LED_DATA_PIN, NEO_GRB + NEO_KHZ800);

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

  strip.begin();
  strip.show(); // Initialize all pixels to 'off'
  strip.setBrightness(BRIGHTNESS);

  pinMode(HALL_SENSE_ROW_1, INPUT);
  pinMode(HALL_SENSE_ROW_2, INPUT);
  pinMode(HALL_SENSE_ROW_3, INPUT);
  pinMode(HALL_SENSE_ROW_4, INPUT);
  pinMode(HALL_SENSE_ROW_5, INPUT);
  pinMode(HALL_SENSE_ROW_6, INPUT);
  pinMode(HALL_SENSE_ROW_7, INPUT);
  pinMode(HALL_SENSE_ROW_8, INPUT);

  pinMode(HALL_COL_1, OUTPUT);
  pinMode(HALL_COL_2, OUTPUT);
  pinMode(HALL_COL_3, OUTPUT);
  pinMode(HALL_COL_4, OUTPUT);
  pinMode(HALL_COL_5, OUTPUT);
  pinMode(HALL_COL_6, OUTPUT);
  pinMode(HALL_COL_7, OUTPUT);
  pinMode(HALL_COL_8, OUTPUT);

  digitalWrite(HALL_COL_1, HIGH);
  digitalWrite(HALL_COL_2, HIGH);
  digitalWrite(HALL_COL_3, HIGH);
  digitalWrite(HALL_COL_4, HIGH);
  digitalWrite(HALL_COL_5, HIGH);
  digitalWrite(HALL_COL_6, HIGH);
  digitalWrite(HALL_COL_7, HIGH);
  digitalWrite(HALL_COL_8, HIGH);
  delay(10);
}

/* ---------------------------------------
 *  Function to write array to LED shift registers.
 *  Activates LEDs immediately.
 *  @params[in] byte array (max size 8 bytes)
 *  @return void
 */
void shiftOut(byte led_data_array[])
{

  strip.clear();

  for (int i = 0; i < NUM_ROW; i++)
  {
    for (int k = 0; k < NUM_COL; k++)
    {
      if (led_data_array[i] & (1 << k))
      {
        setLEDs(i, k, strip.Color(0, 0, 255));
      }
      else
      {
        uint index = i + k;

        if (index % 2 == 0)
        {
          setLEDs(i, k, strip.Color(20, 5, 0));
        }
        else
        {
          setLEDs(i, k, strip.Color(20, 20, 20));
        }
      }
    }
  }

  for (int z = 0; z < NUM_LEDS; z++)
  {
    strip.setPixelColor(z, leds[z]);
  }

  strip.show();
  delay(1);
}

void setLEDs(int row, int col, uint32_t color)
{
  int fieldIndex;

  if ((row % 2) == 0) {
    fieldIndex = (row * NUM_COL) + col;
  } else {
    fieldIndex = (row * NUM_COL) + NUM_COL - col - 1;
  }

  int startIndex = (fieldIndex * NUM_LEDS_PER_FIELD);

  for (int t = 0; t < NUM_LEDS_PER_FIELD; t++) {
    leds[startIndex + t] = color;
  }
}

void readHall(byte read_hall_array[])
{
  bool hall_val[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  for (int row_index = 0; row_index < 8; row_index++)
  {
    digitalWrite(HALL_COL_1, row_index != 0);
    digitalWrite(HALL_COL_2, row_index != 1);
    digitalWrite(HALL_COL_3, row_index != 2);
    digitalWrite(HALL_COL_4, row_index != 3);
    digitalWrite(HALL_COL_5, row_index != 4);
    digitalWrite(HALL_COL_6, row_index != 5);
    digitalWrite(HALL_COL_7, row_index != 6);
    digitalWrite(HALL_COL_8, row_index != 7);

    delay(1);
    hall_val[0] = digitalRead(HALL_SENSE_ROW_1);
    hall_val[1] = digitalRead(HALL_SENSE_ROW_2);
    hall_val[2] = digitalRead(HALL_SENSE_ROW_3);
    hall_val[3] = digitalRead(HALL_SENSE_ROW_4);
    hall_val[4] = digitalRead(HALL_SENSE_ROW_5);
    hall_val[5] = digitalRead(HALL_SENSE_ROW_6);
    hall_val[6] = digitalRead(HALL_SENSE_ROW_7);
    hall_val[7] = digitalRead(HALL_SENSE_ROW_8);

    for (int j = 0; j < 8; j++)
    {
      if (hall_val[j])
      {
        read_hall_array[j] &= ~(1UL << (row_index));
      }
      else
      {
        read_hall_array[j] |= 1UL << (row_index);
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
// DEBUG_SERIAL.println("trace: getMoveInput");
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
        if (state1 == 1 && state2 == 0)
        {
          ledBoardState[row_index] |= 1UL << (col_index);
          mvInput = mvInput + (String)columns[col_index] + (String)(row_index + 1);
          mvStarted = true;
          break;
        }
      }
    }
    if (StreamClient.available() && board_startupType == "WiFi")
    {
      moveStreamHandler();
    }
  }

  shiftOut(ledBoardState);

  // wait for end move event
  while (!mvFinished && is_game_running)
  {
    readHall(hallBoardState2);

    for (int row_index = 0; row_index < 8; row_index++)
    {
      for (int col_index = 0; col_index < 8; col_index++)
      {

        int state_prev = bitRead(hallBoardState1[row_index], col_index);

        int state1 = bitRead(hallBoardState2[row_index], col_index);

        if ((state1 == 1 && state_prev == 0))
        {
          mvFinished = true;
          ledBoardState[row_index] |= 1UL << (col_index);
          mvInput = mvInput + (String)columns[col_index] + (String)(row_index + 1);
        }
        else if ((state1 == 0 && state_prev == 1))
        {
          // took away a piece because he wants to take -> reset hallBoardState1
          readHall(hallBoardState1);
        }
      }
    }
    if (StreamClient.available() && board_startupType == "WiFi")
    {
      moveStreamHandler();
    }
  }

  shiftOut(ledBoardState);

  if (dimLEDs)
  {
    strip.setBrightness(150);
  }
  else
  {
    strip.setBrightness(BRIGHTNESS);
  }

  delay(300);
#endif
  clearDisplay();
  DEBUG_SERIAL.println(mvInput);
  return mvInput;
}

inline String createRow(const byte hallBoardState[], int row_index)
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

inline String createPiecesPlacement(const byte hallBoardState[])
{
  String piecesPlacement{};

  for (int row_index = 7; row_index > 0; row_index--)
  {
    piecesPlacement += createRow(hallBoardState, row_index);
    piecesPlacement += '/';
  }
  piecesPlacement += createRow(hallBoardState, 0);
  return piecesPlacement;
}

String createFen(void)
{
  byte hallBoardState[8];
  readHall(hallBoardState);
  //rotate90CounterClockwise(hallBoardState);
  //rotate180(hallBoardState);

  return createPiecesPlacement(hallBoardState);
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
 *  Function that displays vection animation.
 *  Writes to specific shift registers and flips states periodically by isr.
 *  @params[in] void
 *  @return void
 */
void displayConnectWait(void)
{
  byte connect_led_array[8] = {0};

  if (update_flipstate)
  {
    connect_led_array[0] = 0x10;
  }
  update_flipstate ^= true;

  shiftOut(connect_led_array);
  if (dimLEDs)
  {
    strip.setBrightness(150);
  }
  else
  {
    strip.setBrightness(BRIGHTNESS);
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
  led_data_array[row1] |= 1UL << col1;
  led_data_array[row2] |= 1UL << col2;
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

  if (update_flipstate)
  {
    boot_led_array[0] = 0x10;
  }
  update_flipstate ^= true;

  shiftOut(boot_led_array);
  DEBUG_SERIAL.println();
  if (dimLEDs)
  {
    strip.setBrightness(150);
  }
  else
  {
    strip.setBrightness(BRIGHTNESS);
  }
}

void displayUpdateWait(void)
{
  byte update_led_array[8] = {0};

  if (update_flipstate)
  {
    update_led_array[0] = 0x80;
  }
  update_flipstate ^= true;

  shiftOut(update_led_array);
  DEBUG_SERIAL.println();

  if (dimLEDs)
  {
    strip.setBrightness(150);
  }
  else
  {
    strip.setBrightness(BRIGHTNESS);
  }
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
    strip.setBrightness(150);
  }
  else
  {
    strip.setBrightness(BRIGHTNESS);
  }
}

void calculateDifference(byte result[], byte a[], byte b[])
{
  for (int i = 0; i < 8; i++)
  {
    result[i] = b[i] & ~a[i];
  }
}

void rotate180(byte arr[8])
{
  for (int i = 0; i < 4; i++)
  {
    // Reverse the bits in the byte at arr[i] and arr[7-i] and swap them
    byte temp = arr[i];
    arr[i] = arr[7 - i];
    arr[7 - i] = temp;

    // Reverse the bits in each byte after swapping
    arr[i] = (arr[i] & 0xF0) >> 4 | (arr[i] & 0x0F) << 4;
    arr[i] = (arr[i] & 0xCC) >> 2 | (arr[i] & 0x33) << 2;
    arr[i] = (arr[i] & 0xAA) >> 1 | (arr[i] & 0x55) << 1;

    arr[7 - i] = (arr[7 - i] & 0xF0) >> 4 | (arr[7 - i] & 0x0F) << 4;
    arr[7 - i] = (arr[7 - i] & 0xCC) >> 2 | (arr[7 - i] & 0x33) << 2;
    arr[7 - i] = (arr[7 - i] & 0xAA) >> 1 | (arr[7 - i] & 0x55) << 1;
  }
}

void displayArray(byte ledBoardState[])
{
  shiftOut(ledBoardState);
  if (dimLEDs)
  {
    strip.setBrightness(150);
  }
  else
  {
    strip.setBrightness(BRIGHTNESS);
  }
}

void displayFrame(byte frame[8])
{
  shiftOut(frame);
  if (dimLEDs)
  {
    strip.setBrightness(150);
  }
  else
  {
    strip.setBrightness(BRIGHTNESS);
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

void displayMoveRecect(String move)
{
  for (int k = 0; k < 3; k++)
  {
    clearDisplay();
    delay(200);
    displayMove(move);
    delay(200);
  }
}