#include <FillPat.h>
#include <LaunchPad.h>
#include <OrbitBoosterPackDefs.h>
#include <OrbitOled.h>
#include <OrbitOledChar.h>
#include <OrbitOledGrph.h>
#include <string.h>
#include <delay.h>
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
change game structure to revolve around a 2D array instead of a linked list
refactor button code
merge button "handler" with the current button structure
comment "moveSnake()" function
*/

void onButtonDown(void);
void setupLeftButton(void);
void initializeOLED(void);

struct Input *updateInputs(void); 

int genRandNum(int n);

void initializeSnake(struct point[], int);
void drawSnake(void);
void eraseTail(void);
void drawHead(void);

void appendToHead(int, int); 

void drawPixel(struct point*); 
void drawPixel(int, int); 

/*
 * Input
 */
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
int active;
int FPS = 10;
struct Input *lastUpdatedInput;

/*
 * Game control constants
 */
struct point {
  int x;
  int y;
} food;

struct snakePart {
	struct point coords;
	struct snakePart* prev;
};

const int X_BOUND_LEFT=0;
const int X_BOUND_RIGHT=127;
const int Y_BOUND_TOP=0;
const int Y_BOUND_BOTTOM=31;

char map[Y_BOUND_BOTTOM+1][X_BOUND_RIGHT+1] = {0};

struct snakePart* head = NULL;
struct snakePart* tail = NULL;
boolean isGrowing = false;

enum direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
} direction=RIGHT;

void setup() {

  Serial.begin(9600);

  initializeOLED();

  for (int i = 0; i < NUM_INPUTS; i ++) {
    pinMode(INPUTS[i].code, INPUT);
  }
  
	initGame();
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
    }
  } 
  moveSnake();
  
  OrbitOledUpdate();
  
  DelayMs(1000/FPS);
}

void moveSnake() {
	switch(direction) {
		case LEFT:
			head.x --;
			break;
		case RIGHT:
			head.x ++;
			break;
		case UP:
			head.y --;
			break;
		case DOWN:
			head.y ++;
			break;
	}
  // To move the snake, the tail node has its coordinates moved ahead of the head (in some direction) and becomes the new head. The node that was previously ahead of the tail
  // becomes the new tail.
	if (!isGrowing)
		eraseTail();
  else {
    eraseTail();
    switch(direction) {
      case LEFT:
				head
        break;
      case RIGHT:
        break;
      case UP:
        break;
      case DOWN:
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

  if (head->coords.x >= X_BOUND_RIGHT) {
    head->coords.x = 0;
  }
  else if (head->coords.x < X_BOUND_LEFT) {
    head->coords.x = X_BOUND_RIGHT-1;
  }
  if (head->coords.y >= Y_BOUND_BOTTOM) {
    head->coords.y = 0;
  }
  if (head->coords.y < Y_BOUND_TOP) {
    head->coords.y = Y_BOUND_BOTTOM-1;
  }

  if (pointsIntersect(&(head->coords), &food)) {
    generateFood();
    isGrowing = true;
  } else {
    // the snake can never intersect with its first three segments
    if (intersectsWithSnake(head->prev->prev->prev, &(head->coords))) {
      OrbitOledMoveTo(0, 0);
      OrbitOledDrawString("You died!");
    } 
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
 * Returns a pointer to the input which has just had its state changed
 *  - if more than one input meets the above criteria, return a pointer to the one that is furthest along from the start of the INPUTS array
 *  - if no input meets the above criteria, return a NULL pointer
 */
struct Input *updateInputs(void) {
  struct Input* lastUpdatedInput = NULL;
  active = 0;
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
* Generates a random number between 0 and 'n' inclusive
*/
int genRandNum(int n){
  return rand()%(n+1);
}

/*
* Generate a new one in a random location on the screen.
*/
void generateFood(void) {
  // make sure the food does not spawn within the snake or within one pixel of its head or tail
  do {
    food.x = genRandNum(X_BOUND_RIGHT);
    food.y = genRandNum(Y_BOUND_BOTTOM);
  } while (!validateFoodLocation(&food));
	map[food.y][food.x] = 1;

  drawFood();
}
/*
* Checks if the attempted food spawn location intersects with the snake or within one pixel of any of its segments
* return true if no intersections were found; Otherwise, return false
*/
boolean validateFoodLocation(struct point* food) {
	if (map[food->y][food->x] == 1 || 
		map[food->y-1][food->x-1] == 1 || 
		map[food->y][food->x-1] == 1 ||
		map[food->y+1][food->x-1] == 1 ||
		map[food->y+1][food->x] == 1 ||
		map[food->y+1][food->x+1] == 1 ||
		map[food->y][food->x+1] == 1 ||
		map[food->y-1][food->x+1] == 1 ||
		map[food->y-1][food->x] == 1 ||
	) 
		return false;
	return true;
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

/*
* Initialize game elements such as the map and the food starting location
*/
void initGame(void) {
	isGrowing = false;
	memset(map, 0, (X_BOUND_RIGHT+1)*(Y_BOUND_BOTTOM+1));

	appendToHead(11, 17);
	appendToHead(12, 17);
	appendToHead(13, 17);
	appendToHead(14, 17);
	appendToHead(15, 17);
	appendToHead(16, 17);
	appendToHead(17, 17);

	struct snakePart current* = head;
	while (current) {
		drawPixel(current);
		current = current->prev;
	}

	generateFood();
	direction = RIGHT;
}

void appendToHead(int x, int y) {
  struct snakePart* newHead = (struct Segment*)malloc(sizeof(struct Segment));
  newHead->coords = {.x=x,.y=y};

	newHead->prev = head;

  if (head) {
    if (!tail) {
      tail = head;
    }
  }
  head = newHead;
}

void drawPixel(struct point* point) {
	drawPixel(point->x, point->y);
}
void drawPixel(int x, int y) {
	OrbitOledSetDrawMode(modOledSet);
	OrbitOledMoveTo(x, y);
	OrbitOledDrawPixel();
}

/*
 * Create a linked list based off a list of coordinates
 */
void initializeSnake(struct point coords[], int numCoords) {
  for (int i = 0; i < numCoords; i ++) {
    appendToHead(&coords[i]);
  }
}

void setupLeftButton(void) {
  // SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);        // Enable port F
  GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);  // Init PF4 as input
  GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4,
      GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);  // Enable weak pullup resistor for PF4

  // Interrupt setup
  GPIOIntDisable(GPIO_PORTF_BASE, GPIO_PIN_4);        // Disable interrupt for PF4 (in case it was enabled)
  GPIOIntClear(GPIO_PORTF_BASE, GPIO_PIN_4);      // Clear pending interrupts for PF4
  GPIOIntRegister(GPIO_PORTF_BASE, onButtonDown);     // Register our handler function for port F
  GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_4,
      GPIO_FALLING_EDGE);             // Configure PF4 for falling edge trigger
  GPIOIntEnable(GPIO_PORTF_BASE, GPIO_PIN_4);     // Enable interrupt for PF4
  direction = RIGHT;
}
void onButtonDown(void) {
    if (GPIOIntStatus(GPIO_PORTF_BASE, false) & GPIO_PIN_4) {
        // PF4 was interrupt cause
        if (direction!=RIGHT){
					direction = LEFT;
        }
        // active = 1;
        //GPIOIntRegister(GPIO_PORTF_BASE, onButtonUp);   // Register our handler function for port F
        GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_4,
            GPIO_RISING_EDGE);          // Configure PF4 for rising edge trigger
        GPIOIntClear(GPIO_PORTF_BASE, GPIO_PIN_4);  // Clear interrupt flag
    }
}
