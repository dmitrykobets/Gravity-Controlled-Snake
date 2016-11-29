#ifndef SCREEN_H
#define SCREEN_H

struct Screen;

void initScreen(void);
void endScreen(void);
void runScreen(void);
void switchScreen(struct Screen*);

#endif SCREEN_H
