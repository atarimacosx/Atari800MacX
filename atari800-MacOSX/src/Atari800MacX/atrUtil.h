
void ADos2Host( char* host, unsigned char* aDos );
void Host2ADos( char* host, unsigned char* aDos );
void NameFilter( char* host, char* atari );
void AtariLFToHost(unsigned char *buffer, int len);
void HostLFToAtari(unsigned char *buffer, int len);


typedef struct ADosFileEntry {
    UBYTE aname[11];
    UWORD sectors;
    ULONG bytes;
    UBYTE flags;
	UBYTE day;
	UBYTE month;
	UBYTE year;
	UBYTE hour;
	UBYTE minute;
	UBYTE second;
	UBYTE createdDay;
	UBYTE createdMonth;
	UBYTE createdYear;
} ADosFileEntry;


