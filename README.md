# Gravity-Controlled-Snake

Group project for Software Engineering 101 @ University of Waterloo
 - this project was imported from SVN, so the December 31, 1969 is some default apparently
 - please excuse my partner's commit messages... they were made after the project was submitted, as we were testing some different boards

The project used an OLED display to display the game, and an accelerometer to both control the snake and navigate through the scrolling menus

The snake is implemented with a linked list, so instead of redrawing the entire screen each time the snake moves, all that needs to be done is to move the tail of the snake in front of the head and re-link those two nodes.

The goal of the game is simple: eat the food and don't crash into yourself or the obstacles!
