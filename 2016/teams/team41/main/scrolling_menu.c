#include <math.h>
#include <stdlib.h>
#include <OrbitOled.c>
#include "scrolling_menu.h"

char upArrow[] = {
  0x20, 0x40, 0xFF, 0x40, 0x20  
};
char downArrow[] = {
  0x04, 0x02, 0xFF, 0x02, 0x04  
};

// the offset is the number of pixels by which the options are offset from the top of 
// the screen.
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

		// draw scrolling options
    OrbitOledSetDrawMode(modOledSet);
    for (int i = 0; i < numScrollingMenuOptions; i ++) {
      OrbitOledMoveTo(10, offset+i*10);
      OrbitOledDrawString(scrollingMenuOptions[i]);
    }
		// draw top and bottom rectangles
    OrbitOledMoveTo(0, 0);
    OrbitOledFillRect(ccolOledMax-1, 9);
    OrbitOledMoveTo(0, crowOledMax-1-9);
    OrbitOledFillRect(ccolOledMax-1, crowOledMax-1);

		// draw title
    OrbitOledSetDrawMode(modOledXor);
    OrbitOledMoveTo(0,1);
    OrbitOledDrawString(scrollingMenuTitle);

		// draw arrows on the right
    OrbitOledMoveTo(ccolOledMax-1-6, 1);
    OrbitOledPutBmp(5, 8, upArrow);
    OrbitOledMoveTo(ccolOledMax-1-6, crowOledMax-1-8);
    OrbitOledPutBmp(5, 8, downArrow);

    OrbitOledUpdate();
}

/* Function: getSelectedOptionIndex
 * --------------------------------
 * Returns the index of the option currently exposed between the top and bottom
 * horizontal bars of the scrolling menu.
 *
 * To determine whether an item is within clickable range:
 *  - word height is 7 pixels tall, the spacing between words is 3 spaces. This makes
 *    the "center" of each word 10 pixels apart. The initial offset from the top is 
 *    12 pixels
 *  - for a word to be within range of clicking, the relative offset cannot be more than
 *    3 (in either direction) away from the nearest multiple of 10. The item index is the
 *    multiplicative factor required to obtian that multiple of 10.
 *
 * Parameters: none
 * 
 * Return value: the index (base 0) of the item within range starting from the top of the 
 * list; 
 * Otherwise, -1 if no option is within clickable range.
 */
int getSelectedOptionIndex(void) {
	if ((abs(offset-12)+3)%10 <= 6)
		return (abs(offset-12)+3)/10;
	return -1;
}

/* Function: scroll
 * ----------------
 * Moves the scrolling menu items up or down depending on the current tilt of the 
 * accelerometer.
 *
 * The only value needed to determine the tilt is the "roll" value, which varies when
 * the accelerometer is tilted towards the top or bottom of the scrolling menu.
 *
 * Tilting up results in the menu scrolling up. While tilting down results in the menu
 * scrolling down.
 *
 * Once there are no more items below/above the current item, the list is locked from
 * scrolling in that direction. The numbers in those calculations are explained in the
 * getSelectedOptionIndex() function.
 * 
 * Parameters:
 *  - roll
 *
 * Return value: none
 */
void scroll(float roll) {
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
}

