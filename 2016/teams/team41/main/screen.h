#ifndef SCREEN_H
#define SCREEN_H

struct Screen {
  void (*init)(void);
  void (*run)(void);
  void (*end)(void);
};

void initScreen(void);
void endScreen(void);
void runScreen(void);
void switchScreen(struct Screen*);

#endif SCREEN_H
