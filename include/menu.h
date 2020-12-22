#pragma once

/*
----- MENU -----

The miscellaneous menus.
*/

/* mainMenu
The main menu.

Returns -1 if the player wants to exit, 0 if they want to make a new game, 1 if
they want to load a game, and 2 if they want to view the About menu.
*/
int mainMenu();

/* RAMErrorMenu
A menu to display if the world RAM region test failed.
*/
void RAMErrorMenu();

/* loadFailMenu
A menu to display if the loading of a save file has failed.
*/
void loadFailMenu();

/* saveFailMenu
A menu to display if the saving of a file has failed, mainly due to running
out of storage space.
*/
void saveFailMenu();

/* memoryErrorMenu
A menu to display if a malloc() has failed, which is generally unrecoverable.
*/
void memoryErrorMenu();

/* aboutMenu
The About sub-menu.
*/
void aboutMenu();

/* debugMenu
A useful menu to display various tilesheets and sprites.
*/
void debugMenu();

/* exitMenu
An exit dialog box.

Returns true if the player wishes to exit, false otherwise.
*/
bool exitMenu();

/* lowSpaceMenu
A menu to inform the player that they should optimise their storage.

mediaFree: The.amount of bytes free in the storage memory.
*/
void lowSpaceMenu(int mediaFree);

/* saveVersionDifferenceMenu
A menu to inform the player that the version of their save is different from
the version of the game, and may not load.

saveVersion: Version string of the save.
*/
void saveVersionDifferenceMenu(char *saveVersion);

/* incompatibleMenu
A menu to inform the player that they cannot use this version of the addin.
*/
void incompatibleMenu();

/* itemMenu
A debug menu.
*/
void itemMenu();