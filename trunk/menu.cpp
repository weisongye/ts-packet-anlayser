/*
TSdecode - menu.cpp
Decode MPEG-2 transport streams
Peter Daniel, Final year project 2000/2001
*/


#include <stdio.h>

unsigned int menu(void)
{	// put menu on screen
	char menu_choice;
	puts("Menu:");

	puts("1 - TS header decode");
	puts("2 - Display table of PID addresses");
	puts("3 - Create text file of PID addresses");
	puts("4 - Extract a PES stream to another file");
	puts("5 - Exit");
	// ask for input
	puts("\nEnter choice:");
	scanf("%s", &menu_choice);
	// return the input
	return menu_choice;
}