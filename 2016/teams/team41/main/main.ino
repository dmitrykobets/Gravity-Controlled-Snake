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
#include <math.h>
#include "Wire_Util.h"
#include "screen.h"
#include "scrolling_menu.h"
#include "accelerometer.h"

/*
TODO:
NOTHING LUL
*/

void getLeaderboards(void);
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


int letterCounter = 0;
int rankOfNewScore = 0;
uint32_t score = 0;
uint32_t name[3];

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
boolean gameIsWon = false;

enum direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
} direction=RIGHT;

char world[crowOledMax][ccolOledMax] = {0};

extern float pitchOffset;
extern float rollOffset;
extern float roll;
extern float pitch;

//////////////////////////////////// SCREENS ////////////////////////////////////////
struct Screen mainMenuScreen;
struct Screen gameScreen;
struct Screen settingsScreen;
struct Screen deathScreen;
struct Screen calibrateScreen;
struct Screen winScreen;
struct Screen nameScreen;
struct Screen leaderboardScreen;

//////////////////////////////////// OPTIONS FOR MENU SCREENS ////////////////////////////////////////
char *mainMenuOptions[] = {"play game", "settings", "leaderboards"};
char *nameOptions[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"};
char *deathOptions[] = {"try again", "give up"};
char *settingsOptions[] = {"back to menu", "calibrate"};
char *winOptions[] = {"start again", "main menu"};
char *leaderboards[5];

//////////////////////////////SCREEN FUNCTIONS///////////////////////////////////////////

/////////////////// Main Menu ///////////////////
void initMainMenuScreen(void) {
  initScrollingMenu("main menu", mainMenuOptions, sizeof(mainMenuOptions)/sizeof(mainMenuOptions[0]));
}

void runMainMenuScreen(void) {
  updateInputs();
  if (lastUpdatedInput && lastUpdatedInput->code == PD_2 && lastUpdatedInput->active) {
    int optionIndex = getSelectedOptionIndex();
    switch (optionIndex) {
      case 0:
        switchScreen(&gameScreen);
        return;
      case 1:
        switchScreen(&settingsScreen);
        return;
      case 2:
        getLeaderboards();
        switchScreen(&leaderboardScreen);
        return;
    }
  }
  readAccelerometer(pitchOffset, rollOffset);
  scroll(roll);
}
/////////////////// end Main Menu ///////////////////

/////////////////// Leaderboard ///////////////////
void getLeaderboards(){
  uint32_t highScoreNames[15];
  uint32_t highScores[5];
  char **scoresString;

  EEPROMRead(highScoreNames,  0x400af200, sizeof(highScoreNames)); 
  EEPROMRead(highScores, EEPROMADDR, sizeof(highScores));  
  
  scoresString = (char**)malloc(5 * sizeof(char *));
  for (int i = 0; i < 5; ++i) {
    scoresString[i] = (char *)malloc(6*sizeof(char));
  }

  for (int i = 0; i<5; i++){
    char *str;
    str = (char*)malloc(15*sizeof(char));

    for(int j=0; j<3; j++){
      str[j] =  highScoreNames[i*3+j];
    }

    str[3] = '\0';
    leaderboards[4-i]= str;
  }

  for (int i = 0; i<5; i++){
    sprintf(scoresString[i], ": %lu", highScores[4-i]);
    strcat(leaderboards[i], scoresString[i]);
  }

  for (int i = 0; i < 5; ++i) {
    free(scoresString[i]);
  }

  free(scoresString);
}

void initLeaderboardScreen(void) {
  initScrollingMenu("leaderboards", leaderboards, sizeof(leaderboards)/sizeof(leaderboards[0]));
}

void runLeaderboardScreen(void) {
  updateInputs();
	if (lastUpdatedInput && lastUpdatedInput->code == PD_2 && lastUpdatedInput->active) {
		switchScreen(&mainMenuScreen);
	}
	readAccelerometer(pitchOffset, rollOffset);
	scroll(roll);
}
/////////////////// end Leaderboard ///////////////////

/////////////////// Name Screen ///////////////////
void initNameScreen(void) {
  initScrollingMenu("enter your name", nameOptions, sizeof(nameOptions)/sizeof(nameOptions[0]));
}

void runNameScreen(void) {
  uint32_t highScoreNames[15];
  
  updateInputs();

  if (lastUpdatedInput && lastUpdatedInput->code == PD_2 && lastUpdatedInput->active) {
    int optionIndex = getSelectedOptionIndex();
    if (optionIndex>=0){
      EEPROMRead(highScoreNames,  0x400af200, sizeof(highScoreNames));  

      name[letterCounter] = optionIndex + 65;
      letterCounter++;
      
      if (letterCounter<3){
        switchScreen(&nameScreen);
        return;
      }

      else{
        letterCounter=0;
        for (int i = 0; i<3; i++){
          for (int j = 0; j<3*rankOfNewScore-i+2; j++){
            highScoreNames[j]=highScoreNames[j+1];
          }
        }
      
        for (int i= 3*rankOfNewScore; i<3*rankOfNewScore+3; i++){
          highScoreNames[i]=name[i-3*rankOfNewScore];
        
        }

        EEPROMProgram(highScoreNames,  0x400af200, sizeof(highScoreNames));  
        switchScreen(&mainMenuScreen);
        return;
      }
    }
  }
  readAccelerometer(pitchOffset, rollOffset);
  scroll(roll);
}     
/////////////////// end Name Screen ///////////////////

/////////////////// Game ///////////////////
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

	// draw obstacles for the snake
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
  if (lastUpdatedInput && lastUpdatedInput->code == PE_0 && lastUpdatedInput->active) {
    switchScreen(&mainMenuScreen);
    return;
  }
  
  updateDirection();
  moveSnake();

  if (snakeIsDead) {
    switchScreen(&deathScreen);
    return;
  } else if (gameIsWon) {
		switchScreen(&winScreen);
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
/////////////////// end Game ///////////////////

/////////////////// Settings ///////////////////
void initSettingsScreen(void) {
  initScrollingMenu("settings", settingsOptions, sizeof(settingsOptions)/sizeof(settingsOptions[0]));
}

void runSettingsScreen(void) {
  updateInputs();
  if (lastUpdatedInput && lastUpdatedInput->code == PD_2 && lastUpdatedInput->active) {
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
/////////////////// end Settings ///////////////////

/////////////////// Death ///////////////////
void printScoreMSGs(int x, int y, int time, char* msg){
  OrbitOledMoveTo(x, y);
  OrbitOledDrawString(msg);
  OrbitOledUpdate();
  delay(time);
  OrbitOledClear();
}

void initDeathScreen(void) {
  uint32_t highScores[5];
  char scoreMsg[30] = "Score: ";
  char score1[10];
  boolean isHighScore = false;
 
  OrbitOledSetDrawMode(modOledSet);
  printScoreMSGs(10,10, 1500, "You died!");
  printScoreMSGs(20,20,1500, "... pathetic");

  sprintf(score1, "%u", score);
  strcat(scoreMsg, score1);
  printScoreMSGs(10,10,1500, scoreMsg);
  EEPROMRead(highScores, EEPROMADDR, sizeof(highScores));  

  for (int i =0; i<5; i++){
    if (score>highScores[i]){
      isHighScore = true ;
      rankOfNewScore = i;
    }
  }
  
  if (isHighScore){
    for (int i = 0; i<rankOfNewScore; i++){
      highScores[i]=highScores[i+1];
    }
    highScores[rankOfNewScore]=score;
 
    OrbitOledSetDrawMode(modOledSet);
    OrbitOledMoveTo(0, 0);
    OrbitOledDrawString("You got");
    OrbitOledMoveTo(5, 10);
    OrbitOledDrawString("a highscore!");
    OrbitOledUpdate();
    delay(1500);

    EEPROMProgram(highScores, EEPROMADDR, sizeof(highScores));  
    isHighScore = false;
    score = 0;
    switchScreen(&nameScreen);
    return;
  }
  score = 0;
  switchScreen(&mainMenuScreen);
  return;
}

void runDeathScreen(void) {
  updateInputs();
	if (lastUpdatedInput && lastUpdatedInput->code == PD_2 && lastUpdatedInput->active) {
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
/////////////////// end Death ///////////////////

/////////////////// Win ///////////////////
void initWinScreen(void) {
	OrbitOledMoveTo(0, 0);
	for (int i = 0; i < 6; i ++) {
		OrbitOledSetDrawMode(modOledSet);
		if (i%2 == 0) {
			OrbitOledFillRect(ccolOledMax-1, crowOledMax-1);
			OrbitOledSetDrawMode(modOledXor);
		}
		OrbitOledMoveTo(10, crowOledMax/2-4);
		OrbitOledDrawString("Winner!");
		delay(700);
	}
  initScrollingMenu("You won!", winOptions, sizeof(winOptions)/sizeof(winOptions[0]));
}
void runWinScreen(void) {
  updateInputs();
	if (lastUpdatedInput && lastUpdatedInput->code == PD_2 && lastUpdatedInput->active) {
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
/////////////////// end Win /////////////////// 

/////////////////// Calibrate /////////////////// 
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
	if (lastUpdatedInput && lastUpdatedInput->code == PD_2 && lastUpdatedInput->active) {
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
/////////////////// end Calibrate /////////////////// 

////////////////////////////////// END SCREEN FUNCTIONS //////////////////////////////////

void setup() {
  WireInit();
  delay(200);
  SysCtlPeripheralEnable(SYSCTL_PERIPH_EEPROM0);
  EEPROMInit();
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
  winScreen = {
    .init = initWinScreen,
    .run = runWinScreen,
    .end = NULL
  };
  calibrateScreen = {
    .init = initCalibrateScreen,
    .run = runCalibrateScreen,
    .end = NULL
  };
  nameScreen = {
    .init = initNameScreen,
    .run = runNameScreen,
    .end = NULL
  };
  leaderboardScreen = {
    .init = initLeaderboardScreen,
    .run = runLeaderboardScreen,
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
 * the "lastUpdatedInput" pointer. If no input has been updated, set "lastUpdatedInput" to NULL.
 * Parameters: none
 * Return values: none
 */
void updateInputs(void) {
	lastUpdatedInput = NULL;
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
    score++;
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
    head->coords.x = ccolOledMax-1;
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
	int spawnAttempts = 0;
  do {
    food.coords.x = random(0, ccolOledMax-food.width);
    food.coords.y = random(0, crowOledMax-food.height);
		spawnAttempts ++;
  } while (!validateFoodLocation() && spawnAttempts < 1000);
	if (spawnAttempts >= 1000) {
		gameIsWon = true;
		return;
	}

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

