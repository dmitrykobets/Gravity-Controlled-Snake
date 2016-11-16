#include <delay.h>
#include <FillPat.h>
#include <LaunchPad.h>
#include <OrbitBoosterPackDefs.h>
#include <OrbitOled.h>
#include <OrbitOledChar.h>
#include <OrbitOledGrph.h>

void initializeOLED(void);

struct Input *updateInputs(void); 

void appendToHead(const struct point*);
void initializeSnake(const struct point[], uint32_t);

void drawSnake(void);
void eraseTail(void);
void drawHead(void);

/*
 * Input
 */
struct Input {
  const uint32_t code;
  boolean active; // for switches, this corresponds to the "up" position. For buttons, this corresponds to the "pressed" position
};

const int NUM_INPUTS = 5;
static Input INPUTS[NUM_INPUTS] = {
  {PA_7, false}, // SW_1
  {PA_6, false}, // SW_2
  {PD_2, false}, // BTN_1
  {PE_0, false}, // BTN_2
  {PF_0, false} // right (sw1)
};
struct Input *lastActive;

/*
 * Game control constants
 */
const uint32_t FPS = 10;

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

struct Foodm{

  intPoint coords;
  

};

Foodm food;

//food.coords.x = 15;
//food.coords.y = 15;
struct Segment *head = NULL;
struct Segment *tail = NULL;
struct Segment *current = NULL;
uint32_t numPoints =9;
enum direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
} direction;

void setup() {
  Serial.begin(9600);

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
    {18, 10}
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
        if (lastActive->active)
          direction = DOWN;
        break;
      case PE_0: //BTN_2
        if (lastActive->active)
          direction = UP;
        break;
      case PA_6: //SW_2
        if (lastActive->active)
          direction = RIGHT;
        else
          direction = LEFT;
        break;
    }
  }
  moveFood();
  moveSnake();
  OrbitOledUpdate();

  DelayMs(1000/FPS);
}

/*
 * Create a new node and set it as the new head of the snake's linked list 
 */
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
      Serial.println("moving DOWN");  
      break;
  }
  
  // make the tail the new head of the snake
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
  Serial.print(a);
  return a;

}


void moveFood(void) {
  struct Segment *g = head;
  int a[2][9];
  int randx;
  
  if (head->coords.y==food.coords.y &&head->coords.x==food.coords.x){

    for (int i=0; i<9; i++){  

      a[0][i]=g->coords.x;
      a[1][i]=g->coords.y;
      g=g->prev;


    }

    food.coords.x = randNum(128);
    food.coords.y = randNum(30);


  }
  

  drawFood();
}

void drawFood(){
  OrbitOledSetDrawMode(modOledSet); 
  OrbitOledMoveTo(food.coords.x, food.coords.y);
  OrbitOledDrawPixel();
}


