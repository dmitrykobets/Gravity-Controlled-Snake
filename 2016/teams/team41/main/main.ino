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
change game structure to revolve around a 2D array instead of a linked list
drawPixel/erasePixel with just coords aren't being used currently
need to investigate the WriteByte/WriteArray functions in the acceleromter updateDirection() function
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

int genRandNum(int n);
boolean intersectsWithSnake(struct Segment *, struct point*);

void updateDirection(void); 

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

/*
 * Game control constants
 */
struct point {
  int x;
  int y;
} food;

/*
 * Snake components
 */
struct Segment {
  struct point coords;
  struct Segment *next; // towards head
  struct Segment *prev; // towards tail
};
const int NUM_POINTS = 9;
struct point points[NUM_POINTS] = {
    {11, 11}, 
    {12, 11}, 
    {13, 11}, 
    {14, 11}, 
    {15, 11}, 
    {16, 11}, 
    {17, 11}, 
    {18, 11}, 
    {19, 11}};

struct Segment *head = NULL;
struct Segment *tail = NULL;
struct Segment *current = NULL;

boolean snakeIsGrowing = false;

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


float LSBperG = 128.0;

void setup() {
  WireInit();
  delay(200);
  
  WireWriteRegister(0x1D, 0x2D, 1 << 3); // set standby in the POWER_CTL register
  WireWriteRegister(0x1D, 0x31, 1); // set 10-bit res with 4g range

  initializeOLED();
  
  initGame();
  OrbitOledUpdate();
}

void loop() {

  updateDirection();
  moveSnake();
  
  OrbitOledUpdate();
  
  DelayMs(1000/FPS);
}

/*
* Read accelerometer data and return the snake's updated direction based on the pitch & roll of the accelerometer
*/
void updateDirection(void) {
  uint32_t data[6] = {0}; // instead of getting X0, X1, Y0, ... data separately, simply get all 6 bytes in one read. According to the specification, this removes the risk
                          // of the data in those registers changing between access calls
  WireWriteByte(0x1D, 0x32); // 
  WireRequestArray(0x1D, data, 6);

  uint16_t xi = (data[1] << 8) | data[0];
  uint16_t yi = (data[3] << 8) | data[2];
  uint16_t zi = (data[5] << 8) | data[4];

  
  float x = *(int16_t*)(&xi); // LSBperG
  float y = *(int16_t*)(&yi); // LSBperG
  float z = *(int16_t*)(&zi); // LSBperG

  float pitch = (atan2(x,sqrt(y*y+z*z)) * 180.0) / PI;
  float roll = (atan2(y,sqrt(x*x+z*z)) * 180.0) / PI;

  if (fabs(pitch) <= 10 && fabs(roll) <= 10) {
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
        break;
      case RIGHT:
        appendToHead(head->coords.x+1, head->coords.y);
        break;
      case UP:
        appendToHead(head->coords.x, head->coords.y-1);
        break;
      case DOWN:
        appendToHead(head->coords.x, head->coords.y+1);
        break;
    }
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
    OrbitOledMoveTo(0, 0);
    OrbitOledDrawString("You died!");
  }
  else {
    if (world[head->coords.y][head->coords.x] == 2) {
      eraseFood();
      generateFood();
      snakeIsGrowing = true;
    }
    drawPixel(&(head->coords));
    world[head->coords.y][head->coords.x] = 1;
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
  erasePixel(&food);
}

void initializeOLED(void) {
  OrbitOledInit();
  OrbitOledClear();
  OrbitOledClearBuffer();
  OrbitOledSetFillPattern(OrbitOledGetStdPattern(iptnSolid));
}

void initGame() {
  direction = RIGHT;

  (void) memset(world, 0, (X_BOUND_RIGHT+1)*(Y_BOUND_BOTTOM+1)*sizeof(char));
  initializeSnake(points, NUM_POINTS); 

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
* Generates a random number between 0 and @n inclusive
*/
int genRandNum(int n){
  return rand()%(n+1);
}

/*
* Generate a new one in a random location on the screen.
*/
void generateFood(void) {
  // make sure the food does not spawn within the snake or within one pixel of any of its segments
  do {
    food.x = genRandNum(X_BOUND_RIGHT);
    food.y = genRandNum(Y_BOUND_BOTTOM);
  } while (!validateFoodLocation(&food));

  world[food.y][food.x] = 2;

  drawPixel(&food);
}
/*
* Checks if the attempted food spawn location intersects with the snake or within one pixel of each of its segments
* return true if no intersections were found; Otherwise, return false
*/
boolean validateFoodLocation(struct point* food) {
  if (
    world[food->y][food->x] == 1 || 
    world[food->y-1][food->x-1] == 1 || 
    world[food->y][food->x-1] == 1 ||
    world[food->y+1][food->x-1] == 1 ||
    world[food->y+1][food->x] == 1 ||
    world[food->y+1][food->x+1] == 1 ||
    world[food->y][food->x+1] == 1 ||
    world[food->y-1][food->x+1] == 1 ||
    world[food->y-1][food->x] == 1
  ) 
    return false;
  return true;
}


