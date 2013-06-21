/*
TSdecode - setup.h
Decode MPEG-2 transport streams
Peter Daniel, Final year project 2000/2001
*/

// Prototype functions

//	TSdecode.cpp
	void display_sync_byte_table(void);
	void find_TS_sync_byte(void);
	void TS_header_decode(void);
	void display_transport_header(void);
	void display_pid_table(void);
	void add_found_packet(void);
	unsigned int menu(void);
	void adaption_field(void);
	void file_seek(int offset);
	void save_pid_table(void);
	void menu_option1(void);
	void menu_option2(void);
	void menu_option3(void);
	void menu_option4(void);
	long get_ip_file_length(char ip_filepath[]);

//	menu.cpp
	unsigned int menu(void);

//	misc.cpp
	void press_key(char press_key_text[]);
	void display_usage(void);
	void exit_prog(char error_msg[]);
