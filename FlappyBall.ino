/*
  FlappyBall (aka FloatyBall) for the Arduboy
  Written by Chris Martinez, 3/5/2014
  Modified by Scott Allen, April 2016
*/
#include <SPI.h>
#include <EEPROM.h>
#include <Arduboy.h>
#include "bitmaps.h"

Arduboy arduboy;

// Things that make the game work the way it does
#define FRAMES_PER_SECOND 30   // The update and refresh speed
#define FRAC_BITS 6            // The number of bits in the fraction part of a fixed point int

// The following values define how fast the ball will accelerate and how high it will jump.
// They are given as fixed point integers so the true value is multiplied by (1 << FRAC_BITS)
// to give the value used. The resulting values must be integers.
#define BALL_ACCELERATION 16      // (0.25) the ball acceleration in pixels per frame squared
#define BALL_JUMP_VELOCITY (-144) // (-2.25) The inital velocity of a ball jump in pixels per frame
// ---------------------------

// This value is an offset to make it easier to work with negative numbers.
// It must be greater than the maximum number of pixels that Floaty will jump above
// the start height (based on the acceleration and initial velocity values),
// but must be low enough not to cause an overflow when added to the maximum
// screen height as an integer.
#define NEG_OFFSET 64

// Pipe
#define PIPE_ARRAY_SIZE 4  // At current settings only 3 sets of pipes can be onscreen at once
#define PIPE_MOVE_DISTANCE 2   // How far each pipe moves per frame
#define PIPE_GAP_MAX 30        // Maximum pipe gap
#define PIPE_GAP_MIN 18        // Minimum pipe gap
#define PIPE_GAP_REDUCE 5      // Number of points scored to reduce gap size
#define PIPE_WIDTH 12
#define PIPE_CAP_WIDTH 2
#define PIPE_CAP_HEIGHT 3      // Caps push back into the pipe, it's not added length
#define PIPE_MIN_HEIGHT 6      // Higher values center the gaps more
#define PIPE_GEN_FRAMES 32     // How many frames until a new pipe is generated

// Ball
#define BALL_RADIUS 4
#define BALL_Y_START ((HEIGHT / 2) - 1) // The height Floaty begins at
#define BALL_X 32              // Floaty's X Axis

#define ACTIVE 0
#define LEFT_EDGE 1
#define GAP_LOCATION 2
#define GAP_WIDTH 3

// Storage Vars
byte gameState = 0;
unsigned int gameScore = 0;
unsigned int gameHighScore = 0;
char pipes[4 ][PIPE_ARRAY_SIZE]; // Row 0 for x values, row 1 for gap location, row 3 for active
byte pipeGap = PIPE_GAP_MAX;    // Height of gap between pipes to fly through
byte pipeReduceCount = PIPE_GAP_REDUCE; // Score tracker for pipe gap reduction
char ballY = BALL_Y_START;      // Floaty's height
char ballYprev = BALL_Y_START;  // Previous height
char ballYi = BALL_Y_START;     // Floaty's initial height for the current arc
int ballV = 0;                  // For height calculations (Vi + ((a * t) / 2))
byte ballFrame = 0;             // Frame count for the current arc
char ballFlapper = BALL_RADIUS; // Floaty's wing length
char gameScoreX = 0;
char gameScoreY = 0;
byte gameScoreRiser = 0;

// Sounds
const byte PROGMEM bing [] = {
0x90,0x30, 0,107, 0x80, 0x90,0x60, 1,244, 0x80, 0xf0};
const byte PROGMEM intro [] = {
0x90,72, 1,244, 0x80, 0x90,60, 1,244, 0x80, 0x90,64, 1,244, 0x80, 0x90,69, 1,244, 0x80, 0x90,67, 
1,244, 0x80, 0x90,60, 1,244, 0x80, 0x90,72, 1,244, 0x80, 0x90,60, 1,244, 0x80, 0x90,64, 1,244, 
0x80, 0x90,69, 1,244, 0x80, 0x90,67, 1,244, 0x80, 0x90,60, 1,244, 0x80, 0x90,69, 1,244, 0x80, 
0x90,57, 1,244, 0x80, 0x90,60, 1,244, 0x80, 0x90,65, 1,244, 0x80, 0x90,62, 1,244, 0x80, 0x90,57, 
1,244, 0x80, 0x90,69, 1,244, 0x80, 0x90,57, 1,244, 0x80, 0x90,60, 1,244, 0x80, 0x90,67, 1,244, 
0x80, 0x90,62, 1,244, 0x80, 0x90,65, 1,244, 0x80, 0x90,72, 7,208, 0x80, 0xf0};
const byte PROGMEM point [] = {
0x90,83, 0,75, 0x80, 0x90,88, 0,225, 0x80, 0xf0};
const byte PROGMEM flap [] = {
0x90,24, 0,125, 0x80, 0xf0};
const byte PROGMEM horns [] = {
0x90,60, 1,44, 0x80, 0x90,50, 0,150, 0x80, 0,150, 0x90,48, 0,150, 0x80, 0x90,55, 2,238, 
0x80, 0x90,62, 3,132, 0x80, 0x90,60, 0,37, 0x80, 0x90,59, 0,37, 0x80, 0x90,57, 0,37, 0x80, 
0x90,55, 0,37, 0x80, 0x90,53, 0,18, 0x80, 0x90,52, 0,18, 0x80, 0x90,50, 0,18, 0x80, 0x90,48, 
0,18, 0x80, 0x90,47, 0,18, 0x80, 0x90,45, 0,18, 0x80, 0x90,43, 0,18, 0x80, 0x90,41, 0,18, 
0x80, 0xf0};
const byte PROGMEM hit [] = { 
0x90,60, 0,31, 0x80, 0x90,61, 0,31, 0x80, 0x90,62, 0,31, 0x80, 0xf0};

void setup() {
  arduboy.begin();
  arduboy.setFrameRate(FRAMES_PER_SECOND);
  playSound(bing);
  delay(1500);
  arduboy.clear();
  arduboy.drawSlowXYBitmap(0,0,floatyball,128,64,1);
  arduboy.display();
  playSound(intro);
  delay(500);
  arduboy.setCursor(18,55);
  arduboy.print("Press Any Button");
  arduboy.display();

  while (!arduboy.buttonsState()) { } // Wait for any key press
  debounceButtons();

  arduboy.initRandomSeed();
  for (byte x = 0; x < PIPE_ARRAY_SIZE; x++) { pipes[ACTIVE][x] = 0; }  // Set all pipes offscreen
}

void loop() {
  if (!arduboy.nextFrame())
    return;

  if (arduboy.pressed(LEFT_BUTTON)) { // If the button for sound toggle is pressed
    if (arduboy.audio.enabled()) {    // If sound is enabled
      stopSound();                    // Stop anything that's playing
      arduboy.audio.off();            // Mute sounds
    } else {
      arduboy.audio.on();             // Enable sound
      playSound(bing);                // Play a sound to indicate sound has been turned on
    }
    debounceButtons();                // Wait for button release
  }

  arduboy.clear();

  // ===== State: Wait to begin =====
  if (gameState == 0) {     // If waiting to begin
    drawFloor();
    drawFloaty();
    drawInfo();             // Display usage info

    if (jumpPressed()) {    // Wait for a jump button press
      gameState = 1;        // Then start the game
      stopSound();          // Stop any playing sound
      beginJump();          // And make Floaty jump
    }
  }

  // ===== State: Playing =====
  if (gameState == 1) {     // If the game is playing
    // If the ball isn't already rising, check for jump
    if ((ballYprev <= ballY) && jumpPressed()) {
      beginJump();          // Jump
    }

    moveFloaty();

    if (ballY < BALL_RADIUS) {  // If Floaty has moved above the top of the screen
      ballY = BALL_RADIUS;      // Set position to top
      startFalling();           // Start falling
    }

    if (arduboy.everyXFrames(PIPE_GEN_FRAMES)) { // Every PIPE_GEN_FRAMES worth of frames
      generatePipe();                  // Generate a pipe
    }

    for (byte x = 0; x < PIPE_ARRAY_SIZE; x++) {  // For each pipe array element
      if (pipes[ACTIVE][x] != 0) {           // If the pipe is active
        pipes[LEFT_EDGE][x] = pipes[LEFT_EDGE][x] - PIPE_MOVE_DISTANCE;  // Then move it left
        if (pipes[LEFT_EDGE][x] + PIPE_WIDTH < 0) {  // If the pipe's right edge is off screen
          pipes[ACTIVE][x] = 0;              // Then set it inactive
        }
        if (pipes[LEFT_EDGE][x] + PIPE_WIDTH == (BALL_X - BALL_RADIUS)) {  // If the pipe passed Floaty
          gameScore++;                  // Increment the score
          pipeReduceCount--;            // Decrement the gap reduction counter
          gameScoreX = BALL_X;                  // Load up the floating text with
          gameScoreY = ballY - BALL_RADIUS - 8; //  current ball x/y values
          gameScoreRiser = 15;          // And set it for 15 frames
          playSound(point);
        }
      }
    }

    if (gameScoreRiser > 0) {  // If we have floating text
      gameScoreY--;
      if (gameScoreY >= 0) { // If the score will still be on the screen
        arduboy.setCursor(gameScoreX, gameScoreY);
        arduboy.print(gameScore);
        gameScoreX = gameScoreX - 2;
        gameScoreRiser--;
      } else {
        gameScoreRiser = 0;
      }
    }

    if (ballY + BALL_RADIUS > (HEIGHT-1)) {  // If the ball has fallen below the screen
      ballY = (HEIGHT-1) - BALL_RADIUS;      // Don't let the ball go under :O
      gameState = 2;                        // Game over. State is 2.
    }
    // Collision checking
    for (byte x = 0; x < PIPE_ARRAY_SIZE; x++) { // For each pipe array element
      if (pipes[ACTIVE][x] != 0) {                 // If the pipe is active (not 0)
        if (checkPipe(x)) { gameState = 2; }  // If the check is true, game over
      }
    }

    drawPipes();
    drawFloor();
    drawFloaty();

    // Reduce pipe gaps as the game progresses
    if ((pipeGap > PIPE_GAP_MIN) && (pipeReduceCount <= 0)) {
      pipeGap--;
      pipeReduceCount = PIPE_GAP_REDUCE;  // Restart the countdown
    }
  }

  // ===== State: Game Over =====
  if (gameState == 2) {  // If the gameState is 2 then we draw a Game Over screen w/ score
    if (gameScore > gameHighScore) { gameHighScore = gameScore; }
    arduboy.display();              // Make sure final frame is drawn
    playSound(hit);                 // Hit sound
    delay(100);                     // Pause for the sound
    startFalling();                 // Start falling from current position
    while (ballY + BALL_RADIUS < (HEIGHT-1)) {  // While floaty is still airborne
      moveFloaty();
      arduboy.clear();
      drawPipes();
      drawFloor();
      drawFloaty();
      arduboy.display();
      while (!arduboy.nextFrame()) { }  // Wait for next frame
    }
    playSound(horns);                    // Sound the loser's horn
    arduboy.drawRect(16,8,96,48, WHITE); // Box border
    arduboy.fillRect(17,9,94,46, BLACK); // Black out the inside
    arduboy.drawSlowXYBitmap(30,12,gameover,72,14,1);
    arduboy.setCursor(56 - getOffset(gameScore),30);
    arduboy.print(gameScore);
    arduboy.setCursor(69,30);
    arduboy.print("Score");

    arduboy.setCursor(56 - getOffset(gameHighScore),42);
    arduboy.print(gameHighScore);
    arduboy.setCursor(69,42);
    arduboy.print("High");

    arduboy.display();
    delay(1000);         // Give some time to stop pressing buttons

    while (!jumpPressed()) { } // Wait for a jump button to be pressed
    debounceButtons();

    gameState = 0;       // Then start the game paused
    playSound(intro);    // Play the intro
    gameScore = 0;       // Reset score to 0
    gameScoreRiser = 0;  // Clear the floating score
    for (byte x = 0; x < PIPE_ARRAY_SIZE; x++) { pipes[ACTIVE][x] = 0; }  // set all pipes inactive
    ballY = BALL_Y_START;   // Reset to initial ball height
    pipeGap = PIPE_GAP_MAX; // Reset the pipe gap height
    pipeReduceCount = PIPE_GAP_REDUCE; // Init the pipe gap reduction counter
  }

  arduboy.display();  // Finally draw this thang
}

void drawInfo() {
  byte ulStart;     // Start of underline to indicate sound status
  byte ulLength;    // Length of underline

  arduboy.setCursor(6, 3);
  arduboy.print("A,B,Up,Down: Jump");
  arduboy.setCursor(6, 51);
  arduboy.print("Left: Sound On/Off");

  if (arduboy.audio.enabled()) {
    ulStart = 13 * 6;
    ulLength = 2 * 6 - 1;
  } else {
    ulStart = 16 * 6;
    ulLength = 3 * 6 - 1;
  }
  arduboy.drawFastHLine(ulStart, 51 + 8, ulLength, WHITE); // Underline the current sound mode
  arduboy.drawFastHLine(ulStart, 51 + 9, ulLength, WHITE);
}

void drawFloor() {
  arduboy.drawFastHLine(0, HEIGHT-1, WIDTH, WHITE);
}

void drawFloaty() {
  ballFlapper--;
  if (ballFlapper < 0) { ballFlapper = BALL_RADIUS; }  // Flapper starts at the top of the ball
  arduboy.drawCircle(BALL_X, ballY, BALL_RADIUS, BLACK);  // Black out behind the ball
  arduboy.drawCircle(BALL_X, ballY, BALL_RADIUS, WHITE);  // Draw outline
  arduboy.drawLine(BALL_X, ballY, BALL_X - (BALL_RADIUS+1), ballY - ballFlapper, WHITE);  // Draw wing
  arduboy.drawPixel(BALL_X - (BALL_RADIUS+1), ballY - ballFlapper + 1, WHITE);  // Dot the wing
  arduboy.drawPixel(BALL_X + 1, ballY - 2, WHITE);  // Eye
}

void drawPipes() {
  for (byte x = 0; x < PIPE_ARRAY_SIZE; x++){
    if (pipes[ACTIVE][x] != 0) {    // Value set to 0 if array element is inactive,
                               // otherwise it is the xvalue of the pipe's left edge
      // Pipes
      arduboy.drawRect(pipes[LEFT_EDGE][x], -1, PIPE_WIDTH, pipes[GAP_LOCATION][x], WHITE);
      arduboy.drawRect(pipes[LEFT_EDGE][x], pipes[GAP_LOCATION][x] + pipes[GAP_WIDTH][x], PIPE_WIDTH, HEIGHT - pipes[GAP_LOCATION][x] - pipes[GAP_WIDTH][x], WHITE);
      // Caps
      arduboy.drawRect(pipes[LEFT_EDGE][x] - PIPE_CAP_WIDTH, pipes[GAP_LOCATION][x] - PIPE_CAP_HEIGHT, PIPE_WIDTH + (PIPE_CAP_WIDTH*2), PIPE_CAP_HEIGHT, WHITE);
      arduboy.drawRect(pipes[LEFT_EDGE][x] - PIPE_CAP_WIDTH, pipes[GAP_LOCATION][x] + pipes[GAP_WIDTH][x], PIPE_WIDTH + (PIPE_CAP_WIDTH*2), PIPE_CAP_HEIGHT, WHITE);
      // Detail lines
      arduboy.drawLine(pipes[LEFT_EDGE][x]+2, 0, pipes[LEFT_EDGE][x]+2, pipes[GAP_LOCATION][x]-5, WHITE);
      arduboy.drawLine(pipes[LEFT_EDGE][x]+2, pipes[GAP_LOCATION][x] + pipes[GAP_WIDTH][x] + 5, pipes[LEFT_EDGE][x]+2, HEIGHT - 3,WHITE);
    }
  }
}

void generatePipe() {
  for (byte x = 0; x < PIPE_ARRAY_SIZE; x++) {
    if (pipes[ACTIVE][x] == 0) { // If the element is inactive
      pipes[LEFT_EDGE][x] = WIDTH;  // Then create it starting right of the screen
      pipes[GAP_LOCATION][x] = random(PIPE_MIN_HEIGHT, HEIGHT - PIPE_MIN_HEIGHT - pipeGap);
      pipes[ACTIVE][x] = 1;
      pipes[GAP_WIDTH][x] = pipeGap;
      return;
    }
  }
}

boolean checkPipe(byte x) {  // Collision detection, x is pipe to check
  byte AxA = BALL_X - (BALL_RADIUS-1);  // Hit box for floaty is a square
  byte AxB = BALL_X + (BALL_RADIUS-1);  // If the ball radius increases too much, corners
  byte AyA = ballY - (BALL_RADIUS-1);  // of the hitbox will go outside of floaty's
  byte AyB = ballY + (BALL_RADIUS-1);  // drawing
  byte BxA, BxB, ByA, ByB;
  
  // check top cylinder
  BxA = pipes[LEFT_EDGE][x];
  BxB = pipes[LEFT_EDGE][x] + PIPE_WIDTH;
  ByA = 0;
  ByB = pipes[GAP_LOCATION][x];
  if (AxA < BxB && AxB > BxA && AyA < ByB && AyB > ByA) { return true; } // Collided with top pipe
  
  // check top cap
  BxA = pipes[LEFT_EDGE][x] - PIPE_CAP_WIDTH;
  BxB = BxA + PIPE_WIDTH + (PIPE_CAP_WIDTH*2);
  ByA = pipes[GAP_LOCATION][x] - PIPE_CAP_HEIGHT;
  if (AxA < BxB && AxB > BxA && AyA < ByB && AyB > ByA) { return true; } // Collided with top cap
  
  // check bottom cylinder
  BxA = pipes[LEFT_EDGE][x];
  BxB = pipes[LEFT_EDGE][x] + PIPE_WIDTH;
  ByA = pipes[GAP_LOCATION][x] + pipes[GAP_WIDTH][x];
  ByB = HEIGHT-1;
  if (AxA < BxB && AxB > BxA && AyA < ByB && AyB > ByA) { return true; } // Collided with bottom pipe
  
  // check bottom cap
  BxA = pipes[LEFT_EDGE][x] - PIPE_CAP_WIDTH;
  BxB = BxA + PIPE_WIDTH + (PIPE_CAP_WIDTH*2);
  ByB = ByA + PIPE_CAP_HEIGHT;
  if (AxA < BxB && AxB > BxA && AyA < ByB && AyB > ByA) { return true; } // Collided with bottom pipe

  return false; // Nothing hits
}

boolean jumpPressed() { // Return "true" if a jump button is pressed
  return (arduboy.buttonsState() & (UP_BUTTON | DOWN_BUTTON | A_BUTTON | B_BUTTON)) != 0;
}

void beginJump() {
  playSound1(flap);     // Play "flap" sound only if nothing is playing
  ballV = BALL_JUMP_VELOCITY;
  ballFrame = 0;
  ballYi = ballY;
}

void startFalling() {   // Start falling from current height
  ballFrame = 0;        // Start a new fall
  ballYi = ballY;       // Set initial arc position
  ballV = 0;            // Set velocity to 0
}

void moveFloaty() {
  ballYprev = ballY;                   // Save the previous height
  ballFrame++;                         // Next frame
  ballV += (BALL_ACCELERATION / 2);    // Increase the velocity
                                     // Calculate Floaty's new height:
  ballY = ((((ballV * ballFrame)       // Complete "distance from initial height" calculation
           + (NEG_OFFSET << FRAC_BITS) // Add an offset to make sure the value is positive
           + (1 << (FRAC_BITS - 1)))   // Add 0.5 to round up
            >> FRAC_BITS)              // shift down to remove the fraction part
             - NEG_OFFSET)             // Remove previously added offset
              + ballYi;                // Add the result to the start height to get the new height
}

void playSound(const byte *score) {
  stopSound();                        // Stop any currently playing sound
  if (arduboy.audio.enabled()) {      // If sound is on
    arduboy.tunes.playScore(score);   // play the new sound
  }
}

void playSound1(const byte *score) {
  if (!arduboy.tunes.playing() && arduboy.audio.enabled()) { // If not playing and sound is on
    arduboy.tunes.playScore(score);   // play the new sound
  }
}

void stopSound() {
  if (arduboy.tunes.playing()) {      // If something is playing
    arduboy.tunes.stopScore();        // stop playing
  }
}

void debounceButtons() { // prevent "noisy" buttons from appearing as multiple presses
  delay(100);
  while (arduboy.buttonsState()) { }  // Wait for all keys released
  delay(100);
}

byte getOffset(byte s) {
  if (s > 9999) { return 20; }
  if (s > 999) { return 15; }
  if (s > 99) { return 10; }
  if (s > 9) { return 5; }
  return 0;
}
