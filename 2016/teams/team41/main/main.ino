#include <delay.h>
#include <FillPat.h>
#include <LaunchPad.h>
#include <OrbitBoosterPackDefs.h>
#include <OrbitOled.h>
#include <OrbitOledChar.h>
#include <OrbitOledGrph.h>

void initializeOLED(void);

struct segment *appendToHead(struct segment*, const struct point*);
struct segment *initializeSnake(const struct point[], uint32_t);

void drawSnake(struct segment*);

/*
 * Control constants
 */
const uint32_t NUM_SWITCHES = 2;
const uint32_t SWITCHES[NUM_SWITCHES] = { 
  PA_7, // SW_1
  PA_6 // SW_2
};
const uint32_t NUM_BUTTONS = 3;
const uint32_t BUTTONS[NUM_BUTTONS] = { 
  PD_2, // BTN_1
  PE_0, // BTN_2
  PF_0 // right (sw1)
};
const uint32_t POTENTIOMETER = PE_3;

/*
 * Game control constants
 */
const uint32_t FPS = 60;


struct point {
  uint32_t x;
  uint32_t y;
};

/*
 * Snake components
 */
struct segment {
  struct point coords;
  struct segment *next; // towards head
  struct segment *prev; // towards tail
};

struct part *head;
struct part *tail;

enum direction {
  UP,
  DOWN,
  LEFT,
  RIGHT
} dir;

void setup() {

  Serial.begin(9600);

  initializeOLED();
  for (int i = 0; i < NUM_BUTTONS; i ++) {
    pinMode(BUTTONS[i], INPUT);
  }
  for (int i = 0; i < NUM_SWITCHES; i ++) {
    pinMode(SWITCHES[i], INPUT);
  }

  uint32_t numPoints = 9;
  struct segment *head = initializeSnake((const struct point[]) {
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
  
  drawSnake(head);
  OrbitOledUpdate();
}

void loop() {
/*
if (alive) {

  OrbitOledMoveTo(tail->x, tail->y);
  OrbitOledSetDrawMode(modOledXor);
  OrbitOledDrawPixel();
  Serial.println("erasing");
  Serial.print(tail->x,DEC);
  Serial.print(" ");
  Serial.println(tail->y,DEC);
  
  if (thing == LEFT) {
    tail->x = head->x - 1;
    tail->y = head->y;
  }
  else if (thing == RIGHT) {
    tail->x = head->x + 1;
    tail->y = head->y;
  }
  else if(thing == DOWN) {
    tail->y = head->y + 1;
    tail->x = head->x;
  }
  else if (thing == UP) {
    tail->y = head->y - 1;
    tail->x = head->x;
  }

  Serial.println("moving");
  Serial.print(head->x, DEC);
  Serial.print(" ");
  Serial.print(head->y, DEC);
  Serial.print(" -- ");
  Serial.print(tail->x, DEC);
  Serial.print(" ");
  Serial.println(tail->y, DEC);

  tail->prev = head;
  current = tail;
  tail = tail->next;
  tail->prev = NULL;
  head->next = current;
  head = current;

  Serial.println("swapping");
  Serial.print(head->x, DEC);
  Serial.print(" ");
  Serial.print(head->y, DEC);
  Serial.print(" -- ");
  Serial.print(tail->x, DEC);
  Serial.print(" ");
  Serial.println(tail->y, DEC);

  if (tail == NULL) {

    Serial.println("tail is null");
  }

  OrbitOledMoveTo(head->x, head->y);
  if (OrbitOledGetPixel()) {
    alive = false;
  } else {
    OrbitOledSetDrawMode(modOledSet);
    OrbitOledDrawPixel();
    OrbitOledUpdate();
  }
} else if (!done) {
  done = true;
  OrbitOledClear();
  OrbitOledMoveTo(0, 0);
OrbitOledDrawString("You Died");
  OrbitOledUpdate();
}
  // BTN_1
  if (digitalRead(PD_2)) {
    thing = DOWN;
  }
  else if (digitalRead(PF_0)) {
    thing = UP;
  }
  else if (!digitalRead(PA_6)) {
    thing = LEFT;
  } else {
    thing = RIGHT;
  }*/
  DelayMs(1000/FPS);
}

//void readControls()

/*
 * Create a new node and set it as the new head of the linked list
 * 
 * struct segment* head -- the head of the linked list
 * uint32_t x, y -- the coordinates of the new head
 */
struct segment *appendToHead(struct segment* head, const struct point *coords) {
  struct segment* newHead = (struct segment*)malloc(sizeof(struct segment));
  newHead->coords = *coords;
  
  newHead->next = NULL;
  newHead->prev = head;
  if (head) {
    head->next = newHead;
  }

  return newHead;
}

/*
 * Create a linked list based off an array of coordinates and return the head
 */
struct segment *initializeSnake(const struct point coords[], uint32_t numCoords) {
  struct segment *head = NULL;
  for (int i = 0; i < numCoords; i ++) {
    head = appendToHead(head, &coords[i]);
  }
  return head;
}

void drawSnake(struct segment *head) {
  OrbitOledSetDrawMode(modOledSet);
  
  while (head) {
    OrbitOledMoveTo(head->coords.x, head->coords.y);
    OrbitOledDrawPixel();
    head = head->prev;
  }
}

void initializeOLED(void) {
  OrbitOledInit();
  OrbitOledClear();
  OrbitOledClearBuffer();
  OrbitOledSetFillPattern(OrbitOledGetStdPattern(iptnSolid));
}


