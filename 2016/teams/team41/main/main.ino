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


struct Input *lastActive;

/*
 * Game control constants
 */

struct point {
  uint32_t x;
  uint32_t y;
};

struct intPoint{
  int x;
  int y;
};

/*
 * Snake components
 */
struct Segment {
  struct point coords;
  struct Segment *next; // towards head
  struct Segment *prev; // towards tail
};

int genRandYesNo=0;
const int upperLimitX=128;
const int lowerLimitX=0;
const int lowerLimitY=0;
const int upperLimitY=30;


struct Segment *head = NULL;
struct Segment *tail = NULL;
struct Segment *current = NULL;

point food={100,15};

uint32_t numPoints =9;
enum direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
} direction;

struct directions{
  enum direction prev;
  enum direction current;
};


void setup() {
  Serial.begin(9600);
  point food={100,15};
  direction = RIGHT;

  initializeOLED();

  for (int i = 0; i < NUM_INPUTS; i ++) {
    pinMode(INPUTS[i].code, INPUT);
  }
  
  initializeSnake((const struct point[]) {
    {11, 11}, 
    {12, 11}, 
    {13, 11}, 
    {14, 11}, 
    {15, 11}, 
    {16, 11}, 
    {17, 11}, 
    {18, 11}, 
    {19, 11}
    }, numPoints); 
  

  drawSnake();
  drawFood();
  OrbitOledUpdate();
}

void loop() {
  lastActive = updateInputs();
  if (lastActive) {
    switch(lastActive->code) {
      case PD_2: //BTN_1
        if (lastActive->active&&direction!=UP)
          direction = DOWN;
        break;
      case PE_0: //BTN_2
        if (lastActive->active&&direction!=DOWN)
          direction = UP;
        break;
      case PF_0: //SW_2
        if (lastActive->active&&direction!=LEFT)
          direction = RIGHT;
       break;
      // case PA_7:
      //  if (lastActive->active)
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

/*
 * Create a new node and set it as the new head of the snake's linked list 
 */



void directionAction(struct Segment *previous, struct Segment *next){
  switch(direction) {
        
      case LEFT:
        next->coords.x = head->coords.x-1;
        tail->coords.y = head->coords.y;
        
        //Serial.println("moving LEFT");  
        break;
      case RIGHT:
        tail->coords.x = head->coords.x+1;
        tail->coords.y = head->coords.y;
        
        //Serial.println("moving RIGHT"); 
        break;
      case UP:
        tail->coords.y = head->coords.y-1;
        tail->coords.x = head->coords.x;
        
        //Serial.println("moving UP");  
        break;
      case DOWN:
        tail->coords.y = head->coords.y+1;
        tail->coords.x = head->coords.x;
        
        //Serial.println("moving DOWN");  
        break;
    }




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

void moveSnake() {

  eraseTail();
        current = head->prev->prev;

  while (current) {
      if (current->coords.x ==head->coords.x &&current->coords.y ==head->coords.y){
        OrbitOledClear();
          OrbitOledSetCursor(0,0);


          OrbitOledPutString("You Died");
          
          OrbitOledUpdate();
          delay(3000);
          OrbitOledClear();
          head=NULL;
          tail = NULL;
          current = NULL;
          food = {100, 15};
          genRandYesNo=0;
          numPoints = 9;
          direction = RIGHT;
          initializeOLED();

          initializeSnake((const struct point[]) {
      {11, 11}, 
      {12, 11}, 
      {13, 11}, 
      {14, 11}, 
      {15, 11}, 
      {16, 11}, 
      {17, 11}, 
      {18, 11}, 
      {19, 11}
      }, numPoints); 
    

    drawSnake();
    drawFood();
    OrbitOledUpdate();
      } 
      if (current->coords.x == food.x&& current->coords.y==food.y){
        OrbitOledSetDrawMode(modOledSet); 
        OrbitOledMoveTo(current->coords.x, current->coords.y);
        OrbitOledDrawPixel();
      }
      current = current->prev;
    
  }
  
  // make the tail the new head of the snake

  if (head->coords.x>upperLimitX||head->coords.y>upperLimitY){
          OrbitOledClear();
          OrbitOledSetCursor(0,0);
          output = (char*)malloc(strlen(output)+1);
          sprintf(output, "%d", numPoints-9);
          OrbitOledPutString(output);
          
          OrbitOledUpdate();
          delay(3000);
          OrbitOledClear();
          head=NULL;
          tail = NULL;
          current = NULL;
          food = {100, 15};
          genRandYesNo=0;
          numPoints = 9;
          direction = RIGHT;
          initializeOLED();

          initializeSnake((const struct point[]) {
      {11, 11}, 
      {12, 11}, 
      {13, 11}, 
      {14, 11}, 
      {15, 11}, 
      {16, 11}, 
      {17, 11}, 
      {18, 11}, 
      {19, 11}
      }, numPoints); 
    

    drawSnake();
    drawFood();
    OrbitOledUpdate();


    }
    else
    {

    switch(direction) {
      
      case LEFT:
        tail->coords.x = head->coords.x-1;
        tail->coords.y = head->coords.y;
        
        //Serial.println("moving LEFT");  
        break;
      case RIGHT:
        tail->coords.x = head->coords.x+1;
        tail->coords.y = head->coords.y;
        
        //Serial.println("moving RIGHT"); 
        break;
      case UP:
        tail->coords.y = head->coords.y-1;
        tail->coords.x = head->coords.x;
        
        //Serial.println("moving UP");  
        break;
      case DOWN:
        tail->coords.y = head->coords.y+1;
        tail->coords.x = head->coords.x;
        
        //Serial.println("moving DOWN");  
        break;
    }
      if (tail->coords.y==food.y &&tail->coords.x==food.x){
        genRandYesNo=1;
        
        current = tail;

        switch(direction) {
      
      case LEFT:
        current->coords.x = tail->coords.x-1;
        current->coords.y = tail->coords.y;
        
        //Serial.println("moving LEFT");  
        break;
      case RIGHT:
        current->coords.x = tail->coords.x+1;
        current->coords.y = tail->coords.y;
        
        //Serial.println("moving RIGHT"); 
        break;
      case UP:
        current->coords.y = tail->coords.y-1;
        current->coords.x = tail->coords.x;
        
        //Serial.println("moving UP");  
        break;
      case DOWN:
        current->coords.y = tail->coords.y+1;
        current->coords.x = tail->coords.x;
        
        //Serial.println("moving DOWN");  
        break;
    }
          appendToHead(&(tail->coords));
          numPoints++;
      }
    

      




  }

  current = tail;
  tail->prev = head;
  current = tail;
  tail = tail->next;
  tail->prev = NULL;
  head->next = current;
  head = current;

  drawHead();
}

void drawSnake(void) {
  OrbitOledSetDrawMode(modOledSet);
  current = head;
  int counter = 0;
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
void eraseFood(void){
  OrbitOledSetDrawMode(modOledXor); 
  OrbitOledMoveTo(food.x, food.y);
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
 * Returns a pointer to the input which has just had its active state flipped to "true"
 *  - if more than one input meets the above criteria, return a pointer to the one that is furthest along from the start of the INPUTS array
 *  - if no input meets the above criteria, return a NULL pointer
 */
struct Input *updateInputs(void) {
  struct Input* lastActive = NULL;
  int active = 0;
  for (int i = 0; i < NUM_INPUTS; i ++) {
    active = digitalRead(INPUTS[i].code);
    if (active != INPUTS[i].active) {
      lastActive = &INPUTS[i];
      INPUTS[i].active = active;
    }
  }
  return lastActive;
}

int randNum(int n){
  int a;
  a = rand()%n;
  
  return a;

 }


void moveFood(void) {

  eraseFood();
  if (genRandYesNo){
  current = head;
  food.x = randNum(128);
    food.y = randNum(32);
    
    while (current) {
      if (current->coords.x ==food.x &&current->coords.y ==food.y){
        food.x = randNum(128);
        food.y = randNum(32);
        current =head;
      }
      else{
        current = current->prev;
      }
      

    }
    
    

    genRandYesNo=0;
  }
  

  drawFood();
}

void drawFood(){
  OrbitOledSetDrawMode(modOledSet); 
  OrbitOledMoveTo(food.x, food.y);
  OrbitOledDrawPixel();
}
