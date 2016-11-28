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
#include "screen.c"
#include "scrolling_menu.c"

/*
TODO:
fix global variables vs local variables created in each loop() call... for example: lastUpdatedInput.
coords are being copied around instead of being kept as pointers
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
struct Input *lastUpdatedInput;

int FPS = 10;

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

char world[crowOledMax][ccolOledMax] = {0};

enum direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
} direction=RIGHT;

float pitchOffset = 0, rollOffset = 0;
//////////////////////////////////// SCREENS ////////////////////////////////////////
/////// Main Menu ////////////
struct Screen *mainMenuScreen;

char *mainMenuOptions[] = {"play game", "settings"};

void initMainMenuScreen(void) {
  initScrollingMenu("main menu", mainMenuOptions, sizeof(mainMenuOptions)/sizeof(mainMenuOptions[0]));
}
void runMainMenuScreen(void) {
  lastUpdatedInput = updateInputs();
	if (lastUpdatedInput->code == PD_2 && lastUpdatedInput->active) {
		int optionIndex = getSelectedOptionIndex();
		switch (optionIndex) {
			case 0:
				switchScreen(&gameScreen);
				return;
			case 1:
				switchScreen(&settingsScreen);
				return;
		}
	}
	readAccelerometer(&pitch, &roll, pitchOffset, rollOffset);
	scroll(pitch, roll);
}

mainMenuScreen->init = initMainMenuScreen;
mainMenuScreen->run = runMainMenuScreen;
mainMenuScreen->end = NULL;
/////// end Main Menu ///////////

//////// Game ///////////////////
struct Screen *gameScreen;

void initGame(void) {
  snakeIsGrowing = false;
  snakeIsDead = false;

  // count in
  OrbitOledSetDrawMode(modOledSet);
  char output[2];
  for (int i = 10; i >= 1; i --) {
    OrbitOledMoveTo(ccolOledMax/2,crowOledMax/2);
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

  (void) memset(world, 0, (ccolOledMax)*(crowOledMax)*sizeof(char));
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
    world[i][ccolOledMax-11] = 1; 
    drawPixel(ccolOledMax-11, i);
  }
  world[11][ccolOledMax-12] = 1;
  drawPixel(ccolOledMax-12, 11);
  world[20][ccolOledMax-12] = 1;
  drawPixel(ccolOledMax-12, 20);

  food.width = 3;
  food.height = 3;
  randomSeed(analogRead(0));
  generateFood();
}


void runGameScreen(void) {
  lastUpdatedInput = updateInputs();
  switch(lastUpdatedInput->code) {
    case PF_0:
      zeroAccelerometer();
      break;
    case PE_0:
      if (lastUpdatedInput->active)
        switchScreen(&mainMenuScreen);
      return;
  }

  updateDirection();
  moveSnake();

  if (snakeIsDead) {
    switchScreen(&deathTransitionScreen);
    return;
  }
  OrbitOledUpdate();
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

gameScreen->init = initGameScreen;
gameScreen->run = runGameScreen;
gameScreen->end = endGameScreen;
/////////////////////// end Game /////////////////////

///////////////// Settings ///////////////////////////
struct Screen *settingsScreen;

char *settingsOptions[] = {"back to menu", "calibrate"};

void initSettings(void) {
  initScrollingMenu("settings", settingsOptions, sizeof(settingsOptions)/sizeof(settingsOptions[0]));
}

void runSettings(void) {
  lastUpdatedInput = updateInputs();
	if (lastUpdatedInput->code == PD_2 && lastUpdatedInput->active) {
		int optionIndex = getSelectedOptionIndex();
		switch (optionIndex) {
			case 0:
				switchScreen(&mainMenuScreen);
				return;
			case 1:
				switchScreen(&calibrateScreen);
				return;
		}
	}
	readAccelerometer(&pitch, &roll, pitchOffset, rollOffset);
	scroll(pitch, roll);
}
settingScreen->init = initSettings;
settingScreen->run = runSettings;
settingScreen->end = NULL;
/////////////////// end Settings ////////////////

//////////////////////// Death //////////////////
struct Screen *deathScreen;

char *deathOptions[] = {"try again", "give up"};

void initDeathScreen(void) {

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
	OrbitOledClear();

  initScrollingMenu("The void", deathOptions, sizeof(deathOptions)/sizeof(deathOptions[0]));
}
void runDeathScreen(void) {
  lastUpdatedInput = updateInputs();
	if (lastUpdatedInput->code == PD_2 && lastUpdatedInput->active) {
		int optionIndex = getSelectedOptionIndex();
		switch (optionIndex) {
			case 0:
				switchScreen(&gameScreen);
				return;
			case 1:
				switchScreen(&mainMenuScreen);
				return;
		}
	}
	readAccelerometer(&pitch, &roll, pitchOffset, rollOffset);
	scroll(pitch, roll);
}

deathScreen->init = initDeathScreen;
deathScreen->run = runDeathScreen;
deathScreen->end = NULL;
///////////////////// end Death //////////////

/////////////////// Calibrate ///////////////
struct Screen *calibrateScreen;

void initCalibrateScreen() {
  OrbitOledSetDrawMode(modOledSet);
  OrbitOledMoveTo(0, 0);
  OrbitOledDrawString("Orient the board");
  OrbitOledMoveTo(5, 10);
  OrbitOledDrawString("& Press BTN1");
  OrbitOledUpdate();
}

void runCalibrateScreen() {
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

calibrateScreen->init = initCalibrateScreen; 
calibrateScreen->run = runCalibrateScreen; 
calibrateScreen->end = NULL; 
/////////////////// end Calibrate //////////
//////////////////////////////END SCREENS///////////////////////////////////////////

void setup() {
  WireInit();
  delay(200);
  
  WireWriteRegister(0x1D, 0x2D, 1 << 3); // set standby in the POWER_CTL register
  WireWriteRegister(0x1D, 0x31, 1); // set 10-bit res with 4g range

  for (int i = 0; i < NUM_INPUTS; i ++) {
    pinMode(INPUTS[i].code, INPUT);
  
  pinMode(GREEN_LED, OUTPUT);

  initializeOLED();
  
  OrbitOledUpdate();

	switchScreen(&mainMenuScreen);

  Serial.begin(9600);
}

void loop() {
	runScreen();
  DelayMs(1000/FPS);
}

/*
* Stores the current pitch and roll as offsets for future readings
*/
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
    
		// the acceleration data should be in signed int form (unsigned hides negatives)
    float x = *(int16_t*)(&xi); 
    float y = *(int16_t*)(&yi);
    float z = *(int16_t*)(&zi);

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

  if (head->coords.x >= ccolOledMax) {
    head->coords.x = 0;
  }
  else if (head->coords.x < 0) {
    head->coords.x = ccolOledMax;
  }
  if (head->coords.y >= crowOledMax) {
    head->coords.y = 0;
  }
  if (head->coords.y < 0) {
    head->coords.y = crowOledMax-1;
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
    food.coords.x = random(0, ccolOledMax-food.width);
    food.coords.y = random(0, crowOledMax-food.height);
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
  for (int y = food.coords.y - (food.coords.y == 0 ? 0 : 1); y <= food.coords.y + food.height && y < crowOledMax; y ++) {
    for (int x = food.coords.x - (food.coords.x == 0 ? 0: 1);  x <= food.coords.x + food.width && x < ccolOledMax;  x ++) {
      if (world[y][x] == 1) {
        return false;
      }
    }
  }
  return true;
}
