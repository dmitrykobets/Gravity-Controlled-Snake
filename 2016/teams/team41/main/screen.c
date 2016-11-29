
struct Screen {
	void (*init)(void);
	void (*run)(void);
	void (*end)(void);
};
struct Screen *curScreen;

void initScreen(void) {
	if (curScreen && curScreen->init)
		curScreen->init();
}
void endScreen(void) {
	if (curScreen && curScreen->end)
		curScreen->end();
	OrbitOledClear();
}
void runScreen(void) {
	if (curScreen && curScreen->run)
		curScreen->run();
}
void switchScreen(struct Screen *newScreen) {
	endScreen();
	initScreen();
	curScreen = newScreen;
}
