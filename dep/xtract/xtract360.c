#include <stdlib.h>
#include <string.h>
#include <direct.h>

#define MAX_CALLS 200

extern "C" int __cdecl _fseeki64(FILE *, __int64, int);

// 20 sublevels max * 20 sectors each
char buffer[20*2048*20];
char dirprefix[200] = {0};
char currdir[200] = {0};
char newdir[200] = {0};
bool doWildcard = false;
char strWildcard[100] = {0};
unsigned int totalbytes = 0;
int level = 0;
char ripdirname[_MAX_PATH] = {0};
char rootdirname[_MAX_PATH] = {0};

int readBlock(FILE *fp, __int64 LBA, unsigned char* data, unsigned short num);
int makeint(char in);
void printentry(char *entry, __int64 offset, char* dirprefix);
void printprogress(int current, int last, int total);
void parse( char *entry , __int64 offset, int level, FILE* fp, bool rip, char* dirprefix);
void extract( char *entry, __int64 offset, FILE* fp);
void extractFile( char *entry, __int64 offset, FILE* fp, char* dirprefix);
int match(const char *mask, const char *name);
bool fnameMatches( char *entry, __int64 offset);
void multicreate(char *dirname);
unsigned long getint(char*);
unsigned short getword(char*);

int main(int argc, char* argv[])
{
    FILE *fp;
    unsigned long rootsector;
    unsigned long rootsize;

    printf("***********************************\n");
    printf("** PI XBOX-360 DUMP EXTRACT V1.0 **\n");
    printf("***********************************\n\n");

    if ( argc < 2 )
    {
        printf("Usage: extract360.exe gamename.360 <wildcard>...\n\n");
        return 1;
    }

    fp = fopen(argv[1], "rb");
    if (fp==0) 
    {
        printf(" * Error, could not open the file %s\n", argv[1]);
        return 1;
    }

    if ( argc == 3 )
    {
        // wildcard
        doWildcard = true;

        // copy wildcard string
        strcpy(strWildcard, argv[2]);

        printf(" Doing wildcard extraction, wildcard is: \"%s\"\n\n", strWildcard);
    }

    // now we are ready to read.. first let's see if this is an xbox360 dvd by reading sector 32
    readBlock(fp, 32, (unsigned char*) buffer, 1);
    if (strncmp(buffer, "MICROSOFT*XBOX*MEDIA", 0x14) != 0)
    {
        printf(" * Error, This is not an XBOX-360 DVD image! Exiting...\n");
        return 1;
    }

    printf("Detecting... relative ROOT-Sector = ");
    rootsector = getint(buffer+0x14);
    rootsize = getint(buffer+0x18);
    printf("%d (0x%x), length is %d (0x%x)\n", rootsector,rootsector, rootsize, rootsize);

    // now we have the rootsector address.. we should read it now
    readBlock(fp, rootsector, (unsigned char*)buffer, 20);

    // print TOC
    printf("\n");
    printf("sector        length   filename\n");
    printf("---------------------------------------------------------------------------\n");

    parse(buffer, 0, 0, fp, false, dirprefix);
    printf("\nTotal bytes used on this XBOX dvd: %u\n", totalbytes);

    printf("\n\nNow extracting files\n");
    printf("====================\n\n");

    // extract all files
    int dirpostfix = 0;
    bool done = false;

    while (!done)
    {
        sprintf(ripdirname, "PI_XBOX360_%u", dirpostfix);
        if (_mkdir(ripdirname) == 0)
            done = true;
        else
        {
            dirpostfix++;
            memset(ripdirname, 0, 20);
        }
    }
    
    _getcwd(rootdirname, _MAX_PATH);

    // now that we found a new dirname, change to it
    _chdir(rootdirname);
    _chdir(ripdirname);

    currdir[0] = '/';

    // finally we can rip all files...
    parse(buffer, 0, 0, fp, true, dirprefix);

    printf("\nAll done! Enjoy ;-)\n");

    return 0;
}

int match(const char *mask, const char *name)
{
	int calls=0, wild=0, q=0;
	const char *m=mask, *n=name, *ma=mask, *na=name;

	for(;;) 
	{
		if (++calls > MAX_CALLS) 
			return 1;
			if (*m == '*') 
			{
				while (*m == '*') ++m;
					wild = 1;
					ma = m;
					na = n;
			}

		if (!*m) 
		{
			if (!*n) return 0;

			for (--m; (m > mask) && (*m == '?'); --m) ;

			if ((*m == '*') && (m > mask) && (m[-1] != '\\')) 
				return 0;
			if (!wild) 
				return 1;
			m = ma;
		} 
		else if (!*n) 
		{
			while(*m == '*') ++m;
			return (*m != 0);
		}
		if ((*m == '\\') && ((m[1] == '*') || (m[1] == '?'))) 
		{
			++m;
			q = 1;
		} 
		else 
		{
			q = 0;
		}

		if ((tolower(*m) != tolower(*n)) && ((*m != '?') || q)) 
		{
			if (!wild) 
				return 1;
			m = ma;
			n = ++na;
		} 
		else 
		{
			if (*m) ++m;
			if (*n) ++n;
		}
	}
}

void multicreate(char dirname[])
{
    if (dirname[0] == 0)
        return;

    char pir[100] = {0};
    char backup[100] = {0};

    int currpos = 0;
    char* pos = strchr(dirname, '/');

    memset(pir, 0, 100);
    strncpy(pir, dirname+currpos, pos-dirname);
    _mkdir(pir);
    _chdir(pir);

    if (strchr(pos+1, '/') == 0)
        return;

    currpos = pos-dirname+1;
    while(pos != 0)
	{
		dirname = pos+1;
        pos = strchr(dirname, '/');
        memset(pir, 0, 100);
        if (pos == 0)
            strcpy(pir, dirname);
        else
            strncpy(pir, dirname, pos-dirname);

        _mkdir(pir);
        _chdir(pir);
    }
        
}

void parse( char *entry, __int64 offset, int level, FILE* fp, bool rip, char* dirprefix)
{
    // THERE SEEM TO BE GAMES THAT HAVE INVALID TOC ENTRIES IN THEM!!
	unsigned long sector = getint(entry+offset+4);
    if (sector == 0xFFFFFFFF)
    {
        if (rip)
        {
            _chdir(rootdirname);
            _chdir(ripdirname);
            multicreate(dirprefix);
        }
        // invalid shit!!
        printf("]>> INVALID TOCENTRY DETECTED! <<[\n");
        return;
    }

    unsigned short left = getword(entry+offset);
    unsigned short right = getword(entry+offset+2);

    if (left)
        parse(entry, left*4, level, fp, rip, dirprefix);

    if (*(entry+offset+0x0c) == 0x10 || *(entry+offset+0x0c) == 0x30 /* 0x30? yeah, 0x30! */)
    {
        // directory
        level++;
        unsigned long tocsector = getint(entry+offset+4);

        char dirname[20];
        int dirnamesize = 0;

        dirnamesize = makeint(*(entry+offset+0xd));
        strncpy(dirname, entry+offset+0x0e, dirnamesize);
        dirname[dirnamesize] = 0x0;

        char newdirprefix[200] = {0};
        if (*dirprefix == 0)
            strcpy(newdirprefix, dirname);
        else
            sprintf(newdirprefix, "%s%s", dirprefix, dirname);
        strcat(newdirprefix, "/");

        // special!! a sector is not mandatory! (empty directories...)
        if ( (tocsector == 0) && rip)
        {
            // no files below this, so just create the directory
            strcpy(newdir, dirprefix);
            _chdir(rootdirname);
            _chdir(ripdirname);
            multicreate(dirprefix);
        }
        else
        {
            readBlock(fp, tocsector, (unsigned char*)(buffer+level*2048*20), 20);
            parse(buffer+(level*2048*20), 0, level, fp, rip, newdirprefix);
        }
    }
    else
    {
        strcpy(newdir, dirprefix);

        if (rip)
        {
            if ( (doWildcard && fnameMatches(entry, offset)) || !doWildcard )
            {
                // we here should create the new dir and descend into it
                _chdir(rootdirname);
                _chdir(ripdirname);
                multicreate(dirprefix);
    
                extractFile(entry, offset, fp, dirprefix);
            }
        }
        else
        {
            printentry(entry, offset, dirprefix);
        }
    }
    if (right)
        parse(entry, right*4, level, fp, rip, dirprefix);
}

void printentry(char *entry, __int64 offset, char* dirprefix)
{
    // print sector
    unsigned long sector = getint(entry+offset+4);
    printf("%07d   ", sector,sector);
    unsigned long size = getint(entry+offset+8);
    totalbytes+=size;
    printf("%10u   ", size,size);

    if (*dirprefix != 0)
        printf("%s", dirprefix);
    printf("%.*s\n", makeint(*(entry+offset+0xd)), entry+offset+0x0e);
}

void printprogress(int current, int last, int total)
{
    int numleft = last - current;
    int numdone = total - numleft;
    int counter = numdone * 100;
    int process = counter/total;
    printf("\b\b\b\b");
    printf("%3u%%", process);

}

bool fnameMatches( char *entry, __int64 offset)
{
    char filename[200] = {0};
    int fnamesize = 0;
    fnamesize = makeint(*(entry+offset+0xd));
    strncpy(filename, entry+offset+0x0e, fnamesize);
    filename[fnamesize] = 0x0;

    if (match(strWildcard, filename) == 0)
        return true;
    else
        return false;
}

void extractFile( char *entry, __int64 offset, FILE* fp, char* dirprefix)
{
    char filename[200] = {0};
    int fnamesize = 0;
    char buffer[2048];
    char bogobuffer[2048] = {0};

    fnamesize = makeint(*(entry+offset+0xd));
    strncpy(filename, entry+offset+0x0e, fnamesize);

    filename[fnamesize] = 0x0;

    if (strcmp(currdir, newdir) != 0)
    {
        if ( *newdir == 0 )
            printf("\n ==> Processing directory: /\n");
        else
            printf("\n ==> Processing directory: %s\n", newdir);
        strcpy(currdir, newdir);
    }


    printf("Ripping '%s' ", filename);
    unsigned long sector = getint(entry+offset+4);
    unsigned long size = getint(entry+offset+8);
    unsigned long numsectors = size / 0x800;                // number of sectors to read
    unsigned long restbytes = size - (numsectors*0x800);

    FILE *ripfile;
    ripfile = fopen(filename, "wb");
    bool error = false;
    int iTry = 0;

    printf("[sector: %u - size: %u bytes]...     ", sector, size);

    // now let's see
    for (int i = sector; (unsigned)i < sector+numsectors; i++) // skip last sector, we will read manually
    {
        if (readBlock(fp, i, (unsigned char*)buffer, 1) != 0)
            error = true;
        else
        {
            // print progress bar stuff
            printprogress(i, sector+numsectors-1, numsectors);
        
            fwrite(buffer, 2048, 1, ripfile);
        }

        if (error)
        {
            printf(" ...Error at sector %u! Skipping!\n", i);
            break;
        }
    }

    if (error)
    {
        fclose(ripfile);
        return;
    }

    // get the restbytes
    if (restbytes != 0)
    {
        if (readBlock(fp, sector+numsectors, (unsigned char*)buffer, 1) != 0)
            error = true;
        else
        {
            // print progress bar stuff
            printprogress(1, 1, 1);
            fwrite(buffer, restbytes, 1, ripfile);
        }
    }

    if (error)
    {
        printf(" ...Error at sector %u! Skipping!\n", i);
        fclose(ripfile);
        return;
    }

    printf(" ...Done!\n");

    // now let's see
    fclose(ripfile);
}

int readBlock(FILE *fp, __int64 LBA, unsigned char* data, unsigned short num)
{
    if(_fseeki64(fp, LBA*2048, SEEK_SET)) return 1;
    if( fread(data, 1, num*2048, fp) != (unsigned)num*2048 ) return 1;
    return 0;

}

int makeint(char in)
{
    int result = 0;
    result = in;
    return result;
}

unsigned short getword(char* ptr)
{
    unsigned short ret;
    ret = (((unsigned char) *(ptr+1)) & 0xFF) << 8;
    ret |= (((unsigned char) *ptr)) & 0xFF;
    
    return ret;
}

unsigned long getint(char* ptr)
{

    unsigned long ret;
    ret = (*(ptr+3) & 0xFF) << 24;
    ret |= (*(ptr+2) & 0xFF) << 16;
    ret |= (*(ptr+1) & 0xFF) << 8;
    ret |= *(ptr+0) & 0xFF;

    return ret;
}