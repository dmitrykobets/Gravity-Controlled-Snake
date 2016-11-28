#ifndef SCREEN_H
#define SCREEN_H

struct Screen;

void initScreen(void);
void endScreen(void);
void runScreen(void);
void switchScreen(struct Screen *screen);

#endif SCREEN_H
