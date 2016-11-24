#include <FillPat.h>
#include <LaunchPad.h>
#include <OrbitBoosterPackDefs.h>
#include <OrbitOled.h>
#include <OrbitOledChar.h>
#include <OrbitOledGrph.h>
#include <string.h>
#include <delay.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <Wire.h>
#include "Wire_Util.h"

/*
TODO:
fix global variables vs local variables created in each loop() call... for example: lastUpdatedInput.
coords are being copied around instead of being kept as pointers
need to investigate the WriteByte/WriteArray functions in the acceleromter updateDirection() function
fix threshold between up/left, up/right, ...
change food spawn algorithm to detect when it's impossible to spawn food (game is over) and minimize random attempts (so basically make it not entirely random
*/

void onButtonDown(void);
void initializeOLED(void);

struct Input *updateInputs(void); 

void appendToHead(struct point*);
void appendToHead(int,int);
void initializeSnake(struct point[], int);

void drawPixel(int,int);
void erasePixel(int,int);
void drawPixel(struct point*);
void erasePixel(struct point*);

boolean validateFoodLocation(void);
void eraseFood(void);

int genRandNum(int n);

void updateDirection(void); 
void zeroAccelerometer(void);
void readAccelerometer(float*,float*,float,float);

void runGame(void);
void initGame(void); 
void runMainMenu(void);
void initMainMenu(void); 

struct Input {
  const int code;
  boolean active; // for switches, this corresponds to the "up" position. For buttons, this corresponds to the "pressed" position
};

const int NUM_INPUTS = 3;
static Input INPUTS[NUM_INPUTS] = {
  {PF_0, false}, // SW_1
  {PD_2, false}, // BTN_1
  {PE_0, false}, // BTN_2
};
int FPS = 10;
struct Input *lastUpdatedInput;

char upArrow[] = {
  0x20, 0x40, 0xFF, 0x40, 0x20  
};
char downArrow[] = {
  0x04, 0x02, 0xFF, 0x02, 0x04  
};

struct point {
  int x;
  int y;
};

struct food {
  struct point coords;
  int width;
  int height;
} food;

/*
 * Snake components
 */
struct Segment {
  struct point coords;
  struct Segment *next; // towards head
  struct Segment *prev; // towards tail
};
const int NUM_POINTS = 10;
struct point points[NUM_POINTS] = {
    {16, 15}, 
    {17, 15}, 
    {18, 15}, 
    {19, 15}, 
    {20, 15}, 
    {21, 15}, 
    {22, 15}, 
    {23, 15}, 
    {24, 15}, 
    {25, 15}, 
};

struct Segment *head = NULL;
struct Segment *tail = NULL;
struct Segment *current = NULL;

boolean snakeIsGrowing = false;
boolean snakeIsDead = false;

const int X_BOUND_LEFT=0;
const int X_BOUND_RIGHT=127;
const int Y_BOUND_TOP=0;
const int Y_BOUND_BOTTOM=31;

char world[Y_BOUND_BOTTOM+1][X_BOUND_RIGHT+1] = {0};

enum direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
} direction=RIGHT;

float pitchOffset = 0, rollOffset = 0;
float LSBperG = 128.0;

enum Screen {
  GAME, MAIN_MENU, SETTINGS, DEATH, CALIBRATE, DEATH_TRANSITION 
} screen = MAIN_MENU;

char *scrollingMenuOptions[5] = {'\0'};
int numScrollingMenuOptions = 0;
char *scrollingMenuTitle = "";
char *deathOptions[] = {"try again", "give up"};
char *settingsOptions[] = {"back to menu", "calibrate"};
char *mainMenuOptions[] = {"play game", "settings"};

void setup() {
  WireInit();
  delay(200);
  
  WireWriteRegister(0x1D, 0x2D, 1 << 3); // set standby in the POWER_CTL register
  WireWriteRegister(0x1D, 0x31, 1); // set 10-bit res with 4g range

  for (int i = 0; i < NUM_INPUTS; i ++) {
    pinMode(INPUTS[i].code, INPUT);
  }
  pinMode(GREEN_LED, OUTPUT);

  initializeOLED();
  
  OrbitOledUpdate();

  initScreen();

  Serial.begin(9600);
}

void initScreen() {
  switch(screen) {
    case GAME:
      initGame();
      break;
    case MAIN_MENU:
      initMainMenu();
      break;
    case SETTINGS:
      initSettings();
      break;
    case DEATH:
      initDeath();
      break;
    case CALIBRATE:
      initCalibrate();
      break;
  }
}
void endScreen() {
  switch(screen) {
    case GAME:
      endGame();
      break;
  }
  OrbitOledClear();
}

void switchScreen(enum Screen newScreen) {
  endScreen();
  screen = newScreen;
  initScreen();
}

void loop() {
  switch(screen) {
    case GAME:
      runGame();
      break;
    case MAIN_MENU:
      runMainMenu();
      break;
    case SETTINGS:
      runSettings();
      break;
    case DEATH:
      runDeath();
      break;
    case DEATH_TRANSITION:
      runDeathTransition();
      break;
    case CALIBRATE:
      runCalibrate();
      break;
  }
  DelayMs(1000/FPS);
}

void runDeathTransition() {
  OrbitOledSetDrawMode(modOledSet);
  OrbitOledMoveTo(10, 10);
  OrbitOledDrawString("You died!");
  OrbitOledUpdate();
  delay(1500);
  OrbitOledClear();
  OrbitOledMoveTo(20, 20);
  OrbitOledDrawString("... pathetic");
  OrbitOledUpdate();
  delay(1500);
  switchScreen(DEATH);
}

void initDeath(void) {
  initScrollingMenu("The void", deathOptions, sizeof(deathOptions)/sizeof(deathOptions[0]));
}
void runDeath(void) {
  int optionIndex = runScrollingMenu();
  switch (optionIndex) {
    case 0:
      switchScreen(GAME);
      return;
    case 1:
      switchScreen(MAIN_MENU);
      return;
  }
}

void initCalibrate() {
  OrbitOledSetDrawMode(modOledSet);
  OrbitOledMoveTo(0, 0);
  OrbitOledDrawString("Orient the board");
  OrbitOledMoveTo(5, 10);
  OrbitOledDrawString("& Press BTN1");
  OrbitOledUpdate();
}
void runCalibrate() {
  lastUpdatedInput = updateInputs();
  switch(lastUpdatedInput->code) {
    case PD_2:
      if (lastUpdatedInput->active) {
        OrbitOledClear();
        OrbitOledMoveTo(0,0);
        OrbitOledDrawString("Calibrating...");
        OrbitOledUpdate();
        digitalWrite(GREEN_LED, HIGH);
        zeroAccelerometer();
        delay(1000);
        digitalWrite(GREEN_LED, LOW);
        switchScreen(SETTINGS);
        return;
      }
  }
}

void runSettings(void) {
  int optionIndex = runScrollingMenu();
  switch (optionIndex) {
    case 0:
      switchScreen(MAIN_MENU);
      return;
    case 1:
      switchScreen(CALIBRATE);
      return;
  }
}
void initSettings(void) {
  initScrollingMenu("settings", settingsOptions, sizeof(settingsOptions)/sizeof(settingsOptions[0]));
}

void runGame(void) {
  lastUpdatedInput = updateInputs();
  switch(lastUpdatedInput->code) {
    case PF_0:
      zeroAccelerometer();
      break;
    case PE_0:
      if (lastUpdatedInput->active)
        switchScreen(MAIN_MENU);
      return;
  }

  updateDirection();
  moveSnake();

  if (snakeIsDead) {
    switchScreen(DEATH_TRANSITION);
    return;
  }
  OrbitOledUpdate();
}

int offset = 12;
void initScrollingMenu(char *newTitle, char *newOptions[], int numOptions) {
  offset = 12;
  scrollingMenuTitle = newTitle;
  for (int i = 0; i < numOptions; i ++) {
    scrollingMenuOptions[i] = newOptions[i];
  }
  numScrollingMenuOptions = numOptions;
  drawScrollingMenu();
}

int runScrollingMenu() {
  lastUpdatedInput = updateInputs();
  switch(lastUpdatedInput->code) {
    case PD_2:
      if (lastUpdatedInput->active) {
        // words are 7 tall and have padding of 4, so 3 to 3, next is 7 to 13, then 17 to 23 // and midpoints become 0, 10, 20...
        if ((abs(offset-12)+3)%10 <= 6)
          return (abs(offset-12)+3)/10;
      }
    break;
  }

  float pitch, roll;
  readAccelerometer(&pitch, &roll, pitchOffset, rollOffset);
  if (fabs(roll) > 7) {
    if (roll > 0) {
      if (offset-12 < 3) {
        offset ++;
        drawScrollingMenu();
      }
    }
    else if (offset-12 > - (numScrollingMenuOptions-1)*10 - 3) {
      offset --;
      drawScrollingMenu();
    }
  }
  return -1;
}

void drawScrollingMenu() {
    OrbitOledClear();
    OrbitOledSetDrawMode(modOledSet);
    for (int i = 0; i < numScrollingMenuOptions; i ++) {
      OrbitOledMoveTo(10, offset+i*10);
      OrbitOledDrawString(scrollingMenuOptions[i]);
    }
    OrbitOledMoveTo(0, 0);
    OrbitOledFillRect(X_BOUND_RIGHT, 9);
    OrbitOledMoveTo(0, Y_BOUND_BOTTOM-9);
    OrbitOledFillRect(X_BOUND_RIGHT, Y_BOUND_BOTTOM);

    OrbitOledSetDrawMode(modOledXor);
    OrbitOledMoveTo(0,1);
    OrbitOledDrawString(scrollingMenuTitle);

    OrbitOledMoveTo(X_BOUND_RIGHT-6, 1);
    OrbitOledPutBmp(5, 8, upArrow);
    OrbitOledMoveTo(X_BOUND_RIGHT-6, Y_BOUND_BOTTOM-8);
    OrbitOledPutBmp(5, 8, downArrow);
    OrbitOledUpdate();
}

void runMainMenu(void) {
  int optionIndex = runScrollingMenu();
  switch (optionIndex) {
    case 0:
      switchScreen(GAME);
      return;
    case 1:
      switchScreen(SETTINGS);
      return;
  }
}
void initMainMenu(void) {
  initScrollingMenu("main menu", mainMenuOptions, sizeof(mainMenuOptions)/sizeof(mainMenuOptions[0]));
}

void zeroAccelerometer(void) {
  readAccelerometer(&pitchOffset, &rollOffset, 0,0);
}
/* 
* Reads accelerometer x, y, z data and compute pitch and roll.
*/ 
void readAccelerometer(float *pitch, float *roll, float pitchOffset, float rollOffset) {

  uint32_t data[6] = {0}; // instead of getting X0, X1, Y0, ... data separately, simply get all 6 bytes in one read. According to the specification, this removes the risk
                          // of the data in those registers changing between access calls

  for (int i = 0; i < 5; i ++) {
    WireWriteByte(0x1D, 0x32); //
    WireRequestArray(0x1D, data, 6);

    uint16_t xi = (data[1] << 8) | data[0];
    uint16_t yi = (data[3] << 8) | data[2];
    uint16_t zi = (data[5] << 8) | data[4];
    
    float x = *(int16_t*)(&xi); // LSBperG
    float y = *(int16_t*)(&yi); // LSBperG
    float z = *(int16_t*)(&zi); // LSBperG

    *pitch += (atan2(x,sqrt(y*y+z*z)) * 180.0) / PI;
    *roll += (atan2(y,sqrt(x*x+z*z)) * 180.0) / PI;
  }
  *pitch /= 5;
  *roll /= 5;

  *pitch -= pitchOffset;
  *roll -= rollOffset;
}

/*
* Read accelerometer data and return the snake's updated direction based on the pitch & roll of the accelerometer
*/
void updateDirection(void) {
  float pitch = 0, roll = 0;
  readAccelerometer(&pitch, &roll, pitchOffset, rollOffset);

  if (fabs(pitch) <= 7 && fabs(roll) <= 7 || fabs(pitch-roll) <= 15) {
    return;
  } else if (pitch > 0 && roll > 0) {
    if (pitch >= roll && direction != RIGHT) {
      direction = LEFT;
    } else if (direction != UP) {
      direction = DOWN;
    }
  } else if (pitch > 0 && roll < 0) {
    if (pitch >= fabs(roll) && direction != RIGHT) {
      direction = LEFT; 
    } else if (direction != DOWN) {
      direction = UP;
    }
  } else if (pitch < 0 && roll < 0) {
    if (pitch <= roll && direction != LEFT) {
      direction = RIGHT;
    } else if (direction != DOWN) {
      direction = UP;
    }
  } else {
    if (fabs(pitch) >= roll && direction != LEFT) {
      direction = RIGHT;
    } else if (direction != UP) {
      direction = DOWN;
    }
  }
}

void appendToHead(int x, int y) {
  struct point newPoint = {x, y};
  appendToHead(&newPoint);
}

void appendToHead(struct point *coords) {
  struct Segment* newHead = (struct Segment*)malloc(sizeof(struct Segment));
  newHead->coords = *coords;
  
  newHead->next = NULL;
  newHead->prev = head;
  
  if (head) {
    head->next = newHead;
    if (!tail) {
      tail = head;
    }
  }
  head = newHead;
}

/*
 * Create a linked list based off a list of coordinates
 */
void initializeSnake(struct point coords[], int numCoords) {
  for (int i = 0; i < numCoords; i ++) {
    appendToHead(&coords[i]);
    world[coords[i].y][coords[i].x] = 1;
    drawPixel(&(coords[i]));
  }
}

void moveSnake() {
  if (snakeIsGrowing) {
    switch(direction) {
      case LEFT:
        appendToHead(head->coords.x-1, head->coords.y);
        appendToHead(head->coords.x-1, head->coords.y);
        break;
      case RIGHT:
        appendToHead(head->coords.x+1, head->coords.y);
        appendToHead(head->coords.x+1, head->coords.y);
        break;
      case UP:
        appendToHead(head->coords.x, head->coords.y-1);
        appendToHead(head->coords.x, head->coords.y-1);
        break;
      case DOWN:
        appendToHead(head->coords.x, head->coords.y+1);
        appendToHead(head->coords.x, head->coords.y+1);
        break;
    }
    drawPixel(&(head->prev->coords));
    world[head->prev->coords.y][head->prev->coords.x] = 1;
    snakeIsGrowing = false;
  } 
  // To move the snake, the tail node has its coordinates moved ahead of the head (in some direction) and becomes the new head. The node that was previously ahead of the tail
  // becomes the new tail.
  else {
    erasePixel(&(tail->coords));
    world[tail->coords.y][tail->coords.x] = 0;
    switch(direction) {
      case LEFT:
        tail->coords.x = head->coords.x-1;
        tail->coords.y = head->coords.y;
        break;
      case RIGHT:
        tail->coords.x = head->coords.x+1;
        tail->coords.y = head->coords.y;
        break;
      case UP:
        tail->coords.y = head->coords.y-1;
        tail->coords.x = head->coords.x;
        break;
      case DOWN:
        tail->coords.y = head->coords.y+1;
        tail->coords.x = head->coords.x;
        break;
    }
    // swap the head node with the tail node
    current = tail;
    tail->prev = head;
    current = tail;
    tail = tail->next;
    tail->prev = NULL;
    head->next = current;
    head = current;
  }

  if (head->coords.x > X_BOUND_RIGHT) {
    head->coords.x = 0;
  }
  else if (head->coords.x < X_BOUND_LEFT) {
    head->coords.x = X_BOUND_RIGHT-1;
  }
  if (head->coords.y > Y_BOUND_BOTTOM) {
    head->coords.y = 0;
  }
  if (head->coords.y < Y_BOUND_TOP) {
    head->coords.y = Y_BOUND_BOTTOM-1;
  }

  if (world[head->coords.y][head->coords.x] == 1) {
    snakeIsDead = true;
  }
  else {
    if (world[head->coords.y][head->coords.x] == 2) {
      eraseFood();
      snakeIsGrowing = true;
    }
    
    drawPixel(&(head->coords));
    world[head->coords.y][head->coords.x] = 1;

    if (snakeIsGrowing) {
      generateFood();
    } 
  }
}

void drawPixel(struct point* point) {
  drawPixel(point->x, point->y);
}
void erasePixel(struct point* point) {
  erasePixel(point->x, point->y);
}
void drawPixel(int x, int y) {
  OrbitOledSetDrawMode(modOledSet);
  OrbitOledMoveTo(x, y);
  OrbitOledDrawPixel();
}
void erasePixel(int x, int y) {
  OrbitOledSetDrawMode(modOledXor);
  OrbitOledMoveTo(x, y);
  OrbitOledDrawPixel();
}
void eraseFood() {
  for (int y = food.coords.y; y < food.coords.y+food.height; y ++) {
    for (int x = food.coords.x;  x < food.coords.x+food.width;  x ++) {
      erasePixel(x,y);
      world[y][x] = 0;
    }
  }
}

void initializeOLED(void) {
  OrbitOledInit();
  OrbitOledClear();
  OrbitOledClearBuffer();
  OrbitOledSetFillPattern(OrbitOledGetStdPattern(iptnSolid));
}

void endGame(void) {
  // free each node forming the snake's linked list
  current = head;
  while (current && current->prev) {
    current = current->prev;
    free(current->next);
  }
  free(current);

  tail = head = NULL;
}
void initGame(void) {
  snakeIsGrowing = false;
  snakeIsDead = false;

  // count in
  OrbitOledSetDrawMode(modOledSet);
  char output[2];
  for (int i = 10; i >= 1; i --) {
    OrbitOledMoveTo(X_BOUND_RIGHT/2,Y_BOUND_BOTTOM/2);
    sprintf(output, "%d", i); 
    OrbitOledDrawString(output);
    OrbitOledUpdate();
    if (i >= 7)
      delay(1000);
    else if (i >= 1)
      delay(35);
    OrbitOledClear();
  }

  direction = RIGHT;

  (void) memset(world, 0, (X_BOUND_RIGHT+1)*(Y_BOUND_BOTTOM+1)*sizeof(char));
  initializeSnake(points, NUM_POINTS); 

  for (int i = 11; i <= 20; i ++) {
    world[i][11] = 1; 
    drawPixel(11, i);
  }
  world[11][12] = 1;
  drawPixel(12, 11);
  world[20][12] = 1;
  drawPixel(12, 20);

  for (int i = 11; i <= 20; i ++) {
    world[i][X_BOUND_RIGHT-11] = 1; 
    drawPixel(X_BOUND_RIGHT-11, i);
  }
  world[11][X_BOUND_RIGHT-12] = 1;
  drawPixel(X_BOUND_RIGHT-12, 11);
  world[20][X_BOUND_RIGHT-12] = 1;
  drawPixel(X_BOUND_RIGHT-12, 20);

  food.width = 3;
  food.height = 3;
  randomSeed(analogRead(0));
  generateFood();
}

/*
 * Reads and updates all of the input states
 * Returns a pointer to the input which has just had its state changed
 *  - if more than one input meets the above criteria, return a pointer to the one that is furthest along from the start of the INPUTS array
 *  - if no input meets the above criteria, return a NULL pointer
 */
struct Input *updateInputs(void) {
  struct Input* lastUpdatedInput = NULL;
  int active = 0;
  for (int i = 0; i < NUM_INPUTS; i ++) {
    active = digitalRead(INPUTS[i].code);
    if (active != INPUTS[i].active) {
      lastUpdatedInput = &INPUTS[i];
      INPUTS[i].active = active;
    }
  }
  return lastUpdatedInput;
}

/*
* Generate a new one in a random location on the screen.
*/
void generateFood(void) {
  // make sure the food does not spawn within the snake or within one pixel of an of its segments
  do {
    food.coords.x = random(0, X_BOUND_RIGHT-(food.width-1));
    food.coords.y = random(0, Y_BOUND_BOTTOM-(food.height-1));
  } while (!validateFoodLocation());

  for (int y = food.coords.y; y < food.coords.y+food.height; y ++) {
    for (int x = food.coords.x;  x < food.coords.x+food.width;  x ++) {
      world[y][x] = 2;
      drawPixel(x,y);
    }
  }
}
/*
* Checks if the attempted food spawn location intersects with the snake or within one pixel of each of its segments
* return true if no intersections were found; Otherwise, return false
*/
boolean validateFoodLocation(void) {
  for (int y = food.coords.y - (food.coords.y == Y_BOUND_TOP ? 0 : 1); y < food.coords.y + food.height + (food.coords.y+food.height-1 == Y_BOUND_BOTTOM ? 0 : 1); y ++) {
    for (int x = food.coords.x - (food.coords.x == X_BOUND_LEFT ? 0: 1);  x < food.coords.x + food.width + (food.coords.x+food.width-1 == X_BOUND_RIGHT ? 0 : 1);  x ++) {
      if (world[y][x] == 1) {
        return false;
      }
    }
  }
  return true;
}


