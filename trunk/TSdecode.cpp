/*
TSdecode.cpp
Decode MPEG-2 transport streams
Peter Daniel, Final year project 2000/2001
*/

#include "setup.h" // prototype functions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>

	char version[] = "1.00";    			// current version number
	unsigned int current_header_add = 0;	// add of header being decoded/displayed
	int index = 0;							// index for found PID table
	unsigned int search_PID = 0;			// PID value to search for, from command line
	long int payload_start = 0;				// add of payload start when found
	long ip_file_length = 0;				// how long?
	char *ip_filepath;						// from command line
	int quite_mode = 0;						// display progress when extracting? yes/no
	int sync_bytes[10];						// array of where the sync_bytes are found
	int end_of_file = 0;					// check for end of file
	unsigned char TS_raw_header[4];			// 4 byte space to read data from file
	unsigned char adaption_field_length;

	struct found_packet
	{
		struct found_packet *next_ptr; // linked list, pointer to address of next list
		int index;
		unsigned int header_address;
		unsigned int payload_address;
		unsigned char payload_start_indicator;
		unsigned int PID;
		unsigned char continuity_counter;
	} ;

	struct found_packet *new_item_ptr = NULL; // pointer for new list
	struct found_packet *first_ptr = NULL;    // pointer for first list
	struct found_packet *last_ptr = NULL;     // pointer for last list

	struct TS_header
	{
		unsigned char sync_byte;
		unsigned char transport_error_indicator;
		unsigned char payload_start_indicator;
		unsigned char transport_priority;
		unsigned int PID;
		unsigned char transport_scrambling_control;
		unsigned char adaption_field_control;
		unsigned char continuity_counter;
	} TS_header;

	FILE *ip_file;
	FILE *op_pid_table_file;
	FILE *op_pes_file;

long get_ip_file_length(char *ip_filepath)
{
//  find length of file

	int ip_file2 = 0;
	ip_file2 = _open( ip_filepath, _O_RDONLY );
	if( ip_file2 == -1 )
	{
		exit_prog("Error: cannot open input file");
	}
	ip_file_length = _filelength(ip_file2);
	_close( ip_file2 );
	return ip_file_length;
}

void file_seek(int offset)
{
// seek forward offset bytes, checking for end of file
// needed because fseek can look beyond the end of the file without reporting an error.
	if ( ftell(ip_file) + (offset + 200) > ip_file_length )
	{
		end_of_file = 1;
//		press_key("End of file");
	}
	else
	fseek(ip_file, offset, 1);
}


int main(int argc, char *argv[])
{

	puts  ("MPEG-2 decoder : TSdecode.exe");
	printf("Peter Daniel   Version : %s \n", version);
	puts  ("=============================\n");

	if (argc == 1)
	{ 	display_usage();
		exit_prog(""); }

	while ((argc > 1) && (argv[1][0] == '-')) {
		switch (argv[1][1]) {
		case 'i':
			ip_filepath = &argv[1][2];
			break;
		case 'p':
			search_PID = atoi(&argv[1][2]);
			break;
		case 'q':
			quite_mode = 1;
			break;
		default:	
			printf("Bad option %s\n", argv[1]);
			display_usage();
			exit_prog("");
		}
		++argv;
		--argc;
	}


	ip_file_length = get_ip_file_length(ip_filepath);
	printf("Opening MPEG-2 file %s, %d bytes.\n", ip_filepath, ip_file_length);
	printf("Search PID : %d", search_PID);
	ip_file = fopen(ip_filepath, "rb");

	char menu_choice = 0;

	find_TS_sync_byte();	// synchronise to sync_bytes
//	display_sync_byte_table();		// display location of sync_bytes
    puts("\n5 occurances of Transport Stream sync_byte 0x47 found ok.\n");
	fseek(ip_file, sync_bytes[1], 0);	// go back to first sync_byte
	menu_choice = menu();	// display menu options
	switch (menu_choice)	// decode menu choice
	{
	case '1' : // TS header decode
	menu_option1();
	break;
	case '2' : // PID address table
	menu_option2();
	break;
	case '3' : // Save PID address table
	menu_option3();
	break;
	case '4' : // Extract PES
	menu_option4();
	break;
	case '5' : // exit
	exit_prog("");
	break;
	default:
	puts("\nERROR : Not a valid menu entry.");
	}
	exit_prog("");
	return 0;
}




void TS_header_decode()
{
//  printf("\nSize of TS-header: %dbytes", sizeof(TS_raw_header));
	current_header_add = ftell(ip_file);
	fread(&TS_raw_header, sizeof(TS_raw_header), 1, ip_file); // read the header
//	for (count=0; count <=sizeof(TS_raw_header); count++)
//		printf("\nByte %d: \t \t x%x",count, TS_raw_header[count]);
	// use bit masking and shifting to get the data we want from header
	TS_header.sync_byte = TS_raw_header[0];
	TS_header.transport_error_indicator = (TS_raw_header[1] & 0x80) >> 7;
	TS_header.payload_start_indicator = (TS_raw_header[1] & 0x40) >> 6;
	TS_header.transport_priority = (TS_raw_header[1] & 0x20) >> 5;
	TS_header.PID = ((TS_raw_header[1] & 31) << 8) | TS_raw_header[2];
	TS_header.transport_scrambling_control = (TS_raw_header[3] & 0xC0);
	TS_header.adaption_field_control = (TS_raw_header[3] & 0x30) >> 4;
	TS_header.continuity_counter = (TS_raw_header[3] & 0xF);
}

void add_found_packet(void)
{	// save found packets in an linked list
	struct found_packet *new_item_ptr = NULL;
	// create a new structure if there is enough memory
	if (( new_item_ptr = (struct found_packet* ) malloc(sizeof(struct found_packet))) == NULL)
	{
		exit_prog("FATAL ERROR : out of memory!");
	}
	new_item_ptr->index = index;
	new_item_ptr->header_address = current_header_add;
	new_item_ptr->payload_address = current_header_add + 4;
	new_item_ptr->PID = TS_header.PID;
	new_item_ptr->continuity_counter = TS_header.continuity_counter;
	new_item_ptr->payload_start_indicator = TS_header.payload_start_indicator;

	new_item_ptr->next_ptr	= NULL;
	if ( first_ptr == NULL ) first_ptr = new_item_ptr;
	else last_ptr->next_ptr = new_item_ptr;
	last_ptr = new_item_ptr;
	index ++;
}

void find_TS_sync_byte(void)
{	// scan file looking for sync_bytes
	int sync_data = 0; // storage for data read from file
	int occurance = 0; // keep track of how many sync_bytes have been found
	long int current_file_pos = 0; // where are we?
	
	while ( !feof(ip_file) && occurance < 6)
	{
		current_file_pos = ftell(ip_file);
		sync_data = 0;
		fread(&sync_data, 1, 1, ip_file);
		switch (sync_data)
		{	// if sync_byte is found then remember it in sync_bytes[]
		case 0x47 :
			sync_bytes[occurance] = current_file_pos;
			occurance++;
			file_seek(187); // look for next one
			break;

		default : // if byte is not equal to 0x47
			occurance = 0;
		}
	}
	if ( occurance < 6 )
	{	// report error if 5 cannot be found
		exit_prog("Error: End of file reached before 5 sync_bytes found.");
	}
}

void display_sync_byte_table()
{	// display information about sync_bytes
	int count;
	printf("\nTransport packet sync bytes found:");
	printf("\nOccurance \t Address");
	for (count=0; count < 6; count++)
		printf("\n %d \t\t x%x", count, sync_bytes[count]);
}

void save_pid_table(void)
{
	struct found_packet *current_ptr = NULL;
	current_ptr = first_ptr;
		while ( current_ptr != NULL )
		{
			fprintf(op_pid_table_file, "d%d \t x%x \t x%x \t d%d \t d%d \t d%d\n",
							current_ptr->index,
							current_ptr->header_address,
							current_ptr->payload_address,
							current_ptr->PID,
							current_ptr->continuity_counter,
							current_ptr->payload_start_indicator);
			current_ptr = current_ptr->next_ptr;
		}
	fclose(op_pid_table_file);
}

void display_pid_table(void)
{	// display list of found PID's and addresses etc

		puts("\n\nIndex \t Payload Add \t Header Add \t PID \t Cont \t Payload");
		puts("================================================================\n");

		struct found_packet *current_ptr = NULL;

		current_ptr = first_ptr;
			while ( current_ptr != NULL )
			{
				printf("d%d \t x%x \t x%x \t d%d \t d%d \t d%d\n",
							current_ptr->index,
							current_ptr->header_address,
							current_ptr->payload_address,
							current_ptr->PID,
							current_ptr->continuity_counter,
							current_ptr->payload_start_indicator);
				current_ptr = current_ptr->next_ptr;
			}
}


void display_transport_header(void)
{	// display the TS header and add extra info
		printf("\nHeader address:   \t x%x \t d%d",current_header_add, current_header_add);
		printf("\nSync byte:     \t \t x%x",TS_header.sync_byte);
		printf("\t should be x47");
		printf("\nError indicator:  \t d%d",TS_header.transport_error_indicator);
		printf("\nPayload start: \t \t d%d",TS_header.payload_start_indicator);
		if (TS_header.payload_start_indicator == 0x1)
			printf("\t Payload is start of PID %d", TS_header.PID);
		printf("\nTans priority: \t \t d%d",TS_header.transport_priority);
		printf("\nPID:           \t \t x%x \t d%d",TS_header.PID, TS_header.PID);
		printf("\nScrambling:    \t \t d%d",TS_header.transport_scrambling_control);
		if (TS_header.transport_scrambling_control == 0x0)
			printf("\t not scrambled");
		printf("\nAdaption:      \t \t d%d",TS_header.adaption_field_control);
		if (TS_header.adaption_field_control == 0x01)
		{	printf("\t no adaption field, payload only"); }
		if (TS_header.adaption_field_control == 0x02)
		{	printf("\t adaption field only, no payload");
			adaption_field(); }
		if (TS_header.adaption_field_control == 0x03)
		{	printf("\t adaption field followed by payload");
			adaption_field(); }
		printf("\nContinuity:    \t \t d%d",TS_header.continuity_counter);
}

void adaption_field(void)
{	// find length of the adaption field

	fread(&adaption_field_length, sizeof(adaption_field_length), 1, ip_file);
	printf("\nAdaption length: \t d%d \t bytes", adaption_field_length);
	//  return file pointer
	fseek (ip_file, -1, 1);
}


void menu_option1()
{
while ( end_of_file == 0 )
{
	TS_header_decode();
	if (TS_header.PID == search_PID)
	{
		display_transport_header();
		press_key("\nPress a key for next header or Control-C to exit.\n");
	}
	file_seek(184);
}
	puts("ERROR : End of file reached");
}

void menu_option2(void)
{
	printf("Looking for payload start of PID %d...", search_PID);
	int payload_start_found = 0;

while ( end_of_file==0 )
{
	TS_header_decode();
	if (TS_header.PID == search_PID)
	{
		if ( payload_start_found == 0  && TS_header.payload_start_indicator == 1)
		{
			payload_start = ftell(ip_file);
			printf("\nPayload start found, header add: x%x", current_header_add);
			payload_start_found = 1;
		}

		if ( payload_start_found == 1 )
		{
		add_found_packet();
//		display_transport_header();
		display_pid_table();
		press_key("\nPress a key for next PID or Control-C to exit.\n");
		}
	}

	file_seek(184);
}
	puts("\nERROR : End of file reached");
}

void menu_option3()
{
	printf("Looking for payload start of PID %d...", search_PID);
	int payload_start_found = 0;
	char op_pid_table_filepath[1024];

while ( end_of_file == 0 )
{
	TS_header_decode();
	if (TS_header.PID == search_PID)
	{
		if ( payload_start_found == 0 && TS_header.payload_start_indicator == 1)
		{
			payload_start = ftell(ip_file);
			printf("\nPayload start found at: %x", current_header_add);
			printf("\nScanning the entire file... (may take some time)");
			payload_start_found = 1;
		}
		if ( payload_start_found == 1 )
		{
			add_found_packet();
		}
	}
	file_seek(184);
}
	display_pid_table();
	puts("Enter a filepath to save this table:");
	scanf("%s", op_pid_table_filepath);
	if (( op_pid_table_file = fopen(op_pid_table_filepath, "w")) == NULL)
	{
		exit_prog("ERROR : File cannot be created");
	}
	fputs("Table showing PID address", op_pid_table_file);
	fputs("\nCreated by TSdecode from ", op_pid_table_file);
	fputs(ip_filepath, op_pid_table_file);
	fputs("\n\nIndex \t Payload Add \t Header Add \t PID \t Cont \t Payload", op_pid_table_file);
	fputs("\n================================================================\n", op_pid_table_file);
save_pid_table();
printf("This table has been saved as %s.\n", op_pid_table_filepath);
}

void menu_option4()
{
	int count = 0;
	int payload_add = 0;
	int next_packet = 0;
	int payload_start_found = 0;
	char op_pes_filepath[1024];

	printf("Enter a filepath to extract the PES of PID %d to:\n", search_PID);
	scanf("%s", op_pes_filepath);
	if (( op_pes_file = fopen(op_pes_filepath, "wb")) == NULL)
	{
		exit_prog("ERROR : File cannot be created");
	}

	printf("\nLooking for payload start of PID %d...", search_PID);

while ( end_of_file == 0 )
{
	int data = 0; // temp space used when copying between files
	int loop = 0; // loop counter for copying payload (184 bytes)

	// decode the header
	TS_header_decode();

	if (TS_header.PID == search_PID)
	{   // loop until payload start is found
		if ( payload_start_found == 0 && TS_header.payload_start_indicator == 1)
		{
				payload_start = ftell(ip_file);
				printf("\nPayload start found at: x%x", current_header_add);
				printf("\nProcessing...");
				payload_start_found = 1;
//				store_found_packet();

		}

		if ( payload_start_found == 1 )
		{ // ie, payload start has been found

		add_found_packet();   // record data about TS header in array found_packets[]
		next_packet = last_ptr->continuity_counter;

		//		check continuity
//		printf("\nCurrent packet continuity: %d \nTS header continuity: %d ",next_packet, TS_header.continuity_counter);
//		press_key("");
		if (TS_header.continuity_counter != next_packet)
		{	printf("\nERROR : The continuity count is not correct. \nContinuity count from stream: %d \nExpected: %d ", TS_header.continuity_counter, next_packet);
			press_key("\nPress a key to continue or Control C to exit");
		}

//		copy payload to new file
			payload_add = ftell(ip_file);
			if (quite_mode == 0)
			printf("\nSaving payload, start address: x%x \t continuity: d%d", payload_add, TS_header.continuity_counter);
//			press_key("... press key");
		for (loop = 0; loop < 184; loop ++)
		{ // loop for 184 bytes of payload and copy to new file
			fread(&data, 1, 1, ip_file);
			fwrite(&data, 1, 1, op_pes_file);
		} // end of loop
		fseek(ip_file, -184, 1); // move back because next header will probably have a different PID


		next_packet = ++ next_packet %16;

		}
	}
	file_seek(184); // next header
//	printf("Current location: x%x", ftell(ip_file));
//	printf("payload_add: x%x", payload_add);
//	press_key("");
}
	printf("\nPES stream with PID %d extracted from %s and saved as %s.",search_PID, ip_filepath, op_pes_filepath);
	printf("\nEnd of source file, %s.\n", ip_filepath);
	press_key("Press a key...");
	fclose(op_pes_file);
}
