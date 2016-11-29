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
#include "accelerometer.c"

/*
TODO:
change food spawn algorithm to detect when it's impossible to spawn food (game is over) and minimize random attempts (so basically make it not entirely random
separate scrolling menu from input and accelerometer logic (not too big of a deal)

see if can added a low-pass filter to the data instead of taking an average of 5 readings
see if can change the data array in accelerometer.c to be of type uint16_t
*/

void updateInputs(void); 

void initSnake(struct point[], int);
void appendToHead(struct point*);
void appendToHead(int,int);
void updateDirection(void); 
void moveSnake(void);

void eraseFood(void);
void generateFood(void);
boolean validateFoodLocation(void);

void initOLED(void);
void drawPixel(struct point*);
void drawPixel(int,int);
void erasePixel(struct point*);
void erasePixel(int,int);

int FPS = 10;

struct Input {
  const int code;
  boolean active; // for switches, this corresponds to the "up" position. For buttons, this corresponds to the "pressed" position
};
struct Input *lastUpdatedInput = NULL;

const int NUM_INPUTS = 3;
struct Input INPUTS[NUM_INPUTS] = {
  {PF_0, false}, // SW_1
  {PD_2, false}, // BTN_1
  {PE_0, false}, // BTN_2
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

struct SnakeSegment {
  struct point coords;
  struct SnakeSegment *next; // towards head
  struct SnakeSegment *prev; // towards tail
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

struct SnakeSegment *head = NULL;
struct SnakeSegment *tail = NULL;

boolean snakeIsGrowing = false;
boolean snakeIsDead = false;

enum direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
} direction=RIGHT;

char world[crowOledMax][ccolOledMax] = {0};

//////////////////////////////////// SCREENS ////////////////////////////////////////
struct Screen mainMenuScreen;
struct Screen gameScreen;
struct Screen settingsScreen;
struct Screen deathScreen;
struct Screen calibrateScreen;
/////// Main Menu ////////////
char *mainMenuOptions[] = {"play game", "settings"};

void initMainMenuScreen(void) {
  initScrollingMenu("main menu", mainMenuOptions, sizeof(mainMenuOptions)/sizeof(mainMenuOptions[0]));
}
void runMainMenuScreen(void) {
  updateInputs();
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
	readAccelerometer(pitchOffset, rollOffset);
	scroll(roll);
}
/////// end Main Menu ///////////

//////// Game ///////////////////
void initGameScreen(void) {
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
  initSnake(points, NUM_POINTS); 

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
  updateInputs();
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
    switchScreen(&deathScreen);
    return;
  }
  OrbitOledUpdate();
}

void endGameScreen(void) {
  // free each node forming the snake's linked list
  struct SnakeSegment *current = head;
  while (current && current->prev) {
    current = current->prev;
    free(current->next);
  }
  free(current);

  tail = head = NULL;
}
/////////////////////// end Game /////////////////////

///////////////// Settings ///////////////////////////
char *settingsOptions[] = {"back to menu", "calibrate"};

void initSettingsScreen(void) {
  initScrollingMenu("settings", settingsOptions, sizeof(settingsOptions)/sizeof(settingsOptions[0]));
}

void runSettingsScreen(void) {
  updateInputs();
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
	readAccelerometer(pitchOffset, rollOffset);
	scroll(roll);
}
/////////////////// end Settings ////////////////

//////////////////////// Death //////////////////
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
  updateInputs();
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
	readAccelerometer(pitchOffset, rollOffset);
	scroll(roll);
}
///////////////////// end Death //////////////

/////////////////// Calibrate ///////////////
void initCalibrateScreen() {
  OrbitOledSetDrawMode(modOledSet);
  OrbitOledMoveTo(0, 0);
  OrbitOledDrawString("Orient the board");
  OrbitOledMoveTo(5, 10);
  OrbitOledDrawString("& Press BTN1");
  OrbitOledUpdate();
}

void runCalibrateScreen() {
  updateInputs();
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
        switchScreen(&settingsScreen);
        return;
      }
  }
}
/////////////////// end Calibrate //////////
//////////////////////////////END SCREENS///////////////////////////////////////////

void setup() {
  WireInit();
  delay(200);
 
 	initAccelerometer();

  for (int i = 0; i < NUM_INPUTS; i ++)
    pinMode(INPUTS[i].code, INPUT);
  pinMode(GREEN_LED, OUTPUT);

  initOLED();
  
  OrbitOledUpdate();

  mainMenuScreen = {
    .init = initMainMenuScreen,
    .run = runMainMenuScreen,
    .end = NULL
  };
  gameScreen = {
    .init = initGameScreen,
    .run = runGameScreen,
    .end = endGameScreen
  };
  settingsScreen = {
    .init = initSettingsScreen,
    .run = runSettingsScreen,
    .end = NULL
  };
  deathScreen = {
    .init = initDeathScreen,
    .run = runDeathScreen,
    .end = NULL
  };
  calibrateScreen = {
    .init = initCalibrateScreen,
    .run = runCalibrateScreen,
    .end = NULL
  };
  
	switchScreen(&mainMenuScreen);

  Serial.begin(9600);
}

void loop() {
	runScreen();
  DelayMs(1000/FPS);
}

/* Function: updateInputs
 * ---------------------
 * Update input states. The last input in the list to be updated is assigned to 
 * the "lastUpdatedInput" pointer.
 * Parameters: none
 * Return values: none
 */
void updateInputs(void) {
  for (int i = 0; i < NUM_INPUTS; i ++) {
    int active = digitalRead(INPUTS[i].code);
    if (active != INPUTS[i].active) {
      INPUTS[i].active = active;
      lastUpdatedInput = &(INPUTS[i]);
    }
  }
}

void initSnake(struct point coords[], int numCoords) {
  for (int i = 0; i < numCoords; i ++) {
    appendToHead(&(coords[i]));
    world[coords[i].y][coords[i].x] = 1;
    drawPixel(&(coords[i]));
  }
}

void appendToHead(int x, int y) {
  struct point newPoint = {x, y};
  appendToHead(&newPoint);
}

void appendToHead(struct point *coords) {
  struct SnakeSegment* newHead = (struct SnakeSegment*) malloc(sizeof(struct SnakeSegment));
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

/* Function: updateDirection
 * -------------------------
 * Read the pitch and roll of the accelerometer, and change the snake's direction accordingly. If the pitch and roll are within 7 degrees away from 0, the device is considered stationary. To prevent rapid switching between directions at transition points (ex: 45 degrees top left is the line between left and up), require that the pitch and roll value be different by 15 degrees
 *
 * Parameters: none
 * Return values: none
 */
void updateDirection(void) {
  readAccelerometer(pitchOffset, rollOffset);

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

/* Function moveSnake
 * -------------------
 * Move the snake by one pixel in the direction of movement
 * If the snake collides with a piece of food, grow by two pixels and generate a new food location
 * If the snake collides with itself, the game is over
 * Parameters: none
 * Return values: none
 */
void moveSnake(void) {
	// grow the snake by 2 pixels at a time
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
    struct SnakeSegment *current = tail;
    tail->prev = head;
    current = tail;
    tail = tail->next;
    tail->prev = NULL;
    head->next = current;
    head = current;
  }

	// wrap around the screen
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
		
		// food is generated after the snake has been drawn so that it does not intersect with the snake
    if (snakeIsGrowing) {
      generateFood();
    } 
  }
}

void eraseFood(void) {
  for (int y = food.coords.y; y < food.coords.y+food.height; y ++) {
    for (int x = food.coords.x; x < food.coords.x+food.width;  x ++) {
      erasePixel(x,y);
      world[y][x] = 0;
    }
  }
}

void generateFood(void) {
  // make sure the food does not spawn within the snake or within one pixel of any of its segments
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
/* Function: validateFoodLocation
 * ------------------------------
 * Check if any pixel of food is within one pixel of any of the snake's segments.
 * 
 * Parameters: none
 * Return values: true if the food does not come within any part of the snake; Otherwise,
 false.
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

void initOLED(void) {
  OrbitOledInit();
  OrbitOledClear();
  OrbitOledClearBuffer();
  OrbitOledSetFillPattern(OrbitOledGetStdPattern(iptnSolid));
}
void drawPixel(int x, int y) {
  OrbitOledSetDrawMode(modOledSet);
  OrbitOledMoveTo(x, y);
  OrbitOledDrawPixel();
}
void drawPixel(struct point* point) {
  drawPixel(point->x, point->y);
}
void erasePixel(int x, int y) {
  OrbitOledSetDrawMode(modOledXor);
  OrbitOledMoveTo(x, y);
  OrbitOledDrawPixel();
}
void erasePixel(struct point* point) {
  erasePixel(point->x, point->y);
}
