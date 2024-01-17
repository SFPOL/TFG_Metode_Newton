#include "Game.h"

int main()
{
	const char* a = "Point Cloud Visualizer 2.0";
	//Game game(a, 1920, 1080,4,5, false);
	Game game(a, 1080, 720,4,5, true);
	
	//MAIN LOOP
	while (!game.getWindowShouldClose())
	{
		//UPDATE INPUT --
		game.update();
		game.render();
	}

	return 0;
}