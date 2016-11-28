#include "scrolling_menu.h"
#include <OrbitOled.c>

char upArrow[] = {
  0x20, 0x40, 0xFF, 0x40, 0x20  
};
char downArrow[] = {
  0x04, 0x02, 0xFF, 0x02, 0x04  
};

const int INITIAL_OFFSET = 12;
int offset = 12;

char *scrollingMenuTitle = "";

const int MAX_SCROLLING_MENU_OPTIONS	= 5;
int numScrollingMenuOptions = 0;
char *scrollingMenuOptions[5] = {'\0'};

void initScrollingMenu(char *newTitle, char *newOptions[], int numOptions) {
	offset = INITIAL_OFFSET;

  scrollingMenuTitle = newTitle;
  for (int i = 0; i < numOptions; i ++) {
    scrollingMenuOptions[i] = newOptions[i];
  }
  numScrollingMenuOptions = numOptions;
  drawScrollingMenu();
}

void drawScrollingMenu(void) {
    OrbitOledClear();
    OrbitOledSetDrawMode(modOledSet);
    for (int i = 0; i < numScrollingMenuOptions; i ++) {
      OrbitOledMoveTo(10, offset+i*10);
      OrbitOledDrawString(scrollingMenuOptions[i]);
    }
    OrbitOledMoveTo(0, 0);
    OrbitOledFillRect(ccolOledMax-1, 9);
    OrbitOledMoveTo(0, crowOledMax-1-9);
    OrbitOledFillRect(ccolOledMax-1, crowOledMax-1);

    OrbitOledSetDrawMode(modOledXor);
    OrbitOledMoveTo(0,1);
    OrbitOledDrawString(scrollingMenuTitle);

    OrbitOledMoveTo(ccolOledMax-1-6, 1);
    OrbitOledPutBmp(5, 8, upArrow);
    OrbitOledMoveTo(ccolOledMax-1-6, crowOledMax-1-8);
    OrbitOledPutBmp(5, 8, downArrow);
    OrbitOledUpdate();
}

int getSelectedOptionIndex(void) {
	if ((abs(offset-12)+3)%10 <= 6)
		return (abs(offset-12)+3)/10;
	return -1;
}

int scroll(float pitch, float roll) {
  if (fabs(roll) > 7) {
    if (roll > 0) {
      if (offset-12 < 3) {
        offset ++;
        drawScrollingMenu();
      }
    }
    else if (offset-12 > - (numScrollingMenuOptions-1)*10 - 3) {
      offset --;
      drawScrollingMenu();
    }
  }
  return -1;
}

