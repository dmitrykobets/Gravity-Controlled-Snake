#include <FillPat.h>
#include <LaunchPad.h>
#include <OrbitBoosterPackDefs.h>
#include <OrbitOled.h>
#include <OrbitOledChar.h>
#include <OrbitOledGrph.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/sysctl.h"
#include "inc/hw_gpio.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
/*
TODO:
fix global variables vs local variables created in each loop() call... for example: lastUpdatedInput.
coords are being copied around instead of being kept as pointers
prevent food from spawning directly in front of the snake (or maybe even within a range around its head, and also its tail)
*/


void initializeOLED(void);

struct Input *updateInputs(void); 

void appendToHead(const struct point*);
void initializeSnake(const struct point[], uint32_t);

void drawSnake(void);
void eraseTail(void);
void drawHead(void);

char* output = "You Died. Your score was: ";

/*
 * Input
 */
struct Input {
  const uint32_t code;
  boolean active; // for switches, this corresponds to the "up" position. For buttons, this corresponds to the "pressed" position
};

const int NUM_INPUTS = 4;
static Input INPUTS[NUM_INPUTS] = {
  {PF_0, false}, // SW_1
  {PE_3, false}, // SW_2
  {PD_2, false}, // BTN_1
  {PE_0, false}, // BTN_2

};

uint32_t FPS = 10;

struct Input *lastUpdatedInput;

/*
 * Game control constants
 */
struct point {
  uint32_t x;
  uint32_t y;
};

/*
 * Snake components
 */
struct Segment {
  struct point coords;
  struct Segment *next; // towards head
  struct Segment *prev; // towards tail
};
const uint32_t NUM_POINTS =9;
uint32_t points[NUM_POINTS] = {
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

boolean isGrowing = false;

const int X_BOUND_LEFT=0;
const int X_BOUND_RIGHT=128;
const int Y_BOUND_TOP=0;
const int Y_BOUND_BOTTOM=30;

enum direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
} direction;

void setup() {
  Serial.begin(9600);
  direction = RIGHT;

  initializeOLED();

  for (int i = 0; i < NUM_INPUTS; i ++) {
    pinMode(INPUTS[i].code, INPUT);
  }
  
  initializeSnake(points, NUM_POINTS); 
  

  drawSnake();
  drawFood();
  OrbitOledUpdate();
}

void loop() {
  lastUpdatedInput = updateInputs();
  if (lastUpdatedInput) {
    switch(lastUpdatedInput->code) {
      case PD_2: //BTN_1
        if (lastUpdatedInput->active && direction != UP)
          direction = DOWN;
        break;
      case PE_0: //BTN_2
        if (lastUpdatedInput->active && direction != DOWN)
          direction = UP;
        break;
      case PF_0: //SW_2
        if (lastUpdatedInput->active && direction != LEFT)
          direction = RIGHT;
       break;
      // case PA_7:
      //  if (lastUpdatedInput->active)
      //     direction = LEFT;
      case PE_3:
        if (direction!=RIGHT)
          direction = LEFT;

        //else
          //direction = LEFT;
        break;
    }
  } 
  moveSnake();

  moveFood();
  OrbitOledUpdate();
  
  DelayMs(1000/FPS);
}

void appendToHead(const struct point *coords) {
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
void initializeSnake(const struct point coords[], uint32_t numCoords) {
  for (int i = 0; i < numCoords; i ++) {
    appendToHead(&coords[i]);
  }
}

/*
*/
void moveSnake() {

  eraseTail();

	if (isGrowing) {
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
  // To move the snake, the tail node has its coordinates moved ahead of the head (in some direction) and becomes the new head. The node that was previously ahead of the tail
  // becomes the new tail.
	} else {
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
	}
	if (pointsIntersect(&(head->coords), &food)) {
		generateFood();
		isGrowing = true;
  }

	// swap the head node with the tail node
  current = tail;
  tail->prev = head;
  current = tail;
  tail = tail->next;
  tail->prev = NULL;
  head->next = current;
  head = current;

	// the snake can never intersect with its first three segments
	if (intersectsWithSnake(head->prev->prev->prev, head->coords)) {
		OrbitOledMoveTo(0, 0);
		OrbitOledDrawString("You died!");
	} 

  drawHead();
}

void drawSnake(void) {
  OrbitOledSetDrawMode(modOledSet);
  current = head;
  while (current) {
    OrbitOledMoveTo(current->coords.x, current->coords.y);
    OrbitOledDrawPixel();
    current = current->prev;
  }
}

void eraseTail(void) {
  OrbitOledSetDrawMode(modOledXor); 
  OrbitOledMoveTo(tail->coords.x, tail->coords.y);
  OrbitOledDrawPixel();
}
void drawHead(void) {
  OrbitOledSetDrawMode(modOledSet); 
  OrbitOledMoveTo(head->coords.x, head->coords.y);
  OrbitOledDrawPixel();
}

void initializeOLED(void) {
  OrbitOledInit();
  OrbitOledClear();
  OrbitOledClearBuffer();
  OrbitOledSetFillPattern(OrbitOledGetStdPattern(iptnSolid));
}

/*
 * Reads and updates all of the input states
 * Returns a pointer to the input which has just had its active changed
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

boolean intersectsWithSnake(struct Segment *head, struct point* point) {
	while (head) {
		if (pointsIntersect(point, &(head->coords))) {
			return true;
		}
		head = head->next;
	}
	return false;
}

/*
* Return true if two points have identical x and y components; Otherwise, return false
*/
boolean pointsIntersect(struct point* point1, struct point* point2) {
	return point1->x == point2->x && point1->y == point2->y;
}

/*
* Generate a new one in a random location on the screen.
*/
void generateFood(void) {
	// make sure the food does not spawn within the snake
	do {
		food.x = randNum(128);
		food.y = randNum(32);
	} while (intersectWithSnake(food));

  drawFood();
}

void eraseFood(void){
  OrbitOledSetDrawMode(modOledXor); 
  OrbitOledMoveTo(food.x, food.y);
  OrbitOledDrawPixel();
}
void drawFood(){
  OrbitOledSetDrawMode(modOledSet); 
  OrbitOledMoveTo(food.x, food.y);
  OrbitOledDrawPixel();
}
