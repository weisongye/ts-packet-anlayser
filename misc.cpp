/*
TSdecode - misc.cpp
Decode MPEG-2 transport streams
Peter Daniel, Final year project 2000/2001
*/

#include <stdio.h>
#include <conio.h>
#include <stdlib.h>

void press_key(char press_key_text[])
{	// wait for a key press
	printf("%s", press_key_text);
	while ( !_kbhit() )
	{;}
	_getch();
}

void display_usage(void)
{
	puts("Usage:\t TSDECODE -i<input filepath> of TS file to decode");
	puts("\t \t  -p<search PID>");
	puts("\t \t  -q do not show progress when extracting");

	puts("\nExample: TSDECODE -ic:\\test.mpg -p600");
	puts("\t will open file c:\\test.mpg and process PID 600 packets\n");

}

void exit_prog(char error_msg[])
{
	printf("\n%s\n", error_msg);
	_fcloseall( );
	press_key("Press a key to exit...");
	exit(0);
}