////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                            //
//          Improved AtTiny85 Tetris Game                                                                     //
//             by: Benjamin Cance (bjc@tdx.li) on 11-Sept-2024                                                //
//                                                                                                            //
//          Use of this code is permitted in line with the MIT license                                        //
//                                                                                                            //
//          You may view a working sample of this code at Wokwi, at this link:                                //
//                                                                                                            //
//                                                                                                            //
//                                                                                                            //
//          It is intended that you connect your Tiny85 as follows:                                           //
//                                                                                                            //
//                                                    +-------+                                               //
//                                           Drop ----| 1   8 |---- VCC                                       //
//                                     Left Button ---| 2   7 |---- SCL (OLED)                                //
//                                    Right Button ---| 3   6 |---- SDA (OLED)                                //
//                                             GND ---| 4   5 |---- Rotate Button                             //
//                                                    +-------+                                               //
//                                                                                                            //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include <EEPROM.h>
#include <TinyWireM.h>
#define TINY4KOLED_QUICK_BEGIN
#include <Tiny4kOLED.h>
#include <avr/wdt.h>

#define PIN_LEFT PB1
#define PIN_RIGHT PB4
#define PIN_ROTATE PB3
#define PIN_DROP PB0

#define SCREEN_WIDTH 16
#define SCREEN_HEIGHT 24
#define EEPROM_ADDR 4
#define DEBOUNCE_TIME 50 
#define RESET_DURATION 2000 // hold Reset for 2 seconds 

extern const uint8_t nums[80] PROGMEM = {
0x0,0xe,0x11,0x19,0x15,0x13,0x11,0xe,
0x0,0xe,0x4,0x4,0x4,0x4,0xc,0x4,
0x0,0x1f,0x8,0x4,0x2,0x1,0x11,0xe,
0x0,0xe,0x11,0x1,0x2,0x4,0x2,0x1f,
0x0,0x2,0x2,0x1f,0x12,0xa,0x6,0x2,
0x0,0xe,0x11,0x1,0x1,0x1e,0x10,0x1f,
0x0,0xe,0x11,0x11,0x1e,0x10,0x8,0x6,
0x0,0x8,0x8,0x8,0x4,0x2,0x1,0x1f,
0x0,0xe,0x11,0x11,0xe,0x11,0x11,0xe,
0x0,0xc,0x2,0x1,0xf,0x11,0x11,0xe,
};

extern const uint8_t pieces[7][16] PROGMEM = {{
0,0,1,0,
0,0,1,0,
0,0,1,0,
0,0,1,0
},{
0,0,0,0,
0,1,1,0,
0,1,1,0,
0,0,0,0
},{
0,0,0,0,
0,1,1,0,
0,0,1,1,
0,0,0,0
},{
0,0,0,0,
0,1,1,0,
1,1,0,0,
0,0,0,0
},{
0,0,0,0,
1,1,1,0,
0,1,0,0,
0,0,0,0
},{
0,1,0,0,
0,1,0,0,
1,1,0,0,
0,0,0,0
},{
0,1,0,0,
0,1,0,0,
0,1,1,0,
0,0,0,0
}};

struct GameState {
    uint16_t piece;
    int8_t x;
    int8_t y;
    uint8_t nextPiece : 3;
    uint8_t level : 5;
    uint8_t linesCleared : 7;
    uint16_t score;
    uint16_t highScore;
    uint8_t gameOver : 1;
    uint8_t madeLine : 1;
} game;

uint8_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT / 8];
uint16_t await;
unsigned long ms, elapsed, lastMoved, lastRotated;
const uint16_t awaitTimes[] PROGMEM = {720, 640, 580, 500, 440, 360, 300, 220, 140, 100, 80, 60, 40, 20};

ISR(TIMER1_COMPA_vect) {
    ms += 10; 
} // Timer1 Comparison that increments every 10ms


// This handles button presses for all game controls (drop, left, right, rotate)
// It implements debouncing for each button and triggers the appropriate game action
// when a button press is detected. The routine is called whenever the state of any
// of the configured pins changes (both on press and release).
ISR(PCINT0_vect) {
    static uint8_t lastState = 0xFF;
    static uint32_t lastDebounceTime[4] = {0}; 
    uint8_t state = PINB;
    uint8_t changed = lastState ^ state;
    uint32_t now = ms;

    uint8_t buttons[4] = {PIN_DROP, PIN_LEFT, PIN_RIGHT, PIN_ROTATE};

    for (uint8_t i = 0; i < 4; i++) {
        if (changed & (1 << buttons[i])) {
            if (now - lastDebounceTime[i] > DEBOUNCE_TIME) {
                if (!(state & (1 << buttons[i]))) { 
                    switch (buttons[i]) {
                        case PIN_LEFT:
                            if (canMove(-1, 0)) game.x--;
                            break;
                        case PIN_RIGHT:
                            if (canMove(1, 0)) game.x++;
                            break;
                        case PIN_ROTATE:
                            rotatePiece();
                            break;
                        case PIN_DROP:
                            await = 20; 
                            break;
                    }
                }
                lastDebounceTime[i] = now;
            }
        }
    }
    lastState = state;
} // Pin Change (PC...) Interrupt Service Routine

void setup() {
    TCCR1 = (1 << CTC1) | (1 << CS12) | (1 << CS11); // CTC mode, prescaler 32
    OCR1C = 77;                                      // For 10ms at 8MHz
    TIMSK |= (1 << OCIE1A);                          // Enable Timer1 Compare Interrupt

    // Set up pin change interrupt for buttons
    PCMSK |= (1 << PIN_DROP) | (1 << PIN_LEFT) | (1 << PIN_RIGHT) | (1 << PIN_ROTATE);
    GIMSK |= (1 << PCIE);                            // Enable pin change interrupt

    // Set up ADC for random seed
    ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1); // Enable ADC, prescaler 64

    // Configure button pins as inputs with pull-ups
    DDRB &= ~((1 << PIN_DROP) | (1 << PIN_LEFT) | (1 << PIN_RIGHT) | (1 << PIN_ROTATE));
    PORTB |= (1 << PIN_DROP) | (1 << PIN_LEFT) | (1 << PIN_RIGHT) | (1 << PIN_ROTATE);

    randomSeed(getRandomSeed());
    oled.begin();
    initGame();

    sei(); 
    wdt_enable(WDTO_2S);  // Enable 2-second watchdog in case we hang up
}

uint16_t getRandomSeed() {
    uint16_t seed = 0;
    for (uint8_t i = 0; i < 16; i++) {
        ADCSRA |= (1 << ADSC);
        while (ADCSRA & (1 << ADSC)); 
        seed = (seed << 1) | (ADC & 1); 
    }
    return seed;
}

void reseedRNG() {
    static uint32_t lastReseed = 0;
    if (ms - lastReseed > 10000) { 
        uint16_t seed = PINB; 
        for (uint8_t i = 0; i < 8; i++) {
            seed = (seed << 1) | (analogRead(0) & 1); 
        }
        randomSeed(seed);
        lastReseed = ms;
    }
}

void initGame() {
    game = {0};
    uint8_t lowByte, highByte;
    eepromReadWrite(EEPROM_ADDR, &lowByte, false);
    eepromReadWrite(EEPROM_ADDR + 1, &highByte, false);
    game.highScore = ((uint16_t)highByte << 8 | lowByte) * 10;
    spawnNewPiece();
    elapsed = ms;
    getAwait();
    
    pinMode(PIN_LEFT, INPUT_PULLUP);
    pinMode(PIN_RIGHT, INPUT_PULLUP);
    pinMode(PIN_ROTATE, INPUT_PULLUP);
    
    oled.clear();
    drawNextPiece();
    writeHighScore();
    oled.on();
}

void draw() {
    oled.clear();

    for (uint8_t y = 0; y < SCREEN_HEIGHT; y++) {
        for (uint8_t x = 0; x < SCREEN_WIDTH; x++) {
            if (pixels[y * SCREEN_WIDTH / 8 + x / 8] & (1 << (x % 8))) {
                oled.setCursor(x * 3, y / 8);
                oled.startData();
                oled.sendData(0b111 << (y % 8));
                oled.endData();
            }
        }
    }

    for (uint8_t i = 0; i < 4; i++) {
        for (uint8_t j = 0; j < 4; j++) {
            if (game.piece & (1 << (i * 4 + j))) {
                uint8_t x = game.x + j;
                uint8_t y = game.y + i;
                oled.setCursor(x * 3, y / 8);
                oled.startData();
                oled.sendData(0b111 << (y % 8));
                oled.endData();
            }
        }
    }
}

void drawNextPiece() {
    for (uint8_t y = 0; y < 4; y++) {
        oled.setCursor(SCREEN_WIDTH * 3 + 3, y);
        oled.startData();
        oled.sendData(0);
        oled.sendData(0);
        oled.sendData(0);
        oled.endData();
    }

    uint16_t nextPiece = pgm_read_word(&pieces[game.nextPiece]);
    for (uint8_t i = 0; i < 4; i++) {
        for (uint8_t j = 0; j < 4; j++) {
            if (nextPiece & (1 << (i * 4 + j))) {
                oled.setCursor(SCREEN_WIDTH * 3 + 3 + j * 3, i);
                oled.startData();
                oled.sendData(0b111);
                oled.endData();
            }
        }
    }
}

void writeScore() {
    oled.setCursor(SCREEN_WIDTH * 3 + 3, 5);
    oled.print(game.score);
}

void writeHighScore() {
    oled.setCursor(SCREEN_WIDTH * 3 + 3, 6);
    oled.print(game.highScore);
}

void handleInput() {
    uint8_t buttonState = PINB;
    uint8_t pressedButtons = ~buttonState & ((1 << PIN_RIGHT) | (1 << PIN_LEFT) | (1 << PIN_ROTATE) | (1 << PIN_DROP));
    
    if (pressedButtons) {
        if (ms - lastMoved >= 100) {
            if (pressedButtons & (1 << PIN_RIGHT) && canMove(1, 0)) game.x++;
            if (pressedButtons & (1 << PIN_LEFT) && canMove(-1, 0)) game.x--;
            if (pressedButtons & (1 << PIN_ROTATE) && ms - lastRotated >= 150) {
                rotatePiece();
                lastRotated = ms;
            }
            if (pressedButtons & (1 << PIN_DROP)) await = 20;
            lastMoved = ms;
        }
    }

    getAwait();
}

void updateGameState() {
    if (ms - elapsed >= await) {
        if (canMove(0, 1)) {
            game.y++;
        } else {
            placePiece();
            checkLines();
            spawnNewPiece();
            checkGameOver();
        }
        elapsed = ms;
    }
}

bool canMove(int8_t dx, int8_t dy) {
    uint16_t piece = game.piece;
    int8_t newX = game.x + dx;
    int8_t newY = game.y + dy;
    
    for (uint8_t i = 0; i < 4; i++) {
        for (uint8_t j = 0; j < 4; j++) {
            if (piece & (1 << (i * 4 + j))) {
                int8_t x = newX + j;
                int8_t y = newY + i;
                if (x < 0 || x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT || 
                    (y >= 0 && (pixels[y * SCREEN_WIDTH / 8 + x / 8] & (1 << (x % 8))))) {
                    return false;
                }
            }
        }
    }
    return true;
}

void rotatePiece() {
    uint16_t rotated = 0;
    uint16_t original = game.piece;

    for (uint8_t i = 0; i < 4; i++) {
        for (uint8_t j = 0; j < 4; j++) {
            if (original & (1 << (i * 4 + j))) {
                rotated |= (1 << (j * 4 + (3 - i)));
            }
        }
    }

    int8_t oldX = game.x;
    game.piece = rotated;

    if (!canMove(0, 0)) {
        if (canMove(-1, 0)) game.x--;
        else if (canMove(1, 0)) game.x++;
        else if (canMove(-2, 0)) game.x -= 2;
        else if (canMove(2, 0)) game.x += 2;
        else {
            game.piece = original;
            game.x = oldX;
        }
    }
}

void placePiece() {
    for (uint8_t i = 0; i < 4; i++) {
        for (uint8_t j = 0; j < 4; j++) {
            if (game.piece & (1 << (i * 4 + j))) {
                int pos = (game.y + i) * SCREEN_WIDTH + game.x + j;
                pixels[pos / 8] |= 1 << (pos % 8);
            }
        }
    }
}

void checkLines() {
    uint8_t nlines = 0;
    for (int8_t y = SCREEN_HEIGHT - 2; y >= 0; y--) {
        uint16_t row = (pixels[y * SCREEN_WIDTH / 8] << 8) | pixels[y * SCREEN_WIDTH / 8 + 1];
        if ((row & 0x07FE) == 0x07FE) {
            nlines++;
            for (int8_t y2 = y; y2 > 0; y2--) {
                pixels[y2 * SCREEN_WIDTH / 8] = pixels[(y2 - 1) * SCREEN_WIDTH / 8];
                pixels[y2 * SCREEN_WIDTH / 8 + 1] = pixels[(y2 - 1) * SCREEN_WIDTH / 8 + 1];
            }
            pixels[0] = 0x80;
            pixels[1] = 0x01;
            y++;
        }
    }
    if (nlines > 0) {
        updateScore(nlines);
        game.madeLine = true;
        oled.setInverse(true);
    }
}

void spawnNewPiece() {
    game.piece = pgm_read_word(&pieces[game.nextPiece]);
    game.x = 4;
    game.y = 0;
    game.nextPiece = getRandomPiece();
    drawNextPiece();
    writeHighScore();
}

void checkGameOver() {
    if (game.y <= 1) {
        game.gameOver = true;
        oled.setInverse(true);
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
        sleep_mode();
    }
}

uint8_t getRandomPiece() {
    static uint8_t bag = 0x7F;
    if (bag == 0) bag = 0x7F;
    uint8_t piece = __builtin_ctz(bag);
    bag &= bag - 1;
    return piece;
}

void updateScore(uint8_t lines) {
    static const uint8_t scoreTable[] PROGMEM = {0, 4, 10, 30, 120};
    game.score += pgm_read_byte(&scoreTable[lines]) * (game.level + 1);
    game.linesCleared += lines;
    if (game.linesCleared > (game.level + 1) * 10) {
        game.level++;
        getAwait();
    }
    if (game.score > game.highScore) {
        game.highScore = game.score;
        uint8_t lowByte = (uint8_t)(game.highScore / 10);
        uint8_t highByte = (uint8_t)((game.highScore / 10) >> 8);
        eepromReadWrite(EEPROM_ADDR, &lowByte, true);
        eepromReadWrite(EEPROM_ADDR + 1, &highByte, true);
    }
}

void drawGame() {
    static uint8_t lastPixels[SCREEN_WIDTH * SCREEN_HEIGHT / 8];
    static uint16_t lastPiece;
    static int8_t lastX, lastY;

    oled.switchRenderFrame();

    for (uint8_t i = 0; i < sizeof(pixels); i++) {
        if (pixels[i] != lastPixels[i]) {
            updatePixelRow(i);
            lastPixels[i] = pixels[i];
        }
    }

    if (lastPiece) {
        drawPiece(lastPiece, lastX, lastY, true);
    }

    drawPiece(game.piece, game.x, game.y, false);

    lastPiece = game.piece;
    lastX = game.x;
    lastY = game.y;

    writeScore();
    oled.switchDisplayFrame();

    if (game.madeLine) {
        game.madeLine = false;
        oled.setInverse(false);
    }
} 
 
void updatePixelRow(uint8_t row) {
    oled.setCursor(0, row);
    oled.startData();
    for (uint8_t x = 0; x < SCREEN_WIDTH; x++) {
        oled.sendData((pixels[row] & (1 << x)) ? 0xFF : 0x00);
    }
    oled.endData();
}

void drawPiece(uint16_t piece, int8_t x, int8_t y, bool erase) {
    for (uint8_t i = 0; i < 4; i++) {
        for (uint8_t j = 0; j < 4; j++) {
            if (piece & (1 << (i * 4 + j))) {
                uint8_t px = x + j;
                uint8_t py = y + i;
                oled.setCursor(px * 3, py / 8);
                oled.startData();
                oled.sendData(erase ? 0x00 : (0b111 << (py % 8)));
                oled.endData();
            }
        }
    }
}

bool checkReset() {
    static uint32_t resetStartTime = 0;
    uint8_t buttonState = PINB;
    
    if (!(buttonState & (1 << PIN_LEFT)) && !(buttonState & (1 << PIN_RIGHT))) {
        if (resetStartTime == 0) {
            resetStartTime = ms;
        } else if (ms - resetStartTime > RESET_DURATION) {
            resetStartTime = 0;
            return true;
        }
    } else {
        resetStartTime = 0;
    }
    return false;
}

void getAwait() {
    await = pgm_read_word(&awaitTimes[game.level < 14 ? game.level : 13]);
}

bool eepromReadWrite(uint8_t address, uint8_t* data, bool isWrite) {
    if (isWrite) {
        eepromReadWrite(address, *data);
    } else {
        *data = EEPROM.read(address);
    }
    return true;
} 


void loop() {
    if (checkReset()) {
        initGame();
        return;
    }
    
    if (ms - elapsed >= await) {
        updateGameState();
        elapsed = ms;
    }
    reseedRNG();
    drawGame();
    wdt_reset(); 
}