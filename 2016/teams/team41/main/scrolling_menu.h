#ifndef SCREEN_H
#define SCREEN_H

void initScrollingMenu(char *newTitle, char *newOptions[], int numOptions);
void drawScrollingMenu(void);
int getSelectedOptionIndex(void);
int scroll(float pitch, float roll);
#endif SCREEN_H
