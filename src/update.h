#pragma once

/*
----- UPDATE -----

Player and keyboard updates.
*/

enum UpdateReturnCodes {
	UPDATE_EXIT, 		// Exit the game
	UPDATE_CONTINUE,	// Continue as normal
	UPDATE_AGAIN		// Run update again ASAP
};

/* keyboardUpdate
Gets all keyboard inputs and performs the according actions.

Returns UPDATE_EXIT if the game should be quit, UPDATE_CONTINUE if nothing
interesting happened, and UPDATE_AGAIN if keyboardUpdate should be run again
immediately.
*/
enum UpdateReturnCodes keyboardUpdate();

/* playerUpdate
Updates the player one frame, including physics and animations.

frames: How many frames have passed since the game was started.
*/
void playerUpdate(int frames);