/*
=========================================================
            1D FLAPPY BIRD LED GAME v3.0
=========================================================

ПЛАТФОРМА:
- Arduino Nano / Uno

ПІДКЛЮЧЕННЯ:
------------------------------------------------
WS2812B DIN  -> D6
FLY BUTTON   -> D2
START BUTTON -> D3
BUZZER       -> D9

TM1637 CLK   -> D4
TM1637 DIO   -> D5

БІБЛІОТЕКИ:
------------------------------------------------
1. FastLED
2. TM1637Display

=========================================================
*/

#include <FastLED.h>
#include <TM1637Display.h>

// =====================================================
// LED STRIP
// =====================================================

#define LED_PIN        6
#define NUM_LEDS       120
#define BRIGHTNESS     200

CRGB leds[NUM_LEDS];

// =====================================================
// BUTTONS
// =====================================================

#define FLY_BUTTON     2
#define START_BUTTON   3

// =====================================================
// BUZZER
// =====================================================

#define BUZZER_PIN     9

// =====================================================
// TM1637 DISPLAY
// =====================================================

#define CLK            4
#define DIO            5

TM1637Display display(CLK, DIO);

// =====================================================
// GAME STATES
// =====================================================

bool waitingStart = true;
bool gameOver = false;

// =====================================================
// BIRD SETTINGS
// =====================================================

// Позиція пташки
float birdPos = 60;

// Швидкість пташки
float velocity = 0;

// Гравітація
// Менше = плавніше падіння
float gravity = 0.06;

// Сила стрибка
// Менше по модулю = плавніше керування
float flapForce = -0.48;

// =====================================================
// PIPE SETTINGS
// =====================================================

// Центр проходу
float gapCenter = 60;

// Ціль руху проходу
float targetGapCenter = 60;

// Розмір проходу
// Більше = легше
int gapSize = 40;

// Колір труб
CRGB pipeColor = CRGB::Green;

// =====================================================
// TIMERS
// =====================================================

// Таймер руху труб
unsigned long lastPipeMove = 0;

// Швидкість труб
// МЕНШЕ = ШВИДШЕ
unsigned long pipeInterval = 60;

// Таймер очок
unsigned long lastScore = 0;

// Таймер складності
unsigned long lastDifficulty = 0;

// =====================================================
// SCORE
// =====================================================

int score = 0;

// =====================================================
// SETUP
// =====================================================

void setup()
{
    // Кнопки

    pinMode(FLY_BUTTON, INPUT_PULLUP);
    pinMode(START_BUTTON, INPUT_PULLUP);

    // Buzzer

    pinMode(BUZZER_PIN, OUTPUT);

    // LED STRIP

    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);

    FastLED.setBrightness(BRIGHTNESS);

    // DISPLAY

    display.setBrightness(7);

    // RANDOM

    randomSeed(analogRead(A0));

    // IDLE ANIMATION

    idleAnimation();
}

// =====================================================
// MAIN LOOP
// =====================================================

void loop()
{
    // =================================================
    // WAITING FOR START
    // =================================================

    if(waitingStart)
    {
        if(digitalRead(START_BUTTON) == LOW)
        {
            delay(250);

            startGame();
        }

        return;
    }

    // =================================================
    // GAME OVER
    // =================================================

    if(gameOver)
    {
        gameOverAnimation();

        waitingStart = true;

        idleAnimation();

        return;
    }

    // =================================================
    // FLY BUTTON
    // =================================================

    if(digitalRead(FLY_BUTTON) == LOW)
    {
        velocity = flapForce;

        tone(BUZZER_PIN, 1200, 20);
    }

    // =================================================
    // PHYSICS
    // =================================================

    velocity += gravity;

    birdPos += velocity;

    // =================================================
    // LIMITS
    // =================================================

    if(birdPos < 0)
    {
        birdPos = 0;
        velocity = 0;
    }

    if(birdPos > NUM_LEDS - 1)
    {
        birdPos = NUM_LEDS - 1;
        velocity = 0;
    }

    int birdLED = round(birdPos);

    // =================================================
    // PIPE MOVEMENT
    // =================================================

    if(millis() - lastPipeMove > pipeInterval)
    {
        lastPipeMove = millis();

        // Плавний випадковий рух труб

        targetGapCenter += random(-4, 5);

        // Межі проходу

        if(targetGapCenter < gapSize / 2)
            targetGapCenter = gapSize / 2;

        if(targetGapCenter > NUM_LEDS - gapSize / 2)
            targetGapCenter = NUM_LEDS - gapSize / 2;

        // Новий колір труб

        pipeColor = CHSV(random(0,255), 255, 255);
    }

    // =================================================
    // CINEMATIC MOVEMENT
    // =================================================

    // Дуже плавний рух труб

    gapCenter += (targetGapCenter - gapCenter) * 0.05;

    // =================================================
    // SCORE
    // =================================================

    // +1 очко кожну секунду

    if(millis() - lastScore >= 1000)
    {
        lastScore = millis();

        score++;

        display.showNumberDec(score);

        tone(BUZZER_PIN, 1800, 20);
    }

    // =================================================
    // DIFFICULTY SYSTEM
    // =================================================

    // Кожні 10 секунд гра стає трохи складнішою

    if(millis() - lastDifficulty >= 10000)
    {
        lastDifficulty = millis();

        // Прохід стає меншим

        if(gapSize > 24)
            gapSize--;

        // Гравітація трохи росте

        if(gravity < 0.09)
            gravity += 0.003;

        // Максимум +50% швидкості труб

        if(pipeInterval > 40)
            pipeInterval -= 1;

        // Трохи сильніший стрибок

        if(flapForce > -0.65)
            flapForce -= 0.01;

        // Звук нового рівня

        tone(BUZZER_PIN, 2200, 100);
    }

    // =================================================
    // COLLISION
    // =================================================

    int gapTop = gapCenter - gapSize / 2;

    int gapBottom = gapCenter + gapSize / 2;

    // Якщо врізались у трубу

    if(birdLED < gapTop || birdLED > gapBottom)
    {
        gameOver = true;
    }

    // =================================================
    // DRAW GAME
    // =================================================

    drawGame();

    // FPS

    delay(16);
}

// =====================================================
// DRAW GAME
// =====================================================

void drawGame()
{
    // Очистка

    FastLED.clear();

    int gapTop = gapCenter - gapSize / 2;

    int gapBottom = gapCenter + gapSize / 2;

    // =================================================
    // DRAW PIPES
    // =================================================

    for(int i = 0; i < NUM_LEDS; i++)
    {
        if(i < gapTop || i > gapBottom)
        {
            leds[i] = pipeColor;
        }
    }

    // =================================================
    // DRAW BIRD
    // =================================================

    int birdLED = round(birdPos);

    // Центральний піксель

    leds[birdLED] = CRGB::Blue;

    // Glow ефект

    if(birdLED > 0)
        leds[birdLED - 1] = CRGB(0, 0, 70);

    if(birdLED < NUM_LEDS - 1)
        leds[birdLED + 1] = CRGB(0, 0, 70);

    // Показати стрічку

    FastLED.show();
}

// =====================================================
// START GAME
// =====================================================

void startGame()
{
    waitingStart = false;

    gameOver = false;

    // =================================================
    // RESET VALUES
    // =================================================

    birdPos = 60;

    velocity = 0;

    gapCenter = 60;

    targetGapCenter = 60;

    gapSize = 40;

    gravity = 0.06;

    flapForce = -0.48;

    pipeInterval = 60;

    score = 0;

    display.showNumberDec(score);

    // =================================================
    // TRAFFIC LIGHT START
    // =================================================

    // RED

    fill_solid(leds, NUM_LEDS, CRGB::Red);

    FastLED.show();

    tone(BUZZER_PIN, 400, 300);

    delay(700);

    // YELLOW

    fill_solid(leds, NUM_LEDS, CRGB::Yellow);

    FastLED.show();

    tone(BUZZER_PIN, 700, 300);

    delay(700);

    // GREEN

    fill_solid(leds, NUM_LEDS, CRGB::Green);

    FastLED.show();

    tone(BUZZER_PIN, 1200, 500);

    delay(700);

    // =================================================
    // START MELODY
    // =================================================

    int melody[] = {523, 659, 784, 1047};

    for(int i = 0; i < 4; i++)
    {
        tone(BUZZER_PIN, melody[i], 100);

        delay(130);
    }

    // Очистка

    FastLED.clear();

    FastLED.show();
}

// =====================================================
// IDLE ANIMATION
// =====================================================

void idleAnimation()
{
    for(int i = 0; i < NUM_LEDS; i++)
    {
        leds[i] = CHSV(i * 2, 255, 255);
    }

    FastLED.show();
}

// =====================================================
// GAME OVER ANIMATION
// =====================================================

void gameOverAnimation()
{
    for(int k = 0; k < 5; k++)
    {
        // Червона стрічка

        fill_solid(leds, NUM_LEDS, CRGB::Red);

        FastLED.show();

        // Звук програшу

        tone(BUZZER_PIN, 250, 180);

        delay(180);

        // Вимкнення

        FastLED.clear();

        FastLED.show();

        delay(120);
    }
}