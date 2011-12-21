/******************************************************************************

abgx360.c

The ultimate tool for Xbox 360 ISOs and Stealth files!

Copyright 2008-2010 by Seacrest <Seacrest[at]abgx360[dot]net>

******************************************************************************/

#if (defined(_WIN32) || defined(__WIN32__)) && !defined(WIN32)
#define WIN32
#endif

#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64
#define _GNU_SOURCE

#define PFI_HEX 1
#define DMI_HEX 2

#define WEB_INIDIR 1
#define WEB_CSV 2
#define WEB_DAT 3
#define WEB_STEALTHDIR 4
#define WEB_AUTOUPLOAD 5
#define WEB_UNVERIFIEDINIDIR 6

#define XEX_INI                  1
#define SSXEX_INI                2
#define SSXEX_INI_FROM_XEX_INI   3
#define UNVERIFIED_INI           4
#define SS_FILE                  5
#define STEALTH_FILE             6
#define GIANT_VIDEO_FILE         7
#define SMALL_VIDEO_FILE         8
#define AP25_BIN_FILE            9
#define AP25_HASH_FILE           10

#define	MAX_FILENAMES 100000

// MAX_DIR_LEVELS * MAX_DIR_SECTORS * 2048 will be the maximum size of fsbuffer during execution
#define MIN_DIR_SECTORS 20
#define MAX_DIR_SECTORS 200  // needs to be an even multiple of MIN_DIR_SECTORS; largest observed was cod4 (53)
#define MIN_DIR_LEVELS 5
#define MAX_DIR_LEVELS 150  // needs to be an even multiple of MIN_DIR_LEVELS; largest observed was dark messiah (22)

#define WOW_THATS_A_LOT_OF_RAM 134217728  // 128 MB

#include <stdio.h> // standard i/o
#include <stddef.h> // for offsetof
#include <stdbool.h> // true/false macro for bools
#include <stdlib.h> // standard library definitions
#include <string.h> // for string operations
#include <strings.h> // for more string operations
#include <errno.h> // for errors
#include <sys/types.h> // type definitions like off_t
#include <time.h> // for time()
#include <dirent.h> // for opendir, readdir
#include <sys/stat.h> // for stat(), bsd/posix mkdir()
#include "rijndael-alg-fst.h"
#include "sha1.h"
#include "mspack/mspack.h"
#include "mspack/system.h"
#include "mspack/lzx.h"

#ifdef WIN32
    #define ABGX360_OS "Windows"
    #include "zlib.h"
    #include "curl/curl.h"
    #include "fnmatch.h" // for fnmatch
    #include <windows.h> // for GetTickCount, SetConsoleTextAttribute, ReadFile, RegOpenKeyEx, RegQueryValueEx, CreateFile, SetFilePointerEx, SetEndOfFile, ...
    #include <conio.h> // for getch
    #include <direct.h> // for _mkdir
    #include <ddk/ntddstor.h> // device i/o stuff
    #include <ddk/ntddscsi.h> // SCSI_PASS_THROUGH_DIRECT
    #include "spti.h" // SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER
    char winbuffer[2048];
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    #define mkdir(a,b) _mkdir(a)
    #define strcasecmp(a,b) _stricmp(a,b)
    #define strncasecmp(a,b,c) _strnicmp(a,b,c)
    #define fseeko(a,b,c) myfseeko64(a,b,c)
    #define ftello(a) ftello64(a)
    #define DATA_NONE SCSI_IOCTL_DATA_UNSPECIFIED
    #define DATA_IN   SCSI_IOCTL_DATA_IN
    #define DATA_OUT  SCSI_IOCTL_DATA_OUT
    #define LL "I64"
#else
    #define LL "ll"
    #include <zlib.h>
    #include <curl/curl.h>
    #include <fnmatch.h> // for fnmatch
    #include <sys/time.h> // for gettimeofday
    #include <pwd.h> // for getpwuid
    #include <unistd.h> // for getuid, read()
    #include <fcntl.h> // for open()
    int fd;
    #if defined(__linux__)
        #define ABGX360_OS "Linux"
        #include <scsi/sg.h> // for sg_io_hdr
        #include <sys/ioctl.h> // for ioctl
        struct sg_io_hdr sgio;
        #define DATA_NONE SG_DXFER_NONE
        #define DATA_IN   SG_DXFER_FROM_DEV
        #define DATA_OUT  SG_DXFER_TO_DEV
    #elif defined(__APPLE__)
        #define ABGX360_OS "Mac OS X"
        // not sure if all of this is really needed
        #include <CoreFoundation/CoreFoundation.h>
        #include <IOKit/IOKitLib.h>
        #include <IOKit/scsi/SCSITaskLib.h>
        #include <IOKit/storage/IODVDTypes.h>
        #include <mach/mach.h>
        #define normal applesucks  // avoid compiler error
        IOCFPlugInInterface **plugInInterface;
        MMCDeviceInterface **mmcDeviceInterface;
        SCSITaskDeviceInterface **scsiTaskDeviceInterface;
        SCSITaskInterface **taskInterface;
        IOVirtualRange *range;
        #define DATA_NONE kSCSIDataTransfer_NoDataTransfer
        #define DATA_IN   kSCSIDataTransfer_FromTargetToInitiator
        #define DATA_OUT  kSCSIDataTransfer_FromInitiatorToTarget
    #elif defined(__FreeBSD__)
        #define ABGX360_OS "FreeBSD"
        #include <camlib.h> // for CAM library functions
        struct cam_device *cam_dev = NULL;
        #define DATA_NONE CAM_DIR_NONE
        #define DATA_IN   CAM_DIR_IN
        #define DATA_OUT  CAM_DIR_OUT
    #elif defined(__OpenBSD__)
        #define ABGX360_OS "OpenBSD"
        #include <sys/scsiio.h> // for scsireq and SCIOCCOMMAND
        //struct scsireq scsireq_t;
        #define DATA_NONE SCCMD_IOV // this is an educated guess...
        #define DATA_IN   SCCMD_READ
        #define DATA_OUT  SCCMD_WRITE
    #elif defined(__NetBSD__)
        #define ABGX360_OS "NetBSD"
        #include <sys/scsiio.h> // for scsireq and SCIOCCOMMAND
        //struct scsireq scsireq_t;
        #define DATA_NONE SCCMD_IOV // this is an educated guess...
        #define DATA_IN   SCCMD_READ
        #define DATA_OUT  SCCMD_WRITE
    #else
        #define ABGX360_OS "Unknown OS"
        // unsupported OS - defining these will enable compilation but anything involving device i/o will fail with error "Unsupported Operating System"
        #define DATA_NONE 0
        #define DATA_IN   0
        #define DATA_OUT  0
    #endif

#endif  // ifdef WIN32

#define BIGBUF_SIZE 32768  // 32 KB, changing this could cause some problems

#define NUM_CURRENTPFIENTRIES   5  // update this when adding new pfi entries
#define NUM_CURRENTVIDEOENTRIES 9  // update this when adding new video entries

#define NUM_CURRENTAP25MEDIAIDS 6  // update this when adding new AP25 media ids for games that have no AP25 flag in the default.xex

// update version values here
char *headerversion = "v1.0.5";
char *curluseragent = "abgx360 v1.0.5 ("ABGX360_OS")";
unsigned long currentversion = 0x010005L;  // MSB (1.2.3 = 0x010203)

// this will be replaced with the value from abgx360.dat if it exists
unsigned long latestversion = 0L;

// update this value before release (unrecognized pfi/video on a game authored before this date is labeled almost certainly corrupt, otherwise it might be a brand new wave)
// this will be replaced with the value from abgx360.dat if it exists
unsigned long long lastknownwave = 0x01CA3FCE9F2FC000LL;  // 2009-09-28

// update this value if additional stealth sectors are needed in our ISOs (in the area between L0 video padding
// and AP25) so that older versions of abgx360 will not blank them out if fixing video padding is enabled
// this will be replaced with the value from abgx360.dat if it exists
unsigned long total_sectors_available_for_video_data = 129820;  // decrease if needed, never increase

// local directories
char homedir[2048];
#ifdef WIN32
    char *abgxdir = "\\abgx360\\";
    char *stealthdir = "StealthFiles\\";
    char *userstealthdir = "UserStealthFiles\\";
    char *imagedir = "Images\\";
#else
    char *abgxdir = "/.abgx360/";
    char *stealthdir = "StealthFiles/";
    char *userstealthdir = "UserStealthFiles/";
    char *imagedir = "Images/";
#endif

// load replacements from abgx360.ini if it exists (make sure to update checkini() if these addresses are changed)
char *webinidir = "http://abgx360.net/Apps/verified/";  // dir that contains verified ini files
char *webunverifiedinidir = "http://abgx360.net/Apps/unverified/";  // dir that contains unverified ini files
char *webcsv = "http://abgx360.net/Apps/Stealth360/GameNameLookup.csv";  // http path to GameNameLookup.csv
char *webdat = "http://abgx360.net/Apps/Stealth360/abgx360.dat";  // http path to abgx360.dat
char *webstealthdir = "http://abgx360.net/Apps/StealthFiles/";  // dir that contains SS/DMI/PFI/Video stealth files
char *autouploadwebaddress = "http://abgx360.net/Apps/Control/AutoUpload.php";  // form for submitting AutoUploads

struct waveentry {unsigned long crc; uchar sha1[20]; char *description; bool hosted;};
struct waveentry currentpfientries[NUM_CURRENTPFIENTRIES];
struct waveentry *mostrecentpfientries;
struct waveentry *datfilepfientries;
unsigned long num_pfientries;
struct waveentry currentvideoentries[NUM_CURRENTVIDEOENTRIES];
struct waveentry *mostrecentvideoentries;
struct waveentry *datfilevideoentries;
unsigned long num_videoentries;
unsigned long xbox1pficrc;
uchar xbox1pfisha1[20];
unsigned long xbox1videocrc;
uchar xbox1videosha1[20];

struct mediaidshort {unsigned char mediaid[4];};
struct mediaidshort currentap25mediaids[NUM_CURRENTAP25MEDIAIDS];
struct mediaidshort *mostrecentap25mediaids;
struct mediaidshort *datfileap25mediaids;
unsigned long num_ap25mediaids;

int mediumangledev_value = 3, highangledev_value = 9, fixangledev_value = 3;

bool verbose = true, stealthcheck = true, autofix = true, autofixuncertain = true, verify = true, onlineupdate = true, csvupdate = false;
bool checkdvdfile = true, checkpadding = false, fixdeviation = true, fixDRT = true, increasescreenbuffersize = true;
bool autofixalways = false, autoupload = false, keeporiginaliso = false, dontparsefs = false;
bool extraverbose = false, debug = false, debugfs = false, altlayerbreak = false;
bool noheader = false, justheader = false, justfooter = false;
bool minimal = false, html = false, stripcolors = false, script = false, justhelp = false;
bool terminal = false, stayoffline = false;
bool pauseshell = false, maximize = false;
bool addsplitvid = true, showfiles = false;
bool fixangle359 = false, showfulltable = false;
bool homeless = false, makedatfile = false;
bool patchvalidfilesonly = true, patchifstealthpasses = false, manualpatch = false, manualextract = false;
bool rebuildlowspace = false, norebuild = false, truncatefile = false, checkcorruption = false, foldermode = false;
bool matchonly = false, testing = false, testingdvd = false;
bool localonly = false, recursesubdirs = false, clobber = false;
bool showachievements = false, hidesecretachievements = false, showavatarawards = false, unicode = false, imagedirmissing = false;
bool skiplayerboundaryinfo = false, devkey = false, trustssv2angles = true, useinstalldir = false;
struct badshit {unsigned char c[21], d[21], data[21]; int count; char* explanation;};
char unrecognizedRTarray[21];
// don't forget to add new args to the list before stat()
int truncatearg = 0, userregionarg = 0, folderarg = 0, matcharg = 0, specialarg = 0, readretryarg = 0, layerbreakarg = 0;
int patchvideoarg = 0, patchpfiarg = 0, patchdmiarg = 0, patchssarg = 0;
int extractvideoarg = 0, extractvideopartitionarg = 0, extractpfiarg = 0, extractdmiarg = 0, extractssarg = 0;
int autouploaduserarg = 0, autouploadpassarg = 0, fixangledevarg = 0, connectiontimeoutarg = 0, dvdtimeoutarg = 0, dvdarg = 0, userlangarg = 0;
//int riparg = 0, ripdestarg = 0;
long connectiontimeout = 20, dvdtimeout = 20, layerbreak = 1913760, userlang = 0;
unsigned long curlprogressstartmsecs, userregion = 0L;
char *green = "\033[1;32;40m", *yellow = "\033[1;33;40m", *red = "\033[1;31;40m", *cyan = "\033[1;36;40m", *blue = "\033[1;34;40m";
char *darkblue = "\033[0;34;40m", *white = "\033[1;37;40m", *arrow = "\033[1;34;40m", *box = "\033[1;34;40m", *normal = "\033[0;37;40m";
char *wtfhexcolor = "\033[1;31;40m", *wtfcharcolor = "\033[1;37;41m", *reset = "\033[0m", *brown = "\033[0;33;40m", *filename = "\033[0;37;44m";
#ifdef __APPLE__
    char *hexoffsetcolor = "\033[0;37;40m", *darkgray = "\033[0;37;40m";  // can't do dark gray apparently (shows completely black) so just use normal gray
#else
    char *hexoffsetcolor = "\033[1;30;40m", *darkgray = "\033[1;30;40m";
#endif
char *newline = "\n", *quotation = "\"", *ampersand = "&", *lessthan = "<", *greaterthan = ">", *numbersign = "#";
char *sp0 = "\0", *sp1 = " ", *sp2 = "  ", *sp3 = "   ", *sp4 = "    ", *sp5 = "     ";
char *sp6 = "      ", *sp7 = "       ", *sp8 = "        ", *sp9 = "         ";
char *sp10 = "          ", *sp11 = "           ", *sp12 = "            ", *sp18 = "                  ";
char *sp20 = "                    ", *sp21 = "                     ", *sp28 = "                            ";
char buffer[2048], buffer2[2048];
char inifilename[24], xexinifilename[17], gamename[151];
char sense[20], specialerror[200];
char installdirvideofilepath[2048] = {0};
unsigned char ubuffer[2048], ss[2048], fixed_ss[2048], cdb[12];
unsigned char bigbuffer[BIGBUF_SIZE];
unsigned long filecount = 0L;
unsigned long seek;
unsigned long getuint(unsigned char* ptr), getuintmsb(unsigned char* ptr), getint(char* ptr);
sha1_context ctx;
/*
static uchar pfisha1[3][20] =
{  // {xbox1, x360 wave 1, x360 wave 2} (0,1,2)
    {0x1b,0x1c,0x6e,0x61,0x83,0x57,0x99,0xdd,0x18,0x2d,0xea,0x5b,0x3f,0x3f,0x35,0x44,0x72,0x16,0xa8,0xac},
    {0xf5,0x13,0xa4,0x52,0xd1,0x32,0xb4,0x3d,0x85,0xfa,0xe0,0xd2,0x2d,0x4c,0xae,0x85,0xfe,0x8d,0xde,0x67},
    {0xa1,0x65,0xf4,0x8d,0x93,0x8e,0x41,0x47,0xde,0xc9,0xed,0x40,0xf2,0xdf,0x44,0xa8,0xd3,0x97,0x50,0x50}
};
static uchar videosha1[3][20] =
{  // {xbox1, x360 wave 1, x360 wave 2} (0,1,2)
    {0xe2,0x7f,0x0c,0x94,0xae,0xf7,0x9c,0xed,0x70,0x4d,0x12,0x2d,0x50,0x62,0x57,0x4a,0x23,0x76,0xc6,0x36},
    {0x12,0x00,0xd3,0x04,0x1f,0x8d,0x19,0x64,0x59,0x9a,0x1c,0x5a,0xee,0xd3,0x10,0x55,0xd4,0x1a,0xda,0x87},
    {0xa3,0x95,0xd4,0x45,0x07,0x37,0x4e,0xea,0x04,0xd7,0x37,0xc9,0x4b,0x92,0xa7,0xd4,0x1a,0x41,0x16,0x77}
};
*/
unsigned char dmi_mediaid[16], ss_mediaid[16], xex_mediaid[16];
unsigned long pfi_sectorstotal, pfi_sectorsL0, pfi_sectorsL1;
unsigned long long pfi_offsetL1, pfi_offsetL0end;
unsigned long long dmi_authored, ss_authored, ss_mastered;
void printhtmltop(int argc, char *argv[]), printhtmlbottom(), printheader(), color(char *color), printwtfhexcolor(), printwtfcharcolor();
void printwin32filetime(unsigned long long win32filetime), printmediaid(unsigned char* mediaid), hexdump(unsigned char* ptr, int stealthtype, int bytes);
void checkvideo(char *isofilename, FILE *stream, bool justavideoiso, bool checkvideopadding), checkdmi(unsigned char *dmi), checkpfi(unsigned char *pfi);
void checkap25();
unsigned char ap25[2048];
int checkss(), doautofix(), doverify(), checkgame();
unsigned long getzeros(unsigned char* ptr, unsigned long firstbyte, unsigned long lastbyte);
bool lookslikepfi(unsigned char* pfi), lookslike360dmi(unsigned char* dmi), lookslikexbox1dmi(unsigned char* dmi);
bool lookslike360ss(unsigned char* ss), lookslikexbox1ss(unsigned char* ss);
unsigned long fix_ss_crc32;
long long fpfilesize;
int isofilearg, readretries = 20, charsprinted;
unsigned long readerrorstotal, readerrorsrecovered, writeerrorstotal, writeerrorsrecovered;
int readerrorcharsprinted;
int returnvalue;
long longreturnvalue;
unsigned long sizeoverbuffer, bufferremainder;
int checkreadandprinterrors(void *ptr, size_t size, size_t nmemb, FILE *stream, unsigned long loop, unsigned long long startoffset, char *name, char *action);
int checkwriteandprinterrors(const void *ptr, size_t size, size_t nmemb, FILE *stream, unsigned long loop, unsigned long long startoffset, char *name, char *action);
void initcheckread(), donecheckread(char *name);
long long getfilesize(FILE *fp);
int docheckgamecrc();
int docheckdvdfile();
void donecheckwrite(char *name), initcheckwrite();
void domanualextraction(char *argv[]), domanualpatch(char *argv[]);
int doautoupload(char *argv[]);
int trytowritestealthfile(const void *ptr, size_t size, size_t nmemb, FILE *stream, char *filename, long long offset);
int trytoreadstealthfile(void *ptr, size_t size, size_t nmemb, FILE *stream, char *filename, long long offset);
int padzeros(FILE *stream, char *filename, long long startoffset, long long endoffset);
int writeini(char *inifilename, char *ini_discsource, char *ini_gamename, char *ini_gamertag, char *ini_drivename, char *ini_drivefw, char *ini_notes);
int extractstealthfile(FILE *isofile, char *isofilename, long long offset, char *name, char *stealthfilename);
FILE *openstealthfile(char *stealthfilename, char *localdir, char *webdir, int type, char *location);
unsigned char regioncode[4];
char *readstdin(char *dest, int size);
void checkdat(), makedat();
int dotruncate(char *filename, long long filesize, long long truncatesize, bool stfu);
char *isofilename = NULL;
void checkini();
unsigned long long corruptionoffset[100];
bool unverifiediniexists();
int rebuildiso(char *filename);
void printseekerror(char *filename, char *action);
int doaddsplitvid();
struct filesys { unsigned long datasector, datalength; } *filesystem, *holes;
CURL *curl;
CURLcode res;
struct MyCurlFile { const char *filename; FILE *stream; };
char curlerrorbuffer[CURL_ERROR_SIZE+1];
struct stat buf;
void parsetitleidresource(unsigned char *resourcebuffer, unsigned long resourcesize, unsigned char *titleid);
int checkdefaultxex(unsigned char *defaultxexbuffer, unsigned long defaultxexsize);
size_t dontcare;

// global vars that might need to be reset after after every fileloop
bool writefile = true;
bool checkgamecrcalways = false, checkgamecrcnever = false, gamecrcfailed = false, verifyfailed = false;
bool stealthfailed = false, stealthuncertain = false, matchedbefore = false, xbox1iso = false;
bool ss_stealthfailed = false, dmi_stealthfailed = false, pfi_stealthfailed = false, video_stealthfailed = false;
bool dmi_stealthuncertain = false, ss_stealthuncertain = false, pfi_stealthuncertain = false, video_stealthuncertain = false;
bool dmi_foundmediaid = false, ss_foundmediaid = false, xex_foundmediaid = false, foundregioncode = false, foundgamename = false;
bool ss_foundtimestamp = false, usercancelledgamecrc = false;
bool pfi_alreadydumped = false, pfi_foundsectorstotal = false, pfiexception = false;
bool wtfhex = false, checkssbin = false;
bool justastealthfile = false, isotoosmall = false;
bool drtfucked = false, fixedss = false, fixedap25 = false;
bool printstderr = false, rebuildfailed = false, curlheaderprinted = false;
int unrecognizedRTcount = 0;
//int videowave = 0, pfiwave = 0, truepfiwave = 0;
unsigned long long video = 0;
unsigned long game_crc32 = 0, xex_crc32 = 0, ss_crc32 = 0, ss_rawcrc32 = 0, dmi_crc32 = 0, pfi_crc32 = 0;
unsigned long video_crc32 = 0, videoL0_crc32 = 0, videoL1_crc32 = 0;
//uchar xex_sha1[20] = {0};
uchar pfi_sha1[20] = {0};
//video_sha1[20] = {0};
int ini_dmi_count = 0;
unsigned long ini_ss = 0, ini_pfi = 0, ini_video = 0, ini_rawss = 0, ini_v0 = 0, ini_v1 = 0, ini_game = 0, ini_xexhash = 0;
unsigned long ini_dmi[30] = {0};
int corruptionoffsetcount = 0;
FILE *fp = NULL, *csvfile = NULL, *inifile = NULL, *xexinifile = NULL;
int buffermaxdir = MIN_DIR_SECTORS;
int bufferlevels = MIN_DIR_LEVELS;
char *fsbuffer;
char dirprefix[2048] = {0};
unsigned long long totalbytes = 0;
unsigned long totalfiles = 0, totaldirectories = 0;
long long L0capacity = -1;
int level = 0;
bool parsingfsfailed = false;
bool extractimages = false, embedimages = false, noxexiniavailable = false, offlinewarningprinted = false;
bool verify_found_bad_pfi_or_video = false;
bool game_has_ap25 = false;

// reset these global vars after every fileloop and parse cmd line again
void resetvars() {
    writefile = true;
    checkgamecrcalways = false; checkgamecrcnever = false; gamecrcfailed = false; verifyfailed = false;
    stealthfailed = false; stealthuncertain = false; matchedbefore = false; xbox1iso = false;
    ss_stealthfailed = false; dmi_stealthfailed = false; pfi_stealthfailed = false; video_stealthfailed = false;
    dmi_stealthuncertain = false; ss_stealthuncertain = false; pfi_stealthuncertain = false; video_stealthuncertain = false;
    dmi_foundmediaid = false; ss_foundmediaid = false; xex_foundmediaid = false; foundregioncode = false; foundgamename = false;
    ss_foundtimestamp = false; usercancelledgamecrc = false;
    pfi_alreadydumped = false; pfi_foundsectorstotal = false; pfiexception = false;
    wtfhex = false; checkssbin = false;
    justastealthfile = false; isotoosmall = false;
    drtfucked = false; fixedss = false, fixedap25 = false;
    printstderr = false; rebuildfailed = false; curlheaderprinted = false;
    unrecognizedRTcount = 0;
    //videowave = 0; pfiwave = 0; truepfiwave = 0;
    video = 0;
    game_crc32 = 0; xex_crc32 = 0; ss_crc32 = 0; ss_rawcrc32 = 0; dmi_crc32 = 0; pfi_crc32 = 0;
    video_crc32 = 0; videoL0_crc32 = 0; videoL1_crc32 = 0;
    //memset(xex_sha1, 0, 20);
    memset(pfi_sha1, 0, 20);
    //memset(video_sha1, 0, 20);
    ini_dmi_count = 0;
    ini_ss = 0; ini_pfi = 0; ini_video = 0; ini_rawss = 0; ini_v0 = 0; ini_v1 = 0; ini_game = 0; ini_xexhash = 0;
    int i;
    for(i=0;i<30;i++) ini_dmi[i] = 0L;
    corruptionoffsetcount = 0;
    fp = NULL; csvfile = NULL; inifile = NULL; xexinifile = NULL;
    buffermaxdir = MIN_DIR_SECTORS;
    bufferlevels = MIN_DIR_LEVELS;
    memset(dirprefix, 0, 2048);
    totalbytes = 0;
    totalfiles = 0; totaldirectories = 0;
    L0capacity = -1;
    level = 0;
    parsingfsfailed = false;
    extractimages = false; embedimages = false; noxexiniavailable = false; offlinewarningprinted = false;
    verify_found_bad_pfi_or_video = false;
    game_has_ap25 = false;
  return;
}

#ifndef WIN32

// create unix versions of kbhit() and getch()
// this code is adapted from the book "Beginning Linux Programming" from Wrox Press

#ifndef KBHITh
    #define KBHITh
    void init_keyboard(void);
    void close_keyboard(void);
    int kbhit(void);
    int getch(void);
#endif

#include <termios.h>
#include <signal.h>

bool rawterminal = false;
struct termios initial_settings, new_settings;
int peek_character = -1;

void init_keyboard() {
    if (rawterminal) return;
    if (debug) printf("running init_keyboard()%s", newline);
    sigset_t block_ttio;
    if (tcgetattr(0, &initial_settings) == 0) {
        new_settings = initial_settings;
        new_settings.c_lflag &= ~ICANON;
        new_settings.c_lflag &= ~ECHO;
        new_settings.c_lflag &= ~ISIG;
        new_settings.c_cc[VMIN] = 1;
        new_settings.c_cc[VTIME] = 0;
        // block SIGTTOU to prevent hanging when backgrounded
        if (sigemptyset(&block_ttio) == 0 &&
            sigaddset(&block_ttio, SIGTTOU) == 0 &&
            sigprocmask(SIG_BLOCK, &block_ttio, NULL) == 0 &&
            tcsetattr(0, TCSANOW, &new_settings) == 0) {
                rawterminal = true;
        }
        else if (debug) printf("unix init_keyboard() - blocking SIGTTOU and setting new terminal attributes failed (%s)%s", strerror(errno), newline);
        if (sigprocmask(SIG_UNBLOCK, &block_ttio, NULL) != 0 && debug) printf("unix init_keyboard() - unblocking SIGTTOU failed (%s)%s", strerror(errno), newline);
    }
    else if (debug) printf("unix init_keyboard() - getting terminal attributes failed (%s)%s", strerror(errno), newline);
}

void close_keyboard() {
    if (!rawterminal) return;
    if (debug) printf("running close_keyboard()%s", newline);
    sigset_t block_ttio;
    // block SIGTTOU to prevent hanging when backgrounded
    if (sigemptyset(&block_ttio) == 0 &&
        sigaddset(&block_ttio, SIGTTOU) == 0 &&
        sigprocmask(SIG_BLOCK, &block_ttio, NULL) == 0 &&
        tcsetattr(0, TCSANOW, &initial_settings) == 0) {
            rawterminal = false;
    }
    else if (debug) printf("unix close_keyboard() - blocking SIGTTOU and resetting initial terminal attributes failed (%s)%s", strerror(errno), newline);
    if (!rawterminal && sigprocmask(SIG_UNBLOCK, &block_ttio, NULL) != 0 && debug) printf("unix close_keyboard() - unblocking SIGTTOU failed (%s)%s", strerror(errno), newline);
}

int kbhit() {
    if (!rawterminal) return 0;
    if (peek_character != -1) return 1;
    unsigned char ch;
    int nread;
    sigset_t block_ttio;
    new_settings.c_cc[VMIN] = 0;
    // block SIGTTIN and SIGTTOU to prevent hanging when backgrounded
    if (sigemptyset(&block_ttio) == 0 &&
        sigaddset(&block_ttio, SIGTTOU) == 0 &&
        sigaddset(&block_ttio, SIGTTIN) == 0 &&
        sigprocmask(SIG_BLOCK, &block_ttio, NULL) == 0 &&
        tcsetattr(0, TCSANOW, &new_settings) == 0) {
            nread = read(0, &ch, 1);
            new_settings.c_cc[VMIN] = 1;
            if (tcsetattr(0, TCSANOW, &new_settings) == 0) {
                //if (nread < 0 && debug) printf("unix kbhit() - read returned %d (%s)%s", nread, strerror(errno), newline);
                if (nread == 1) {
                    peek_character = ch;
                    if (sigprocmask(SIG_UNBLOCK, &block_ttio, NULL) != 0 && debug) printf("unix kbhit() - unblocking TTOU after nread == 1 failed (%s)%s", strerror(errno), newline);
                  return 1;
                }
                if (sigprocmask(SIG_UNBLOCK, &block_ttio, NULL) != 0 && debug) printf("unix kbhit() - unblocking TTOU after nread != 1 failed (%s)%s", strerror(errno), newline);
            }
            else if (debug) printf("unix kbhit() - resetting VMIN to 1 after read failed (%s)%s", strerror(errno), newline);
    }
    else if (debug) printf("unix kbhit() - initial blocking SIGTTOU/SIGTTIN and setting VMIN to 0 failed (%s)%s", strerror(errno), newline);
  return 0;
}

int getch() {
    char ch;
    int nread;
    if (peek_character != -1) {
        ch = peek_character;
        peek_character = -1;
      return ch;
    }
    nread = read(0, &ch, 1);
    if (nread < 0) {
        if (debug) printf("unix getch() - read returned %d (%s)%s", nread, strerror(errno), newline);
      return '?';
    }
  return ch;
}

#endif // #ifndef WIN32

size_t my_curl_write(void *buffer, size_t size, size_t nmemb, void *data) {
    if (size == 0 || nmemb == 0) return 0;
    struct MyCurlFile *out = (struct MyCurlFile *) data;
    if (out && !out->stream) {
        // open file for writing
        out->stream = fopen(out->filename, "wb");
        if (out->stream == NULL) {
            if (debug) {
                color(red);
                printf("my_curl_write ERROR: Failed to open '%s' for writing (%s)%s", out->filename, strerror(errno), newline);
                color(normal);
            }
          return 0;
        }
    }
  return fwrite(buffer, size, nmemb, out->stream);
}

int compfilesys(const void *f1, const void *f2) {
    struct filesys *file1 = (struct filesys *) f1;
    struct filesys *file2 = (struct filesys *) f2;
  return  file1->datasector <= file2->datasector ? ( file1->datasector < file2->datasector ? -1 : 0) : 1;
}

unsigned long converttosectors(unsigned long size) {
    unsigned long sectors = 0;
    sectors = size / 2048;
    if (size % 2048 != 0) sectors += 1;
  return sectors;
}

int readblock(char* filename, char* action, FILE *fp, long long LBA, unsigned char* data, unsigned short num) {
    if (debugfs) {
        color(green);
        printf("reading sector: %"LL"d (0x%"LL"X) num=%u%s", LBA, LBA*2048+video, num, newline);
        color(blue);
    }
    if (fseeko(fp, LBA*2048+video, SEEK_SET) != 0) {
        printseekerror(filename, action);
      return 1;
    }
    initcheckread();
    if (checkreadandprinterrors(data, 1, num*2048, fp, 0, LBA*2048+video, filename, action) != 0) return 1;
    donecheckread(filename);
  return 0;
}

int makeint(char in) {
    int result = 0;
    result = in;
  return result;
}

unsigned short getword(char* ptr) {
    unsigned short ret;
    ret = (((unsigned char) *(ptr+1)) & 0xFF) << 8;
    ret |= (((unsigned char) *ptr)) & 0xFF;
  return ret;
}

unsigned short getwordmsb(unsigned char* ptr) {
    unsigned short ret;
    ret = (*ptr & 0xFF) << 8;
    ret |= *(ptr+1) & 0xFF;
  return ret;
}

void printentry(long long entry, long long offset, char* dirprefix, bool justcount, bool emptydir) {
    unsigned long size = getint(fsbuffer+entry+offset+8);
    totalbytes += (unsigned long long) size;
    if ((!justcount && showfiles) || debugfs) {
        unsigned long sector = getint(fsbuffer+entry+offset+4);
        printf("%s%07lu %s", sp5, sector, sp2);
        if (size < 10) printf("%s", sp9);
        else if (size < 100) printf("%s", sp8);
        else if (size < 1000) printf("%s", sp7);
        else if (size < 10000) printf("%s", sp6);
        else if (size < 100000) printf("%s", sp5);
        else if (size < 1000000) printf("%s", sp4);
        else if (size < 10000000) printf("%s", sp3);
        else if (size < 100000000) printf("%s", sp2);
        else if (size < 1000000000) printf("%s", sp1);
        printf("%lu %s", size, sp2);
    
        if (*dirprefix != 0) printf("%s", dirprefix);
        if (emptydir) printf("%s", newline);
        else printf("%.*s%s", (int) fsbuffer[entry+offset+13], fsbuffer+entry+offset+14, newline);
    }
}

int parse(char *filename, char* action, long long entry, long long offset, int level, FILE* fp, bool justcount, bool storeit, char* dirprefix) {
	int returncode;
    unsigned long sector = getint(fsbuffer+entry+offset+4);
    unsigned short left = getword(fsbuffer+entry+offset);
    unsigned short right = getword(fsbuffer+entry+offset+2);
    unsigned long size = getint(fsbuffer+entry+offset+8);
    if (debugfs) {
        color(normal);
        printf("entry=0x%07"LL"X, offset=0x%07"LL"X, level=%02d, left=%04u, right=%04u, sector=%07lu (0x%09"LL"X), "
               "size=%010lu bytes, type=0x%02X, strlen=%02d, %.*s%s",
               entry, offset, level, left, right, sector, (long long) sector*2048+video, size,
               (unsigned char) fsbuffer[entry+offset+12], fsbuffer[entry+offset+13],
               (int) fsbuffer[entry+offset+13], fsbuffer+entry+offset+14, newline);
        color(blue);
    }
    if (sector == 0xFFFFFFFFL) {
        // invalid toc entry
        if (debugfs) {
            color(red);
            printf("INVALID ENTRY!%s", newline);
            color(blue);
        }
      return 0;
    }
    if (left) {
        if ((long) left * 4 > (long) buffermaxdir * 2048 - 20) {
            // entry is outside of the directory space
            parsingfsfailed = true;
            if (verbose) printf("%s", sp5);
            color(yellow);
            printf("Unable to parse filesystem because it appears to be corrupt%s",newline);
            color(normal);
          return 1;
        }
        if (debugfs) {
            color(darkgray);
            printf("about to parse left=%02u (0x%02X), left*4=%03d (0x%03X), entry=%07"LL"d (0x%07"LL"X), level=%02d%s",
                          left, left, (int) left*4, (int) left*4, entry, entry, level, newline);
            color(blue);
        }
        returncode = parse(filename, action, entry, (long long) left * 4, level, fp, justcount, storeit, dirprefix);
        if (returncode) {
            if (returncode != 2) parsingfsfailed = true;
          return returncode;
        }
    }

    if (((unsigned char) fsbuffer[entry+offset+12] & 0x10) == 0x10) {
        // directory
        if (storeit) {
            filesystem[totalfiles + totaldirectories].datasector = sector;
            filesystem[totalfiles + totaldirectories].datalength = converttosectors(size);
        }
        totaldirectories++;
        level++;
        if (debugfs) { color(yellow); printf("level = %02d, bufferlevels = %02d%s", level, bufferlevels, newline); color(normal); }
        if (level == MAX_DIR_LEVELS) {
            if (debugfs) {
                color(red);
                printf("ERROR: a directory is %d levels deep! (parsing folder aborted)%s", level, newline);
                color(blue);
            }
            parsingfsfailed = true;
            bufferlevels = MAX_DIR_LEVELS;
          return 2;
        }
        if (level == bufferlevels) {
            if (debugfs) {
                color(cyan);
                printf("reallocating fsbuffer from %ld to %ld: level = %d, bufferlevels = %d, MIN_DIR_LEVELS = %d, "
                       "buffermaxdir = %d, sizeof(char) = %d%s",
                        (long) bufferlevels * buffermaxdir * 2048 * sizeof(char),
                        (long) (bufferlevels + MIN_DIR_LEVELS) * buffermaxdir * 2048 * sizeof(char),
                        level, bufferlevels, MIN_DIR_LEVELS, buffermaxdir, (int) sizeof(char), newline);
            }
            bufferlevels += MIN_DIR_LEVELS;
            if (debugfs) printf("bufferlevels = %d%s", bufferlevels, newline);
            char *newfsbuffer = (char *) realloc(fsbuffer, (size_t) bufferlevels * buffermaxdir * 2048 * sizeof(char));
            if (newfsbuffer == NULL) {
                color(red);
                printf("ERROR: memory allocation for fsbuffer failed! Game over man... Game over!%s", newline);
                color(normal);
              exit(1);
            }
            fsbuffer = newfsbuffer;
            if (debugfs) { printf("reallocation successful%s", newline); color(blue); }
        }
        if (debugfs) { color(normal); printf("directory: "); }
        unsigned long tocsector = getint(fsbuffer+entry+offset+4);
        if (debugfs) printf("tocsector = %lu, ", tocsector);
        char dirname[2048];
        int dirnamesize = (int) fsbuffer[entry+offset+13];
        if (debugfs) { printf("dirnamesize = %d%s", dirnamesize, newline); color(blue); }
        if (dirnamesize + strlen(dirprefix) > 2046) {  // need space for "/" and terminating null
            if (verbose) printf("%s", sp5);
            color(yellow);
            printf("Unable to parse filesystem because a pathname is %d chars long!%s", dirnamesize + (int) strlen(dirprefix), newline);
            color(normal);
            if (debugfs) {
                color(red);
                printf("strlen(dirprefix) = %d, dirprefix: %s%sdirnamesize = %d, dirname (max 20KB): %.*s%s",
                        (int) strlen(dirprefix), dirprefix, newline,
                        dirnamesize, dirnamesize < 20480 ? dirnamesize : 20480, fsbuffer+entry+offset+14, newline);
                color(blue);
            }
            parsingfsfailed = true;
          return 1;
        }
        strncpy(dirname, fsbuffer+entry+offset+14, dirnamesize);
        dirname[dirnamesize] = 0;  // terminating null
        
        char newdirprefix[2048];
        newdirprefix[0] = 0;
        if (dirprefix[0] == 0) strcpy(newdirprefix, dirname);
        else sprintf(newdirprefix, "%s%s", dirprefix, dirname);
        strcat(newdirprefix, "/");
        if (debugfs) { color(normal); printf("dirprefix: %s, newdirprefix: %s%s", dirprefix, newdirprefix, newline); color(blue); }
        
        if (size) {  // don't parse empty directories!
            if (size > (unsigned long) buffermaxdir*2048) {
                if (debugfs) {
                    color(yellow);
                    printf("ERROR: %s is %lu sectors long! (parsing folder aborted)%s", newdirprefix, converttosectors(size), newline);
                    color(blue);
                }
                buffermaxdir = converttosectors(size);
              return 2;
            }
            memset(fsbuffer+level*buffermaxdir*2048, 0, buffermaxdir*2048);
            if (debugfs) {
                color(darkgray);
                printf("about to readblock: tocsector=%06lu, size=%06lu, converttosectors(size)=%06lu%s",
                        tocsector, size, converttosectors(size), newline);
                color(blue);
            }
            if (readblock(filename, action, fp, tocsector, (unsigned char*) (fsbuffer+level*buffermaxdir*2048), converttosectors(size))) {
                parsingfsfailed = true;
              return 1;
            }
            if (debugfs) {
                color(darkgray);
                printf("about to parse directory: %s, level=%02d%s", newdirprefix, level, newline);
                color(blue);
            }
            returncode = parse(filename, action, (long long) level*buffermaxdir*2048, 0, level, fp, justcount, storeit, newdirprefix);
            if (returncode) {
                if (returncode != 2) parsingfsfailed = true;
              return returncode;
            }
        }
        else printentry(entry, offset, newdirprefix, justcount, true);
    }
    else {
        if (storeit) {
            filesystem[totalfiles + totaldirectories].datasector = sector;
            filesystem[totalfiles + totaldirectories].datalength = converttosectors(size);
        }
        totalfiles++;
        printentry(entry, offset, dirprefix, justcount, false);
    }
    if (right) {
        if ((long) right * 4 > (long) buffermaxdir * 2048 - 20) {
            // entry is outside of the directory space
            parsingfsfailed = true;
            if (verbose) printf("%s", sp5);
            color(yellow);
            printf("Unable to parse filesystem because it appears to be corrupt%s",newline);
            color(normal);
          return 1;
        }
        if (debugfs) {
            color(darkgray);
            printf("about to parse right=%02u (0x%02X), right*4=%03d (0x%03X), entry=%07"LL"d (0x%07"LL"X), level=%02d%s",
                    right, right, (int) right*4, (int) right*4, entry, entry, level, newline);
            color(blue);
        }
        returncode = parse(filename, action, entry, (long long) right * 4, level, fp, justcount, storeit, dirprefix);
        if (returncode) {
            if (returncode != 2) parsingfsfailed = true;
          return returncode;
        }
    }
  return 0;
}

void doexitfunction() {
    if (html) printhtmlbottom();
    if (fp != NULL) fclose(fp);
    if (curl != NULL) curl_easy_cleanup(curl);
    if (!html && pauseshell) {
        printstderr = true; color(normal);
        #ifdef WIN32
            fprintf(stderr, "\nPress any key to exit . . . ");
            getch();
        #else
            init_keyboard();
            fprintf(stderr, "\nPress any key to exit . . . ");
            if (maximize) fprintf(stderr, "\n\n\n\n");  // otherwise this text could be hidden under the taskbar (kde?)
            getch();
            close_keyboard();
        #endif
        printstderr = false;
    }
    #ifndef WIN32
        printf("%s", newline);
        color(reset);
        close_keyboard();
    #endif
  return;
}

static int filesort(const void *f1, const void *f2) {
  return strcasecmp(* (char * const *) f1, * (char * const *) f2);
}

unsigned long getmsecs() {
    #ifdef WIN32
        unsigned long msecs = GetTickCount();
    #else
        struct timeval time;
        gettimeofday(&time, NULL);
        unsigned long msecs = time.tv_sec * 1000 + time.tv_usec / 1000;
    #endif
  return msecs;
}

void printcurlinfo(CURL *curl, char *filename) {
    // this function should only be called after a transfer which returns CURLE_OK
    CURLcode code;
    double downloadsize;
    code = curl_easy_getinfo(curl, CURLINFO_SIZE_DOWNLOAD, &downloadsize);
    if (code != CURLE_OK) {
        printf("Error getting curl transfer info for %s%s", filename, newline);
        if (debug) printf("CURLINFO_SIZE_DOWNLOAD ERROR: %d%s", code, newline);
      return;
    }
    if (downloadsize == 0) printf("%sServer file %s no newer than local file - not retrieving%s", sp5, filename, newline);
    else printf("%s%s was downloaded successfully%s", sp5, filename, newline);
  return;
}

int curlprogress(char *curlprogressdata, double dltotal, double dlnow, double ultotal, double ulnow) {
    if ((dltotal && dlnow) || (ultotal && ulnow)) {
        if(!curlheaderprinted) {
            fprintf(stderr, "%s\n", curlprogressdata);
            curlheaderprinted = true;
            curlprogressstartmsecs = getmsecs();
        }
        int chars = 0;
        int percent;
        long total = (long) dltotal + (long) ultotal;
        long now = (long) dlnow + (long) ulnow;
        percent = (int) (100*now/total);
        chars = fprintf(stderr, "\r%3d%% [", percent);  // percent done + start of progress bar
        int i;
        for (i=0;i<percent/3 - 1;i++) chars += fprintf(stderr, "=");
        if (percent > 2) chars += fprintf(stderr, ">");
        for (i=0;i<33 - percent/3;i++) chars += fprintf(stderr, " ");
        bool negative = now < 0;
        if (negative) now *= -1;
        static char nowdelimited[32];
        char *p = nowdelimited + 31;
        *--p = 0x0;
        i = 0;
        while (1) {
            *--p = now % 10 + '0';
            now /= 10;
            if (now == 0) break;
            if (i++ == 2) {
                *--p = ',';
                i = 0;
            }
        }
        if (negative) *--p = '-';
        chars += fprintf(stderr, "] %-11s ", p);  // end of progress bar + total transferred so far (with commas)
        unsigned long currentmsecs = getmsecs();
        double dlKBps = 0, ulKBps = 0;
        if (currentmsecs - curlprogressstartmsecs) {  // don't divide by 0
            dlKBps = dlnow / 1024 / (((double) currentmsecs - (double) curlprogressstartmsecs) / 1000);
            ulKBps = ulnow / 1024 / (((double) currentmsecs - (double) curlprogressstartmsecs) / 1000);
        }
        if (dlKBps || ulKBps) {
            // display combined dl/ul speed (average)
            if (dlKBps + ulKBps >= 1024) chars += fprintf(stderr, "%6.1f MB/s ", (dlKBps + ulKBps) / 1024);
            else chars += fprintf(stderr, "%6.1f KB/s ", dlKBps + ulKBps);
        }
        // take the slower of dl or ul for eta estimate
        long dletasecs = -1, uletasecs = -1;
        if (dltotal && dlnow && dlKBps) {
            dletasecs = (long) ((dltotal - dlnow) / 1024 / dlKBps);
        }
        if (ultotal && ulnow && ulKBps) {
            uletasecs = (long) ((ultotal - ulnow) / 1024 / ulKBps);
        }
        if (dletasecs > -1 || uletasecs > -1) {
            if (dletasecs > uletasecs) {
                if (dletasecs / 3600)
                    chars += fprintf(stderr, " ETA %ld:%02ld:%02ld", dletasecs / 3600, dletasecs / 60 % 60, dletasecs % 60);
                else
                    chars += fprintf(stderr, "   ETA %02ld:%02ld", dletasecs / 60 % 60, dletasecs % 60);
            }
            else {
                if (uletasecs / 3600)
                    chars += fprintf(stderr, " ETA %ld:%02ld:%02ld", uletasecs / 3600, uletasecs / 60 % 60, uletasecs % 60);
                else
                    chars += fprintf(stderr, "   ETA %02ld:%02ld", uletasecs / 60 % 60, uletasecs % 60);
            }
        }
        for (i=0;i<80-chars;i++) fprintf(stderr, " ");  // this will overwrite any leftover crap from the last progress update (80 instead of 79 because of \r)
        fflush(stderr);
    }
  return 0;
}

void createsecretpng(char *secretpngpath) {
    // this function creates the file secret.png (this will be the icon used if we're hiding secret achievements)
    static unsigned char secretpngdata[357] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 
	0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0x08, 0x03, 0x00, 0x00, 0x00, 0x9D, 0xB7, 0x81, 
	0xEC, 0x00, 0x00, 0x00, 0x18, 0x50, 0x4C, 0x54, 0x45, 0xFE, 0xFE, 0xFE, 0x8F, 0x8F, 0x8F, 0x3A, 
	0x3A, 0x3A, 0x32, 0x32, 0x32, 0x25, 0x25, 0x25, 0x5B, 0x5B, 0x5B, 0xC7, 0xC7, 0xC7, 0xDE, 0xDE, 
	0xDE, 0x85, 0xC2, 0xAD, 0x84, 0x00, 0x00, 0x01, 0x08, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0xEC, 
	0x97, 0xEB, 0x12, 0x84, 0x20, 0x08, 0x85, 0x41, 0x40, 0xDF, 0xFF, 0x8D, 0xD7, 0xCA, 0x4B, 0xB5, 
	0x69, 0x2C, 0xEE, 0xCC, 0x36, 0x3B, 0x9E, 0x7F, 0x66, 0x7C, 0x29, 0x1E, 0xA8, 0x80, 0xDC, 0x98, 
	0x80, 0x69, 0x48, 0x8F, 0x01, 0xB0, 0x4D, 0x15, 0x60, 0xCB, 0x5F, 0x05, 0xB8, 0x09, 0xF8, 0x6F, 
	0x80, 0x44, 0xD9, 0x01, 0x18, 0x60, 0x55, 0x40, 0x31, 0x00, 0x08, 0x61, 0x27, 0x2F, 0x1F, 0x03, 
	0x02, 0x1C, 0x85, 0x1F, 0x02, 0xCE, 0xF1, 0x57, 0x84, 0x0E, 0x80, 0x3C, 0xBC, 0x4B, 0xCE, 0xA5, 
	0xDC, 0x06, 0x90, 0x94, 0xBD, 0xFB, 0xBA, 0x14, 0xAF, 0x07, 0x70, 0x4E, 0xBF, 0x8B, 0x45, 0x5F, 
	0x60, 0xE0, 0xB4, 0x80, 0x78, 0x75, 0xD3, 0xD6, 0x68, 0xA4, 0xB5, 0x87, 0x26, 0x80, 0xD3, 0x09, 
	0xE2, 0x76, 0x5B, 0x5E, 0x4F, 0x1A, 0xDF, 0x03, 0x88, 0xFD, 0x7E, 0xC9, 0x9C, 0x87, 0x5A, 0x40, 
	0x9C, 0x48, 0x8F, 0x24, 0x03, 0x20, 0x85, 0xB0, 0x13, 0xC4, 0x74, 0x3F, 0x3B, 0x75, 0x0E, 0xEA, 
	0x4C, 0xEE, 0xB9, 0xEB, 0xA0, 0x78, 0xE2, 0xE6, 0x18, 0x5B, 0x8D, 0xBF, 0xD4, 0x04, 0xF6, 0x00, 
	0xED, 0x17, 0x47, 0x68, 0xDA, 0x60, 0x0F, 0x68, 0xC6, 0x4B, 0xC7, 0xC9, 0x1A, 0x40, 0x37, 0x5E, 
	0x01, 0x28, 0x16, 0x82, 0x20, 0x64, 0x00, 0x54, 0x13, 0xE3, 0xF5, 0xBC, 0x1A, 0x20, 0x6C, 0x04, 
	0x60, 0x37, 0x5E, 0x0F, 0x68, 0xA5, 0x58, 0x0B, 0xF0, 0x76, 0x80, 0xF8, 0x45, 0x68, 0x07, 0x1C, 
	0x6A, 0xC2, 0xE2, 0x03, 0xEA, 0xC6, 0x2B, 0x00, 0x08, 0xDD, 0x2C, 0xAA, 0x4F, 0x61, 0x02, 0x7E, 
	0x0A, 0x58, 0x5A, 0x33, 0x96, 0xF6, 0x6C, 0x76, 0x22, 0x0F, 0x38, 0xB1, 0xAF, 0x09, 0x98, 0x80, 
	0xAF, 0x00, 0x68, 0xF4, 0x73, 0xFF, 0x51, 0x7F, 0xAE, 0x43, 0x80, 0x31, 0xD1, 0x4B, 0x80, 0x01, 
	0x00, 0x46, 0xB0, 0x2A, 0x14, 0x90, 0xB0, 0xDF, 0x76, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4E, 
	0x44, 0xAE, 0x42, 0x60, 0x82};
	FILE *secretpng = fopen(secretpngpath, "wb");
	if (secretpng == NULL) {
        // fail silently
        if (debug) printf("ERROR: Failed to open '%s' for writing (%s)%s", secretpngpath, strerror(errno), newline);
      return;
    }
    initcheckwrite();
    if (checkwriteandprinterrors(secretpngdata, 1, 357, secretpng, 0, 0, secretpngpath, "creating secret.png") != 0) {
        fclose(secretpng);
      return;
    }
    donecheckwrite("secret.png");
    fclose(secretpng);
  return;
}

void createcheckedpng(char *checkedpngpath) {
    // this function creates the file checked.png (this will be the graphic used for checking off achievements)
    static unsigned char checkedpngdata[810] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52, 
	0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x40, 0x08, 0x06, 0x00, 0x00, 0x00, 0x7B, 0x8E, 0x75, 
	0xAE, 0x00, 0x00, 0x02, 0xF1, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0xEC, 0x58, 0xCF, 0x6B, 0x13, 
	0x41, 0x14, 0x7E, 0x9B, 0x6D, 0xA2, 0x62, 0xCD, 0xA1, 0x45, 0x0C, 0x44, 0x3C, 0x15, 0x14, 0xC1, 
	0xAB, 0x50, 0x30, 0xA7, 0x10, 0xF1, 0x20, 0x8A, 0x45, 0xF1, 0xC7, 0x49, 0xD0, 0x53, 0x41, 0xE8, 
	0xC5, 0x3F, 0xA0, 0x27, 0xB5, 0x52, 0x10, 0x14, 0x8A, 0x27, 0x41, 0x30, 0x50, 0x94, 0x82, 0x22, 
	0x08, 0x4A, 0xF1, 0xA0, 0x82, 0x08, 0x51, 0xB4, 0x28, 0x06, 0x45, 0x51, 0x6C, 0x0D, 0xC6, 0x9F, 
	0x6D, 0x9A, 0xE8, 0xA6, 0xED, 0xF8, 0xCD, 0xDB, 0xE7, 0x66, 0xD3, 0x08, 0xDA, 0xEC, 0x26, 0xB9, 
	0xCC, 0xC0, 0xB7, 0xF3, 0x76, 0x66, 0x77, 0xBF, 0x6F, 0xDE, 0x7B, 0xF3, 0x23, 0xB1, 0x94, 0x52, 
	0xD4, 0xC9, 0x12, 0xA1, 0x0E, 0x17, 0x23, 0xC0, 0x08, 0x30, 0x02, 0x8C, 0x00, 0x23, 0xC0, 0x08, 
	0x30, 0x02, 0x8C, 0x00, 0x8B, 0x02, 0x1E, 0x88, 0x54, 0xC0, 0x0F, 0x44, 0x5A, 0x3B, 0x3A, 0x6B, 
	0x53, 0x47, 0x04, 0x80, 0x38, 0x09, 0xDC, 0xA2, 0x5F, 0x74, 0xAC, 0xED, 0x02, 0x40, 0x9C, 0x41, 
	0xF5, 0x0C, 0xD8, 0x45, 0x0F, 0xA9, 0xD0, 0x56, 0x01, 0x20, 0xDF, 0x87, 0xEA, 0x26, 0xD0, 0x43, 
	0xEF, 0x70, 0x2D, 0xB4, 0x51, 0x00, 0xC8, 0x8F, 0xA0, 0xBA, 0x0A, 0xC4, 0xB8, 0xE1, 0x0D, 0x7F, 
	0xFD, 0x6B, 0x5B, 0x04, 0xC8, 0xC8, 0x2F, 0x03, 0x36, 0x37, 0x7C, 0x01, 0x66, 0x59, 0xCA, 0x8F, 
	0x7F, 0xBD, 0xDB, 0x15, 0x02, 0xF9, 0xB6, 0x3A, 0x72, 0x5D, 0x66, 0x80, 0xD5, 0x6C, 0x75, 0xB7, 
	0x5A, 0x40, 0x0F, 0x30, 0xD1, 0x40, 0xF4, 0x1D, 0x58, 0xC3, 0xD6, 0xFA, 0x56, 0x87, 0xE0, 0x12, 
	0xD0, 0x57, 0xD7, 0xF2, 0x00, 0xF8, 0x26, 0x43, 0xEB, 0xA6, 0xED, 0xAD, 0x5C, 0x09, 0x8F, 0x8A, 
	0x80, 0x5A, 0x19, 0x07, 0x5E, 0x48, 0x30, 0x36, 0x00, 0x71, 0xCA, 0xAB, 0xC3, 0x6A, 0x4B, 0x2B, 
	0x3C, 0x90, 0x04, 0x46, 0xEB, 0x5A, 0x74, 0xFE, 0x3F, 0x06, 0xE6, 0x81, 0x4F, 0xC0, 0x14, 0x65, 
	0x29, 0x47, 0x03, 0x41, 0x72, 0x40, 0x4F, 0x27, 0x87, 0xE6, 0x70, 0x5D, 0xD7, 0xD0, 0x37, 0x2A, 
	0xF1, 0x77, 0xCB, 0x6D, 0x59, 0x7A, 0x96, 0x18, 0x25, 0xF8, 0xF5, 0xB8, 0x3A, 0xAF, 0xC6, 0x9B, 
	0xDD, 0x0B, 0x74, 0x42, 0x9D, 0x03, 0x26, 0x39, 0x99, 0x9C, 0x86, 0xFE, 0x7E, 0xE0, 0xA0, 0x77, 
	0xF7, 0x44, 0x46, 0xEE, 0x70, 0x40, 0x0B, 0x70, 0x7F, 0xEA, 0x7F, 0xC9, 0xFF, 0xE6, 0x81, 0xFD, 
	0x42, 0x9E, 0xE4, 0xBB, 0x0A, 0x65, 0xF0, 0xC4, 0x9D, 0x65, 0xCF, 0x8C, 0x78, 0xD6, 0x07, 0xE0, 
	0xBE, 0x64, 0x3D, 0xD1, 0x7B, 0x08, 0x48, 0xAB, 0x11, 0xF5, 0xBA, 0x99, 0xDD, 0x50, 0xEF, 0x5A, 
	0xD7, 0x25, 0x92, 0x49, 0x5F, 0xFF, 0x30, 0x2D, 0xD6, 0x3D, 0xBF, 0x07, 0xD8, 0xC1, 0x96, 0x76, 
	0xF7, 0x5D, 0x60, 0x9A, 0xEF, 0x8A, 0x4C, 0x7E, 0x66, 0x65, 0xE4, 0x7F, 0x04, 0x9C, 0x04, 0x9E, 
	0xCB, 0xC7, 0x6B, 0x65, 0x96, 0x5D, 0xDA, 0x8F, 0x59, 0x92, 0x91, 0x16, 0x9D, 0xDB, 0xA7, 0xBC, 
	0x7E, 0xED, 0x97, 0x3C, 0x5B, 0x7A, 0xB5, 0x4B, 0xAB, 0xD3, 0x2B, 0x27, 0x77, 0x05, 0xCC, 0x50, 
	0x02, 0x59, 0x6B, 0x33, 0xE1, 0x9C, 0x60, 0x01, 0xF8, 0xE9, 0xF9, 0x67, 0x58, 0x9E, 0xD5, 0x6B, 
	0xFD, 0x56, 0xB6, 0xF4, 0x16, 0xE3, 0xD2, 0x39, 0x10, 0x39, 0x00, 0xF2, 0xA9, 0xE6, 0x57, 0xD2, 
	0x09, 0xBE, 0x26, 0x68, 0x15, 0x5D, 0x64, 0x2F, 0x68, 0xD2, 0x04, 0xD0, 0x2B, 0x19, 0x12, 0x65, 
	0x7B, 0xA7, 0xE4, 0x86, 0x2B, 0x20, 0x2B, 0xC9, 0x47, 0x34, 0xA8, 0xCE, 0xAA, 0xB1, 0x60, 0x27, 
	0xA2, 0x28, 0xAF, 0xDB, 0x7A, 0x4C, 0x7B, 0x81, 0x13, 0x68, 0xA9, 0x50, 0x59, 0x3C, 0xA0, 0x44, 
	0x08, 0xD1, 0x15, 0x8F, 0x5C, 0x27, 0xDD, 0x2B, 0xB6, 0xC6, 0x04, 0x21, 0x1C, 0xC9, 0x6C, 0x99, 
	0xF5, 0x71, 0xBA, 0x80, 0x49, 0x98, 0x82, 0x5D, 0x84, 0x0C, 0xFF, 0xF6, 0xE2, 0xAE, 0xE9, 0x2F, 
	0x81, 0x1C, 0x3B, 0x3E, 0x07, 0xD1, 0x43, 0xB4, 0x36, 0x8C, 0x53, 0x71, 0x54, 0x46, 0x6A, 0x8B, 
	0x9C, 0x18, 0x53, 0xA4, 0xA8, 0x8A, 0x69, 0xA5, 0xB7, 0xD5, 0x12, 0xB0, 0x28, 0xA3, 0xBE, 0x07, 
	0x7C, 0xE6, 0xA4, 0x3B, 0x80, 0x59, 0xE0, 0x70, 0xAE, 0x04, 0x2C, 0x5D, 0x88, 0xBF, 0x1B, 0xE7, 
	0x98, 0xD4, 0x6E, 0x66, 0xE4, 0x01, 0x2D, 0x62, 0x12, 0x27, 0x9B, 0x3E, 0x3E, 0x56, 0x3C, 0x05, 
	0x3E, 0xF2, 0xF4, 0x1B, 0x84, 0xD0, 0xB7, 0x72, 0x24, 0x0E, 0xC9, 0x03, 0x31, 0x81, 0xE5, 0x83, 
	0x0D, 0x0F, 0xD8, 0x94, 0x86, 0x5D, 0xE4, 0xFB, 0x25, 0x26, 0xBC, 0x86, 0x37, 0xB2, 0xFC, 0x8E, 
	0x15, 0xD6, 0x0F, 0x93, 0x5E, 0x39, 0x3C, 0xD4, 0x93, 0xBB, 0xB0, 0x78, 0x75, 0xDB, 0x0D, 0xC2, 
	0x0A, 0x66, 0x49, 0x09, 0x22, 0x87, 0x50, 0xD7, 0x66, 0x87, 0x1D, 0x46, 0x08, 0x36, 0xCA, 0x11, 
	0x6A, 0xDE, 0x97, 0x0B, 0xB6, 0x4F, 0x4C, 0x99, 0x1E, 0x21, 0xEA, 0x87, 0x10, 0xF1, 0xCD, 0xF0, 
	0xC2, 0x34, 0xB7, 0x2D, 0xF8, 0x84, 0x06, 0x16, 0x10, 0xF7, 0x7D, 0xAC, 0xBC, 0xCC, 0x13, 0x55, 
	0x59, 0x11, 0x0B, 0x74, 0x83, 0xEB, 0xAA, 0xB4, 0x47, 0xBC, 0x9F, 0x45, 0xC1, 0xCF, 0x93, 0xE6, 
	0xCF, 0x6A, 0x23, 0xC0, 0x08, 0x30, 0x02, 0x8C, 0x00, 0x23, 0xC0, 0x08, 0x30, 0x02, 0x8C, 0x80, 
	0x4E, 0x0B, 0xF8, 0x2D, 0xC0, 0x00, 0xED, 0x71, 0xBB, 0xB0, 0xA1, 0x92, 0x04, 0xA6, 0x00, 0x00, 
	0x00, 0x00, 0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82};
	FILE *checkedpng = fopen(checkedpngpath, "wb");
	if (checkedpng == NULL) {
        // fail silently
        if (debug) printf("ERROR: Failed to open '%s' for writing (%s)%s", checkedpngpath, strerror(errno), newline);
      return;
    }
    initcheckwrite();
    if (checkwriteandprinterrors(checkedpngdata, 1, 810, checkedpng, 0, 0, checkedpngpath, "creating checked.png") != 0) {
        fclose(checkedpng);
      return;
    }
    donecheckwrite("checked.png");
    fclose(checkedpng);
  return;
}

void docheckdirectories() {
    #ifdef WIN32
        char *dirdelimiter = "\\";
    #else
        char *dirdelimiter = "/";
    #endif
    char dirbuffer[2048];
    memset(dirbuffer, 0, 2048);
    // check for homedir
    strcat(dirbuffer, homedir);
    // stat will fail if a dir ends with a slash (at least on windows)
    if (dirbuffer[strlen(dirbuffer) - 1] == '\\' || dirbuffer[strlen(dirbuffer) - 1] == '/') {
        dirbuffer[strlen(dirbuffer) - 1] = 0x0;
    }
    if (stat(dirbuffer, &buf) == -1) {
        if (debug) printf("stat failed for '%s' (%s)%s", dirbuffer, strerror(errno), newline);
        if (mkdir(dirbuffer, 0777) == -1) {
            printf("ERROR: Failed to create the apparently missing home directory '%s' (%s) "
                    "abgx360 will now use the current working directory to store/retrieve files%s",
                    dirbuffer, strerror(errno), newline);
            homeless = true;
          return;
        }
    }
    // check for homedir/abgxdir
    if (abgxdir[0] != '\\' && abgxdir[0] != '/') strcat(dirbuffer, dirdelimiter);
    strcat(dirbuffer, abgxdir);
    if (dirbuffer[strlen(dirbuffer) - 1] == '\\' || dirbuffer[strlen(dirbuffer) - 1] == '/') {
        dirbuffer[strlen(dirbuffer) - 1] = 0x0;
    }
    if (stat(dirbuffer, &buf) == -1) {
        if (debug) printf("stat failed for '%s' (%s)%s", dirbuffer, strerror(errno), newline);
        if (mkdir(dirbuffer, 0777) == -1) {
            printf("ERROR: Failed to create the apparently missing abgx360 directory '%s' (%s) "
                    "abgx360 will now use the current working directory to store/retrieve files%s",
                    dirbuffer, strerror(errno), newline);
            homeless = true;
          return;
        }
    }
    // check for homedir/abgxdir/stealthdir
    if (stealthdir[0] != '\\' && stealthdir[0] != '/') strcat(dirbuffer, dirdelimiter);
    strcat(dirbuffer, stealthdir);
    if (dirbuffer[strlen(dirbuffer) - 1] == '\\' || dirbuffer[strlen(dirbuffer) - 1] == '/') {
        dirbuffer[strlen(dirbuffer) - 1] = 0x0;
    }
    if (stat(dirbuffer, &buf) == -1) {
        if (debug) printf("stat failed for '%s' (%s)%s", dirbuffer, strerror(errno), newline);
        if (mkdir(dirbuffer, 0777) == -1) {
            printf("ERROR: Failed to create the apparently missing StealthFiles directory '%s' (%s) "
                    "abgx360 will now use the current working directory to store/retrieve files%s",
                    dirbuffer, strerror(errno), newline);
            homeless = true;
          return;
        }
    }
    if (autoupload && !stayoffline && autouploaduserarg && autouploadpassarg) {
        // check for homedir/abgxdir/userstealthdir
        memset(dirbuffer, 0, 2048);
        strcat(dirbuffer, homedir);
        strcat(dirbuffer, abgxdir);
        strcat(dirbuffer, userstealthdir);
        if (dirbuffer[strlen(dirbuffer) - 1] == '\\' || dirbuffer[strlen(dirbuffer) - 1] == '/') {
            dirbuffer[strlen(dirbuffer) - 1] = 0x0;
        }
        if (stat(dirbuffer, &buf) == -1) {
            if (debug) printf("stat failed for '%s' (%s)%s", dirbuffer, strerror(errno), newline);
            if (mkdir(dirbuffer, 0777) == -1) {
                printf("ERROR: Failed to create the apparently missing UserStealthFiles directory '%s' (%s) "
                       "abgx360 will now use the current working directory to store/retrieve files%s",
                        dirbuffer, strerror(errno), newline);
                homeless = true;
              return;
            }
        }
    }
    if (extractimages) {
        // check for homedir/abgxdir/imagedir
        memset(dirbuffer, 0, 2048);
        strcat(dirbuffer, homedir);
        strcat(dirbuffer, abgxdir);
        strcat(dirbuffer, imagedir);
        if (dirbuffer[strlen(dirbuffer) - 1] == '\\' || dirbuffer[strlen(dirbuffer) - 1] == '/') {
            dirbuffer[strlen(dirbuffer) - 1] = 0x0;
        }
        if (stat(dirbuffer, &buf) == -1) {
            if (debug) printf("stat failed for '%s' (%s)%s", dirbuffer, strerror(errno), newline);
            if (mkdir(dirbuffer, 0777) == -1) {
                printf("ERROR: Failed to create the apparently missing Images directory '%s' (%s) "
                       "Extracting images will be disabled%s",
                        dirbuffer, strerror(errno), newline);
                extractimages = false;
                imagedirmissing = true;
              return;
            }
            else {
                // image dir was just created so create secret.png and checked.png
                strcat(dirbuffer, dirdelimiter);
                strcat(dirbuffer, "secret.png");
                createsecretpng(dirbuffer);
                dirbuffer[strlen(dirbuffer) - 10] = 0x0;
                strcat(dirbuffer, "checked.png");
                createcheckedpng(dirbuffer);
            }
        }
        else {
            // image dir exists so check for secret.png and checked.png and create them if they don't exist
            strcat(dirbuffer, dirdelimiter);
            strcat(dirbuffer, "secret.png");
            if (stat(dirbuffer, &buf) == -1) {
                if (debug) printf("stat failed for '%s' (%s)%s", dirbuffer, strerror(errno), newline);
                createsecretpng(dirbuffer);
            }
            dirbuffer[strlen(dirbuffer) - 10] = 0x0;
            strcat(dirbuffer, "checked.png");
            if (stat(dirbuffer, &buf) == -1) {
                if (debug) printf("stat failed for '%s' (%s)%s", dirbuffer, strerror(errno), newline);
                createcheckedpng(dirbuffer);
            }
        }
    }
    if (debug) printf("all necessary directories were created or already existed%s", newline);
  return;
}

void printbadgamecrcerror() {
    color(red);
    printf("Game partition CRC does not match the verified ini! There are 4 possibilities:%s%s", newline, newline);
    color(normal);
    printf("1. This is a bad rip (the most likely cause). If you're checking a DVD backup%s"
           "and have AnyDVD or a similar app installed, make absolutely sure the process is%s"
           "killed! If this is a scene release, search for it on abgx.net and look at the%s"
           "column \"Bad CRC or Needs Fixing\". If you see a %s!%s icon, hover over it with%s"
           "your mouse to read the alt text. If it says something like \"Bad game data CRC\"%s"
           "or gives the name of a fix (PPF patch), we already know about it. It's also%s"
           "highly recommended to run abgx360 again with AutoFix set to level 3 if you%s"
           "applied a PPF to make sure stealth files haven't been corrupted. See \"What is%s"
           "AnyDVD style corruption?\" in the GUI Quickstart tab for more info on game data%s"
           "corruption.%s%s"
           "2. Your CPU/RAM/HDD/data bus is unstable and corrupted the data during%s"
           "extraction/burning or while checking the CRC. Make sure to run the sfv and try%s"
           "extracting it again (assuming you have the sfv/rars) and recheck the ISO/DVD to%s"
           "see if you get the same CRC.%s%s"
           "3. Game data was intentionally modified for some reason. Microsoft does not%s"
           "take kindly to modifications for any purpose, and even benign changes to game%s"
           "data or settings will get you banned from Xbox Live!%s%s"
           "4. The verified Game CRC in the database is wrong (very unlikely).%s%s",
           newline, newline, newline, lessthan, greaterthan, newline, newline, newline, newline, newline, newline, newline,
           newline, newline, newline, newline, newline, newline, newline, newline, newline, newline, newline, newline);
  return;
}

char* cdberror(char *sense) {
    int i;
    int match = -1;
    struct { char *sensecode; char *string; } error[249];
    error[0].sensecode = "00/00/00"; error[0].string = "NO ADDITIONAL SENSE INFORMATION";
    error[1].sensecode = "00/04/04"; error[1].string = "LOGICAL UNIT NOT READY, FORMAT IN PROGRESS";
    error[2].sensecode = "00/34/00"; error[2].string = "ENCLOSURE FAILURE";
    error[3].sensecode = "00/35/00"; error[3].string = "ENCLOSURE SERVICES FAILURE";
    error[4].sensecode = "00/35/01"; error[4].string = "UNSUPPORTED ENCLOSURE FUNCTION";
    error[5].sensecode = "00/35/02"; error[5].string = "ENCLOSURE SERVICES UNAVAILABLE";
    error[6].sensecode = "00/35/03"; error[6].string = "ENCLOSURE SERVICES TRANSFER FAILURE";
    error[7].sensecode = "00/35/04"; error[7].string = "ENCLOSURE SERVICES TRANSFER REFUSED";
    error[8].sensecode = "00/35/05"; error[8].string = "ENCLOSURE SERVICES CHECKSUM ERROR";
    error[9].sensecode = "01/0B/00"; error[9].string = "WARNING";
    error[10].sensecode = "01/0B/01"; error[10].string = "WARNING - SPECIFIED TEMPERATURE EXCEEDED";
    error[11].sensecode = "01/0B/02"; error[11].string = "WARNING - ENCLOSURE DEGRADED";
    error[12].sensecode = "01/0B/03"; error[12].string = "WARNING - BACKGROUND SELF-TEST FAILED";
    error[13].sensecode = "01/0B/04"; error[13].string = "WARNING - BACKGROUND PRE-SCAN DETECTED MEDIUM ERROR";
    error[14].sensecode = "01/0B/05"; error[14].string = "WARNING - BACKGROUND MEDIUM SCAN DETECTED MEDIUM ERROR";
    error[15].sensecode = "01/0C/01"; error[15].string = "WRITE ERROR - RECOVERED WITH AUTO-REALLOCATION";
    error[16].sensecode = "01/17/00"; error[16].string = "RECOVERED DATA WITH NO ERROR CORRECTION APPLIED";
    error[17].sensecode = "01/17/01"; error[17].string = "RECOVERED DATA WITH RETRIES";
    error[18].sensecode = "01/17/02"; error[18].string = "RECOVERED DATA WITH POSITIVE HEAD OFFSET";
    error[19].sensecode = "01/17/03"; error[19].string = "RECOVERED DATA WITH NEGATIVE HEAD OFFSET";
    error[20].sensecode = "01/17/04"; error[20].string = "RECOVERED DATA WITH RETRIES AND/OR CIRC APPLIED";
    error[21].sensecode = "01/17/05"; error[21].string = "RECOVERED DATA USING PREVIOUS SECTOR ID";
    error[22].sensecode = "01/17/07"; error[22].string = "RECOVERED DATA WITHOUT ECC - RECOMMEND REASSIGNMENT";
    error[23].sensecode = "01/17/08"; error[23].string = "RECOVERED DATA WITHOUT ECC - RECOMMEND REWRITE";
    error[24].sensecode = "01/17/09"; error[24].string = "RECOVERED DATA WITHOUT ECC - DATA REWRITTEN";
    error[25].sensecode = "01/18/00"; error[25].string = "RECOVERED DATA WITH ERROR CORRECTION APPLIED";
    error[26].sensecode = "01/18/01"; error[26].string = "RECOVERED DATA WITH ERROR CORR. & RETRIES APPLIED";
    error[27].sensecode = "01/18/02"; error[27].string = "RECOVERED DATA - DATA AUTO-REALLOCATED";
    error[28].sensecode = "01/18/03"; error[28].string = "RECOVERED DATA WITH CIRC";
    error[29].sensecode = "01/18/04"; error[29].string = "RECOVERED DATA WITH L-EC";
    error[30].sensecode = "01/18/05"; error[30].string = "RECOVERED DATA - RECOMMEND REASSIGNMENT";
    error[31].sensecode = "01/18/06"; error[31].string = "RECOVERED DATA - RECOMMEND REWRITE";
    error[32].sensecode = "01/18/08"; error[32].string = "RECOVERED DATA WITH LINKING";
    error[33].sensecode = "01/37/00"; error[33].string = "ROUNDED PARAMETER";
    error[34].sensecode = "01/5D/00"; error[34].string = "FAILURE PREDICTION THRESHOLD EXCEEDED";
    error[35].sensecode = "01/5D/01"; error[35].string = "MEDIA FAILURE PREDICTION THRESHOLD EXCEEDED";
    error[36].sensecode = "01/5D/02"; error[36].string = "LOGICAL UNIT FAILURE PREDICTION THRESHOLD EXCEEDED";
    error[37].sensecode = "01/5D/03"; error[37].string = "SPARE AREA EXHAUSTION FAILURE PREDICTION THRESHOLD EXCEEDED";
    error[38].sensecode = "01/5D/FF"; error[38].string = "FAILURE PREDICTION THRESHOLD EXCEEDED (FALSE)";
    error[39].sensecode = "01/73/01"; error[39].string = "POWER CALIBRATION AREA ALMOST FULL";
    error[40].sensecode = "01/73/06"; error[40].string = "RMA/PMA IS ALMOST FULL";
    error[41].sensecode = "02/04/00"; error[41].string = "LOGICAL UNIT NOT READY, CAUSE NOT REPORTABLE";
    error[42].sensecode = "02/04/01"; error[42].string = "LOGICAL UNIT IS IN PROCESS OF BECOMING READY";
    error[43].sensecode = "02/04/02"; error[43].string = "LOGICAL UNIT NOT READY, INITIALIZING CMD. REQUIRED";
    error[44].sensecode = "02/04/03"; error[44].string = "LOGICAL UNIT NOT READY, MANUAL INTERVENTION REQUIRED";
    error[45].sensecode = "02/04/04"; error[45].string = "LOGICAL UNIT NOT READY, FORMAT IN PROGRESS";
    error[46].sensecode = "02/04/07"; error[46].string = "LOGICAL UNIT NOT READY, OPERATION IN PROGRESS";
    error[47].sensecode = "02/04/08"; error[47].string = "LOGICAL UNIT NOT READY, LONG WRITE IN PROGRESS";
    error[48].sensecode = "02/04/09"; error[48].string = "LOGICAL UNIT NOT READY, SELF-TEST IN PROGRESS";
    error[49].sensecode = "02/0C/07"; error[49].string = "WRITE ERROR - RECOVERY NEEDED";
    error[50].sensecode = "02/0C/0F"; error[50].string = "DEFECTS IN ERROR WINDOW";
    error[51].sensecode = "02/30/00"; error[51].string = "INCOMPATIBLE MEDIUM INSTALLED";
    error[52].sensecode = "02/30/01"; error[52].string = "CANNOT READ MEDIUM - UNKNOWN FORMAT";
    error[53].sensecode = "02/30/02"; error[53].string = "CANNOT READ MEDIUM - INCOMPATIBLE FORMAT";
    error[54].sensecode = "02/30/03"; error[54].string = "CLEANING CARTRIDGE INSTALLED";
    error[55].sensecode = "02/30/04"; error[55].string = "CANNOT WRITE MEDIUM - UNKNOWN FORMAT";
    error[56].sensecode = "02/30/05"; error[56].string = "CANNOT WRITE MEDIUM - INCOMPATIBLE FORMAT";
    error[57].sensecode = "02/30/06"; error[57].string = "CANNOT FORMAT MEDIUM - INCOMPATIBLE MEDIUM";
    error[58].sensecode = "02/30/07"; error[58].string = "CLEANING FAILURE";
    error[59].sensecode = "02/30/11"; error[59].string = "CANNOT WRITE MEDIUM - UNSUPPORTED MEDIUM VERSION";
    error[60].sensecode = "02/3A/00"; error[60].string = "MEDIUM NOT PRESENT";
    error[61].sensecode = "02/3A/01"; error[61].string = "MEDIUM NOT PRESENT - TRAY CLOSED";
    error[62].sensecode = "02/3A/02"; error[62].string = "MEDIUM NOT PRESENT - TRAY OPEN";
    error[63].sensecode = "02/3A/03"; error[63].string = "MEDIUM NOT PRESENT - LOADABLE";
    error[64].sensecode = "02/3E/00"; error[64].string = "LOGICAL UNIT HAS NOT SELF-CONFIGURED YET";
    error[65].sensecode = "03/02/00"; error[65].string = "NO SEEK COMPLETE";
    error[66].sensecode = "03/06/00"; error[66].string = "NO REFERENCE POSITION FOUND";
    error[67].sensecode = "03/0C/00"; error[67].string = "WRITE ERROR";
    error[68].sensecode = "03/0C/02"; error[68].string = "WRITE ERROR - AUTO-REALLOCATION FAILED";
    error[69].sensecode = "03/0C/03"; error[69].string = "WRITE ERROR - RECOMMEND REASSIGNMENT";
    error[70].sensecode = "03/0C/07"; error[70].string = "WRITE ERROR - RECOVERY NEEDED";
    error[71].sensecode = "03/0C/08"; error[71].string = "WRITE ERROR - RECOVERY FAILED";
    error[72].sensecode = "03/0C/09"; error[72].string = "WRITE ERROR - LOSS OF STREAMING";
    error[73].sensecode = "03/0C/0A"; error[73].string = "WRITE ERROR - PADDING BLOCKS ADDED";
    error[74].sensecode = "03/11/00"; error[74].string = "UNRECOVERED READ ERROR";
    error[75].sensecode = "03/11/01"; error[75].string = "READ RETRIES EXHAUSTED";
    error[76].sensecode = "03/11/02"; error[76].string = "ERROR TOO LONG TO CORRECT";
    error[77].sensecode = "03/11/05"; error[77].string = "L-EC UNCORRECTABLE ERROR";
    error[78].sensecode = "03/11/06"; error[78].string = "CIRC UNRECOVERED ERROR";
    error[79].sensecode = "03/11/0F"; error[79].string = "ERROR READING UPC/EAN NUMBER";
    error[80].sensecode = "03/11/10"; error[80].string = "ERROR READING ISRC NUMBER";
    error[81].sensecode = "03/15/00"; error[81].string = "RANDOM POSITIONING ERROR";
    error[82].sensecode = "03/15/01"; error[82].string = "MECHANICAL POSITIONING ERROR";
    error[83].sensecode = "03/15/02"; error[83].string = "POSITIONING ERROR DETECTED BY READ OF MEDIUM";
    error[84].sensecode = "03/31/00"; error[84].string = "MEDIUM FORMAT CORRUPTED";
    error[85].sensecode = "03/31/01"; error[85].string = "FORMAT COMMAND FAILED";
    error[86].sensecode = "03/31/02"; error[86].string = "ZONED FORMATTING FAILED DUE TO SPARE LINKING";
    error[87].sensecode = "03/51/00"; error[87].string = "ERASE FAILURE";
    error[88].sensecode = "03/51/01"; error[88].string = "ERASE FAILURE - INCOMPLETE ERASE OPERATION DETECTED";
    error[89].sensecode = "03/57/00"; error[89].string = "UNABLE TO RECOVER TABLE-OF-CONTENTS";
    error[90].sensecode = "03/72/00"; error[90].string = "SESSION FIXATION ERROR";
    error[91].sensecode = "03/72/01"; error[91].string = "SESSION FIXATION ERROR WRITING LEAD-IN";
    error[92].sensecode = "03/72/02"; error[92].string = "SESSION FIXATION ERROR WRITING LEAD-OUT";
    error[93].sensecode = "03/73/00"; error[93].string = "CD CONTROL ERROR";
    error[94].sensecode = "03/73/02"; error[94].string = "POWER CALIBRATION AREA IS FULL";
    error[95].sensecode = "03/73/03"; error[95].string = "POWER CALIBRATION AREA ERROR";
    error[96].sensecode = "03/73/04"; error[96].string = "PROGRAM MEMORY AREA UPDATE FAILURE";
    error[97].sensecode = "03/73/05"; error[97].string = "PROGRAM MEMORY AREA IS FULL";
    error[98].sensecode = "03/73/06"; error[98].string = "RMA/PMA IS ALMOST FULL";
    error[99].sensecode = "03/73/10"; error[99].string = "CURRENT POWER CALIBRATION AREA IS ALMOST FULL";
    error[100].sensecode = "03/73/11"; error[100].string = "CURRENT POWER CALIBRATION AREA IS FULL";
    error[101].sensecode = "04/00/17"; error[101].string = "CLEANING REQUESTED";
    error[102].sensecode = "04/05/00"; error[102].string = "LOGICAL UNIT DOES NOT RESPOND TO SELECTION";
    error[103].sensecode = "04/08/00"; error[103].string = "LOGICAL UNIT COMMUNICATION FAILURE";
    error[104].sensecode = "04/08/01"; error[104].string = "LOGICAL UNIT COMMUNICATION TIMEOUT";
    error[105].sensecode = "04/08/02"; error[105].string = "LOGICAL UNIT COMMUNICATION PARITY ERROR";
    error[106].sensecode = "04/08/03"; error[106].string = "LOGICAL UNIT COMMUNICATION CRC ERROR (ULTRA-DMA/32)";
    error[107].sensecode = "04/09/00"; error[107].string = "TRACK FOLLOWING ERROR";
    error[108].sensecode = "04/09/01"; error[108].string = "TRACKING SERVO FAILURE";
    error[109].sensecode = "04/09/02"; error[109].string = "FOCUS SERVO FAILURE";
    error[110].sensecode = "04/09/03"; error[110].string = "SPINDLE SERVO FAILURE";
    error[111].sensecode = "04/09/04"; error[111].string = "HEAD SELECT FAULT";
    error[112].sensecode = "04/15/00"; error[112].string = "RANDOM POSITIONING ERROR";
    error[113].sensecode = "04/15/01"; error[113].string = "MECHANICAL POSITIONING ERROR";
    error[114].sensecode = "04/1B/00"; error[114].string = "SYNCHRONOUS DATA TRANSFER ERROR";
    error[115].sensecode = "04/3B/16"; error[115].string = "MECHANICAL POSITIONING OR CHANGER ERROR";
    error[116].sensecode = "04/3E/01"; error[116].string = "LOGICAL UNIT FAILURE";
    error[117].sensecode = "04/3E/02"; error[117].string = "TIMEOUT ON LOGICAL UNIT";
    error[118].sensecode = "04/44/00"; error[118].string = "INTERNAL TARGET FAILURE";
    error[119].sensecode = "04/46/00"; error[119].string = "UNSUCCESSFUL SOFT RESET";
    error[120].sensecode = "04/47/00"; error[120].string = "SCSI PARITY ERROR";
    error[121].sensecode = "04/4A/00"; error[121].string = "COMMAND PHASE ERROR";
    error[122].sensecode = "04/4B/00"; error[122].string = "DATA PHASE ERROR";
    error[123].sensecode = "04/4C/00"; error[123].string = "LOGICAL UNIT FAILED SELF-CONFIGURATION";
    error[124].sensecode = "04/53/00"; error[124].string = "MEDIA LOAD OR EJECT FAILED";
    error[125].sensecode = "04/65/00"; error[125].string = "VOLTAGE FAULT";
    error[126].sensecode = "05/00/11"; error[126].string = "AUDIO PLAY OPERATION IN PROGRESS";
    error[127].sensecode = "05/00/12"; error[127].string = "AUDIO PLAY OPERATION PAUSED";
    error[128].sensecode = "05/00/13"; error[128].string = "AUDIO PLAY OPERATION SUCCESSFULLY COMPLETED";
    error[129].sensecode = "05/00/14"; error[129].string = "AUDIO PLAY OPERATION STOPPED DUE TO ERROR";
    error[130].sensecode = "05/00/15"; error[130].string = "NO CURRENT AUDIO STATUS TO RETURN";
    error[131].sensecode = "05/00/16"; error[131].string = "OPERATION IN PROGRESS";
    error[132].sensecode = "05/07/00"; error[132].string = "MULTIPLE PERIPHERAL DEVICES SELECTED";
    error[133].sensecode = "05/1A/00"; error[133].string = "PARAMETER LIST LENGTH ERROR";
    error[134].sensecode = "05/20/00"; error[134].string = "INVALID COMMAND OPERATION CODE";
    error[135].sensecode = "05/21/00"; error[135].string = "LOGICAL BLOCK ADDRESS OUT OF RANGE";
    error[136].sensecode = "05/21/01"; error[136].string = "INVALID ELEMENT ADDRESS";
    error[137].sensecode = "05/21/02"; error[137].string = "INVALID ADDRESS FOR WRITE";
    error[138].sensecode = "05/21/03"; error[138].string = "INVALID WRITE CROSSING LAYER JUMP";
    error[139].sensecode = "05/22/00"; error[139].string = "ILLEGAL FUNCTION";
    error[140].sensecode = "05/24/00"; error[140].string = "INVALID FIELD IN CDB";
    error[141].sensecode = "05/25/00"; error[141].string = "LOGICAL UNIT NOT SUPPORTED";
    error[142].sensecode = "05/26/00"; error[142].string = "INVALID FIELD IN PARAMETER LIST";
    error[143].sensecode = "05/26/01"; error[143].string = "PARAMETER NOT SUPPORTED";
    error[144].sensecode = "05/26/02"; error[144].string = "PARAMETER VALUE INVALID";
    error[145].sensecode = "05/26/03"; error[145].string = "THRESHOLD PARAMETERS NOT SUPPORTED";
    error[146].sensecode = "05/26/04"; error[146].string = "INVALID RELEASE OF PERSISTENT RESERVATION";
    error[147].sensecode = "05/27/00"; error[147].string = "WRITE PROTECTED";
    error[148].sensecode = "05/27/01"; error[148].string = "HARDWARE WRITE PROTECTED";
    error[149].sensecode = "05/27/02"; error[149].string = "LOGICAL UNIT SOFTWARE WRITE PROTECTED";
    error[150].sensecode = "05/27/03"; error[150].string = "ASSOCIATED WRITE PROTECT";
    error[151].sensecode = "05/27/04"; error[151].string = "PERSISTENT WRITE PROTECT";
    error[152].sensecode = "05/27/05"; error[152].string = "PERMANENT WRITE PROTECT";
    error[153].sensecode = "05/2B/00"; error[153].string = "COPY CANNOT EXECUTE SINCE INITIATOR CANNOT DISCONNECT";
    error[154].sensecode = "05/2C/00"; error[154].string = "COMMAND SEQUENCE ERROR";
    error[155].sensecode = "05/2C/03"; error[155].string = "CURRENT PROGRAM AREA IS NOT EMPTY";
    error[156].sensecode = "05/2C/04"; error[156].string = "CURRENT PROGRAM AREA IS EMPTY";
    error[157].sensecode = "05/30/00"; error[157].string = "INCOMPATIBLE MEDIUM INSTALLED";
    error[158].sensecode = "05/30/01"; error[158].string = "CONNOT READ MEDIUM - UNKNOWN FORMAT";
    error[159].sensecode = "05/30/02"; error[159].string = "CANNOT READ MEDIUM - INCOMPATIBLE FORMAT";
    error[160].sensecode = "05/30/03"; error[160].string = "CLEANING CARTRIDGE INSTALLED";
    error[161].sensecode = "05/30/04"; error[161].string = "CANNOT WRITE MEDIUM - UNKNOWN FORMAT";
    error[162].sensecode = "05/30/05"; error[162].string = "CANNOT WRITE MEDIUM - INCOMPATIBLE FORMAT";
    error[163].sensecode = "05/30/06"; error[163].string = "CANNOT FORMAT MEDIUM - INCOMPATIBLE MEDIUM";
    error[164].sensecode = "05/30/07"; error[164].string = "CLEANING FAILURE";
    error[165].sensecode = "05/30/08"; error[165].string = "CANNOT WRITE - APPLICATION CODE MISMATCH";
    error[166].sensecode = "05/30/09"; error[166].string = "CURRENT SESSION NOT FIXATED FOR APPEND";
    error[167].sensecode = "05/30/10"; error[167].string = "MEDIUM NOT FORMATTED";
    error[168].sensecode = "05/30/11"; error[168].string = "CANNOT WRITE MEDIUM - UNSUPPORTED MEDIUM VERSION";
    error[169].sensecode = "05/39/00"; error[169].string = "SAVING PARAMETERS NOT SUPPORTED";
    error[170].sensecode = "05/3D/00"; error[170].string = "INVALID BITS IN IDENTIFY MESSAGE";
    error[171].sensecode = "05/43/00"; error[171].string = "MESSAGE ERROR";
    error[172].sensecode = "05/53/02"; error[172].string = "MEDIUM REMOVAL PREVENTED";
    error[173].sensecode = "05/55/00"; error[173].string = "SYSTEM RESOURCE FAILURE";
    error[174].sensecode = "05/63/00"; error[174].string = "END OF USER AREA ENCOUNTERED ON THIS TRACK";
    error[175].sensecode = "05/63/01"; error[175].string = "PACKET DOES NOT FIT IN AVAILABLE SPACE";
    error[176].sensecode = "05/64/00"; error[176].string = "ILLEGAL MODE FOR THIS TRACK";
    error[177].sensecode = "05/64/01"; error[177].string = "INVALID PACKET SIZE";
    error[178].sensecode = "05/6F/00"; error[178].string = "COPY PROTECTION KEY EXCHANGE FAILURE - AUTHENTICATION FAILURE";
    error[179].sensecode = "05/6F/01"; error[179].string = "COPY PROTECTION KEY EXCHANGE FAILURE - KEY NOT PRESENT";
    error[180].sensecode = "05/6F/02"; error[180].string = "COPY PROTECTION KEY EXCHANGE FAILURE - KEY NOT ESTABLISHED";
    error[181].sensecode = "05/6F/03"; error[181].string = "READ OF SCRAMBLED SECTOR WITHOUT AUTHENTICATION";
    error[182].sensecode = "05/6F/04"; error[182].string = "MEDIA REGION CODE IS MISMATCHED TO LOGICAL UNIT REGION";
    error[183].sensecode = "05/6F/05"; error[183].string = "DRIVE REGION MUST BE PERMANENT/REGION RESET COUNT ERROR";
    error[184].sensecode = "05/6F/06"; error[184].string = "INSUFFICIENT BLOCK COUNT FOR BINDING NONCE RECORDING";
    error[185].sensecode = "05/6F/07"; error[185].string = "CONFLICT IN BINDING NONCE RECORDING";
    error[186].sensecode = "05/72/03"; error[186].string = "SESSION FIXATION ERROR - INCOMPLETE TRACK IN SESSION";
    error[187].sensecode = "05/72/04"; error[187].string = "EMPTY OR PARTIALLY WRITTEN RESERVED TRACK";
    error[188].sensecode = "05/72/05"; error[188].string = "NO MORE TRACK RESERVATIONS ALLOWED";
    error[189].sensecode = "05/72/06"; error[189].string = "RMZ EXTENSION IS NOT ALLOWED";
    error[190].sensecode = "05/72/07"; error[190].string = "NO MORE TEST ZONE EXTENSIONS ARE ALLOWED";
    error[191].sensecode = "05/73/17"; error[191].string = "RDZ IS FULL";
    error[192].sensecode = "06/0A/00"; error[192].string = "ERROR LOG OVERFLOW";
    error[193].sensecode = "06/28/00"; error[193].string = "NOT READY TO READY CHANGE, MEDIUM MAY HAVE CHANGED";
    error[194].sensecode = "06/28/01"; error[194].string = "IMPORT OR EXPORT ELEMENT ACCESSED";
    error[195].sensecode = "06/28/02"; error[195].string = "FORMAT-LAYER MAY HAVE CHANGED";
    error[196].sensecode = "06/29/00"; error[196].string = "POWER ON, RESET, OR BUS DEVICE RESET OCCURRED";
    error[197].sensecode = "06/29/01"; error[197].string = "POWER ON OCCURRED";
    error[198].sensecode = "06/29/02"; error[198].string = "BUS RESET OCCURRED";
    error[199].sensecode = "06/29/03"; error[199].string = "BUS DEVICE RESET FUNCTION OCCURRED";
    error[200].sensecode = "06/29/04"; error[200].string = "DEVICE INTERNAL RESET";
    error[201].sensecode = "06/2A/00"; error[201].string = "PARAMETERS CHANGED";
    error[202].sensecode = "06/2A/01"; error[202].string = "MODE PARAMETERS CHANGED";
    error[203].sensecode = "06/2A/02"; error[203].string = "LOG PARAMETERS CHANGED";
    error[204].sensecode = "06/2A/03"; error[204].string = "RESERVATIONS PREEMPTED";
    error[205].sensecode = "06/2E/00"; error[205].string = "INSUFFICIENT TIME FOR OPERATION";
    error[206].sensecode = "06/2F/00"; error[206].string = "COMMANDS CLEARED BY ANOTHER INITIATOR";
    error[207].sensecode = "06/3B/0D"; error[207].string = "MEDIUM DESTINATION ELEMENT FULL";
    error[208].sensecode = "06/3B/0E"; error[208].string = "MEDIUM SOURCE ELEMENT EMPTY";
    error[209].sensecode = "06/3B/0F"; error[209].string = "END OF MEDIUM REACHED";
    error[210].sensecode = "06/3B/11"; error[210].string = "MEDIUM MAGAZINE NOT ACCESSIBLE";
    error[211].sensecode = "06/3B/12"; error[211].string = "MEDIUM MAGAZINE REMOVED";
    error[212].sensecode = "06/3B/13"; error[212].string = "MEDIUM MAGAZINE INSERTED";
    error[213].sensecode = "06/3B/14"; error[213].string = "MEDIUM MAGAZINE LOCKED";
    error[214].sensecode = "06/3B/15"; error[214].string = "MEDIUM MAGAZINE UNLOCKED";
    error[215].sensecode = "06/3F/00"; error[215].string = "TARGET OPERATING CONDITIONS HAVE CHANGED";
    error[216].sensecode = "06/3F/01"; error[216].string = "MICROCODE HAS BEEN CHANGED";
    error[217].sensecode = "06/3F/02"; error[217].string = "CHANGED OPERATING DEFINITION";
    error[218].sensecode = "06/3F/03"; error[218].string = "INQUIRY DATA HAS CHANGED";
    error[219].sensecode = "06/5A/00"; error[219].string = "OPERATOR REQUEST OR STATE CHANGE INPUT";
    error[220].sensecode = "06/5A/01"; error[220].string = "OPERATOR MEDIUM REMOVAL REQUEST";
    error[221].sensecode = "06/5A/02"; error[221].string = "OPERATOR SELECTED WRITE PROTECT";
    error[222].sensecode = "06/5A/03"; error[222].string = "OPERATOR SELECTED WRITE PERMIT";
    error[223].sensecode = "06/5B/00"; error[223].string = "LOG EXCEPTION";
    error[224].sensecode = "06/5B/01"; error[224].string = "THRESHOLD CONDITION MET";
    error[225].sensecode = "06/5B/02"; error[225].string = "LOG COUNTER AT MAXIMUM";
    error[226].sensecode = "06/5B/03"; error[226].string = "LOG LIST CODES EXHAUSTED";
    error[227].sensecode = "06/5D/00"; error[227].string = "FAILURE PREDICTION THRESHOLD EXCEEDED";
    error[228].sensecode = "06/5D/FF"; error[228].string = "FAILURE PREDICTION THRESHOLD EXCEEDED (FALSE)";
    error[229].sensecode = "06/5E/00"; error[229].string = "LOW POWER CONDITION ON";
    error[230].sensecode = "06/5E/01"; error[230].string = "IDLE CONDITION ACTIVATED BY TIMER";
    error[231].sensecode = "06/5E/02"; error[231].string = "STANDBY CONDITION ACTIVATED BY TIMER";
    error[232].sensecode = "06/5E/03"; error[232].string = "IDLE CONDITION ACTIVATED BY COMMAND";
    error[233].sensecode = "06/5E/04"; error[233].string = "STANDBY CONDITION ACTIVATED BY COMMAND";
    error[234].sensecode = "07/27/00"; error[234].string = "WRITE PROTECTED";
    error[235].sensecode = "07/27/01"; error[235].string = "HARDWARE WRITE PROTECTED";
    error[236].sensecode = "07/27/02"; error[236].string = "LOGICAL UNIT SOFTWARE WRITE PROTECTED";
    error[237].sensecode = "07/27/03"; error[237].string = "ASSOCIATED WRITE PROTECT";
    error[238].sensecode = "07/27/04"; error[238].string = "PERSISTENT WRITE PROTECT";
    error[239].sensecode = "07/27/05"; error[239].string = "PERMANENT WRITE PROTECT";
    error[240].sensecode = "07/27/06"; error[240].string = "CONDITIONAL WRITE PROTECT";
    error[241].sensecode = "0A/1D/00"; error[241].string = "MISCOMPARE DURING VERIFY OPERATION";
    error[242].sensecode = "0B/00/06"; error[242].string = "I/O PROCESS TERMINATED";
    error[243].sensecode = "0B/11/11"; error[243].string = "READ ERROR - LOSS OF STREAMING";
    error[244].sensecode = "0B/45/00"; error[244].string = "SELECT OR RESELECT FAILURE";
    error[245].sensecode = "0B/48/00"; error[245].string = "INITIATOR DETECTED ERROR MESSAGE RECEIVED";
    error[246].sensecode = "0B/49/00"; error[246].string = "INVALID MESSAGE ERROR";
    error[247].sensecode = "0B/4E/00"; error[247].string = "OVERLAPPED COMMANDS ATTEMPTED";
    error[248].sensecode = "UU/OO/SS"; error[248].string = "Unsupported Operating System";
/*
    // increase for loop iterations and error[249] if adding more sense errors
    error[249].sensecode = ""; error[249].string = "";
    error[250].sensecode = ""; error[250].string = "";
    error[251].sensecode = ""; error[251].string = "";
    error[252].sensecode = ""; error[252].string = "";
    error[253].sensecode = ""; error[253].string = "";
    error[254].sensecode = ""; error[254].string = "";
    error[255].sensecode = ""; error[255].string = "";
    error[256].sensecode = ""; error[256].string = "";
    error[257].sensecode = ""; error[257].string = "";
    error[258].sensecode = ""; error[258].string = "";
    error[259].sensecode = ""; error[259].string = "";
    error[260].sensecode = ""; error[260].string = "";
    error[261].sensecode = ""; error[261].string = "";
    error[262].sensecode = ""; error[262].string = "";
    error[263].sensecode = ""; error[263].string = "";
    error[264].sensecode = ""; error[264].string = "";
    error[265].sensecode = ""; error[265].string = "";
    error[266].sensecode = ""; error[266].string = "";
    error[267].sensecode = ""; error[267].string = "";
    error[268].sensecode = ""; error[268].string = "";
    error[269].sensecode = ""; error[269].string = "";
    error[270].sensecode = ""; error[270].string = "";
    error[271].sensecode = ""; error[271].string = "";
    error[272].sensecode = ""; error[272].string = "";
    error[273].sensecode = ""; error[273].string = "";
    error[274].sensecode = ""; error[274].string = "";
    error[275].sensecode = ""; error[275].string = "";
    error[276].sensecode = ""; error[276].string = "";
    error[277].sensecode = ""; error[277].string = "";
    error[278].sensecode = ""; error[278].string = "";
    error[279].sensecode = ""; error[279].string = "";
    error[280].sensecode = ""; error[280].string = "";
    error[281].sensecode = ""; error[281].string = "";
    error[282].sensecode = ""; error[282].string = "";
    error[283].sensecode = ""; error[283].string = "";
    error[284].sensecode = ""; error[284].string = "";
    error[285].sensecode = ""; error[285].string = "";
    error[286].sensecode = ""; error[286].string = "";
    error[287].sensecode = ""; error[287].string = "";
    error[288].sensecode = ""; error[288].string = "";
    error[289].sensecode = ""; error[289].string = "";
    error[290].sensecode = ""; error[290].string = "";
    error[291].sensecode = ""; error[291].string = "";
    error[292].sensecode = ""; error[292].string = "";
    error[293].sensecode = ""; error[293].string = "";
    error[294].sensecode = ""; error[294].string = "";
    error[295].sensecode = ""; error[295].string = "";
    error[296].sensecode = ""; error[296].string = "";
    error[297].sensecode = ""; error[297].string = "";
    error[298].sensecode = ""; error[298].string = "";
    error[299].sensecode = ""; error[299].string = "";
    error[300].sensecode = ""; error[300].string = "";
*/
    
    if (memcmp(sense, "04/40/", 6) == 0) {
        // 04/40/NN  DIAGNOSTIC FAILURE ON COMPONENT NN (80H-FFH)
        sprintf(specialerror, "DIAGNOSTIC FAILURE ON COMPONENT %c%c", sense[6], sense[7]);
      return specialerror;
    }
    if (memcmp(sense, "0B/4D/", 6) == 0) {
        // 0B/4D/NN  TAGGED OVERLAPPED COMMANDS (NN = QUEUE TAG)
        sprintf(specialerror, "TAGGED OVERLAPPED COMMANDS ON QUEUE TAG %c%c", sense[6], sense[7]);
      return specialerror;
    }
    for (i=0;i<249;i++) {  // increase iterations if adding more sense errors
        if (memcmp(sense, error[i].sensecode, 8) == 0) {
            match = i;
            break;
        }
    }
    if (match == -1) {
        // no matching sensecode was found
        sprintf(specialerror, "Unrecognized Sense Error: %s", sense);
      return specialerror;
    }
    else return error[match].string;
}

#ifdef WIN32

int myfseeko64(FILE *stream, off64_t offset, int whence) {
    //if ((dvdarg || riparg) && stream == NULL) return 0;
    if (dvdarg && stream == NULL) return 0;
  return fseeko64(stream, offset, whence);
}

void WinError(LPTSTR lpszFunction, char *textcolor) { 
    // retrieve the system error message for the last error code and print it with the function name in the desired color
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError(); 
    if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      dw,
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPTSTR) &lpMsgBuf,
                      0,
                      NULL) == 0) {
        // FormatMessage failed
        color(textcolor);
        printf("%s failed with error %ld%s", lpszFunction, dw, newline);
        color(normal);
    }
    else {
        color(textcolor);
        printf("%s failed with error %ld: %s%s", lpszFunction, dw, (char *) lpMsgBuf, newline);
        color(normal);
        LocalFree(lpMsgBuf);
    }
  return;
}

char *WinErrorString() {
    // return just the system error string for the last error code
    DWORD dw = GetLastError();
    DWORD len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                              FORMAT_MESSAGE_IGNORE_INSERTS,
                              NULL,
                              dw,
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                              (LPTSTR) &specialerror,
                              200,
                              NULL);
    if (len == 0) {
        // FormatMessage failed
        sprintf(specialerror, "Error %ld", dw);
    }
    else if (len > 2) {
        // remove period (and/or trailing newline) if it exists
        if (specialerror[len-3] == '.') specialerror[len-3] = 0x00;
        else if (specialerror[len-2] == 0x0D) specialerror[len-2] = 0x00;
    }
  return specialerror;
}

void doincreasescreenbuffersize(short length) {
    HANDLE hStdout;
    CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
    hStdout = GetStdHandle(STD_OUTPUT_HANDLE); 
    if (hStdout == INVALID_HANDLE_VALUE) {
        if (debug) WinError("GetStdHandle", red);
      return;
    }
    // get the screen buffer size
    if (GetConsoleScreenBufferInfo(hStdout, &csbiInfo) == 0) {
        // GetConsoleScreenBufferInfo failed - probably because output is being redirected to a text/html file
        if (debug) WinError("GetConsoleScreenBufferInfo", yellow);
      return;
    }
    if (debug) printf("doincreasescreenbuffersize: length = %d, csbiInfo.dwSize.X = %d, csbiInfo.dwSize.Y = %d%s", length, csbiInfo.dwSize.X, csbiInfo.dwSize.Y, newline);
    if (csbiInfo.dwSize.X < 80 || csbiInfo.dwSize.Y < length) {
        if (csbiInfo.dwSize.X < 80) csbiInfo.dwSize.X = 80;
        if (csbiInfo.dwSize.Y < length) csbiInfo.dwSize.Y = length;
        if (SetConsoleScreenBufferSize(hStdout, csbiInfo.dwSize) == 0) {
            if (debug) WinError("SetConsoleScreenBufferSize", red);
          return;
        }
        if (debug) printf("successfully increased screen buffer size%s", newline);
    }
  return;
}

int sendcdb(UCHAR direction, unsigned char *dataBuffer, unsigned long dataBufferSize,
            unsigned char *cdb, int cdbLen, bool checkreturnlength) {
    // fill SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER struct
    SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptdwb;
    memset(&sptdwb, 0, sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));
    sptdwb.sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    sptdwb.sptd.PathId = 0;
    sptdwb.sptd.TargetId = 0;
    sptdwb.sptd.Lun = 0;
    sptdwb.sptd.CdbLength = (UCHAR) cdbLen;
    sptdwb.sptd.DataIn = direction;
    sptdwb.sptd.SenseInfoLength = 32;
    sptdwb.sptd.DataTransferLength = dataBufferSize;
    sptdwb.sptd.TimeOutValue = dvdtimeout; // DeviceIoControl times out after <dvdtimeout> seconds
    sptdwb.sptd.DataBuffer = dataBuffer;
    sptdwb.sptd.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);
    int i;
    for(i=0;i<cdbLen;i++) {  // set cdb bytes
    	sptdwb.sptd.Cdb[i] = cdb[i];
    }
    // send to drive
    ULONG length = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER);
    ULONG returnedbytes;
    bool status = DeviceIoControl(hDevice, IOCTL_SCSI_PASS_THROUGH_DIRECT, &sptdwb, length, &sptdwb, length, &returnedbytes, NULL);
    sprintf(sense, "%02X/%02X/%02X", sptdwb.ucSenseBuf[2], sptdwb.ucSenseBuf[12], sptdwb.ucSenseBuf[13]);
    if (status == 0) {
        // error (or pending?)
        if (debug || testingdvd) {
            WinError("DeviceIoControl", red);
            color(red);
            printf("sense: %s (%s), returnedbytes=%lu, sptdwb.sptd.DataTransferLength=%lu%s",
                    sense, cdberror(sense), returnedbytes, sptdwb.sptd.DataTransferLength, newline);
            color(normal);
        }
      return 1;
    }
    // check sense code (00/00/00 or 01/xx/xx is ok)
    if (sptdwb.ucSenseBuf[2] != 0x00 || sptdwb.ucSenseBuf[12] != 0x00 || sptdwb.ucSenseBuf[13] != 0x00) {
        if (sptdwb.ucSenseBuf[2] != 0x01) {
            if (debug || testingdvd) {
                color(red);
                printf("sense: %s (%s), returnedbytes=%lu, sptdwb.sptd.DataTransferLength=%lu%s",
                        sense, cdberror(sense), returnedbytes, sptdwb.sptd.DataTransferLength, newline);
                color(normal);
            }
          return 1;
        }
        else if (debug || testingdvd) {
            color(yellow);
            printf("sense: %s (%s), returnedbytes=%lu, sptdwb.sptd.DataTransferLength=%lu%s",
                    sense, cdberror(sense), returnedbytes, sptdwb.sptd.DataTransferLength, newline);
            color(normal);
        }
    }
    else if (debug) printf("sense: %s (%s), returnedbytes=%lu, sptdwb.sptd.DataTransferLength=%lu%s",
                            sense, cdberror(sense), returnedbytes, sptdwb.sptd.DataTransferLength, newline);
    // check that the proper amount of bytes were returned
    if (checkreturnlength && (dataBufferSize != sptdwb.sptd.DataTransferLength)) {
        if (debug || testingdvd) {
            color(red);
            printf("dataBufferSize (%lu) != sptdwb.sptd.DataTransferLength (%lu)%s",
                    dataBufferSize, sptdwb.sptd.DataTransferLength, newline);
            color(normal);
        }
      return 1;
    }
  return 0;
}

#else

int opendevice(char *devicename, char *action) {
    #if defined(__FreeBSD__)
        cam_dev = cam_open_device(devicename, O_RDWR);
        if (cam_dev != NULL && cam_dev->fd != -1) fd = cam_dev->fd;
        else {
            color(red);
            if (debug) {
                if (cam_dev == NULL) printf("cam_dev == NULL%s", newline);
                else if (cam_dev->fd == -1) printf("cam_dev->fd == -1%s", newline);
            }
            printf("ERROR: Failed to open device: %s (%s) %s failed!%s", devicename, strerror(errno), action, newline);
            color(normal);
          return 1;
        }
    #else
        fd = open(devicename, O_RDONLY);
        if (fd == -1) {
            color(red);
            printf("ERROR: Failed to open device: %s (%s) %s failed!%s", devicename, strerror(errno), action, newline);
            color(normal);
          return 1;
        }
    #endif
  return 0;
}

#if defined(__linux__)

int sendcdb(int direction, unsigned char *dataBuffer, unsigned long dataBufferSize,
            unsigned char *cdb, unsigned char cdbLen, bool checkreturnlength) {
	if (debug && dataBufferSize > 65535 && sizeof(unsigned int) <= 2) {
        color(red);
        printf("well there's your problem: dataBufferSize (%lu) for linux sendcdb is %s 65535 and sizeof(unsigned int) = %u%s",
                dataBufferSize, greaterthan, (unsigned int) sizeof(unsigned int), newline);
        color(normal);
    }
    unsigned char sbp[32] = {0};
    sgio.interface_id = 'S';
    sgio.dxfer_direction = direction;
    sgio.cmd_len = cdbLen;
	sgio.cmdp = cdb;
	sgio.dxferp = dataBuffer;
	sgio.dxfer_len = (unsigned int) dataBufferSize;
	sgio.sbp = sbp;
	sgio.mx_sb_len = 32;
	sgio.timeout = dvdtimeout * 1000; // millisecs
	
	int rv = ioctl(fd, SG_IO, &sgio);
	sprintf(sense, "%02X/%02X/%02X", sbp[2], sbp[12], sbp[13]);
    if (rv == -1) {
        if (debug || testingdvd) {
            color(red);
            printf("ERROR: ioctl returned -1! (%s)%s", strerror(errno), newline);
            color(normal);
        }
      return 1;
    }
    // check sense code (00/00/00 or 01/xx/xx is ok)
    if (sbp[2] != 0x00 || sbp[12] != 0x00 || sbp[13] != 0x00) {
        if (sbp[2] != 0x01) {
            if (debug || testingdvd) {
                color(red);
                printf("sense: %s (%s)%s", sense, cdberror(sense), newline);
                color(normal);
            }
          return 1;
        }
        else if (debug || testingdvd) {
            color(yellow);
            printf("sense: %s (%s)%s", sense, cdberror(sense), newline);
            color(normal);
        }
    }
    else if (debug) printf("sense: %s (%s)%s", sense, cdberror(sense), newline);
    // check that the proper amount of bytes were returned
    if (checkreturnlength && sgio.resid) {  // resid = dxfer_len - actual_transferred
        if (debug || testingdvd) {
            color(red);
            printf("sgio.resid = %d, sgio.dxfer_len = %u, dataBufferSize = %lu%s", sgio.resid, sgio.dxfer_len, dataBufferSize, newline);
            color(normal);
        }
      return 1;
    }
  return 0;
}

#elif defined(__APPLE__)

int sendcdb(int direction, unsigned char *dataBuffer, unsigned long dataBufferSize,
            unsigned char *cdb, unsigned char cdbLen, bool checkreturnlength) {
    // todo
    
    sprintf(sense, "UU/OO/SS"); // temp -- gives sense error "Unsupported Operating System"
  return 1; // temp
}

#elif defined(__FreeBSD__)

#define MSG_SIMPLE_Q_TAG 0x20 // O/O - ?

int sendcdb(int direction, unsigned char *dataBuffer, unsigned long dataBufferSize,
            unsigned char *cdb, unsigned char cdbLen, bool checkreturnlength) {
    int i;
    union ccb *ccb = NULL;
    ccb = cam_getccb(cam_dev);
    if (ccb == NULL) {
        color(red);
        printf("ERROR: Memory allocation of CAM CCB failed! Game over man... Game over!%s", newline);
        color(normal);
      exit(1);
    }
    cam_fill_csio(&ccb->csio,
                  1,                  // retries
                  NULL,               // cbfcnp      // ?
                  direction,          // flags
                  MSG_SIMPLE_Q_TAG,   // tag_action  // What to do for tag queueing - ?
                  dataBuffer,         // data_ptr
                  dataBufferSize,     // dxfer_len
                  96,                 // sense_len   // Number of bytes to autosense - ?
                  cdbLen,             // cdb_len
                  dvdtimeout * 1000); // timeout (ms)
    memcpy(ccb->csio.cdb_io.cdb_bytes, cdb, cdbLen);
    int rv = cam_send_ccb(cam_dev, ccb);
    if (debug) {
        printf("ccb->csio.sense_data.error_code = %02X%s", ccb->csio.sense_data.error_code, newline);
        printf("ccb->csio.sense_data.segment = %02X%s", ccb->csio.sense_data.segment, newline);
        printf("ccb->csio.sense_data.flags = %02X%s", ccb->csio.sense_data.flags, newline);
        printf("ccb->csio.sense_data.info = %02X%02X%02X%02X%s", ccb->csio.sense_data.info[0], ccb->csio.sense_data.info[1], ccb->csio.sense_data.info[2], ccb->csio.sense_data.info[3], newline);
        printf("ccb->csio.sense_data.extra_len = %02X%s", ccb->csio.sense_data.extra_len, newline);
        printf("ccb->csio.sense_data.cmd_spec_info = %02X%02X%02X%02X%s", ccb->csio.sense_data.cmd_spec_info[0], ccb->csio.sense_data.cmd_spec_info[1], ccb->csio.sense_data.cmd_spec_info[2], ccb->csio.sense_data.cmd_spec_info[3], newline);
        printf("ccb->csio.sense_data.add_sense_code = %02X%s", ccb->csio.sense_data.add_sense_code, newline);
        printf("ccb->csio.sense_data.add_sense_code_qual = %02X%s", ccb->csio.sense_data.add_sense_code_qual, newline);
        printf("ccb->csio.sense_data.fru = %02X%s", ccb->csio.sense_data.fru, newline);
        printf("ccb->csio.sense_data.sense_key_spec = %02X%02X%02X%s", ccb->csio.sense_data.sense_key_spec[0], ccb->csio.sense_data.sense_key_spec[1], ccb->csio.sense_data.sense_key_spec[2], newline);
        printf("ccb->csio.sense_data.extra_bytes = ");
        for (i=0;i<14;i++) printf("%02X", ccb->csio.sense_data.extra_bytes[i]);
        printf("%s", newline);
    }
    sprintf(sense, "%02X/%02X/%02X", ccb->csio.sense_data.flags, ccb->csio.sense_data.add_sense_code, ccb->csio.sense_data.add_sense_code_qual); // not 100% sure about this
    if (rv == -1) {
        if (debug || testingdvd) {
            color(red);
            printf("ERROR: cam_send_ccb returned -1! (%s)%s", strerror(errno), newline);
            color(normal);
        }
	  return 1;
    }
    // check sense code (00/00/00 or 01/xx/xx is ok)
    if (ccb->csio.sense_data.flags != 0x00 || ccb->csio.sense_data.add_sense_code != 0x00 || ccb->csio.sense_data.add_sense_code_qual != 0x00) {
        if (ccb->csio.sense_data.flags != 0x01) {
            if (debug || testingdvd) {
                color(red);
                printf("sense: %s (%s)%s", sense, cdberror(sense), newline);
                color(normal);
            }
          return 1;
        }
        else if (debug || testingdvd) {
            color(yellow);
            printf("sense: %s (%s)%s", sense, cdberror(sense), newline);
            color(normal);
        }
    }
    else if (debug) printf("sense: %s (%s)%s", sense, cdberror(sense), newline);
    // check that the proper amount of bytes were returned
    // todo
    
    if ((ccb->ccb_h.status & CAM_STATUS_MASK) != CAM_REQ_CMP) {
        // error occured (includes sense errors - not sure if 01/xx/xx will trigger this or not)
        if (debug || testingdvd) {
            color(red);
            printf("ERROR: ccb->ccb_h.status (%08X) & CAM_STATUS_MASK (%08X) [%08X] != CAM_REQ_CMP (%08X)%s",
                   ccb->ccb_h.status, CAM_STATUS_MASK, ccb->ccb_h.status & CAM_STATUS_MASK, CAM_REQ_CMP, newline);
            color(normal);
        }
	  return 1;
    }
    cam_freeccb(ccb);
  return 0;
}

#elif (defined(__OpenBSD__) || defined(__NetBSD__))

int sendcdb(int direction, unsigned char *dataBuffer, unsigned long dataBufferSize,
            unsigned char *cdb, unsigned char cdbLen, bool checkreturnlength) {
    // todo
    
    sprintf(sense, "UU/OO/SS"); // temp
  return 1; // temp
}

#else

int sendcdb(int direction, unsigned char *dataBuffer, unsigned long dataBufferSize,
            unsigned char *cdb, unsigned char cdbLen, bool checkreturnlength) {
    // Unsupported OS - this is a fake sense code i created that will cause the error message "Unsupported Operating System" to be displayed
    sprintf(sense, "UU/OO/SS");
  return 1;
}

#endif  // #if defined(__linux__)

#endif  // #ifdef WIN32

int opendeviceandgetname(char *drive) {
    int i, j, n;
    char devicenamebuffer[64] = {0};
    #ifdef WIN32
        // drive should have already been validated as a drive letter A-Z (or a-z)
        char rootpath[strlen(drive) + 3];
        char pathforcreatefile[strlen(drive) + 6];
        sprintf(rootpath, "%s:\\", drive);
        sprintf(pathforcreatefile, "\\\\.\\%s:", drive);
        char *drivetypelist[7] = {
            "The drive type cannot be determined.",
            "The root path is invalid; for example, there is no volume mounted at the path.",
            "The drive has removable media; for example, a floppy drive, thumb drive, or flash card reader.",
            "The drive has fixed media; for example, a hard drive or flash drive.",
            "The drive is a remote (network) drive.",
            "The drive is a CD-ROM drive.",
            "The drive is a RAM disk."
        };
        // check the drive type
        unsigned int drivetype = GetDriveType(rootpath);
        if (drivetype != DRIVE_CDROM) {
            color(red);
            printf("ERROR: %s is not recognized as a multimedia drive!", rootpath);
            if (drivetype < 7 && drivetype != 5) printf(" (%s)", drivetypelist[drivetype]);
            printf("%s", newline);
            color(normal);
            if (debug) printf("GetDriveType(%s) = %d%s", rootpath, GetDriveType(rootpath), newline);
          return 1;
        }
        // open the device
        hDevice = CreateFile(pathforcreatefile, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hDevice == INVALID_HANDLE_VALUE) {
            WinError("CreateFile", red);
            if (debug) printf("drive: %s, rootpath: %s, pathforcreatefile: %s%s", drive, rootpath, pathforcreatefile, newline);
          return 1;
        }
    #else
        // open the device
        #if defined(__FreeBSD__)
        cam_dev = cam_open_device(drive, O_RDWR);
        if (cam_dev != NULL && cam_dev->fd != -1) fd = cam_dev->fd;
        else {
            color(red);
            if (debug) {
                if (cam_dev == NULL) printf("cam_dev == NULL%s", newline);
                else if (cam_dev->fd == -1) printf("cam_dev->fd == -1%s", newline);
            }
            printf("ERROR: Failed to open device: %s (%s)%s", drive, strerror(errno), newline);
            color(normal);
          return 1;
        }
        #else
        fd = open(drive, O_RDONLY);
        if (fd == -1) {
            color(red);
            printf("ERROR: Failed to open device: %s (%s)%s", drive, strerror(errno), newline);
            color(normal);
          return 1;
        }
        #endif
    #endif
    
    // get the device name
    unsigned char inqbuffer[96] = {0};
    memset(cdb, 0, 12);
    cdb[0] = 0x12;  // INQUIRY command
    cdb[4] = 0x60;  // allocation length LSB (0x0060 = 96) - maximum number of bytes that may be returned by the drive
    
    if (sendcdb(DATA_IN, inqbuffer, 96, cdb, 12, false)) {  // false means it won't check that number of bytes returned == 96
        for (i=0;i<readretries;i++) {
            if (sendcdb(DATA_IN, inqbuffer, 96, cdb, 12, false) == 0) {
                // recovered error
                goto inquiryrecovered;
            }
        }
        // unrecovered error
        color(yellow);
        #ifdef WIN32
            printf("Failed to get device parameters from drive %s: (%s)%s", drive, cdberror(sense), newline);
        #else
            printf("Failed to get device parameters from %s (%s)%s", drive, cdberror(sense), newline);
        #endif
        color(normal);
    }
    else {
        inquiryrecovered:
        if (debug || testingdvd) {
            printf("inquiry buffer:%s", newline);
            hexdump(inqbuffer, 0, 96);
        }
        #ifndef WIN32
            // check the device type
            unsigned char peripheraldevicetype = inqbuffer[0] & 0x1F;
            if (peripheraldevicetype != 0x05) {
                color(red);
                printf("ERROR: %s is not recognized as a multimedia device!", drive);
                if      (peripheraldevicetype == 0x00) printf(" (It's a direct-access block device; for example, magnetic disk.)");
                else if (peripheraldevicetype == 0x01) printf(" (It's a sequential-access device; for example, magnetic tape.)");
                else if (peripheraldevicetype == 0x02) printf(" (It's a printer device.)");
                else if (peripheraldevicetype == 0x03) printf(" (It's a processor device.)");
                else if (peripheraldevicetype == 0x04) printf(" (It's a write-once device; for example, some optical disks.)");
                else if (peripheraldevicetype == 0x06) printf(" (It's a scanner device.)");
                else if (peripheraldevicetype == 0x07) printf(" (It's an optical memory device; for example, some optical disks.)");
                else if (peripheraldevicetype == 0x08) printf(" (It's a medium changer device; for example, a jukebox.)");
                else if (peripheraldevicetype == 0x09) printf(" (It's a communications device.)");
                else if (peripheraldevicetype == 0x0A || peripheraldevicetype == 0x0B) printf(" (It's an obsolete device type.)");
                else if (peripheraldevicetype == 0x0C) printf(" (It's a storage array controller device; for example, RAID.)");
                else if (peripheraldevicetype == 0x0D) printf(" (It's an enclosure services device.)");
                else if (peripheraldevicetype == 0x0E) printf(" (It's a simplified direct-access device; for example, magnetic disk.)");
                else if (peripheraldevicetype == 0x0F) printf(" (It's an optical card reader/writer device.)");
                else if (peripheraldevicetype == 0x10) printf(" (It's a bridge controller.)");
                else if (peripheraldevicetype == 0x11) printf(" (It's an object-based storage device.)");
                else if (peripheraldevicetype == 0x12) printf(" (It's an automation/drive interface.)");
                else if (peripheraldevicetype >= 0x13 && peripheraldevicetype <= 0x1E) printf(" (It's a reserved device type.)");
                else if (peripheraldevicetype == 0x1F) printf(" (It's an unknown device type.)");
                printf("%s", newline);
                color(normal);
              return 1;
            }
        #endif
        i = 0;
        // vendor id 8-15
        for (j=8;j<16;j++) {
            if (inqbuffer[j] > 0x1F) {
                devicenamebuffer[i] = inqbuffer[j];
                i++;
            }
        }
        if (i > 0) for (n=i-1;n>=0;n--) {
            if (devicenamebuffer[n] == 0x20) i--;  // backspace over trailing spaces
            else break;
        }
        devicenamebuffer[i] = 0x20;
        i++;
        // product id 16-31
        for (j=16;j<32;j++) {
            if (inqbuffer[j] > 0x1F) {
                devicenamebuffer[i] = inqbuffer[j];
                i++;
            }
        }
        if (i > 0) for (n=i-1;n>=0;n--) {
            if (devicenamebuffer[n] == 0x20) i--;  // backspace over trailing spaces
            else break;
        }
        devicenamebuffer[i] = 0x20;
        i++;
        // revision level 32-35
        for (j=32;j<36;j++) {
            if (inqbuffer[j] > 0x1F) {
                devicenamebuffer[i] = inqbuffer[j];
                i++;
            }
        }
        if (i > 0) for (n=i-1;n>=0;n--) {
            if (devicenamebuffer[n] == 0x20) i--;  // backspace over trailing spaces
            else break;
        }
        devicenamebuffer[i] = 0x00;  // terminating null
    }
    
    if (strlen(devicenamebuffer)) {
        isofilename = calloc(4 + strlen(devicenamebuffer) + strlen(drive) + 1, sizeof(char));
        if (isofilename == NULL) {
            color(red);
            printf("ERROR: Memory allocation for device name failed! Game over man... Game over!%s", newline);
            color(normal);
          exit(1);
        }
        #ifdef WIN32
            sprintf(isofilename, "%s (%s:)", devicenamebuffer, drive);
        #else
            sprintf(isofilename, "%s (%s)", devicenamebuffer, drive);
        #endif
    }
    else {
        isofilename = calloc(7 + strlen(drive) + 1, sizeof(char));
        if (isofilename == NULL) {
            color(red);
            printf("ERROR: Memory allocation for device name failed! Game over man... Game over!%s", newline);
            color(normal);
          exit(1);
        }
        #ifdef WIN32
            sprintf(isofilename, "drive %s:", drive);
        #else
            sprintf(isofilename, "%s", drive);
        #endif
    }
  return 0;
}

long long freediskspace(char* filename) {
    // if non-windows or no colon in filename or GetDiskFreeSpaceEx fails - return 100 GB so that functions don't abort prematurely
    // (unrecoverable write errors will result if there is not enough free space)
    long long freespace = 107374182400LL;
    #ifdef WIN32
        char Drive[4];
        unsigned __int64 FreeBytesAvailable, TotalNumberOfBytes, TotalNumberOfFreeBytes;
        if (filename[1] == ':') {
            Drive[0] = filename[0];
            Drive[1] = ':';
            Drive[2] = '\\';
            Drive[3] = '\0';
        }
        else {
            if (debug) {
                color(red); printf("freediskspace - filename: %s does not have a colon%s", filename, newline); color(normal);
            }
          return freespace;
        }
        if (GetDiskFreeSpaceEx(Drive, (PULARGE_INTEGER) &FreeBytesAvailable,
                                      (PULARGE_INTEGER) &TotalNumberOfBytes,
                                      (PULARGE_INTEGER) &TotalNumberOfFreeBytes)) {
            if (debug) printf("freediskspace - FreeBytesAvailable = %"LL"u, TotalNumberOfBytes = %"LL"u, TotalNumberOfFreeBytes = %"LL"u%s",
                               FreeBytesAvailable, TotalNumberOfBytes, TotalNumberOfFreeBytes, newline);
            freespace = (long long) FreeBytesAvailable;
        }
        else if (debug) WinError("GetDiskFreeSpaceEx", red);
    #else
        // need to find a way to do this portably for unix-based systems...
        if (debug) printf("freediskspace - filename: %s%s", filename, newline);
    #endif
  return freespace;
}

void getL0capacity() {
    // read disc structure with format code 20h (layer boundary information)
    int i;
    bool L0capacityInitStatus = false;
    L0capacity = 0;
    unsigned char capbuffer[20] = {0};
    
    if (skiplayerboundaryinfo) return;
    
    memset(cdb, 0, 12);
    cdb[0] = 0xAD;  // READ DISC STRUCTURE (12 cdb bytes)
    cdb[7] = 0x20;  // format code for layer boundary info
    cdb[9] = 0x14;  // allocation length LSB (0x0014 = 20) - maximum number of bytes that may be returned by the drive
    
    if (sendcdb(DATA_IN, capbuffer, 20, cdb, 12, false)) {  // false means it won't check that number of bytes returned == 20
        for (i=0;i<readretries;i++) {
            if (sendcdb(DATA_IN, capbuffer, 20, cdb, 12, false) == 0) {
                // recovered error
                goto layerboundaryrecovered;
            }
        }
        // unrecovered error
        color(yellow);
        printf("Error reading Layer Boundary Information (%s)%s", cdberror(sense), newline);
        color(normal);
        // From INF-8090:
        // When a READ DISC STRUCTURE command is issued for media that is not supported by the Media Type field, with
        // Format Codes 00h - BFh, this command shall be terminated with CHECK CONDITION status, 5/30/02 CANNOT
        // READ MEDIUM - INCOMPATIBLE FORMAT. When the device/media does not support specified Format Code
        // value, this command shall be terminated with CHECK CONDITION status, 5/24/00 INVALID FIELD IN CDB.
        // For format code 20h:
        // This format is available only for DVD-R DL, DVD+R DL, or HD DVD-R DL discs. For other media, this format is invalid and reserved.
        if (memcmp(sense, "05/24/00", 8) == 0) {
            printf("There are 3 possibilities...%s"
                   "1. This isn't a DVD+R DL disc%s"
                   "2. This DVD drive simply doesn't support checking the layerbreak (try using a%s"
                   "%s different drive or add the option --skiplb to hide this message)%s"
                   "3. The drive has gone nuts and needs to be reset or replaced%s%s",
                   newline, newline, newline, sp2, newline, newline, newline);
        }
    }
    else {
        layerboundaryrecovered:
        // get L0 data area capacity
        L0capacity = (long long) getuintmsb(capbuffer+8);
        // check Init Status bit
        if (capbuffer[4] & 0x80) L0capacityInitStatus = true;
        if (debug || testingdvd) {
            printf("layer boundary info (L0capacity = %"LL"d, Init Status = %X):%s",
                    L0capacity, L0capacityInitStatus, newline);
            hexdump(capbuffer, 0, 20);
        }
        // L0 data area capacity should be 1913760
        if (L0capacity != 1913760) {
            color(red);
            printf("Layerbreak for this DVD was reported as %"LL"d (expected 1913760)%s", L0capacity, newline);
            if (L0capacityInitStatus)
                printf("If this is a burnt Xbox 360 backup, it probably won't boot on your console!%s", newline);
            else 
                printf("This appears to be the default L0 capacity and the disc is probably blank!%s", newline);
            printf("Always burn your backups with the correct layerbreak!%s%s", newline, newline);
            color(normal);
        }
    }
}

long long getdisccapacity(bool checkblocklength) {
    int i;
    unsigned char capbuffer[8];
    long long ret;
    memset(capbuffer, 0, 8);
    memset(cdb, 0, 12);
    cdb[0] = 0x25;  // READ CAPACITY (10 cdb bytes)
    if (sendcdb(DATA_IN, capbuffer, 8, cdb, 10, true)) {
        for (i=0;i<readretries;i++) {
            if (sendcdb(DATA_IN, capbuffer, 8, cdb, 10, true) == 0) {
                // recovered error
                goto getdisccapacityrecovered;
            }
        }
        // unrecovered error
        color(red);
        printf("ERROR: Error reading disc capacity (%s)%s", cdberror(sense), newline);
        color(normal);
        if (debug) {
            printf("cap buffer:%s", newline);
            hexdump(capbuffer, 0, 8);
        }
      return -1LL;
    }
    getdisccapacityrecovered:
    if (debug || testingdvd) {
        printf("disc capacity info:%s", newline);
        hexdump(capbuffer, 0, 8);
    }
    // check that block length == 2048 if checkblocklength is true
    if (checkblocklength && getuintmsb(capbuffer+4) != 2048) {
        color(red);
        printf("ERROR: Block Length for this media was reported as %lu (expected 2048)%s", getuintmsb(capbuffer+4), newline);
        color(normal);
      return -1LL;
    }
    // return size of media in bytes
    ret = (long long) (getuintmsb(capbuffer) + 1) * 2048LL;
  return ret;
}

int processdirectory(char *rootdir, char **filenames, bool *blockdevice,
                     bool matchonly, int matchfiles, char **matchfilelist, char *s) {
    int i, len_ent;
    int len_root = strlen(rootdir) + 1;  // leave room for possible slash
    char path[len_root+1];  // leave room for terminating null
    strcpy(path, rootdir);
    if (path[strlen(path)-1] != '/' && path[strlen(path)-1] != '\\') {
        #ifdef WIN32
            strcat(path, "\\");
        #else
            strcat(path, "/");
        #endif
    }
    DIR *dp;
    struct dirent *ep;
    dp = opendir(path);
    if (debug) printf("processing directory: %s%s", path, newline);
    if (dp != NULL) {
        while ((ep = readdir(dp))) {
            len_ent = strlen(ep->d_name);
            if ((s = (char *) malloc(len_root + len_ent + 1)) == NULL ) {
                color(red);
                printf("ERROR: memory allocation for a filename failed! Game over man... Game over!%s", newline);
                color(normal);
              exit(1);
            }
            if (filecount == MAX_FILENAMES) {
                color(red);
                printf("ERROR: More than %d matching files!%s", MAX_FILENAMES, newline);
                color(normal);
              return 1;
            }
            strcpy(s, path);
        	strcat(s, ep->d_name);
            if (stat(s, &buf) == -1 ) {
                color(yellow);
                printf("ERROR: stat failed for %s%s%s (%s)%s", quotation, s, quotation, strerror(errno), newline);
                color(normal);
              continue;
            }
            if (buf.st_mode & S_IFDIR) {  // directory
                if (recursesubdirs) {
                    if ( (len_ent == 1 && strcmp(ep->d_name, ".") == 0) ||
                         (len_ent == 2 && strcmp(ep->d_name, "..") == 0) ) continue;
                    if (processdirectory(s, filenames, blockdevice, matchonly, matchfiles, matchfilelist, s)) return 1;
                }
            }
            else if (buf.st_mode & (S_IFREG | S_IFBLK)) {  // regular files or block devices
                if (matchonly) {
                    for (i=0;i<matchfiles;i++) {
                        if (fnmatch(matchfilelist[i], ep->d_name, FNM_CASEFOLD) == 0) goto foundmatch;
                    }
                  continue;
                }
                foundmatch:
        	    filenames[filecount] = s;
        	    if (buf.st_mode & S_IFBLK) blockdevice[filecount] = true;
                if (debug) {
                    printf("%ld: %s", filecount, filenames[filecount]);
                    if (blockdevice[filecount]) printf(" (block device)%s", newline);
                    else printf("%s", newline);
                }
                filecount++;
            }
        }
        closedir(dp);
    }
    else {
        color(yellow);
        printf("ERROR: Couldn't open directory %s%s%s (%s)%s", quotation, path, quotation, strerror(errno), newline);
        color(normal);
    }
  return 0;
}

void parsecmdline(int argc, char *argv[]) {
    int i;
    unsigned int u;
    char *endptr;
    for (i=1;i<argc;i++) {
        if (strncmp(argv[i], "-", 1) == 0) {
            if (strncmp(argv[i], "--", 2) == 0) {
                if (strcmp(argv[i], "--") == 0) {
                    specialarg = i;
                    // do not parse any more options
                  return;
                }
                if (strcasecmp(argv[i], "--corrupt") == 0) checkcorruption = true;
                if (strcasecmp(argv[i], "--verbose") == 0) extraverbose = true;
                if (strcasecmp(argv[i], "--noverbose") == 0) verbose = false;
                if (strcasecmp(argv[i], "--help") == 0) justhelp = true;
                if (strcasecmp(argv[i], "--terminal") == 0) terminal = true;
                if (strcasecmp(argv[i], "--stripcolors") == 0) stripcolors = true;
                if (strcasecmp(argv[i], "--html") == 0) html = true;
                
                if (strcasecmp(argv[i], "--noheader") == 0) noheader = true;
                if (strcasecmp(argv[i], "--justheader") == 0) justheader = true;
                if (strcasecmp(argv[i], "--justfooter") == 0) justfooter = true;
                if (strcasecmp(argv[i], "--minimal") == 0) minimal = true;
                if (strcasecmp(argv[i], "--script") == 0) script = true;
                
                if (strcasecmp(argv[i], "--nodvdcheck") == 0) checkdvdfile = false;
                if (strcasecmp(argv[i], "--noautofix") == 0) autofix = false;
                if (strcasecmp(argv[i], "--af0") == 0) autofix = false;
                if (strcasecmp(argv[i], "--autofixfailed") == 0) autofixuncertain = false;
                if (strcasecmp(argv[i], "--af1") == 0) autofixuncertain = false;
                if (strcasecmp(argv[i], "--autofixalways") == 0) autofixalways = true;
                if (strcasecmp(argv[i], "--af3") == 0) autofixalways = true;
                if (strcasecmp(argv[i], "--noverify") == 0) verify = false;
                if (strcasecmp(argv[i], "--autoupload") == 0) autoupload = true;
                if (strcasecmp(argv[i], "--noupdate") == 0) onlineupdate = false;
                if (strcasecmp(argv[i], "--csv") == 0) csvupdate = true;
                if (strcasecmp(argv[i], "--stayoffline") == 0) stayoffline = true;
                if (strcasecmp(argv[i], "--regioncheck") == 0) stealthcheck = false;
                if (strcasecmp(argv[i], "--nogamecrc") == 0) checkgamecrcnever = true;
                if (strcasecmp(argv[i], "--gamecrc") == 0) checkgamecrcalways = true;
                if (strcasecmp(argv[i], "--p-video") == 0 && (i+1 < argc)) { patchvideoarg = i + 1; manualpatch = true; }
                if (strcasecmp(argv[i], "--p-pfi") == 0 && (i+1 < argc)) { patchpfiarg = i + 1; manualpatch = true; }
                if (strcasecmp(argv[i], "--p-dmi") == 0 && (i+1 < argc)) { patchdmiarg = i + 1; manualpatch = true; }
                if (strcasecmp(argv[i], "--p-ss") == 0 && (i+1 < argc)) { patchssarg = i + 1; manualpatch = true; }
                if (strcasecmp(argv[i], "--e-video") == 0 && (i+1 < argc)) { extractvideoarg = i + 1; manualextract = true; }
                if (strcasecmp(argv[i], "--e-videopartition") == 0 && (i+1 < argc)) { extractvideopartitionarg = i + 1; manualextract = true; }
                if (strcasecmp(argv[i], "--e-pfi") == 0 && (i+1 < argc)) { extractpfiarg = i + 1; manualextract = true; }
                if (strcasecmp(argv[i], "--e-dmi") == 0 && (i+1 < argc)) { extractdmiarg = i + 1; manualextract = true; }
                if (strcasecmp(argv[i], "--e-ss") == 0 && (i+1 < argc)) { extractssarg = i + 1; manualextract = true; }
                if (strcasecmp(argv[i], "--patchgarbage") == 0) patchvalidfilesonly = false;
                if (strcasecmp(argv[i], "--patchitanyway") == 0) patchifstealthpasses = true;
                if (strcasecmp(argv[i], "--debug") == 0) { extraverbose = true; debug = true; }
                if (strcasecmp(argv[i], "--debugfs") == 0) { extraverbose = true; debug = true; debugfs = true; }
                if (strcasecmp(argv[i], "--rebuildlowspace") == 0) rebuildlowspace = true;
                if (strcasecmp(argv[i], "--keeporiginaliso") == 0) keeporiginaliso = true;
                if (strcasecmp(argv[i], "--norebuild") == 0) norebuild = true;
                if (strcasecmp(argv[i], "--truncate") == 0 && (i+1 < argc)) { truncatearg = i + 1; truncatefile = true; }
                if (strcasecmp(argv[i], "--pause") == 0) pauseshell = true;
                if (strcasecmp(argv[i], "--max") == 0) maximize = true;
                //if (strcasecmp(argv[i], "--splitvid") == 0) addsplitvid = true;
                //if (strcasecmp(argv[i], "--removesplitvid") == 0) removesplitvid = true;
                if (strcasecmp(argv[i], "--padding") == 0) checkpadding = true;
                if (strcasecmp(argv[i], "--showfiles") == 0) showfiles = true;
                if (strcasecmp(argv[i], "--nofixdev") == 0) fixdeviation = false;
                if (strcasecmp(argv[i], "--fixangle359") == 0) fixangle359 = true;
                if (strcasecmp(argv[i], "--folder") == 0 && (i+1 < argc)) { folderarg = i + 1; foldermode = true; }
                if (strcasecmp(argv[i], "--dir") == 0 && (i+1 < argc)) { folderarg = i + 1; foldermode = true; }
                if (strcasecmp(argv[i], "--match") == 0 && (i+1 < argc)) { matcharg = i + 1; matchonly = true; }
                if (strcasecmp(argv[i], "--showfulltable") == 0) showfulltable = true;
                if (strcasecmp(argv[i], "--nofixdrt") == 0) fixDRT = false;
                if (strcasecmp(argv[i], "--testing") == 0) testing = true;
                if (strcasecmp(argv[i], "--testingdvd") == 0) testingdvd = true;
                if (strcasecmp(argv[i], "--nowrite") == 0) writefile = false;
                if (strcasecmp(argv[i], "--user") == 0 && (i+1 < argc)) { autouploaduserarg = i + 1; }
                if (strcasecmp(argv[i], "--pass") == 0 && (i+1 < argc)) { autouploadpassarg = i + 1; }
                if (strcasecmp(argv[i], "--localonly") == 0) localonly = true;
                if (strcasecmp(argv[i], "--makedat") == 0) makedatfile = true;
                if (strcasecmp(argv[i], "--dontparsefs") == 0) dontparsefs = true;
                if (strcasecmp(argv[i], "--sizedoesntmatter") == 0) increasescreenbuffersize = false;
                //if (strcasecmp(argv[i], "--rip") == 0 && (i+1 < argc)) riparg = i + 1;
                //if (strcasecmp(argv[i], "--dest") == 0 && (i+1 < argc)) ripdestarg = i + 1;
                if (strcasecmp(argv[i], "--rec") == 0) recursesubdirs = true;
                if (strcasecmp(argv[i], "--clobber") == 0) clobber = true;
                if (strcasecmp(argv[i], "--ach") == 0) showachievements = true;
                if (strcasecmp(argv[i], "--achs") == 0) { showachievements = true; hidesecretachievements = true; }
                if (strcasecmp(argv[i], "--aa") == 0) showavatarawards = true;
                if (strcasecmp(argv[i], "--images") == 0) { if (!imagedirmissing) extractimages = true; }
                if (strcasecmp(argv[i], "--embed") == 0) { if (!imagedirmissing) embedimages = true; }
                if (strcasecmp(argv[i], "--skiplb") == 0) skiplayerboundaryinfo = true;
                if (strcasecmp(argv[i], "--devkey") == 0) devkey = true;
                if (strcasecmp(argv[i], "--notrust") == 0) trustssv2angles = false;
                if (strcasecmp(argv[i], "--useinstalldir") == 0) useinstalldir = true;
                #ifdef WIN32
                    if (strcasecmp(argv[i], "--dvd") == 0 && (i+1 < argc)) dvdarg = i + 1;
                #endif
                
                if ((strcasecmp(argv[i], "--myregion") == 0 || strcasecmp(argv[i], "--rgn") == 0) && (i+1 < argc)) {
                    userregionarg = i + 1;
                    if (strlen(argv[i+1]) == 8) {
                        userregion = strtoul(argv[i+1], &endptr, 16);
                        if (*endptr != '\0') {
                            if (debug) {
                                printf("userregion: %s (0x%lX) appears to have invalid characters\n", argv[i+1], userregion);
                                if (html) printf("<br>");
                            }
                            userregion = 0L;
                        }
                    }
                }
                
                if (strcasecmp(argv[i], "--nettimeout") == 0 && (i+1 < argc)) {
                    connectiontimeout = strtol(argv[i+1], NULL, 10);
                    if (connectiontimeout < 0) connectiontimeout = 20;
                    connectiontimeoutarg = i + 1;
                }
                if (strcasecmp(argv[i], "--dvdtimeout") == 0 && (i+1 < argc)) {
                    dvdtimeout = strtol(argv[i+1], NULL, 10);
                    if (dvdtimeout < 0) dvdtimeout = 20;
                    dvdtimeoutarg = i + 1;
                }
                if (strcasecmp(argv[i], "--dev") == 0 && (i+1 < argc)) {
                    fixangledev_value = (int) strtol(argv[i+1], NULL, 10);
                    if (fixangledev_value < 0) fixangledev_value *= -1;
                    fixangledevarg = i + 1;
                }
                if (strcasecmp(argv[i], "--retries") == 0 && (i+1 < argc)) {
                    readretries = (int) strtol(argv[i+1], NULL, 10);
                    if (readretries < 0) readretries = 20;
                    readretryarg = i + 1;
                }
                if (strcasecmp(argv[i], "--layerbreak") == 0 && (i+1 < argc)) {
                    layerbreak = strtol(argv[i+1], NULL, 10);
                    if (layerbreak <= 0) layerbreak = 1913760;
                    else if (layerbreak != 1913760) altlayerbreak = true;
                    layerbreakarg = i + 1;
                }
                if (strcasecmp(argv[i], "--lang") == 0 && (i+1 < argc)) {
                    userlang = strtol(argv[i+1], NULL, 10);
                    if (userlang < 0) userlang = 0;
                    userlangarg = i + 1;
                }
            }
            else for(u=1; u<strlen(argv[i]); u++) {
                if (strncasecmp(argv[i]+u, "c", 1) == 0) checkcorruption = true;
                if (strncasecmp(argv[i]+u, "s", 1) == 0) stripcolors = true;
                if (strncasecmp(argv[i]+u, "n", 1) == 0) verbose = false;
                if (strncasecmp(argv[i]+u, "v", 1) == 0) extraverbose = true;
                if (strncasecmp(argv[i]+u, "r", 1) == 0) stealthcheck = false;
                if (strncasecmp(argv[i]+u, "h", 1) == 0) html = true;
                if (strncasecmp(argv[i]+u, "g", 1) == 0) checkgamecrcnever = true;
                if (strncasecmp(argv[i]+u, "t", 1) == 0) terminal = true;
                if (strncasecmp(argv[i]+u, "o", 1) == 0) stayoffline = true;
                if (strncasecmp(argv[i]+u, "u", 1) == 0) autoupload = true;
                if (strncasecmp(argv[i]+u, "a", 1) == 0) autofixalways = true;
                if (strncasecmp(argv[i]+u, "f", 1) == 0) autofix = false;
                if (strncasecmp(argv[i]+u, "d", 1) == 0) checkdvdfile = false;
                if (strncasecmp(argv[i]+u, "k", 1) == 0) keeporiginaliso = true;
                if (strncasecmp(argv[i]+u, "l", 1) == 0) rebuildlowspace = true;
                if (strncasecmp(argv[i]+u, "b", 1) == 0) norebuild = true;
                if (strncasecmp(argv[i]+u, "p", 1) == 0) checkpadding = true;
                if (strncasecmp(argv[i]+u, "x", 1) == 0) fixangle359 = true;
                if (strncasecmp(argv[i]+u, "w", 1) == 0) writefile = false;
                if (strncasecmp(argv[i]+u, "i", 1) == 0) { if (!imagedirmissing) extractimages = true; }
                if (strncasecmp(argv[i]+u, "e", 1) == 0) showachievements = true;
            }
        }
    }
  return;
}

void initializeglobals() {
    // 1st Wave PFI
    currentpfientries[0].crc = 0x739CEAB3;
    memcpy(currentpfientries[0].sha1, "\xf5\x13\xa4\x52\xd1\x32\xb4\x3d\x85\xfa\xe0\xd2\x2d\x4c\xae\x85\xfe\x8d\xde\x67", 20);
    currentpfientries[0].description = "1st Wave";
    currentpfientries[0].hosted = true;
    // 2nd Wave PFI
    currentpfientries[1].crc = 0xA4CFB59C;
    memcpy(currentpfientries[1].sha1, "\xa1\x65\xf4\x8d\x93\x8e\x41\x47\xde\xc9\xed\x40\xf2\xdf\x44\xa8\xd3\x97\x50\x50", 20);
    currentpfientries[1].description = "2nd Wave";
    currentpfientries[1].hosted = true;
    // 3rd Wave PFI
    currentpfientries[2].crc = 0x2A4CCBD3;
    memcpy(currentpfientries[2].sha1, "\x7f\x4c\xf1\x77\xd0\x72\xb7\xaa\xd8\x62\xa8\x19\xe5\x01\xaa\xbd\xf0\xcf\x7e\xc1", 20);
    currentpfientries[2].description = "3rd Wave";
    currentpfientries[2].hosted = true;
    // 4th - 7th Wave PFI
    currentpfientries[3].crc = 0x05C6C409;
    memcpy(currentpfientries[3].sha1, "\x5e\xf9\xc6\x71\xd5\xa6\xe5\x64\xf8\xaa\x94\x44\x09\x6d\x11\x90\x6e\x0a\xf4\x47", 20);
    currentpfientries[3].description = "4th - 7th Wave";
    currentpfientries[3].hosted = true;
    // 8th - 9th Wave PFI
    currentpfientries[4].crc = 0x0441D6A5;
    memcpy(currentpfientries[4].sha1, "\xcb\xc6\xf7\x98\xb6\x6a\x6f\x7d\xec\x58\x46\xe9\x1e\xcb\xbb\xb9\xb1\x08\x45\xb1", 20);
    currentpfientries[4].description = "8th - 9th Wave";
    currentpfientries[4].hosted = true;
    // increment NUM_CURRENTPFIENTRIES if adding more pfi entries here

    // 1st Wave Video
    currentvideoentries[0].crc = 0x66D0CB54;
    memcpy(currentvideoentries[0].sha1, "\x12\x00\xd3\x04\x1f\x8d\x19\x64\x59\x9a\x1c\x5a\xee\xd3\x10\x55\xd4\x1a\xda\x87", 20);
    currentvideoentries[0].description = "1st Wave";
    currentvideoentries[0].hosted = true;
    // 2nd Wave Video
    currentvideoentries[1].crc = 0x91410773;
    memcpy(currentvideoentries[1].sha1, "\xa3\x95\xd4\x45\x07\x37\x4e\xea\x04\xd7\x37\xc9\x4b\x92\xa7\xd4\x1a\x41\x16\x77", 20);
    currentvideoentries[1].description = "2nd Wave";
    currentvideoentries[1].hosted = true;
    // 3rd Wave Video
    currentvideoentries[2].crc = 0x0E58FB9D;
    memcpy(currentvideoentries[2].sha1, "\xed\x3c\xfb\xa8\xca\xfc\x6c\xb4\xc9\xd6\xfd\xbb\x72\xc2\x31\x3d\x6e\x1a\x91\xdd", 20);
    currentvideoentries[2].description = "3rd Wave";
    currentvideoentries[2].hosted = false;
    // 4th Wave Video
    currentvideoentries[3].crc = 0x1914211B;
    memcpy(currentvideoentries[3].sha1, "\x44\xfc\x2d\xc5\x85\x7f\xa3\xb1\x54\xfb\x68\xe1\xda\x9a\xc6\xdf\x9c\x61\xf4\x81", 20);
    currentvideoentries[3].description = "4th Wave";
    currentvideoentries[3].hosted = false;
    // 5th Wave Video
    currentvideoentries[4].crc = 0xD80BF8B6;
    memcpy(currentvideoentries[4].sha1, "\x68\x57\x6f\xf5\x10\x0c\x42\x67\x56\xf9\x3c\xe7\xe4\x13\x06\x5c\x8c\x15\xf4\xd6", 20);
    currentvideoentries[4].description = "5th Wave";
    currentvideoentries[4].hosted = false;
    // 6th Wave Video
    currentvideoentries[5].crc = 0xE2FA3A26;
    memcpy(currentvideoentries[5].sha1, "\x27\x33\x25\x72\x70\xe9\xc2\x14\x22\x19\x46\xd5\x66\xa8\x2b\xbe\x30\xf4\xc3\x62", 20);
    currentvideoentries[5].description = "6th Wave";
    currentvideoentries[5].hosted = false;
    // 7th Wave Video
    currentvideoentries[6].crc = 0x33B734F8;
    memcpy(currentvideoentries[6].sha1, "\xed\x46\xf5\xd1\x84\x0a\x04\xb2\x1e\x4d\x27\xb0\x93\x69\x6e\x7c\x84\x6e\xe2\x2a", 20);
    currentvideoentries[6].description = "7th Wave";
    currentvideoentries[6].hosted = false;
    // 8th Wave Video
    currentvideoentries[7].crc = 0xF69FD42F;
    memcpy(currentvideoentries[7].sha1, "\xf5\xc0\x8c\xbe\xf2\xf9\x81\x18\x1a\x14\xa9\xa4\x6c\xba\xaf\xfb\x09\xde\x5b\x61", 20);
    currentvideoentries[7].description = "8th Wave";
    currentvideoentries[7].hosted = false;
    // 9th Wave Video
    currentvideoentries[8].crc = 0x77B53B72;
    memcpy(currentvideoentries[8].sha1, "\x93\x12\x96\x43\xa5\xe4\x95\xbb\xdd\xc2\x42\xb1\x69\x70\x72\x68\x55\xb9\xf7\xa1", 20);
    currentvideoentries[8].description = "9th Wave";
    currentvideoentries[8].hosted = false;
    // increment NUM_CURRENTVIDEOENTRIES if adding more video entries here
    
    mostrecentpfientries = currentpfientries;
    datfilepfientries = NULL;
    num_pfientries = NUM_CURRENTPFIENTRIES;
    mostrecentvideoentries = currentvideoentries;
    datfilevideoentries = NULL;
    num_videoentries = NUM_CURRENTVIDEOENTRIES;
    
    xbox1pficrc = 0x8FC52135;
    memcpy(xbox1pfisha1, "\x1b\x1c\x6e\x61\x83\x57\x99\xdd\x18\x2d\xea\x5b\x3f\x3f\x35\x44\x72\x16\xa8\xac", 20);
    xbox1videocrc = 0x231A0A56;
    memcpy(xbox1videosha1, "\xe2\x7f\x0c\x94\xae\xf7\x9c\xed\x70\x4d\x12\x2d\x50\x62\x57\x4a\x23\x76\xc6\x36", 20);
    
    // media ids for AP25 games with no AP25 xex flag
    memcpy(currentap25mediaids[0].mediaid, "\xD9\x9E\x18\xE7", 4);  // Fable III
    memcpy(currentap25mediaids[1].mediaid, "\xBF\x2A\xB0\x04", 4);  // Fable III (spanish)
    memcpy(currentap25mediaids[2].mediaid, "\xA3\x71\x06\x9D", 4);  // Fable III (french?)
    memcpy(currentap25mediaids[3].mediaid, "\xC2\x2D\x0F\x2B", 4);  // Fable III (german)
    memcpy(currentap25mediaids[4].mediaid, "\x63\x7B\xEC\xDF", 4);  // Fable III (japanese)
    memcpy(currentap25mediaids[5].mediaid, "\xFC\xC2\xEE\x4D", 4);  // Fable III (spanish-latino)
    // increment NUM_CURRENTAP25MEDIAIDS if adding more media ids here
    
    mostrecentap25mediaids = currentap25mediaids;
    datfileap25mediaids = NULL;
    num_ap25mediaids = NUM_CURRENTAP25MEDIAIDS;
    
  return;
}

int main(int argc, char *argv[]) {
    int i, a;
    unsigned long m;
    atexit(doexitfunction);
    initializeglobals();
    
    if (argc < 2) {
        usage:
        printheader();
        color(white);
        printf("Usage: %s [options] %sinput file(s)%s%s", argv[0], lessthan, greaterthan, newline);
        #ifdef WIN32
            printf("Or: %s %s [options] --dvd %sdriveletter%s", sp2, argv[0], lessthan, greaterthan);
            color(normal);
            printf(" (DVD Drive Input)%s", newline);
        #else
            color(normal);
        #endif
        printf("%s", newline);
        
        printf("%sinput file(s)%s can be Xbox 360 ISOs, Xex's, or SS/Stealth files - abgx360 will%s", lessthan, greaterthan, newline);
        printf("recognize them automatically and only process the appropriate options.%s%s", newline, newline);
        
        printf("Short options can be combined into one argument (Ex: %s%s -v"
        #ifdef WIN32
               "t"
        #endif
               "w IMAGE.000%s)%s%s", quotation, argv[0], quotation, newline, newline);
        
        color(white);
        printf("Options:%s%s", newline, newline);
        color(normal);
        printf("%s -- %s this is a special option that needs to come after%s", sp6, sp11, newline);
        printf("%s [options] but before %sinput file(s)%s if any filenames%s", sp21, lessthan, greaterthan, newline);
        printf("%s begin with a hyphen%s", sp21, newline);
        printf("%s-v,%s--verbose %s high verbosity%s", sp2, sp2, sp4, newline);
        printf("%s-n,%s--noverbose %s low verbosity%s", sp2, sp2, sp2, newline);
        printf("%s-t,%s--terminal %s use Terminal font characters%s", sp2, sp2, sp3, newline);
        printf("%s-c,%s--corrupt %s always check for AnyDVD style game data corruption%s", sp2, sp2, sp4, newline);
        printf("%s-h,%s--html %s output html (Ex: %s%s -vhiw IMAGE.000 %s IMAGE.html%s)%s", sp2, sp2, sp7, quotation, argv[0], greaterthan, quotation, newline);
        //printf("%s-u,%s--autoupload %s AutoUpload ini and stealth files to the online db if%s", sp2, sp2, sp1, newline);
        //printf("%s stealth passes and verification fails%s", sp21, newline);
        printf("%s --showfiles %s display ISO filesystem%s", sp6, sp2, newline);
        printf("%s-p,%s--padding %s check/fix Video zero padding (doesn't affect stealth)%s", sp2, sp2, sp4, newline);
        printf("%s-s,%s--stripcolors%s strips colors", sp2, sp2, sp1);
        #ifndef WIN32 // windows SetConsoleTextAttribute will not output escape characters
            printf(" (useful if you're directing output to a%s%s text file)", newline, sp21);
        #endif
        printf("%s", newline);
        printf("%s-d,%s--nodvdcheck %s don't check for/create valid .dvd file%s", sp2, sp2, sp1, newline);
        printf("%s-r,%s--regioncheck%s check the region code only (no stealth check)%s", sp2, sp2, sp1, newline);
        printf("%s --noverify %s don't attempt to verify against the verified database%s", sp6, sp3, newline);
        printf("%s (local or online) if stealth passes%s", sp21, newline);
        printf("%s --noupdate %s don't check for updates%s", sp6, sp3, newline);
        printf("%s --csv %s include GameNameLookup.csv in the check for updates%s", sp6, sp8, newline);
        printf("%s-o,%s--stayoffline%s disable all online functions and prevent AutoFix/Verify%s", sp2, sp2, sp1, newline);
        printf("%s from looking in the online database for verified files%s", sp21, newline);
        printf("%s-w,%s--nowrite %s disable all writes to the input file(s) (for when you%s", sp2, sp2, sp4, newline);
        printf("%s just want to check files without modifying them)%s", sp21, newline);
        printf("%s-e,%s--ach %s show achievements%s", sp2, sp2, sp8, newline);
        printf("%s --achs %s show achievements but hide secret ones%s", sp6, sp7, newline);
        printf("%s --aa %s show avatar awards%s", sp6, sp9, newline);
        printf("%s-i,%s--images %s extract images from the Xex (recommended for html output)%s", sp2, sp2, sp5, newline);
        printf("%s --embed %s embed images in html source code for better portability%s%s", sp6, sp6, newline, newline);
        /*
        color(white); printf("Directory Processing:%s%s", newline, newline); color(normal);
        printf("%s --dir %sdir%s %s process all files in %sdir%s%s", sp6, lessthan, greaterthan, sp3, lessthan, greaterthan, newline);
        printf("%s --match %smask%s%s only process files that match the %smask%s (matches are%s", sp6, lessthan, greaterthan, sp1, lessthan, greaterthan, newline);
        printf("%s%s case insensitive) separate multiple masks with commas%s", sp21, sp1, newline);
        printf("%s%s Ex: --match %s*ss*.bin%s or --match %s*.iso, *.000, *.360%s%s%s", sp21, sp1, quotation, quotation, quotation, quotation, newline, newline);
        */
        color(white);
        printf("Game Partition CRC (default behavior is to check it only when needed):%s%s", newline, newline);
        color(normal);
        printf("%s --gamecrc %s always check it%s", sp6, sp4, newline);
        printf("%s-g,%s--nogamecrc %s never check it%s%s", sp2, sp2, sp2, newline, newline);
        
        color(white);
        printf("AutoFix Threshold (use only one option, higher levels include lower ones):%s%s", newline, newline);
        color(normal);
        printf("%s-a,%s--af3,%s--autofixalways%s level 3: AutoFix if stealth passes but fails%s", sp2, sp2, sp2, sp1, newline);
        printf("%s%s verification%s", sp20, sp20, newline);
        printf("%s [default threshold] %s level 2: AutoFix if stealth is uncertain and%s", sp6, sp4, newline);
        printf("%s%s fails verification%s", sp20, sp20, newline);
        printf("%s --af1,%s--autofixfailed%s level 1: AutoFix only if stealth fails%s", sp6, sp2, sp1, newline);
        printf("%s-f,%s--af0,%s --noautofix %s level 0: Do not AutoFix%s%s", sp2, sp2, sp1, sp4, newline, newline);
        
        color(white);
        printf("Where should AutoFix and Verification look for verified files:%s%s", newline, newline);
        color(normal);
        printf("%s [default setting] %s check the the online database for updated inis and%s", sp6, sp1, newline);
        printf("%s%s get new stealth files from it when needed%s", sp21, sp5, newline);
        printf("%s --localonly %s check the local StealthFiles folder only for inis%s", sp6, sp7, newline);
        printf("%s%s and stealth files%s%s", sp21, sp5, newline, newline);
        
        color(white);
        printf("Rebuilding Method (choose the method for rebuilding an ISO if it's missing%s", newline);
        printf("space for a video partition):%s%s", newline, newline);
        color(normal);
        printf("%s [default method] %s requires 7 - 7.5 GB free space on the partition%s", sp6, sp4, newline);
        printf("%s%s your ISO is located%s", sp21, sp7, newline);
        printf("%s-l,%s --rebuildlowspace %s only requires 253 MB free space but will corrupt%s", sp2, sp1, sp3, newline);
        printf("%s%s your ISO if it fails or is aborted during the%s", sp21, sp7, newline);
        printf("%s%s rebuilding process%s", sp21, sp7, newline);
        printf("%s-b,%s --norebuild %s don't rebuild%s", sp2, sp1, sp9, newline);
        printf("%s-k,%s--keeporiginaliso %s don't delete the original ISO after rebuilding%s", sp2, sp2, sp3, newline);
        printf("%s%s (applies to the default method only)%s%s", sp21, sp7, newline, newline);
        
        color(white);
        printf("Manually patch or extract files:%s%s", newline, newline);
        color(normal);
        printf("%s --p-video %sfile%s %s patch video from %sfile%s%s", sp6, lessthan, greaterthan, sp9, lessthan, greaterthan, newline);
        printf("%s --p-pfi %sfile%s %s patch PFI from %sfile%s%s", sp6, lessthan, greaterthan, sp11, lessthan, greaterthan, newline);
        printf("%s --p-dmi %sfile%s %s patch DMI from %sfile%s%s", sp6, lessthan, greaterthan, sp11, lessthan, greaterthan, newline);
        printf("%s --p-ss %sfile%s %s patch SS from %sfile%s%s", sp6, lessthan, greaterthan, sp12, lessthan, greaterthan, newline);
        printf("%s --patchitanyway %s patch files even if stealth passes (default%s", sp6, sp10, newline);
        printf("%s%s behavior is to patch only if stealth fails%s", sp21, sp12, newline);
        printf("%s%s and isn't AutoFixed, or stealth is uncertain%s", sp21, sp12, newline);
        printf("%s%s and isn't verified/AutoFixed)%s", sp21, sp12, newline);
        printf("%s --patchgarbage %s patch files even if they're invalid%s", sp6, sp11, newline);
        printf("%s --e-video %sfile%s %s extract video to %sfile%s%s", sp6, lessthan, greaterthan, sp9, lessthan, greaterthan, newline);
        printf("%s --e-videopartition %sfile%s%s extract entire video partition (253 MB) to%s", sp6, lessthan, greaterthan, sp1, newline);
        printf("%s%s %sfile%s%s", sp21, sp12, lessthan, greaterthan, newline);
        printf("%s --e-pfi %sfile%s %s extract PFI to %sfile%s%s", sp6, lessthan, greaterthan, sp11, lessthan, greaterthan, newline);
        printf("%s --e-dmi %sfile%s %s extract DMI to %sfile%s%s", sp6, lessthan, greaterthan, sp11, lessthan, greaterthan, newline);
        printf("%s --e-ss %sfile%s %s extract SS to %sfile%s%s", sp6, lessthan, greaterthan, sp12, lessthan, greaterthan, newline);
        printf("%s --clobber %s %s overwrite extracted files without prompting%s%s", sp6, sp10, sp5, newline, newline);
        
        color(white);
        printf("Misc:%s%s", newline, newline);
        color(normal);
        printf("%s --lang %snumber%s %s choose a preferred language to use when displaying%s"
               "%s%s strings from the title id resource (default=1)%s"
               "%s%s 1 = English %s 2 = Japanese%s"
               "%s%s 3 = German %s 4 = French%s"
               "%s%s 5 = Spanish %s 6 = Italian%s"
               "%s%s 7 = Korean %s 8 = Traditional Chinese%s"
               "%s%s 9 = Portuguese %s 10 = Simplified Chinese%s"
               "%s%s 11 = Polish %s 12 = Russian%s",
               sp6, lessthan, greaterthan, sp3, newline, sp21, sp5, newline,
               sp21, sp8, sp5, newline,
               sp21, sp8, sp6, newline,
               sp21, sp8, sp5, newline,
               sp21, sp8, sp6, newline,
               sp21, sp8, sp1, newline,
               sp21, sp7, sp5, newline);
        printf("%s --skiplb %s skip checking the layerbreak on burned dvds%s"
               "%s%s (useful if your drive doesn't support checking it)%s", sp6, sp10, newline, sp21, sp5, newline);
        printf("%s --notrust %s don't trust SS v2 angles%s", sp6, sp9, newline);
        printf("%s --nofixdrt %s don't fix SS C/R table if data is invalid%s", sp6, sp8, newline);
        printf("%s --dev %sdeviation%s %s fix angles that deviate more than %sdeviation%s%s", sp6, lessthan, greaterthan, sp1, lessthan, greaterthan, newline);
        printf("%s%s degrees from their CCRT targets (default=3)%s", sp21, sp5, newline);
        printf("%s --nofixdev %s don't fix any deviating angles%s", sp6, sp8, newline);
        printf("%s-x,%s--fixangle359 %s change any 359 degree angles to 0 for compatibility%s", sp2, sp2, sp5, newline);
        printf("%s%s with iXtreme versions previous to v1.4%s", sp21, sp5, newline);
        printf("%s --rgn %scode%s %s tell abgx360 your console's region so it can display%s", sp6, lessthan, greaterthan, sp6, newline);
        printf("%s%s your game's region code in the appropriate color.%s", sp21, sp5, newline);
        printf("%s%s use multiple regions if you have multiple consoles.%s", sp21, sp5, newline);
        printf("%s%s Ex codes: 000000FF (NTSC/U), 00FE0000 (PAL Europe),%s", sp21, sp5, newline);
        printf("%s%s 00FE01FF (PAL Europe, NTSC/J Japan and NTSC/U)%s", sp21, sp5, newline);
        //printf("%s --splitvid %s add SplitVid if it doesn't exist or isn't valid%s", sp6, sp8, newline);
        //printf("%s --removesplitvid %s remove SplitVid if it exists%s", sp6, sp2, newline);
/*      printf("%s --layerbreak %sLB%s %s calculate PFI and SS offsets using a layerbreak%s", sp6, lessthan, greaterthan, sp1, newline);
        printf("%s%s other than the default 1913760 (experimental)%s", sp21, sp5, newline); */
        printf("%s --truncate %ssize%s %s truncate or extend input file to %ssize%s bytes%s", sp6, lessthan, greaterthan, sp1, lessthan, greaterthan, newline);
        printf("%s%s be very careful with this!%s", sp21, sp5, newline);
        printf("%s --retries %snumber%s%s change the number of retries before a read/write%s", sp6, lessthan, greaterthan, sp1, newline);
        printf("%s%s error is considered unrecoverable (default=20)%s", sp21, sp5, newline);
        printf("%s --nettimeout %ssecs%s change the connection timeout to %ssecs%s seconds%s", sp6, lessthan, greaterthan, lessthan, greaterthan, newline);
        printf("%s%s (default=20; 0=use the system's internal timeout)%s", sp21, sp5, newline);
        printf("%s --dvdtimeout %ssecs%s change the timeout for DVD Drive I/O requests to%s", sp6, lessthan, greaterthan, newline);
        printf("%s%s %ssecs%s seconds (default=20)%s", sp21, sp5, lessthan, greaterthan, newline);
        printf("%s --devkey %s use the devkit AES key when decrypting an Xex%s", sp6, sp10, newline);
        printf("%s --help %s display this message (or just use %s%s%s with no%s", sp6, sp12, quotation, argv[0], quotation, newline);
        printf("%s%s arguments)%s%s", sp21, sp5, newline, newline);
        
      return 1;
    }
    
    // parse command line options
    parsecmdline(argc, argv);
    
    if (html) {
        quotation = "&quot;";
        ampersand = "&amp;";
        lessthan = "&lt;";
        greaterthan = "&gt;";
        numbersign = "&#35;";
        sp1 = "<span class=sp>&nbsp;</span>";
        sp2 = "<span class=sp>&nbsp;&nbsp;</span>";
        sp3 = "<span class=sp>&nbsp; &nbsp;</span>";
        sp4 = "<span class=sp>&nbsp; &nbsp;&nbsp;</span>";
        sp5 = "<span class=sp>&nbsp; &nbsp; &nbsp;</span>";
        sp6 = "<span class=sp>&nbsp; &nbsp; &nbsp;&nbsp;</span>";
        sp7 = "<span class=sp>&nbsp; &nbsp; &nbsp; &nbsp;</span>";
        sp8 = "<span class=sp>&nbsp; &nbsp; &nbsp; &nbsp;&nbsp;</span>";
        sp9 = "<span class=sp>&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;</span>";
        sp10 = "<span class=sp>&nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp;</span>";
        sp11 = "<span class=sp>&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;</span>";
        sp12 = "<span class=sp>&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp;</span>";
        sp18 = "<span class=sp>&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp;</span>";
        sp20 = "<span class=sp>&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp;</span>";
        sp21 = "<span class=sp>&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;</span>";
        sp28 = "<span class=sp>&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp;&nbsp;</span>";
        green = "</span><span class=green>"; yellow = "</span><span class=yellow>"; red = "</span><span class=red>";
        cyan = "</span><span class=cyan>"; blue = "</span><span class=blue>"; darkblue = "</span><span class=darkblue>";
        white = "</span><span class=white>"; arrow = "</span><span class=arrow>"; box = "</span><span class=box>";
        normal = "</span><span class=normal>"; hexoffsetcolor = "</span><span class=hexoffset>"; darkgray = "</span><span class=darkgray>";
        wtfhexcolor = "</span><span class=wtfhex>"; wtfcharcolor = "</span><span class=wtfchar>";
        brown = "</span><span class=brown>";
        reset = "</span><span class=normal>";
        filename = "</span><span class=filename>";
        newline = "<br>\n";
    }
    
    if (debug) {
        printf("done parsing command line:%s", newline);
        for (i=0;i<argc;i++) {
            if (i && strcasecmp(argv[i-1], "--pass") == 0) printf("argv[%d]: password%s", i, newline);  // OMG the password is "password"!
            else printf("argv[%d]: %s%s", i, argv[i], newline);
        }
        #ifndef WIN32
            printf("uname -a: ");
            if (system("uname -a") == -1) printf("error executing shell command \"uname -a\" (%s)%s", strerror(errno), newline);
            printf("%s", newline);
        #endif
        if (sha1_self_test(newline)) {
            color(red);
            printf("SHA-1 Self Test Failed!%s", newline);
            color(normal);
        }
    }
    
    #ifdef WIN32
        if (increasescreenbuffersize) {
            // increase screen buffer size if less than 80x5000
            // (default is usually 80x300 which is too small especially when processing multiple files)
            doincreasescreenbuffersize(5000);
        }
    #endif

    if (maximize) {
        // maximize the window
        #ifdef WIN32
            memset(winbuffer, 0, 2048);
            if (GetConsoleTitle(winbuffer, 2048)) {
                if (debug) printf("GetConsoleTitle - winbuffer: %s%s", winbuffer, newline);
                HWND windowhandle = FindWindow(NULL, winbuffer);
                if (windowhandle != NULL) {
                    ShowWindow(windowhandle, SW_SHOWNORMAL);
                    ShowWindow(windowhandle, SW_MAXIMIZE);
                }
            }
        #else
            // this doesn't seem to work any more so we'll try and handle it in the GUI instead
            // (call xterm with the option "-geometry 80x400+0+0" instead of calling abgx360 with the option "--max")
            printf("\033[9;1t");
        #endif
    }
    
    if (debug) printf("setting homedir%s", newline);

    memset(homedir, 0, 2048);
    char *envhome;
    #ifdef WIN32
        if (useinstalldir) {  // best not to use this due to potential problems with permissions and UAC
            // load the abgx360 install directory from the windows registry (written by the installer)
            HKEY hkResult;
            DWORD pcbData = sizeof(homedir);
            longreturnvalue = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\abgx360", 0, KEY_READ, &hkResult);
            if (debug) printf("RegOpenKeyEx returned %ld, pcbData = %ld%s", longreturnvalue, pcbData, newline);
            if (longreturnvalue == ERROR_SUCCESS) {
                longreturnvalue = RegQueryValueEx(hkResult, "Install_Dir", NULL, NULL, homedir, &pcbData);
                if (debug) printf("ReqQueryValueEx returned %ld, pcbData = %ld%s", longreturnvalue, pcbData, newline);
                if (longreturnvalue == ERROR_SUCCESS && strlen(homedir)) {
                    // homedir now includes the abgxdir so we'll change abgxdir to a backslash unless homedir already ends with a slash (in which case we'll make abgxdir blank)
                    // (these strings will be concatenated to get the full path of files)
                    if (homedir[strlen(homedir) - 1] != '\\' && homedir[strlen(homedir) - 1] != '/') abgxdir = "\\";
                    else abgxdir = "";
                }
                else if (strlen(homedir)) memset(homedir, 0, 2048);
            }
        }
        // if that fails, try a few environment variables for the homedir instead (now the default behavior)
        if (!strlen(homedir)) {
            envhome = getenv("APPDATA");
            if (envhome != NULL) {
                strncpy(homedir, envhome, 2047);
                if (debug) printf("homedir - APPDATA: %s, envhome: %s%s", homedir, envhome, newline);
            }
        }
        if (!strlen(homedir)) {
            envhome = getenv("ProgramData");
            if (envhome != NULL) {
                strncpy(homedir, envhome, 2047);
                if (debug) printf("homedir - ProgramData: %s, envhome: %s%s", homedir, envhome, newline);
            }
        }
        if (!strlen(homedir)) {
            envhome = getenv("ALLUSERSPROFILE");
            if (envhome != NULL) {
                strncpy(homedir, envhome, 2047);
                if (debug) printf("homedir - ALLUSERSPROFILE: %s, envhome: %s%s", homedir, envhome, newline);
            }
        }
    #else
        // for other operating systems we'll see if the optional ABGX360_DIR environment variable has been set and use that,
        // if not we'll try the HOME environment variable and failing that we'll try the getpwuid function
        if (!strlen(homedir)) {
            envhome = getenv("ABGX360_DIR");
            if (envhome != NULL) {
                strncpy(homedir, envhome, 2047);
                if (debug) printf("homedir - ABGX360_DIR: %s, envhome: %s%s", homedir, envhome, newline);
            }
        }
        if (!strlen(homedir)) {
            envhome = getenv("HOME");
            if (envhome != NULL) {
                strncpy(homedir, envhome, 2047);
                if (debug) printf("homedir - HOME: %s, envhome: %s%s", homedir, envhome, newline);
            }
        }
        if (!strlen(homedir)) {
            envhome = getpwuid(getuid())->pw_dir;
            if (envhome != NULL) {
                strncpy(homedir, envhome, 2047);
                if (debug) printf("homedir - getpwuid: %s, envhome: %s%s", homedir, envhome, newline);
            }
        }
    #endif
    // if none of that worked, we'll use the current working directory to store and look for files by testing for this boolean value
    if (!strlen(homedir)) homeless = true;
    if (debug) printf("done setting homedir: %s%s", homedir, newline);
    
    if (!homeless) docheckdirectories();  // check that necessary directories exist and create them if not, or set homeless = true if creation fails
    
    if (justfooter) return 0;
    if (html) printhtmltop(argc, argv);
    if (justhelp) goto usage;
    printheader();
    if (justheader) return 0;
    
    if (!stayoffline) {
        // initialize curl
        if (curl_global_init(CURL_GLOBAL_ALL)) {
            stayoffline = true;
            color(yellow);
            printf("ERROR: cURL global initialization failed, all online functions will be disabled%s", newline);
            color(normal);
        }
        else {
            curl = curl_easy_init();
            if (curl == NULL) {
                stayoffline = true;
                color(yellow);
                printf("ERROR: cURL initialization failed, all online functions will be disabled%s", newline);
                color(normal);
            }
            else {
                // check abgx360.ini
                checkini();
                if (debug) {
                    printf("webinidir: %s%s", webinidir, newline);
                    printf("webunverifiedinidir: %s%s", webunverifiedinidir, newline);
                    printf("webcsv: %s%s", webcsv, newline);
                    printf("webdat: %s%s", webdat, newline);
                    printf("webstealthdir: %s%s", webstealthdir, newline);
                    printf("autouploadwebaddress: %s%s%s", autouploadwebaddress, newline, newline);
                }
            }
        }
    }
    
    if (onlineupdate && !stayoffline) {
        color(normal);
        if (csvupdate) {
            printf("Checking for updates to GameNameLookup.csv and abgx360.dat...%s", newline);
            // download an updated GameNameLookup.csv and abgx360.dat if server file is newer than the local one
            memset(curlerrorbuffer, 0, CURL_ERROR_SIZE+1);
            memset(buffer, 0, 2048);
            curl_easy_setopt(curl, CURLOPT_ENCODING, "");  // If a zero-length string is set, then an Accept-Encoding: header containing all supported encodings is sent.
            curl_easy_setopt(curl, CURLOPT_USERAGENT, curluseragent);
            curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlerrorbuffer);
            curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 0);  // refuse redirects (account is probably suspended and we don't want to retrieve the error page as a file)
            curl_easy_setopt(curl, CURLOPT_URL, webcsv);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connectiontimeout);
            curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, (curl_progress_callback) curlprogress);
            curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, (char*) "Downloading GameNameLookup.csv");
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
            if (extraverbose) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
            if (!homeless) {
                strcat(buffer, homedir); strcat(buffer, abgxdir);
            }
            strcat(buffer, "GameNameLookup.csv");
            struct MyCurlFile curlwebcsv = {buffer, NULL};
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &curlwebcsv);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write);
            if (stat(buffer, &buf) == 0) {
                curl_easy_setopt(curl, CURLOPT_TIMECONDITION, 1);
                curl_easy_setopt(curl, CURLOPT_TIMEVALUE, buf.st_mtime);
                if (debug) printf("%s: buf.st_mtime = %ld%s", buffer, (long) buf.st_mtime, newline);
            }
            else {
                curl_easy_setopt(curl, CURLOPT_TIMECONDITION, 0);
                if (debug) printf("stat failed for %s (%s) - no time condition used%s", buffer, strerror(errno), newline);
            }
            curlheaderprinted = false;
            if (extraverbose) {
                fprintf(stderr, "\n");
            }
            printstderr = true;
            color(blue);
            curlprogressstartmsecs = getmsecs();
            res = curl_easy_perform(curl);
            color(normal);
            printstderr = false;
            if (extraverbose || curlheaderprinted) fprintf(stderr, "\n");
            if (res != CURLE_OK) {  // error occurred
                color(yellow);
                if (res == CURLE_HTTP_RETURNED_ERROR) {
                    if (strstr(curlerrorbuffer, "404") != NULL) {
                        printf("The server is online but GameNameLookup.csv is missing (404)%s", newline);
                    }
                    else if (strstr(curlerrorbuffer, "403") != NULL) {
                        printf("The server is online but it's denying access to GameNameLookup.csv (403)%s", newline);
                    }
                    else if (strstr(curlerrorbuffer, "401") != NULL) {
                        printf("The server is online but it's denying access to GameNameLookup.csv (401)%s", newline);
                    }
                    else printf("ERROR: %s%s", curlerrorbuffer, newline);
                }
                else {
                    stayoffline = true;
                    printf("ERROR: %s%s", curlerrorbuffer, newline);
                    printf("There seems to be a problem with the db so online functions have been disabled%s"
                           "Try again later...%s", newline, newline);
                }
                color(normal);
            }
            else {
                color(normal);
                printcurlinfo(curl, "GameNameLookup.csv");
            }
            if (curlwebcsv.stream != NULL) fclose(curlwebcsv.stream);
            if (res != CURLE_OK && res != CURLE_HTTP_RETURNED_ERROR) {
                goto skipdatupdate;
            }
        }
        // now get abgx360.dat
        memset(curlerrorbuffer, 0, CURL_ERROR_SIZE+1);
        memset(buffer, 0, 2048);
        if (!csvupdate) {
            printf("Checking for updates to abgx360.dat...%s", newline);
            curl_easy_setopt(curl, CURLOPT_ENCODING, "");  // If a zero-length string is set, then an Accept-Encoding: header containing all supported encodings is sent.
            curl_easy_setopt(curl, CURLOPT_USERAGENT, curluseragent);
            curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlerrorbuffer);
            curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 0);  // refuse redirects (account is probably suspended and we don't want to retrieve the error page as a file)
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connectiontimeout);
            curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, (curl_progress_callback) curlprogress);
            curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
            if (extraverbose) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write);
        }
        //else if (extraverbose) fprintf(stderr, "\n");
        curl_easy_setopt(curl, CURLOPT_URL, webdat);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, (char*) "Downloading abgx360.dat");
        if (!homeless) {
            strcat(buffer, homedir); strcat(buffer, abgxdir);
        }
        strcat(buffer, "abgx360.dat");
        struct MyCurlFile curlwebdat = {buffer, NULL};
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &curlwebdat);
        if (stat(buffer, &buf) == 0) {
            curl_easy_setopt(curl, CURLOPT_TIMECONDITION, 1);
            curl_easy_setopt(curl, CURLOPT_TIMEVALUE, buf.st_mtime);
            if (debug) printf("%s: buf.st_mtime = %ld%s", buffer, (long) buf.st_mtime, newline);
        }
        else {
            curl_easy_setopt(curl, CURLOPT_TIMECONDITION, 0);
            if (debug) printf("stat failed for %s (%s) - no time condition used%s", buffer, strerror(errno), newline);
        }
        curlheaderprinted = false;
        if (extraverbose) fprintf(stderr, "\n");
        printstderr = true;
        color(blue);
        curlprogressstartmsecs = getmsecs();
        res = curl_easy_perform(curl);
        color(normal);
        printstderr = false;
        if (extraverbose || curlheaderprinted) fprintf(stderr, "\n");
        if (res != CURLE_OK) {  // error occurred
            color(yellow);
            if (res == CURLE_HTTP_RETURNED_ERROR) {
                if (strstr(curlerrorbuffer, "404") != NULL) {
                    printf("The server is online but abgx360.dat is missing (404)%s", newline);
                }
                else if (strstr(curlerrorbuffer, "403") != NULL) {
                    printf("The server is online but it's denying access to abgx360.dat (403)%s", newline);
                }
                else if (strstr(curlerrorbuffer, "401") != NULL) {
                    printf("The server is online but it's denying access to abgx360.dat (401)%s", newline);
                }
                else printf("ERROR: %s%s", curlerrorbuffer, newline);
            }
            else {
                stayoffline = true;
                printf("ERROR: %s%s", curlerrorbuffer, newline);
                printf("There seems to be a problem with the db so online functions have been disabled%s"
                       "Try again later...%s", newline, newline);
            }
            color(normal);
        }
        else {
            color(normal);
            printcurlinfo(curl, "abgx360.dat");
        }
        if (curlwebdat.stream != NULL) fclose(curlwebdat.stream);
        skipdatupdate:
        if (extraverbose) {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);  // reset to avoid annoying "Closing Connection ..." atexit
        }
        printf("%s", newline);
        color(normal);
    }
    
    if (makedatfile) {
        makedat();
      return 0;
    }
    
    // check abgx360.dat
    checkdat();
    
    #ifdef WIN32
        if (dvdarg) {
            // burned dvd input (windows only, unix systems support reading devices as files)
            // argv[dvdarg] should be a single letter A-Z (or a-z)
            if (strlen(argv[dvdarg]) != 1) {
                color(red);
                printf("ERROR: Enter only one character for the DVD drive letter! Example: --dvd D%s", newline);
                color(normal);
              return 1;
            }
            if ( (argv[dvdarg][0] >= 0x0 && argv[dvdarg][0] < 'A') ||
                 (argv[dvdarg][0] > 'Z' && argv[dvdarg][0] < 'a') ||
                 (argv[dvdarg][0] > 'z') ) {
                color(red);
                printf("ERROR: DVD drive letter is invalid! Enter A-Z only! Example: --dvd D%s", newline);
                color(normal);
              return 1;
            }
            
            writefile = false;
            fp = NULL;
            
            if (opendeviceandgetname(argv[dvdarg])) return 1;
            
            // get size of media in bytes
            fpfilesize = getdisccapacity(true);
            if (fpfilesize == -1LL) return 1;
            if (debug) printf("fpfilesize: %"LL"d%s", fpfilesize, newline);
            
            // check that they've burned with the correct layerbreak (1913760)
            getL0capacity();
            
            // check to see if they burned just a game partition (unbootable) by looking for magic bytes at sector 32
            if (fpfilesize < 67584) {
                color(red); printf("ERROR: Media size (%"LL"d bytes) is too small to be an Xbox 360 game!%s", fpfilesize, newline); color(normal);
              return 1;
            }
            video = 0LL;
            initcheckread();
            if (checkreadandprinterrors(ubuffer, 1, 2048, fp, 0, 65536, isofilename, "Reading Sector 32") != 0) return 1;
            donecheckread(isofilename);
            if (debug) { printf("sector 32:%s", newline); hexdump(ubuffer, 0, 2048); }
            
            if (memcmp(ubuffer, "MICROSOFT*XBOX*MEDIA", 20) != 0) {
                // proper backups should have the game partition starting at 0xFD90000
                video = 0xFD90000LL;
                if (fpfilesize < (long long) video + 67584LL) {
                    color(red); printf("ERROR: Media size (%"LL"d bytes) is too small to be an Xbox 360 game!%s", fpfilesize, newline); color(normal);
                  return 1;
                }
                // read sector 32 + video
                memset(ubuffer, 0, 2048);
                initcheckread();
                if (checkreadandprinterrors(ubuffer, 1, 2048, fp, 0, video+65536LL, isofilename, "Reading Sector 32") != 0) return 1;
                donecheckread(isofilename);
                if (debug) { printf("sector 32 + video:%s", newline); hexdump(ubuffer, 0, 2048); }
                if (memcmp(ubuffer, "MICROSOFT*XBOX*MEDIA", 20) != 0) {
                    // this string should be here for all xbox 360 games
                    color(red); printf("ERROR: Media isn't recognized as an Xbox 360 game!%s", newline); color(normal);
                  return 1;
                }
            }
            
            if (manualextract) {
                if (video == 0) {
                    color(yellow);
                    printf("ERROR: This disc is just a game partition (no stealth files to extract)%s%s", newline, newline);
                    color(normal);
                }
                else domanualextraction(argv);
            }
            
            checkgame();
            
            if (xex_foundmediaid && !game_has_ap25) {
                // check media id against list of ap25 games with no ap25 xex flag
                for (m=0;m<num_ap25mediaids;m++) {
                    if (memcmp(xex_mediaid+12, mostrecentap25mediaids[m].mediaid, 4) == 0) {
                        game_has_ap25 = true;
                        break;
                    }
                }
            }
            
            if (stealthcheck) {
                printf("%s", newline);
                if (video == 0) {
                    // means that "MICROSOFT*XBOX*MEDIA" was found at sector 32
                    color(red);
                    printf("Stealth Failed!: Image is missing Video, PFI, DMI and SS%s", newline);
                    printf("%s %s (It's just a game partition and won't boot on ANY firmware!)%s", sp10, sp5, newline);
                    color(normal);
                  return 1;
                }
                
                if (game_has_ap25) {
                    // check ap25 replay data
                    initcheckread();
                    if (checkreadandprinterrors(ap25, 1, 2048, fp, 0, 0xFD8E000, isofilename, "Stealth check") != 0) return 1;
                    donecheckread(isofilename);
                    checkap25();
                    if (verbose) printf("%s", newline);
                }
                
                // check SS
                initcheckread();
                if (checkreadandprinterrors(ss, 1, 2048, fp, 0, 0xFD8F800, isofilename, "Stealth check") != 0) return 1;
                donecheckread(isofilename);
                checkss();
                
                // check DMI
                initcheckread();
                if (checkreadandprinterrors(ubuffer, 1, 2048, fp, 0, 0xFD8F000, isofilename, "Stealth check") != 0) return 1;
                donecheckread(isofilename);
                checkdmi(ubuffer);
                
                // check PFI
                initcheckread();
                if (checkreadandprinterrors(ubuffer, 1, 2048, fp, 0, 0xFD8E800, isofilename, "Stealth check") != 0) return 1;
                donecheckread(isofilename);
                checkpfi(ubuffer);
                
                // check Video
                if (verbose) printf("%s", newline);
                checkvideo(isofilename, fp, false, checkpadding);
                
                if (video_stealthfailed || pfi_stealthfailed || dmi_stealthfailed || ss_stealthfailed || stealthfailed) {
                    color(red);
                    printf("%sStealth check failed!%s", newline, newline);
                    color(normal);
                }
                else if (video_stealthuncertain || pfi_stealthuncertain || dmi_stealthuncertain || ss_stealthuncertain || stealthuncertain) {
                    color(yellow);
                    printf("%sStealth status is uncertain%s", newline, newline);
                    color(normal);
                    if (verify) {
                        returnvalue = doverify();
                        if (returnvalue == 0) {
                            if (drtfucked) {
                                color(yellow);
                                printf("Stealth and game data were verified but angle deviation cannot be verified%s", newline);
                            }
                            else {
                                color(green);
                                printf("Verification was successful, Stealth passed!%s", newline);
                            }
                            color(normal);
                            // this is only here to make the splitvid check possible
                            video_stealthuncertain = false;
                            pfi_stealthuncertain = false;
                        }
                        else if (returnvalue == 2) {
                            color(yellow);
                            printf("Stealth was verified but game data wasn't%s", newline);
                            if (drtfucked) printf("Angle deviation cannot be verified%s", newline);
                            color(normal);
                            // this is only here to make the splitvid check possible
                            video_stealthuncertain = false;
                            pfi_stealthuncertain = false;
                        }
                        else {
                            verifyfailed = true;
                            color(yellow);
                            if (returnvalue == 3) printf("Game data was verified but stealth wasn't%s", newline);
                            if (drtfucked) printf("Angle deviation cannot be verified%s", newline);
                            printf("Verification failed%s", newline);
                            color(normal);
                        }
                    }
                }
                else {
                    color(green);
                    printf("%sBasic Stealth check passed!%s", newline, newline);
                    color(normal);
                    // careful not to verify/autoupload after manual patch (or autofix)
                    if (verify) {
                        returnvalue = doverify();
                        if (returnvalue == 0) {
                            color(green);
                            printf("Verification was successful!%s", newline);
                            color(normal);
                        }
                        else if (returnvalue == 2) {
                            color(yellow);
                            printf("Stealth was verified but game data wasn't%s", newline);
                            color(normal);
                        }
                        else {
                            verifyfailed = true;
                            color(yellow);
                            if (returnvalue == 3) printf("Game data was verified but stealth wasn't%s", newline);
                            else printf("Verification failed%s", newline);
                            color(normal);
                            if (autoupload && !stayoffline && !fixedss && autouploaduserarg && autouploadpassarg) {
                                doautoupload(argv);
                                // should check to see if autoupload resulted in verification
                            }
                        }
                    }
                }
                if (addsplitvid) {
                    // this will only check for valid splitvid since writing is disabled (dvd input)
                    printf("%s", newline);
                    doaddsplitvid();
                }
                if (checkcorruption && game_crc32 == 0 && !checkgamecrcnever) {
                    printf("%sChecking for AnyDVD style game data corruption while running CRC check...%s", newline, newline);
                    if (docheckgamecrc() == 0) {
                        if (corruptionoffsetcount == 0) {
                            color(green);
                            if (verbose) printf("%s", sp5);
                            printf("AnyDVD style corruption was not detected%s", newline);
                            color(normal);
                        }
                        if (verbose) printf("%sGame CRC = %08lX%s%s", sp5, game_crc32, newline, newline);
                    }
                    else {
                        checkgamecrcnever = true;
                        gamecrcfailed = true;
                    }
                }
            }
            
            if (hDevice != INVALID_HANDLE_VALUE) CloseHandle(hDevice);
          return 0;
        }
    #endif  // ifdef WIN32
    
    if (debug) printf("MAX_FILENAMES: %d%s", MAX_FILENAMES, newline);
    char *filenames[MAX_FILENAMES];
    bool blockdevice[MAX_FILENAMES] = { false };
    if (debug) for (m=0;m<MAX_FILENAMES;m++) if (blockdevice[m]) {
        color(red);
        printf("blockdevice array was not properly initialized to false (found true at %lu)%s", m, newline);
        color(normal);
      break;
    }
    
    if (foldermode) {
        int matchfiles = 0;
        char *matchfilelist[100];
        int matchfilestart[100];
        memset(matchfilestart, 0, 100 * sizeof(int));
        int matchfileend[100];
        char *matchbuffer;
        if (matchonly) {
            if (strlen(argv[matcharg])) {
                if (debug) printf("argv[matcharg]: %s%s", argv[matcharg], newline);
                for (i=0;i<(int)strlen(argv[matcharg]);i++) {
                    if (argv[matcharg][i] != ' ' && argv[matcharg][i] != ',') {
                        matchfilestart[0] = i;
                        break;
                    }
                }
                for (i=0;i<(int)strlen(argv[matcharg]);i++) {
                    if (argv[matcharg][i] == ',') {
                        matchfileend[matchfiles] = i;
                        if (matchfileend[matchfiles] - matchfilestart[matchfiles] > 0) matchfiles++;
                        else {
                            if (debug) printf("matchfileend[%d] - matchfilestart[%d] = %d%s", matchfiles, matchfiles, matchfileend[matchfiles] - matchfilestart[matchfiles], newline);
                            continue;
                        }
                        for (a=1;a<(int)strlen(argv[matcharg]) - i;a++) {
                            if (argv[matcharg][i+a] != ' ' && argv[matcharg][i+a] != ',') {
                                matchfilestart[matchfiles] = i + a;
                                if (debug) printf("matchfilestart[%d] = %d%s", matchfiles, i + a, newline);
                                break;
                            }
                        }
                        if (matchfilestart[matchfiles] == 0) {
                            if (debug) printf("matchfilestart[%d] = 0%s", matchfiles, newline);
                            matchfiles--;
                            break;
                        }
                        if (matchfiles > 98) break;
                    }
                }
                if (matchfiles < 99) {
                    a = 0;
                    for (i=matchfilestart[matchfiles];i<(int)strlen(argv[matcharg]);i++) {
                        if (argv[matcharg][i] != ' ' && argv[matcharg][i] != ',') a = i+1;
                    }
                    matchfileend[matchfiles] = a;
                    if (matchfileend[matchfiles] - matchfilestart[matchfiles] > 0) matchfiles++;
                }
                if (debug) {
                    printf("matchfiles = %d%s", matchfiles, newline);
                    for (i=0;i<matchfiles;i++) printf("%d: %d, %d%s", i, matchfilestart[i], matchfileend[i], newline);
                }
                for (i=0;i<matchfiles;i++) {
                    if ( (matchbuffer = (char *) calloc(1, matchfileend[i] - matchfilestart[i] + 1)) == NULL ) {
                        color(red); printf("ERROR: memory allocation for matchbuffer failed! Game over man... Game over!%s", newline); color(normal);
                      exit(1);
                    }
                    strncpy(matchbuffer, argv[matcharg] + matchfilestart[i], matchfileend[i] - matchfilestart[i]);
                    if (debug) printf("matchbuffer: %s%s", matchbuffer, newline);
                    matchfilelist[i] = matchbuffer;
                }
                if (debug) {
                    printf("matchfiles = %d%s", matchfiles, newline);
                    for (i=0;i<matchfiles;i++) printf("matchfilelist[%d]: %s, %d, %d%s", i, matchfilelist[i], matchfilestart[i], matchfileend[i], newline);
                }
                if (!matchfiles) matchonly = false;
            }
            else matchonly = false;
        }
        char *s = "\0";  // initialized to avoid compiler warning
        if (recursesubdirs) printf("Processing %s%s%s and its subdirectories%s%s", quotation, argv[folderarg], quotation, newline, newline);
        else                printf("Processing directory %s%s%s%s%s", quotation, argv[folderarg], quotation, newline, newline);
        if (processdirectory(argv[folderarg], filenames, blockdevice, matchonly, matchfiles, matchfilelist, s)) return 1;
        if (filecount) {
            qsort(filenames, filecount, sizeof(char *), filesort);
            if (debug) {
                printf("sorted filenames:%s", newline);
                for (m=0;m<filecount;m++) printf("%ld: %s%s", m, filenames[m], newline);
            }
        }
        else {
            color(red);
            if (matchonly) {
                if (recursesubdirs) printf("Failed to find any matching files in %s%s%s or its subdirectories!%s", quotation, argv[folderarg], quotation, newline);
                else                printf("Failed to find any matching files in %s%s%s!%s", quotation, argv[folderarg], quotation, newline);
            }
            else {
                if (recursesubdirs) printf("Failed to find any files in %s%s%s or its subdirectories!%s", quotation, argv[folderarg], quotation, newline);
                else                printf("Failed to find any files in %s%s%s!%s", quotation, argv[folderarg], quotation, newline);
            }
            color(normal);
          return 1;
        }
    }
    else {
        // get input filenames from cmd line
        for (i=1;i<argc;i++) {
            if (strncmp(argv[i], "-", 1) == 0) {
                // exclude arguments that start with a hyphen unless they come after the special argument --
                if (specialarg == 0 || i <= specialarg) continue;
            }
            if (i==truncatearg || i==userregionarg || i==folderarg || i==matcharg || i==readretryarg || i==layerbreakarg ||
                i==fixangledevarg || i==patchvideoarg || i==patchpfiarg || i==patchdmiarg || i==patchssarg ||
                i==autouploaduserarg || i==autouploadpassarg || i==extractvideoarg || i==extractvideopartitionarg ||
                i==extractpfiarg || i==extractdmiarg || i==extractssarg || i==connectiontimeoutarg || i==dvdarg ||
                i==dvdtimeoutarg || i==userlangarg /* || i==riparg || i==ripdestarg */) continue;
            if ( stat(argv[i], &buf) == -1 ) {
                printf("ERROR: stat failed for %s (%s)%s", argv[i], strerror(errno), newline);
              continue;
            }
            if ( buf.st_mode & (S_IFREG | S_IFBLK) ) {  // regular files or block devices
        	    filenames[filecount] = argv[i];
        	    if ( buf.st_mode & S_IFBLK ) blockdevice[filecount] = true;
                if (debug) {
                    printf("%ld: %s", filecount, filenames[filecount]);
                    if (blockdevice[filecount]) printf(" (block device)%s", newline);
                    else printf("%s", newline);
                }
                filecount++;
            }
            else if (debug) {
                color(yellow);
                printf("%s is not a regular file or block device! st_mode = 0x%X%s", argv[i], buf.st_mode, newline);
                color(normal);
            }
        }
        if (filecount) {
            qsort(filenames, filecount, sizeof(char *), filesort);
            if (debug) for (m=0;m<filecount;m++) printf("%ld: %s%s", m, filenames[m], newline);
        }
        else {
            color(red);
            printf("ERROR: No valid input files were specified!%s", newline);
            color(normal);
          return 1;
        }
    }
    if (filecount == 1) {
        // open the only file from filenames list
        isofilename = calloc(strlen(filenames[0]) + 1, sizeof(char));
        if (isofilename == NULL) {
            color(red);
            printf("ERROR: Memory allocation for isofilename failed! Game over man... Game over!%s", newline);
            color(normal);
          exit(1);
        }
        strcpy(isofilename, filenames[0]);
        fp = fopen(isofilename, "rb");
        if (fp==NULL) {
            color(red);
            printf("ERROR: Failed to open %s (%s)%s", isofilename, strerror(errno), newline);
            color(normal);
          return 1;
        }
        if (blockdevice[0]) {
            writefile = false;
        }
    }
    
    unsigned long fileloop;
    
    for (fileloop=0;fileloop<filecount;fileloop++) {
        if (filecount > 1) {
            if (fileloop) {
                if (fp != NULL) fclose(fp);
                resetvars();
                parsecmdline(argc, argv);
                printf("%s", newline);
            }
            // open file from filenames list
            color(filename);
            printf("%s:", filenames[fileloop]);
            color(normal);
            printf("%s", newline);
            if (fileloop) isofilename = realloc(isofilename, (strlen(filenames[fileloop]) + 1) * sizeof(char));
            else isofilename = calloc(strlen(filenames[fileloop]) + 1, sizeof(char));
            if (isofilename == NULL) {
                color(red);
                printf("ERROR: memory allocation for isofilename failed! Game over man... Game over!%s", newline);
                color(normal);
              exit(1);
            }
            strcpy(isofilename, filenames[fileloop]);
            fp = fopen(isofilename, "rb");
            if (fp == NULL) {
                // couldn't open the file
                color(red);
                printf("ERROR: Failed to open %s (%s)%s", isofilename, strerror(errno), newline);
                color(normal);
              continue;
            }
            if (blockdevice[fileloop]) writefile = false;
        }
        
        #ifdef WIN32
            fpfilesize = getfilesize(fp);
            if (fpfilesize == -1) continue;
            if (debug) printf("fpfilesize: %"LL"d%s", fpfilesize, newline);
        #else
            if (blockdevice[fileloop]) {
                if (opendevice(isofilename, "Getting media size") != 0) continue;
                fpfilesize = getdisccapacity(true);
                if (fpfilesize == -1) {
                    close(fd);
                  continue;
                }
                if (debug) printf("fpfilesize: %"LL"d%s", fpfilesize, newline);
                if (skiplayerboundaryinfo) L0capacity = 0;
                else getL0capacity();
                close(fd);
            }
            else {
                fpfilesize = getfilesize(fp);
                if (fpfilesize == -1) continue;
                if (debug) printf("fpfilesize: %"LL"d%s", fpfilesize, newline);
            }
        #endif
        
        if (truncatefile) {
            long long truncatesize = 0;
            if (strncasecmp(argv[truncatearg], "0x", 2) == 0) truncatesize = strtoll(argv[truncatearg], NULL, 16);
            else truncatesize = strtoll(argv[truncatearg], NULL, 10);
            if (truncatesize > 0) {
                if (writefile) {
                    if (dotruncate(isofilename, fpfilesize, truncatesize, false) != 0) continue;
                    if (verbose) printf("%s", newline);
                    // get the new filesize
                    fpfilesize = getfilesize(fp);
                    if (fpfilesize == -1) continue;  // seek error
                }
                else {
                    color(yellow);
                    if (truncatesize < fpfilesize) printf("Truncating file was aborted because writing is disabled%s", newline);
                    else printf("Extending file was aborted because writing is disabled%s", newline);
                    color(normal);
                    if (verbose) printf("%s", newline);
                }
            }
        }
        
        if (fpfilesize < 2048LL) {
            color(yellow);
            if (filecount > 1) printf("ERROR: filesize is only %"LL"d bytes (too small to be an Xbox 360 ISO or stealth file)%s", fpfilesize, newline);
            else printf("ERROR: %s is only %"LL"d bytes (too small to be an Xbox 360 ISO or stealth file)%s", isofilename, fpfilesize, newline);
            color(normal);
          continue;
        }
        
        // check to see if it's an spa
        if (strlen(isofilename) > 4 && strncasecmp(isofilename+strlen(isofilename)-4, ".spa", 4) == 0) {
            initcheckread();
            memset(ubuffer, 0, 2048);
            if (checkreadandprinterrors(ubuffer, 1, 2048, fp, 0, 0, isofilename, "checking .spa file") != 0) continue;
            donecheckread(isofilename);
            // look for spa magic bytes
            if (memcmp(ubuffer, "XDBF", 4) != 0) {
                color(red);
                printf("ERROR: %sXDBF%s magic was not found at the start of .spa file %s%s%s!%s",
                        quotation, quotation, quotation, isofilename, quotation, newline);
                color(normal);
              continue;
            }
            if (fpfilesize > WOW_THATS_A_LOT_OF_RAM) {
                if (debug) {
                    color(yellow);
                    printf("fpfilesize (%"LL"d) is greater than WOW_THATS_A_LOT_OF_RAM (%d)%s", fpfilesize, WOW_THATS_A_LOT_OF_RAM, newline);
                    color(normal);
                }
                fprintf(stderr, "Warning: Checking this SPA file will require %.1f MBs of RAM...\n",
                                (float) fpfilesize/1048576);
                char response[4];
                memset(response, 0, 4);
                while (response[0] != 'y' && response[0] != 'n' && response[0] != 'Y' && response[0] != 'N') {
                    fprintf(stderr, "Do you want to continue? (y/n) ");
                    readstdin(response, 4);
                    if (debug) printf("response[0] = %c (0x%02X)%s", response[0], response[0], newline);
                }
                if (response[0] == 'n' || response[0] == 'N') {
                    printf("SPA check was aborted as requested%s", newline);
                  continue;
                }
            }
            unsigned char *resourcebuffer = malloc(fpfilesize * sizeof(char));
            if (resourcebuffer == NULL) {
                color(red);
                printf("ERROR: Memory allocation for resourcebuffer failed! Game over man... Game over!%s", newline);
                color(normal);
              exit(1);
            }
            // seek back to start of the file
            if (fseeko(fp, 0, SEEK_SET) != 0) {
                printseekerror(isofilename, "Checking .spa file");
                free(resourcebuffer);
              continue;
            }
            // read the entire .spa file into the buffer
            initcheckread();
            if (checkreadandprinterrors(resourcebuffer, 1, fpfilesize, fp, 0, 0, isofilename, "checking .spa file") != 0) {
                free(resourcebuffer);
              continue;
            }
            donecheckread(isofilename);
            if (debug) {
                printf(".spa file 1st sector:%s", newline);
                hexdump(resourcebuffer, 0, 2048);
            }
            // check it
            if (verbose) printf("Checking SPA%s", newline);
            parsetitleidresource(resourcebuffer, fpfilesize, NULL);
            free(resourcebuffer);
          continue;
        }
        
        // check to see if it's an xex
        if (strlen(isofilename) > 4 && strncasecmp(isofilename+strlen(isofilename)-4, ".xex", 4) == 0) {
            initcheckread();
            memset(ubuffer, 0, 2048);
            if (checkreadandprinterrors(ubuffer, 1, 2048, fp, 0, 0, isofilename, "checking .xex file") != 0) continue;
            donecheckread(isofilename);
            // look for XEX magic bytes
            if (memcmp(ubuffer, "XEX2", 4) != 0) {
                color(red);
                printf("ERROR: %sXEX2%s magic was not found at the start of .xex file %s%s%s!%s",
                        quotation, quotation, quotation, isofilename, quotation, newline);
                color(normal);
              continue;
            }
            if (fpfilesize > WOW_THATS_A_LOT_OF_RAM) {
                if (debug) {
                    color(yellow);
                    printf("fpfilesize (%"LL"d) is greater than WOW_THATS_A_LOT_OF_RAM (%d)%s", fpfilesize, WOW_THATS_A_LOT_OF_RAM, newline);
                    color(normal);
                }
                fprintf(stderr, "Warning: Checking this Xex file will require %.1f MBs of RAM...\n",
                                (float) fpfilesize/1048576);
                char response[4];
                memset(response, 0, 4);
                while (response[0] != 'y' && response[0] != 'n' && response[0] != 'Y' && response[0] != 'N') {
                    fprintf(stderr, "Do you want to continue? (y/n) ");
                    readstdin(response, 4);
                    if (debug) printf("response[0] = %c (0x%02X)%s", response[0], response[0], newline);
                }
                if (response[0] == 'n' || response[0] == 'N') {
                    printf("Xex check was aborted as requested%s", newline);
                  continue;
                }
            }
            unsigned char *defaultxexbuffer = malloc(fpfilesize * sizeof(char));
            if (defaultxexbuffer == NULL) {
                color(red);
                printf("ERROR: Memory allocation for defaultxexbuffer failed! Game over man... Game over!%s", newline);
                color(normal);
              exit(1);
            }
            // seek back to start of the file
            if (fseeko(fp, 0, SEEK_SET) != 0) {
                printseekerror(isofilename, "Checking .xex file");
                free(defaultxexbuffer);
              continue;
            }
            // read the entire .xex file into the buffer
            initcheckread();
            if (checkreadandprinterrors(defaultxexbuffer, 1, fpfilesize, fp, 0, 0, isofilename, "checking .xex file") != 0) {
                free(defaultxexbuffer);
              continue;
            }
            donecheckread(isofilename);
            if (debug) {
                printf(".xex file 1st sector:%s", newline);
                hexdump(defaultxexbuffer, 0, 2048);
            }
            // check it
            if (verbose) printf("Checking XEX%s", newline);
            if (checkdefaultxex(defaultxexbuffer, fpfilesize) != 0) {
                free(defaultxexbuffer);
              continue;
            }
            free(defaultxexbuffer);
          continue;
        }
        
        // ss and stealth files are 2 KB
        if (fpfilesize == 2048LL) {
            // auto detect if this is dmi, pfi or ss
            initcheckread();
            memset(ubuffer, 0, 2048);
            if (checkreadandprinterrors(ubuffer, 1, 2048, fp, 0, 0, isofilename, "checking 2KB file") != 0) continue;
            donecheckread(isofilename);
            if (getzeros(ubuffer, 0, 2047) == 2048) {
                color(yellow); printf("ERROR: %s is blank!%s", isofilename, newline); color(normal);
              continue;
            }
            else if (lookslike360dmi(ubuffer) || lookslikexbox1dmi(ubuffer)) {
                justastealthfile = true;
                checkdmi(ubuffer);
            }
            else if (lookslikepfi(ubuffer)) {
                justastealthfile = true;
                checkpfi(ubuffer);
            }
            else if (lookslike360ss(ubuffer) || lookslikexbox1ss(ubuffer)) {
                checkssbin = true;
                memcpy(ss, ubuffer, 2048);
                if (filecount > 1 && verbose) printf("%s", newline);
                checkss();
                if (fixedss) {
                    if (verbose) printf("%s", newline);
                    printf("Writing adjusted SS values to %s%s", isofilename, newline);
                    fp = freopen(isofilename, "rb+", fp);
                    if (fp == NULL) {
                        color(red);
                        printf("ERROR: Failed to reopen %s for writing! (%s) Unable to permanently adjust SS values!%s", isofilename, strerror(errno), newline);
                        color(normal);
                      continue;
                    }
                    if (trytowritestealthfile(ss, 1, 2048, fp, isofilename, 0) != 0) continue;
                }
            }
            else {
                color(yellow);
                if (filecount > 1) printf("ERROR: filesize is 2KB but isn't recognized as DMI, PFI or SS!%s", newline);
                else printf("ERROR: %s is 2KB but isn't recognized as DMI, PFI or SS!%s", isofilename, newline);
                color(normal);
                if (verbose) {
                    printf("Displaying suspicious file in hex and ascii:%s", newline);
                    hexdump(ubuffer, 0, 2048);
                }
              continue;
            }
          continue;
        }
        
        if (fpfilesize < 34816LL) {  // 34 KB (prevent EOF when trying to read possible video file)
            color(yellow);
            if (filecount > 1) printf("ERROR: file isn't recognized as an XBOX 360 ISO or Stealth file!%s", newline);
            else printf("ERROR: %s isn't recognized as an XBOX 360 ISO or Stealth file!%s", isofilename, newline);
            color(normal);
          continue;
        }
        
        if (fpfilesize <= 265873408LL) {
            // this is the maximum size of a video partition with padding data (pre-ap25 size so that older full video partition rips will be detected)
            // check to see if this is a video file by seeking to sector 16 and looking for magic bytes
            if (fseeko(fp, 32768, SEEK_SET) != 0) {
                printseekerror(isofilename, "File identification check");
              continue;
            }
            memset(buffer, 0, 2048);
            initcheckread();
            if (checkreadandprinterrors(buffer, 1, 2048, fp, 0, 32768, isofilename, "file identification check") != 0) continue;
            donecheckread(isofilename);
            if (memcmp(buffer+1, "CD001", 5) == 0) {
                if (filecount > 1 && verbose) printf("%s", newline);
                checkvideo(isofilename, fp, true, checkpadding);
              continue;
            }
            else {
                color(yellow);
                if (filecount > 1) printf("ERROR: file isn't recognized as an XBOX 360 ISO or Stealth file!%s", newline);
                else printf("ERROR: %s isn't recognized as an XBOX 360 ISO or Stealth file!%s", isofilename, newline);
                color(normal);
              continue;
            }
        }
        
        // check to see if this is an xbox 360 iso by looking for magic bytes at sector 32
        if (fseeko(fp, 65536, SEEK_SET) != 0) {
            printseekerror(isofilename, "ISO check");
          continue;
        }
        video = 0;
        initcheckread();
        if (checkreadandprinterrors(ubuffer, 1, 2048, fp, 0, 65536LL, isofilename, "Reading Sector 32") != 0) continue;
        donecheckread(isofilename);
        if (memcmp(ubuffer, "MICROSOFT*XBOX*MEDIA", 20) != 0) {
            // the vast majority of images have a video partition and the game partition starts at 0xFD90000
            video = 0xFD90000LL;
            if (fpfilesize < (long long) video + 67584LL) {  // video + 66 KB (prevent EOF when trying to read sector 32)
                color(yellow);
                if (filecount > 1) printf("ERROR: file isn't recognized as an XBOX 360 ISO or Stealth file!%s", newline);
                else printf("ERROR: %s isn't recognized as an XBOX 360 ISO or Stealth file!%s", isofilename, newline);
                color(normal);
              continue;
            }
            if (fseeko(fp, 65536+video, SEEK_SET) != 0) {
                printseekerror(isofilename, "ISO check");
              continue;
            }
            initcheckread();
            if (checkreadandprinterrors(ubuffer, 1, 2048, fp, 0, video+65536LL, isofilename, "Reading Sector 32") != 0) continue;
            donecheckread(isofilename);
            if (memcmp(ubuffer, "MICROSOFT*XBOX*MEDIA", 20) != 0) {
                // this string should be here for all xbox 360 games
                color(yellow);
                if (filecount > 1) printf("ERROR: file isn't recognized as an XBOX 360 ISO or Stealth file!%s", newline);
                else printf("ERROR: %s isn't recognized as an XBOX 360 ISO or Stealth file!%s", isofilename, newline);
                color(normal);
              continue;
            }
        }
        /*
        if (removesplitvid && video == 0xFD90000LL && fpfilesize > 7572881408LL) {
            if (!writefile) {
                color(yellow);
                printf("Unable to remove SplitVid because writing is disabled!%s", newline);
                color(normal);
                if (verbose) printf("%s", newline);
            }
            else {
                // remove splitvid (just truncate file to 7572881408 bytes)
                printf("Removing SplitVid");
                if (verbose) printf(" (truncating file from %"LL"d bytes to 7572881408 bytes)%s", fpfilesize, newline);
                if (verbose) printf("%s", newline);
                if (dotruncate(isofilename, fpfilesize, 7572881408LL, true) != 0) continue;
                // get the new filesize
                fpfilesize = getfilesize(fp);
                if (fpfilesize == -1) continue;  // seek error
            }
        }
        */
        if (checkdvdfile) {
            if (video == 0) {
                if (!stealthcheck) {  // avoid redundant message when checking stealth
                    color(yellow);
                    printf("ERROR: %s is just a game partition, don't burn it unless you want a coaster! (.dvd file check aborted)%s%s", isofilename, newline, newline);
                    color(normal);
                }
            }
            else docheckdvdfile();
        }
        
        if (manualextract) {
            if (video == 0) {
                color(yellow);
                printf("ERROR: %s is just a game partition (no stealth files to extract)%s%s", isofilename, newline, newline);
                color(normal);
            }
            else domanualextraction(argv);
        }
        
        checkgame();
        
        if (xex_foundmediaid && !game_has_ap25) {
            // check media id against list of ap25 games with no ap25 xex flag
            for (m=0;m<num_ap25mediaids;m++) {
                if (memcmp(xex_mediaid+12, mostrecentap25mediaids[m].mediaid, 4) == 0) {
                    game_has_ap25 = true;
                    break;
                }
            }
        }
        
        if (stealthcheck) {
            printf("%s", newline);
            if (video == 0) {
                // means that "MICROSOFT*XBOX*MEDIA" was found at 0x10000
                color(red);
                printf("Stealth Failed!: Image is missing Video, PFI, DMI and SS%s", newline);
                printf("%s %s (It's just a game partition and won't boot on ANY firmware!)%s", sp10, sp5, newline);
                color(normal);
                if (autofix) {
                    returnvalue = doautofix();
                    if (returnvalue == 0) {
                        color(green);
                        printf("AutoFix was successful!%s", newline);
                        color(normal);
                    }
                    else if (returnvalue == 1) {
                        color(red);
                        printf("AutoFix Failed!%s", newline);
                        color(normal);
                        if (manualpatch) domanualpatch(argv);
                    }
                }
                else if (manualpatch) domanualpatch(argv);
                if (game_has_ap25) {
                    // lazy...
                    color(yellow);
                    printf("Note: You will need to run this ISO through abgx360 again after rebuilding to check AP25 replay data%s", newline);
                    color(normal);
                }
              goto finishedstealthcheck;
            }
            
            if (game_has_ap25) {
                // check ap25 replay data
                if (fseeko(fp, 0xFD8E000, SEEK_SET) != 0) {
                    printseekerror(isofilename, "Stealth check");
                  continue;
                }
                initcheckread();
                if (checkreadandprinterrors(ap25, 1, 2048, fp, 0, 0xFD8E000, isofilename, "Stealth check") != 0) continue;
                donecheckread(isofilename);
                checkap25();
                if (fixedap25) {
                    if (verbose) printf("%s", newline);
                    printf("Writing fixed AP25 data to this ISO%s", newline);
                    fp = freopen(isofilename, "rb+", fp);
                    if (fp == NULL) {
                        color(red);
                        printf("ERROR: Failed to reopen %s%s%s for writing! (%s) Unable to fix AP25 replay sector!%s",
                               quotation, isofilename, quotation, strerror(errno), newline);
                        color(normal);
                        fp = fopen(isofilename, "rb");
                        if (fp == NULL) {
                            color(red);
                            printf("ERROR: Failed to reopen %s%s%s for reading! (%s)%s",
                                   quotation, isofilename, quotation, strerror(errno), newline);
                            color(normal);
                          continue;
                        }
                      continue;
                    }
                    if (trytowritestealthfile(ap25, 1, 2048, fp, isofilename, 0xFD8E000LL) != 0) continue;
                    color(green);
                    printf("AP25 replay sector was successfully fixed!%s", newline);
                    color(normal);
                }
                if (verbose) printf("%s", newline);
            }
            
            // check SS
            if (fseeko(fp, 0xFD8F800, SEEK_SET) != 0) {
                printseekerror(isofilename, "Stealth check");
              continue;
            }
            initcheckread();
            if (checkreadandprinterrors(ss, 1, 2048, fp, 0, 0xFD8F800, isofilename, "Stealth check") != 0) continue;
            donecheckread(isofilename);
            checkss();
            if (fixedss) {
                if (verbose) printf("%s", newline);
                printf("Writing adjusted SS values to %s%s", isofilename, newline);
                fp = freopen(isofilename, "rb+", fp);
                if (fp == NULL) {
                    color(red);
                    printf("ERROR: Failed to reopen %s for writing! (%s) Unable to permanently adjust SS values!%s", isofilename, strerror(errno), newline);
                    color(normal);
                    fp = fopen(isofilename, "rb");
                    if (fp == NULL) {
                        color(red);
                        printf("ERROR: Failed to reopen %s for reading! (%s)%s", isofilename, strerror(errno), newline);
                        color(normal);
                      continue;
                    }
                  continue;
                }
                if (trytowritestealthfile(ss, 1, 2048, fp, isofilename, 0xFD8F800LL) != 0) continue;
            }
            
            // check DMI
            if (fseeko(fp, 0xFD8F000, SEEK_SET) != 0) {
                printseekerror(isofilename, "Stealth check");
              continue;
            }
            initcheckread();
            if (checkreadandprinterrors(ubuffer, 1, 2048, fp, 0, 0xFD8F000, isofilename, "Stealth check") != 0) continue;
            donecheckread(isofilename);
            checkdmi(ubuffer);
            
            // check PFI
            if (fseeko(fp, 0xFD8E800, SEEK_SET) != 0) {
                printseekerror(isofilename, "Stealth check");
              continue;
            }
            initcheckread();
            if (checkreadandprinterrors(ubuffer, 1, 2048, fp, 0, 0xFD8E800, isofilename, "Stealth check") != 0) continue;
            donecheckread(isofilename);
            checkpfi(ubuffer);
            
            // check Video
            if (verbose) printf("%s", newline);
            checkvideo(isofilename, fp, false, checkpadding);
            
            if (video_stealthfailed || pfi_stealthfailed || dmi_stealthfailed || ss_stealthfailed || stealthfailed) {
                color(red);
                printf("%sStealth check failed!%s", newline, newline);
                color(normal);
                if (autofix) {
                    returnvalue = doautofix();
                    if (returnvalue == 0) {
                        color(green);
                        printf("AutoFix was successful!%s", newline);
                        color(normal);
                    }
                    else if (returnvalue == 1) {
                        color(red);
                        printf("AutoFix Failed!%s", newline);
                        color(normal);
                        if (manualpatch) domanualpatch(argv);
                    }
                }
                else if (manualpatch) domanualpatch(argv);
            }
            else if (video_stealthuncertain || pfi_stealthuncertain || dmi_stealthuncertain || ss_stealthuncertain || stealthuncertain) {
                color(yellow);
                printf("%sStealth status is uncertain%s", newline, newline);
                color(normal);
                if (verify) {
                    returnvalue = doverify();
                    if (returnvalue == 0) {
                        if (drtfucked) {
                            color(yellow);
                            printf("Stealth and game data were verified but angle deviation cannot be verified%s", newline);
                        }
                        else {
                            color(green);
                            printf("Verification was successful, Stealth passed!%s", newline);
                        }
                        color(normal);
                        // this is only here to make the splitvid check/addition possible
                        video_stealthuncertain = false;
                        pfi_stealthuncertain = false;
                    }
                    else if (returnvalue == 2) {
                        color(yellow);
                        printf("Stealth was verified but game data wasn't%s", newline);
                        if (drtfucked) printf("Angle deviation cannot be verified%s", newline);
                        color(normal);
                        // this is only here to make the splitvid check/addition possible
                        video_stealthuncertain = false;
                        pfi_stealthuncertain = false;
                    }
                    else {
                        verifyfailed = true;
                        color(yellow);
                        if (returnvalue == 3) printf("Game data was verified but stealth wasn't%s", newline);
                        if (drtfucked) printf("Angle deviation cannot be verified%s", newline);
                        printf("Verification failed%s", newline);
                        color(normal);
                    }
                    if (verifyfailed || drtfucked) {
                        if ((autofix && autofixuncertain) || (verify_found_bad_pfi_or_video && autofix)) {
                            returnvalue = doautofix();
                            if (returnvalue == 0) {
                                color(green);
                                printf("AutoFix was successful!%s", newline);
                                color(normal);
                            }
                            else if (returnvalue == 1) {
                                color(yellow);
                                printf("AutoFix Failed, Stealth is still uncertain%s", newline);
                                color(normal);
                                if (manualpatch) domanualpatch(argv);
                            }
                        }
                        else if (manualpatch) domanualpatch(argv);
                    }
                }
                else if (manualpatch) domanualpatch(argv);
            }
            else {
                color(green);
                printf("%sBasic Stealth check passed!%s", newline, newline);
                color(normal);
                // careful not to verify/autoupload after manual patch (or autofix)
                if (verify) {
                    returnvalue = doverify();
                    if (returnvalue == 0) {
                        color(green);
                        printf("Verification was successful!%s", newline);
                        color(normal);
                    }
                    else if (returnvalue == 2) {
                        color(yellow);
                        printf("Stealth was verified but game data wasn't%s", newline);
                        color(normal);
                    }
                    else {
                        verifyfailed = true;
                        color(yellow);
                        if (returnvalue == 3) printf("Game data was verified but stealth wasn't%s", newline);
                        else printf("Verification failed%s", newline);
                        color(normal);
                        if (verify_found_bad_pfi_or_video) {
                            autoupload = false;  // should have been set to false already but it doesn't hurt to make sure
                            if (autofix) {
                                returnvalue = doautofix();
                                if (returnvalue == 0) {
                                    color(green);
                                    printf("AutoFix was successful!%s", newline);
                                    color(normal);
                                }
                                else if (returnvalue == 1) {
                                    color(red);
                                    printf("AutoFix Failed, PFI/Video is still bad%s", newline);
                                    color(normal);
                                    if (manualpatch) domanualpatch(argv);
                                }
                            }
                            else {
                                color(red);
                                printf("%sSet AutoFix Threshold to Level 1 or higher in order to fix the bad PFI/Video%s", newline, newline);
                                color(normal);
                                if (manualpatch) domanualpatch(argv);
                            }
                        }
                        else if (!autofixalways && !noxexiniavailable)
                            printf("%sSet AutoFix Threshold to Level 3 if you want to try and find a verified SS and%s"
                                   "Stealth files that match the Xex, and automatically patch them if found.%s",
                                   newline, newline, newline);
                        if (autoupload && !stayoffline && autouploaduserarg && autouploadpassarg) {
                            if (fixedss) {
                                color(red);
                                printf("%sAutoUpload aborted because SS Challege / Response Data has been altered!%s"
                                       "DO NOT AutoUpload from this ISO! Delete it and rip again!%s%s",
                                       newline, newline, newline, newline);
                                color(normal);
                            }
                            else doautoupload(argv);
                            // should check to see if autoupload resulted in verification
                        }
                        if (!verify_found_bad_pfi_or_video && autofixalways) {
                            returnvalue = doautofix();
                            if (returnvalue == 0) {
                                color(green);
                                printf("AutoFix was successful!%s", newline);
                                color(normal);
                            }
                            else if (returnvalue == 1) {
                                color(yellow);
                                printf("AutoFix Failed, Stealth is still unverified%s", newline);
                                color(normal);
                                if (manualpatch && patchifstealthpasses) domanualpatch(argv);
                            }
                        }
                        else if (manualpatch && patchifstealthpasses) domanualpatch(argv);
                    }
                }
                else if (!verify_found_bad_pfi_or_video && manualpatch && patchifstealthpasses) domanualpatch(argv);
            }
            finishedstealthcheck:
            if (addsplitvid) {
                printf("%s", newline);
                doaddsplitvid();
            }
            if (checkcorruption && game_crc32 == 0 && !checkgamecrcnever) {
                printf("%sChecking for AnyDVD style game data corruption while running CRC check...%s", newline, newline);
                if (docheckgamecrc() == 0) {
                    if (corruptionoffsetcount == 0) {
                        color(green);
                        if (verbose) printf("%s", sp5);
                        printf("AnyDVD style corruption was not detected%s", newline);
                        color(normal);
                    }
                    if (verbose) printf("%sGame CRC = %08lX%s%s", sp5, game_crc32, newline, newline);
                }
                else {
                    checkgamecrcnever = true;
                    gamecrcfailed = true;
                }
            }
        }
    }
  return 0;
}

int dotruncate(char *filename, long long filesize, long long truncatesize, bool stfu) {
    char *action;
    if (truncatesize < filesize) action = "Truncating";
    else action = "Extending";
    if (!writefile) {
        color(yellow); printf("%s file aborted because writing is disabled%s", action, newline); color(normal);
      return 1;
    }
    if (!stfu) printf("%s %s%s%s to %"LL"d Bytes... ", action, quotation, filename, quotation, truncatesize);
    if (truncatesize > filesize) {
        // make sure we have enough free space
        long long freespacerequired = truncatesize - filesize;
        long long freespace = freediskspace(filename);
        if (freespace < freespacerequired) {
            color(red);
            printf("ERROR: Not enough free disk space! You need to free at least %"LL"d MB "
                   "on the partition your file is located. Extending file was aborted!%s",
                    (freespacerequired - freespace) / 1048576, newline);
            color(normal);
          return 1;
        }
    }
    #ifdef WIN32
        HANDLE hFile = CreateFile((LPCTSTR) filename, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            color(red); printf("ERROR: CreateFile returned an invalid handle value! (%s) %s file failed!%s", WinErrorString(), action, newline); color(normal);
          return 1;
        }
        if (SetFilePointerEx(hFile, (LARGE_INTEGER) truncatesize, NULL, FILE_BEGIN) == 0) {
            color(red); printf("ERROR: SetFilePointerEx failed! (%s) %s file failed!%s", WinErrorString(), action, newline); color(normal);
          return 1;
        }
        if (SetEndOfFile(hFile) == 0) {
            color(red); printf("ERROR: SetEndOfFile failed! (%s) %s file failed!%s", WinErrorString(), action, newline); color(normal);
          return 1;
        }
        CloseHandle(hFile);
    #else
        if (truncate(filename, truncatesize) == -1) {
            color(red); printf("ERROR: truncate() returned -1! (%s) %s file failed!%s", strerror(errno), action, newline); color(normal);
          return 1;
        }
    #endif
    if (!stfu) printf("Done%s", newline);
  return 0;
}

void printpfitable(unsigned long startpsnL0, unsigned long endpsnL0, unsigned long startpsnL1, unsigned long endpsnL1,
                   unsigned long sectorsL0, unsigned long sectorsL1, unsigned long long offsetL0, unsigned long long offsetL0end,
                   unsigned long long offsetL1, unsigned long long offsetend, unsigned long sectorstotal) {
    if (altlayerbreak) printf("%sUsing layerbreak: %ld%s%s", sp5, layerbreak, newline, newline);
    printf("%s%06lXh ", sp5, startpsnL0); color(arrow);
    if (terminal) printf("ÄÄÄÄÄÄÄ PSN ÄÄÄÄÄÄ%s", greaterthan);
    else printf("------- PSN ------%s", greaterthan);
    color(normal); printf(" %06lXh %06lXh ", endpsnL0, startpsnL1); color(arrow);
    if (terminal) printf("ÄÄÄÄÄÄÄ PSN ÄÄÄÄÄÄ%s", greaterthan);
    else printf("------- PSN ------%s", greaterthan);
    color(normal); printf(" %06lXh%s", endpsnL1, newline); color(box);
    if (terminal) printf("%sÚÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÂÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ¿%s%s", sp5, newline, sp5);
    else printf("%s+----------------------------------+----------------------------------+%s%s", sp5, newline, sp5);
    if (terminal && !html) printf("³");
    else printf("|");
    color(normal); printf("%sL0 Data Area = %07lu sectors%s", sp2, sectorsL0, sp2); color(box);
    if (terminal && !html) printf("³");
    else printf("|");
    color(normal); printf("%sL1 Data Area = %07lu sectors%s", sp2, sectorsL1, sp2); color(box);
    if (terminal && !html) printf("³");
    else printf("|");
    if (terminal) printf("%s%sÀÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÁÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÙ%s", newline, sp5, newline);
    else printf("%s%s+----------------------------------+----------------------------------+%s", newline, sp5, newline);
    color(normal); printf("%s0x%010"LL"X ", sp5, offsetL0); color(arrow);
    if (terminal) printf("ÄÄÄÄÄÄÄÄ%s", greaterthan);
    else printf("--------%s", greaterthan);
    color(normal); printf(" 0x%010"LL"X 0x%010"LL"X ", offsetL0end, offsetL1); color(arrow);
    if (terminal) printf("ÄÄÄÄÄÄÄÄ%s", greaterthan);
    else printf("--------%s", greaterthan);
    color(normal); printf(" 0x%010"LL"X%s", offsetend, newline); color(arrow);
    if (terminal) printf("%s%sÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ", sp5, lessthan);
    else printf("%s%s-----------------", sp5, lessthan);
    color(normal); printf(" %07lu sectors (%010"LL"u bytes) ", sectorstotal, (unsigned long long) sectorstotal * 2048); color(arrow);
    if (terminal) printf("ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ%s%s%s", greaterthan, newline, newline);
    else printf("----------------%s%s%s", greaterthan, newline, newline);
    color(normal);
  return;
}

void domanualextraction(char *argv[]) {
    unsigned long m;
    char response[4];
    printf("Starting Manual Extraction%s", newline);
    if (extractvideoarg) {
        FILE *extractvideofile;
        printf("Extracting Video to %s%s%s%s", quotation, argv[extractvideoarg], quotation, newline);
        if (!clobber) {
            // check if file already exists
            extractvideofile = fopen(argv[extractvideoarg], "rb");
            if (extractvideofile != NULL) {
                fclose(extractvideofile);
                if (debug) printf("file already exists, asking user for permission to overwrite%s", newline);
                printstderr = true; color(yellow);
                fprintf(stderr, "Warning: \"%s\" already exists...\n", argv[extractvideoarg]);
                color(normal); printstderr = false;
                memset(response, 0, 4);
                while (response[0] != 'y' && response[0] != 'n' && response[0] != 'Y' && response[0] != 'N') {
                    fprintf(stderr, "Do you want to overwrite it? (y/n) ");
                    readstdin(response, 4);
                    if (debug) printf("response[0] = %c (0x%02X)%s", response[0], response[0], newline);
                }
                if (response[0] == 'n' || response[0] == 'N') {
                    printf("Extracting Video was aborted as requested%s", newline);
                  goto endofextractvideo2;
                }
            }
        }
        // check pfi_sectorstotal to get the true size of video data
        if (fseeko(fp, 0xFD8E800LL, SEEK_SET) != 0) {  // seek to pfi
            printseekerror(isofilename, "Extracting Video");
          goto endofextractvideo2;
        }
        memset(ubuffer, 0, 2048);
        initcheckread();
        if (checkreadandprinterrors(ubuffer, 1, 2048, fp, 0, 0xFD8E800LL,
                                    isofilename, "extraction") != 0) goto endofextractvideo2;
        donecheckread(isofilename);
        if (getzeros(ubuffer, 0, 2047) == 2048) {
            color(red);
            printf("PFI is blank! Unable to determine true Video size, Extracting Video aborted!%s", newline);
            color(normal);
          goto endofextractvideo2;
        }
        // get Starting PSN of Data Area
        unsigned char pfi_startpsnL0hex[4] = {ubuffer[7], ubuffer[6], ubuffer[5], 0x00};
        unsigned long pfi_startpsnL0 = getuint(pfi_startpsnL0hex);
        // get End PSN of L0 Data Area
        unsigned char pfi_endpsnL0hex[4] = {ubuffer[15], ubuffer[14], ubuffer[13], 0x00};
        unsigned long pfi_endpsnL0 = getuint(pfi_endpsnL0hex);
        // invert bits in End PSN of L0 Data Area to find Starting PSN of L1 Data Area
        unsigned char pfi_startpsnL1hex[4] = {~ubuffer[15], ~ubuffer[14], ~ubuffer[13], 0x00};
        unsigned long pfi_startpsnL1 = getuint(pfi_startpsnL1hex);
        // get End PSN of L1 Data Area
        unsigned char pfi_endpsnL1hex[4] = {ubuffer[11], ubuffer[10], ubuffer[9], 0x00};
        unsigned long pfi_endpsnL1 = getuint(pfi_endpsnL1hex);
        // here be dragons
        int32_t layerbreakpsn = ~((layerbreak - 1 + 0x030000) ^ 0xFF000000);
        unsigned long pfi_sectorsL0 = pfi_endpsnL0 - pfi_startpsnL0 + 1;
        unsigned long pfi_sectorsL1 = pfi_endpsnL1 - pfi_startpsnL1 + 1;
        unsigned long long pfi_offsetL0 = ((unsigned long long) pfi_startpsnL0 - 0x030000) * 2048;
        unsigned long long pfi_offsetL1 = ((unsigned long long) pfi_startpsnL1 - (unsigned long long) layerbreakpsn) * 2048 + ((unsigned long long) layerbreak * 2048);
        unsigned long long pfi_offsetL0end = (unsigned long long) (pfi_endpsnL0 - pfi_startpsnL0 + 1) * 2048 + pfi_offsetL0 - 1;
        unsigned long long pfi_offsetend = (pfi_endpsnL1 - pfi_startpsnL1 + 1) * 2048 + pfi_offsetL1 - 1;
        unsigned long pfi_sectorstotal = pfi_sectorsL0 + pfi_sectorsL1;
        // print that shit
        if (debug) printpfitable(pfi_startpsnL0, pfi_endpsnL0, pfi_startpsnL1, pfi_endpsnL1, pfi_sectorsL0, pfi_sectorsL1,
                                 pfi_offsetL0, pfi_offsetL0end, pfi_offsetL1, pfi_offsetend, pfi_sectorstotal);
        if (pfi_sectorstotal > total_sectors_available_for_video_data) {
            color(yellow);
            printf("PFI Data Area is too large! (%lu sectors) Extracting Video aborted!%s", pfi_sectorstotal, newline);
            color(normal);
          goto endofextractvideo2;
        }
        // ready to extract
        extractvideofile = fopen(argv[extractvideoarg], "wb");
        if (extractvideofile == NULL) {
            color(red);
            printf("ERROR: Failed to open %s%s%s for writing! (%s) Extracting Video failed!%s",
                    quotation, argv[extractvideoarg], quotation, strerror(errno), newline);
            color(normal);
          goto endofextractvideo2;
        }
        // seek back to the beginning and start extraction
        if (fseeko(fp, 0, SEEK_SET) != 0) {
            printseekerror(isofilename, "Extracting Video");
          goto endofextractvideo;
        }
        sizeoverbuffer = pfi_sectorstotal * 2048 / BIGBUF_SIZE;
        bufferremainder = pfi_sectorstotal * 2048 % BIGBUF_SIZE;
        initcheckread(); initcheckwrite();
        for (m=0;m<sizeoverbuffer;m++) {
            if (checkreadandprinterrors(bigbuffer, 1, BIGBUF_SIZE, fp, m, 0,
                                        isofilename, "extraction") != 0) goto endofextractvideo;
            if (checkwriteandprinterrors(bigbuffer, 1, BIGBUF_SIZE, extractvideofile, m, 0,
                                         argv[extractvideoarg], "extraction") != 0) goto endofextractvideo;
        }
        if (bufferremainder) {
            if (checkreadandprinterrors(bigbuffer, 1, bufferremainder, fp, 0, pfi_sectorstotal * 2048 - bufferremainder,
                                        isofilename, "extraction") != 0) goto endofextractvideo;
            if (checkwriteandprinterrors(bigbuffer, 1, bufferremainder, extractvideofile, 0, pfi_sectorstotal * 2048 - bufferremainder,
                                         argv[extractvideoarg], "extraction") !=0) goto endofextractvideo;
        }
        donecheckread(isofilename);
        donecheckwrite(argv[extractvideoarg]);
        color(green); printf("Extraction was successful%s", newline); color(normal);
        endofextractvideo:
            fclose(extractvideofile);
        endofextractvideo2:
            printf("%s", newline);
    }
    if (extractvideopartitionarg) {
        FILE *extractvideopartitionfile;
        printf("Extracting Video Partition to %s%s%s%s", quotation, argv[extractvideopartitionarg], quotation, newline);
        if (!clobber) {
            // check if file already exists
            extractvideopartitionfile = fopen(argv[extractvideopartitionarg], "rb");
            if (extractvideopartitionfile != NULL) {
                fclose(extractvideopartitionfile);
                if (debug) printf("file already exists, asking user for permission to overwrite%s", newline);
                printstderr = true; color(yellow);
                fprintf(stderr, "Warning: \"%s\" already exists...\n", argv[extractvideopartitionarg]);
                color(normal); printstderr = false;
                memset(response, 0, 4);
                while (response[0] != 'y' && response[0] != 'n' && response[0] != 'Y' && response[0] != 'N') {
                    fprintf(stderr, "Do you want to overwrite it? (y/n) ");
                    readstdin(response, 4);
                    if (debug) printf("response[0] = %c (0x%02X)%s", response[0], response[0], newline);
                }
                if (response[0] == 'n' || response[0] == 'N') {
                    printf("Extracting Video Partition was aborted as requested%s", newline);
                  goto endofextractvideopartition2;
                }
            }
        }
        // ready to extract
        extractvideopartitionfile = fopen(argv[extractvideopartitionarg], "wb");
        if (extractvideopartitionfile == NULL) {
            color(red);
            printf("ERROR: Failed to open %s%s%s for writing! (%s) Extracting Video Partition failedd!%s",
                    quotation, argv[extractvideopartitionarg], quotation, strerror(errno), newline);
            color(normal);
          goto endofextractvideopartition2;
        }
        // extract video partition
        if (fseeko(fp, 0, SEEK_SET) != 0) {
            printseekerror(isofilename, "Extracting Video Partition");
          goto endofextractvideopartition;
        }
        sizeoverbuffer = total_sectors_available_for_video_data*2048 / BIGBUF_SIZE;
        bufferremainder = total_sectors_available_for_video_data*2048 % BIGBUF_SIZE;
        initcheckread(); initcheckwrite();
        for (m=0;m<sizeoverbuffer;m++) {
            if (checkreadandprinterrors(bigbuffer, 1, BIGBUF_SIZE, fp, m, 0,
                                        isofilename, "extraction") != 0) goto endofextractvideopartition;
            if (checkwriteandprinterrors(bigbuffer, 1, BIGBUF_SIZE, extractvideopartitionfile, m, 0,
                                         argv[extractvideopartitionarg], "extraction") != 0) goto endofextractvideopartition;
        }
        if (bufferremainder) {
            if (checkreadandprinterrors(bigbuffer, 1, bufferremainder, fp, 0, total_sectors_available_for_video_data*2048 - bufferremainder,
                                        isofilename, "extraction") != 0) goto endofextractvideopartition;
            if (checkwriteandprinterrors(bigbuffer, 1, bufferremainder, extractvideopartitionfile, 0, total_sectors_available_for_video_data*2048 - bufferremainder,
                                         argv[extractvideopartitionarg], "extraction") !=0) goto endofextractvideopartition;
        }
        donecheckread(isofilename);
        donecheckwrite(argv[extractvideopartitionarg]);
        color(green); printf("Extraction was successful%s", newline); color(normal);
        endofextractvideopartition:
            fclose(extractvideopartitionfile);
        endofextractvideopartition2:
            printf("%s", newline);
    }
    if (extractpfiarg) {
        FILE *extractpfifile;
        printf("Extracting PFI to %s%s%s%s", quotation, argv[extractpfiarg], quotation, newline);
        if (!clobber) {
            // check if file already exists
            extractpfifile = fopen(argv[extractpfiarg], "rb");
            if (extractpfifile != NULL) {
                fclose(extractpfifile);
                if (debug) printf("file already exists, asking user for permission to overwrite%s", newline);
                printstderr = true; color(yellow);
                fprintf(stderr, "Warning: \"%s\" already exists...\n", argv[extractpfiarg]);
                color(normal); printstderr = false;
                memset(response, 0, 4);
                while (response[0] != 'y' && response[0] != 'n' && response[0] != 'Y' && response[0] != 'N') {
                    fprintf(stderr, "Do you want to overwrite it? (y/n) ");
                    readstdin(response, 4);
                    if (debug) printf("response[0] = %c (0x%02X)%s", response[0], response[0], newline);
                }
                if (response[0] == 'n' || response[0] == 'N') {
                    printf("Extracting PFI was aborted as requested%s", newline);
                  goto endofextractpfi2;
                }
            }
        }
        // ready to extract
        extractpfifile = fopen(argv[extractpfiarg], "wb");
        if (extractpfifile == NULL) {
            color(red); printf("ERROR: Failed to open %s%s%s for writing! (%s) Extracting PFI failed!%s",
                                quotation, argv[extractpfiarg], quotation, strerror(errno), newline); color(normal);
          goto endofextractpfi2;
        }
        if (fseeko(fp, 0xFD8E800LL, SEEK_SET) != 0) {
            printseekerror(isofilename, "Extracting PFI");
          goto endofextractpfi;
        }
        memset(buffer, 0, 2048);
        initcheckread();
        if (checkreadandprinterrors(buffer, 1, 2048, fp, 0, 0xFD8E800LL,
                                    isofilename, "Reading stealth file") != 0) goto endofextractpfi;
        donecheckread(isofilename);
        if (trytowritestealthfile(buffer, 1, 2048, extractpfifile, argv[extractpfiarg], 0) != 0) goto endofextractpfi;
        color(green); printf("Extraction was successful%s", newline); color(normal);
        endofextractpfi:
            fclose(extractpfifile);
        endofextractpfi2:
            printf("%s", newline);
    }
    if (extractdmiarg) {
        FILE *extractdmifile;
        printf("Extracting DMI to %s%s%s%s", quotation, argv[extractdmiarg], quotation, newline);
        if (!clobber) {
            // check if file already exists
            extractdmifile = fopen(argv[extractdmiarg], "rb");
            if (extractdmifile != NULL) {
                fclose(extractdmifile);
                if (debug) printf("file already exists, asking user for permission to overwrite%s", newline);
                printstderr = true; color(yellow);
                fprintf(stderr, "Warning: \"%s\" already exists...\n", argv[extractdmiarg]);
                color(normal); printstderr = false;
                memset(response, 0, 4);
                while (response[0] != 'y' && response[0] != 'n' && response[0] != 'Y' && response[0] != 'N') {
                    fprintf(stderr, "Do you want to overwrite it? (y/n) ");
                    readstdin(response, 4);
                    if (debug) printf("response[0] = %c (0x%02X)%s", response[0], response[0], newline);
                }
                if (response[0] == 'n' || response[0] == 'N') {
                    printf("Extracting DMI was aborted as requested%s", newline);
                  goto endofextractdmi2;
                }
            }
        }
        // ready to extract
        extractdmifile = fopen(argv[extractdmiarg], "wb");
        if (extractdmifile == NULL) {
            color(red);
            printf("ERROR: Failed to open %s%s%s for writing! (%s) Extracting DMI failed!%s",
                    quotation, argv[extractdmiarg], quotation, strerror(errno), newline);
            color(normal);
          goto endofextractdmi2;
        }
        if (fseeko(fp, 0xFD8F000LL, SEEK_SET) != 0) {
            printseekerror(isofilename, "Extracting DMI");
          goto endofextractdmi;
        }
        memset(buffer, 0, 2048);
        initcheckread();
        if (checkreadandprinterrors(buffer, 1, 2048, fp, 0, 0xFD8F000LL,
                                    isofilename, "Reading stealth file") != 0) goto endofextractdmi;
        donecheckread(isofilename);
        if (trytowritestealthfile(buffer, 1, 2048, extractdmifile, argv[extractdmiarg], 0) != 0) goto endofextractdmi;
        color(green); printf("Extraction was successful%s", newline); color(normal);
        endofextractdmi:
            fclose(extractdmifile);
        endofextractdmi2:
            printf("%s", newline);
    }
    if (extractssarg) {
        FILE *extractssfile;
        printf("Extracting SS to %s%s%s%s", quotation, argv[extractssarg], quotation, newline);
        if (!clobber) {
            // check if file already exists
            extractssfile = fopen(argv[extractssarg], "rb");
            if (extractssfile != NULL) {
                fclose(extractssfile);
                if (debug) printf("file already exists, asking user for permission to overwrite%s", newline);
                printstderr = true; color(yellow);
                fprintf(stderr, "Warning: \"%s\" already exists...\n", argv[extractssarg]);
                color(normal); printstderr = false;
                memset(response, 0, 4);
                while (response[0] != 'y' && response[0] != 'n' && response[0] != 'Y' && response[0] != 'N') {
                    fprintf(stderr, "Do you want to overwrite it? (y/n) ");
                    readstdin(response, 4);
                    if (debug) printf("response[0] = %c (0x%02X)%s", response[0], response[0], newline);
                }
                if (response[0] == 'n' || response[0] == 'N') {
                    printf("Extracting SS was aborted as requested%s", newline);
                  goto endofextractss2;
                }
            }
        }
        // ready to extract
        extractssfile = fopen(argv[extractssarg], "wb");
        if (extractssfile == NULL) {
            color(red);
            printf("ERROR: Failed to open %s%s%s for writing! (%s) Extracting SS failed!%s",
                    quotation, argv[extractssarg], quotation, strerror(errno), newline);
            color(normal);
          goto endofextractss2;
        }
        if (fseeko(fp, 0xFD8F800LL, SEEK_SET) != 0) {
            printseekerror(isofilename, "Extracting SS");
          goto endofextractss;
        }
        memset(buffer, 0, 2048);
        initcheckread();
        if (checkreadandprinterrors(buffer, 1, 2048, fp, 0, 0xFD8F800LL,
                                    isofilename, "Reading stealth file") != 0) goto endofextractss;
        donecheckread(isofilename);
        if (trytowritestealthfile(buffer, 1, 2048, extractssfile, argv[extractssarg], 0) != 0) goto endofextractss;
        color(green); printf("Extraction was successful%s", newline); color(normal);
        endofextractss:
            fclose(extractssfile);
        endofextractss2:
            printf("%s", newline);
    }
  return;
}

void domanualpatch(char *argv[]) {
    unsigned long m;
    if (!writefile) {
        color(yellow);
        printf("%sAborting Manual Patch because writing is disabled%s", newline, newline);
        color(normal);
      return;
    }
    printf("%sStarting Manual Patch%s", newline, newline);
    // reopen iso file for reading and writing
    fp = freopen(isofilename, "rb+", fp);
    if (fp == NULL) {
        color(red);
        printf("ERROR: Failed to reopen %s for writing! (%s) Unable to patch Stealth files!%s", isofilename, strerror(errno), newline);
        color(normal);
        fp = fopen(isofilename, "rb");
        if (fp == NULL) {
            color(red);
            printf("ERROR: Failed to reopen %s for reading! (%s) Game over man... Game over!%s", isofilename, strerror(errno), newline);
            color(normal);
          exit(1);
        }
      return;
    }
    if (video == 0) {  // just a game partition (iso needs to be rebuilt)
        if (norebuild) {
            color(red);
            printf("ERROR: You have chosen not to rebuild ISOs, but this one needs to be rebuilt!%s", newline);
            color(normal);
          return;
        }
        else if (rebuildfailed) {
            color(red);
            printf("ERROR: This ISO needs to be rebuilt but a previous attempt failed!%s", newline);
            color(normal);
          return;
        }
        fprintf(stderr, "Image is just a game partition, rebuilding... ");
        if (rebuildiso(isofilename) != 0) {
            rebuildfailed = true;
            color(red);
            printf("Rebuilding Failed!");
            if (rebuildlowspace) printf(" Your ISO is now probably corrupt!");
            printf("%s", newline);
            color(normal);
          return;
        }
        else if (checkdvdfile) {
            if (debug) printf("docheckdvdfile isofilename: %s%s", isofilename, newline);
            printf("%s", newline);
            docheckdvdfile();
        }
    }
    if (patchssarg) {
        FILE *patchssfile;
        printf("Patching SS from %s%s%s%s", quotation, argv[patchssarg], quotation, newline);
        patchssfile = fopen(argv[patchssarg], "rb");
        if (patchssfile == NULL) {
            color(red);
            printf("ERROR: Failed to open %s%s%s for reading! (%s) Patching was aborted!%s", quotation, argv[patchssarg], quotation, strerror(errno), newline);
            color(normal);
            goto endofpatchss2;
        }
        initcheckread();
        if (checkreadandprinterrors(ss, 1, 2048, patchssfile, 0, 0, argv[patchssarg], "patching") != 0) goto endofpatchss;
        donecheckread(argv[patchssarg]);
        if (getzeros(ss, 0, 2047) == 2048) {
            // ss is blank
            if (patchvalidfilesonly) {
                color(red);
                printf("ERROR: This SS file is blank! Patching was aborted!%s", newline);
                color(normal);
                goto endofpatchss;
            }
            else {
                ss_stealthfailed = true;
                color(red);
                printf("Warning: This SS file is blank but you disabled the option to patch valid files only! I hope you know what you're doing!%s", newline);
                color(normal);
            }
        }
        else {
            if (verbose) printf("%s", newline);
            checkss();
            if (ss_stealthfailed || ss_stealthuncertain) {
                // ss failed stealth check against the xex
                if (patchvalidfilesonly) {
                    color(red);
                    printf("ERROR: This SS file appears to be invalid! Patching was aborted!%s", newline);
                    color(normal);
                    goto endofpatchss;
                }
                else {
                    if (ss_stealthfailed) color(red);
                    else color(yellow);
                    printf("Warning: This SS file appears to be invalid but you disabled the option to patch valid files only! I hope you know what you're doing!%s", newline);
                    color(normal);
                }
            }
        }
        // patch it
        if (trytowritestealthfile(ss, 1, 2048, fp, isofilename, 0xFD8F800LL) != 0) goto endofpatchss;
        
        color(green);
        printf("Patching SS was successful%s", newline);
        color(normal);
        
        endofpatchss:
            fclose(patchssfile);
        endofpatchss2:
            if (patchdmiarg || patchpfiarg || patchvideoarg) printf("%s", newline);
    }
    if (patchdmiarg) {
        FILE *patchdmifile;
        printf("Patching DMI from %s%s%s%s", quotation, argv[patchdmiarg], quotation, newline);
        patchdmifile = fopen(argv[patchdmiarg], "rb");
        if (patchdmifile == NULL) {
            color(red);
            printf("ERROR: Failed to open %s%s%s for reading! (%s) Patching was aborted!%s", quotation, argv[patchdmiarg], quotation, strerror(errno), newline);
            color(normal);
            goto endofpatchdmi2;
        }
        initcheckread();
        if (checkreadandprinterrors(ubuffer, 1, 2048, patchdmifile, 0, 0, argv[patchdmiarg], "patching") != 0) goto endofpatchdmi;
        donecheckread(argv[patchdmiarg]);
        if (getzeros(ubuffer, 0, 2047) == 2048) {
            // dmi is blank
            if (patchvalidfilesonly) {
                color(red);
                printf("ERROR: This DMI file is blank! Patching was aborted!%s", newline);
                color(normal);
                goto endofpatchdmi;
            }
            else {
                dmi_stealthfailed = true;
                color(red);
                printf("Warning: This DMI file is blank but you disabled the option to patch valid files only! I hope you know what you're doing!%s", newline);
                color(normal);
            }
        }
        else {
            checkdmi(ubuffer);
            if (dmi_stealthfailed || dmi_stealthuncertain) {
                // dmi failed stealth check against the xex/ss
                if (patchvalidfilesonly) {
                    color(red);
                    printf("ERROR: This DMI file appears to be invalid! Patching was aborted!%s", newline);
                    color(normal);
                    goto endofpatchdmi;
                }
                else {
                    if (dmi_stealthfailed) color(red);
                    else color(yellow);
                    printf("Warning: This DMI file appears to be invalid but you disabled the option to patch valid files only! I hope you know what you're doing!%s", newline);
                    color(normal);
                }
            }
        }
        // patch it
        if (trytowritestealthfile(ubuffer, 1, 2048, fp, isofilename, 0xFD8F000LL) != 0) goto endofpatchdmi;
        
        color(green);
        printf("Patching DMI was successful%s", newline);
        color(normal);
        
        endofpatchdmi:
            fclose(patchdmifile);
        endofpatchdmi2:
            if (patchpfiarg || patchvideoarg) printf("%s", newline);
    }
    if (patchpfiarg) {
        FILE *patchpfifile;
        printf("Patching PFI from %s%s%s%s", quotation, argv[patchpfiarg], quotation, newline);
        patchpfifile = fopen(argv[patchpfiarg], "rb");
        if (patchpfifile == NULL) {
            color(red);
            printf("ERROR: Failed to open %s%s%s for reading! (%s) Patching was aborted!%s", quotation, argv[patchpfiarg], quotation, strerror(errno), newline);
            color(normal);
            goto endofpatchpfi2;
        }
        initcheckread();
        if (checkreadandprinterrors(ubuffer, 1, 2048, patchpfifile, 0, 0, argv[patchpfiarg], "patching") != 0) goto endofpatchpfi;
        donecheckread(argv[patchpfiarg]);
        if (getzeros(ubuffer, 0, 2047) == 2048) {
            // pfi is blank
            if (patchvalidfilesonly) {
                color(red);
                printf("ERROR: This PFI file is blank! Patching was aborted!%s", newline);
                color(normal);
                goto endofpatchpfi;
            }
            else {
                pfi_stealthfailed = true;
                color(red);
                printf("Warning: This PFI file is blank but you disabled the option to patch valid files only! I hope you know what you're doing!%s", newline);
                color(normal);
            }
        }
        else {
            checkpfi(ubuffer);
            if (pfi_stealthfailed || pfi_stealthuncertain) {
                // pfi failed stealth check
                if (patchvalidfilesonly) {
                    color(red);
                    printf("ERROR: This PFI file appears to be invalid! Patching was aborted!%s", newline);
                    color(normal);
                    goto endofpatchpfi;
                }
                else {
                    if (pfi_stealthfailed) color(red);
                    else color(yellow);
                    printf("Warning: This PFI file appears to be invalid but you disabled the option to patch valid files only! I hope you know what you're doing!%s", newline);
                    color(normal);
                }
            }
        }
        // patch it
        if (trytowritestealthfile(ubuffer, 1, 2048, fp, isofilename, 0xFD8E800LL) != 0) goto endofpatchpfi;
        
        color(green);
        printf("Patching PFI was successful%s", newline);
        color(normal);
        
        endofpatchpfi:
            fclose(patchpfifile);
        endofpatchpfi2:
            if (patchvideoarg) printf("%s", newline);
    }
    if (patchvideoarg) {
        FILE *patchvideofile;
        printf("Patching Video from %s%s%s%s", quotation, argv[patchvideoarg], quotation, newline);
        patchvideofile = fopen(argv[patchvideoarg], "rb");
        if (patchvideofile == NULL) {
            color(red);
            printf("ERROR: Failed to open %s%s%s for reading! (%s) Patching was aborted!%s", quotation, argv[patchvideoarg], quotation, strerror(errno), newline);
            color(normal);
          return;
        }
        if (verbose) printf("%s", newline);
        checkvideo(argv[patchvideoarg], patchvideofile, false, false);
        if (video_stealthfailed || video_stealthuncertain) {
            // video failed stealth check
            if (patchvalidfilesonly) {
                color(red);
                printf("ERROR: This Video file appears to be invalid! Patching was aborted!%s", newline);
                color(normal);
                goto endofpatchvideo;
            }
            else {
                if (video_stealthfailed) color(red);
                else color(yellow);
                printf("Warning: This Video file appears to be invalid but you disabled the option to patch valid files only! I hope you know what you're doing!%s", newline);
                color(normal);
            }
        }
        // patch video
        long long videofilesize = getfilesize(patchvideofile);
        if (videofilesize == -1) goto endofpatchvideo;  // seek error
        if (pfi_foundsectorstotal) {
            if (pfi_sectorstotal*2048 > videofilesize) {
                // the supplied file is not large enough based on the pfi data area size
                color(red);
                printf("ERROR: %s%s%s (%"LL"d bytes) is smaller than the PFI data area size (%"LL"d bytes) Patching was aborted!%s", quotation, argv[patchvideoarg], quotation, videofilesize, (long long) pfi_sectorstotal*2048, newline);
                color(normal);
                goto endofpatchvideo;
            }
            if (videofilesize > pfi_sectorstotal*2048) {
                // only patch the actual video data and let the rest of the video partition get padded properly
                printf("Note: %s%s%s (%"LL"d bytes) is larger than necessary (this is normal for full video partition files). Only the first %"LL"d bytes will be patched.%s", quotation, argv[patchvideoarg], quotation, videofilesize, (long long) pfi_sectorstotal*2048, newline);
                videofilesize = (long long) pfi_sectorstotal*2048;
            }
        }
        if (videofilesize > total_sectors_available_for_video_data*2048) {
            // only patch the first total_sectors_available_for_video_data*2048 bytes
            printf("Note: %s%s%s (%"LL"d bytes) is larger than the current video partition size (this is normal for older full video partition files). Only the first %"LL"d bytes will be patched.%s", quotation, argv[patchvideoarg], quotation, videofilesize, (long long) total_sectors_available_for_video_data*2048, newline);
            videofilesize = (long long) total_sectors_available_for_video_data*2048;
        }
        sizeoverbuffer = videofilesize / BIGBUF_SIZE;
        bufferremainder = videofilesize % BIGBUF_SIZE;
        if (fseeko(fp, 0, SEEK_SET) != 0) {
            printseekerror(isofilename, "Patching video file");
            goto endofpatchvideo;
        }
        if (fseeko(patchvideofile, 0, SEEK_SET) != 0) {
            printseekerror(argv[patchvideoarg], "Patching video file");
            goto endofpatchvideo;
        }
        initcheckread(); initcheckwrite();
        for (m=0;m<sizeoverbuffer;m++) {
            if (checkreadandprinterrors(bigbuffer, 1, BIGBUF_SIZE, patchvideofile, m, 0, argv[patchvideoarg], "patching") != 0) goto endofpatchvideo;
            if (checkwriteandprinterrors(bigbuffer, 1, BIGBUF_SIZE, fp, m, 0, isofilename, "patching") != 0) goto endofpatchvideo;
        }
        if (bufferremainder) {
            if (checkreadandprinterrors(bigbuffer, 1, bufferremainder, patchvideofile, 0, videofilesize - bufferremainder, argv[patchvideoarg], "patching") != 0) goto endofpatchvideo;
            if (checkwriteandprinterrors(bigbuffer, 1, bufferremainder, fp, 0, videofilesize - bufferremainder, isofilename, "patching") !=0) goto endofpatchvideo;
        }
        donecheckread(argv[patchvideoarg]); donecheckwrite(isofilename);
        
        // pad video
        if (verbose) fprintf(stderr, "Padding Video... ");
        if (padzeros(fp, isofilename, videofilesize, (long long) total_sectors_available_for_video_data*2048) != 0) goto endofpatchvideo;
        if (verbose) fprintf(stderr, "Done\n");
        
        color(green);
        printf("Patching Video was successful%s", newline);
        color(normal);
        
        endofpatchvideo:
            fclose(patchvideofile);
    }
  return;
}

int docheckdvdfile() {
    int i;
    size_t s;
    FILE *dvdfile;
    char dvdfilename[strlen(isofilename) + 1];
    char shortisofilename[strlen(isofilename) + 1];
    memset(dvdfilename, 0, strlen(isofilename) + 1);
    memset(shortisofilename, 0, strlen(isofilename) + 1);
    // copy isofilename to dvdfilename for editing
    memcpy(dvdfilename, isofilename, strlen(isofilename));
    // replace blah.xxx with blah.dvd
    if (memcmp(dvdfilename+strlen(dvdfilename)-4, ".", 1) == 0) {
        memcpy(dvdfilename+strlen(dvdfilename)-4, ".dvd", 4);
        // isofilename might be something like "C:\My Documents\imgname.iso" and
        // we want a string with just imgname.iso to compare or put in the .dvd file
        for (i=strlen(isofilename)-6;i>-1;i--) {  // work backwards to find the last slash
            if (isofilename[i] == '\\' || isofilename[i] == '/') {
                memcpy(shortisofilename, isofilename+i+1, strlen(isofilename)-i-1);  // copy the data after the slash to shortisofilename
                break;
            }
        }
        // see if the .dvd already exists for this isoname
        dvdfile = fopen(dvdfilename, "rb");
        if (dvdfile != NULL) {
            // it does, so we'll check to make sure it's valid
            memset(buffer, 0, 2048);
            if (fgets(buffer, 2048, dvdfile) == NULL) goto dvdfix;
            if (strlen(buffer) != 19 && strlen(buffer) != 20) goto dvdfix;
            if (memcmp(buffer, "LayerBreak=1913760", 18) == 0) {  // first line of .dvd file must match this
                // rest of first line should be one newline
                if (strlen(buffer) == 19) {
                    if (buffer[18] != 0x0D && buffer[18] != 0x0A) goto dvdfix;
                }
                else if (memcmp(buffer+18, "\x0D\x0A", 2) != 0) goto dvdfix;
                // read line 2
                memset(buffer, 0, 2048);
                if (fgets(buffer, 2048, dvdfile) == NULL) goto dvdfix;
                if (strlen(shortisofilename)) {
                    if (memcmp(shortisofilename, buffer, strlen(shortisofilename)) == 0) {  // second line should match shortisofilename
                        // rest of second line should be one newline (or nothing)
                        if (strlen(buffer) == strlen(shortisofilename)) goto dvdmatch;
                        if ((strlen(buffer) != strlen(shortisofilename) + 1) && (strlen(buffer) != strlen(shortisofilename) + 2)) goto dvdfix;
                        if (strlen(buffer) == strlen(shortisofilename) + 1) {
                            if (buffer[strlen(shortisofilename)] != 0x0D && buffer[strlen(shortisofilename)] != 0x0A) goto dvdfix;
                        }
                        else if (memcmp(buffer+strlen(shortisofilename), "\x0D\x0A", 2) != 0) goto dvdfix;
                        goto dvdmatch;
                    }
                    else goto dvdfix;
                }
                else if (memcmp(isofilename, buffer, strlen(isofilename)) == 0) {  // shortisofilename is blank - second line should match isofilename
                    // rest of second line should be one newline (or nothing)
                    if (strlen(buffer) == strlen(isofilename)) goto dvdmatch;
                    if ((strlen(buffer) != strlen(isofilename) + 1) && (strlen(buffer) != strlen(isofilename) + 2)) goto dvdfix;
                    if (strlen(buffer) == strlen(isofilename) + 1) {
                        if (buffer[strlen(isofilename)] != 0x0D && buffer[strlen(isofilename)] != 0x0A) goto dvdfix;
                    }
                    else if (memcmp(buffer+strlen(isofilename), "\x0D\x0A", 2) != 0) goto dvdfix;
                    dvdmatch:
                    // rest of the file should be only newlines if anything
                    memset(buffer, 0, 2048);
                    while (fgets(buffer, 2048, dvdfile) != NULL) {
                        for (s=0;s<strlen(buffer);s++) {
                            if (buffer[s] != '\x0D' && buffer[s] != '\x0A') goto dvdfix;
                        }
                        memset(buffer, 0, 2048);
                    }
                    color(green);
                    printf("%s is valid%s%s", dvdfilename, newline, newline);
                    color(normal);
                    fclose(dvdfile);
                  return 0;
                }
            }
            dvdfix:
            if (!writefile) {
                color(yellow);
                printf("%s needs to be fixed but writing is disabled!%s", dvdfilename, newline);
                color(normal);
                if (verbose) printf("%s", newline);
              return 1;
            }
            color(yellow);
            printf("%s needs to be fixed%s", dvdfilename, newline);
            color(normal);
            // reopen .dvd file and erase contents
            dvdfile = freopen(dvdfilename, "wb+", dvdfile);
            if (dvdfile == NULL) {
                color(red);
                printf("ERROR: Failed to reopen %s for writing. (%s)%s", dvdfilename, strerror(errno), newline);
                color(normal);
              return 1;
            }
            // write the correct .dvd file contents to buffer
            memset(buffer, 0, 2048);
            strcat(buffer, "LayerBreak=1913760\x0D\x0A");
            if (strlen(shortisofilename)) strcat(buffer, shortisofilename);
            else strcat(buffer, isofilename);
            strcat(buffer, "\x0D\x0A");
            // write buffer contents to the file
            initcheckwrite();
            if (checkwriteandprinterrors(buffer, 1, strlen(buffer), dvdfile, 0, 0, dvdfilename, "fixing .dvd file") != 0) return 1;
            donecheckwrite(dvdfilename);
            color(green);
            printf("%s was fixed%s%s", dvdfilename, newline, newline);
            color(normal);
            fclose(dvdfile);
          return 0;
        }
        else {
            if (!writefile) {
                color(yellow);
                printf("%s needs to be created but writing is disabled!%s", dvdfilename, newline);
                color(normal);
                if (verbose) printf("%s", newline);
              return 1;
            }
            // create .dvd file
            color(yellow);
            printf("%s needs to be created%s", dvdfilename, newline);
            color(normal);
            dvdfile = fopen(dvdfilename, "wb+");
            if (dvdfile == NULL) {
                color(red);
                printf("ERROR: Failed to open %s for writing! (%s)%s", dvdfilename, strerror(errno), newline);
                color(normal);
              return 1;
            }
            // write the correct .dvd file contents to buffer
            memset(buffer, 0, 2048);
            strcat(buffer, "LayerBreak=1913760\x0D\x0A");
            if (strlen(shortisofilename)) strcat(buffer, shortisofilename);
            else strcat(buffer, isofilename);
            strcat(buffer, "\x0D\x0A");
            // write buffer contents to the file
            initcheckwrite();
            if (checkwriteandprinterrors(buffer, 1, strlen(buffer), dvdfile, 0, 0, dvdfilename, "creating .dvd file") != 0) return 1;
            donecheckwrite(dvdfilename);
            color(green);
            printf("%s was created%s%s", dvdfilename, newline, newline);
            color(normal);
            fclose(dvdfile);
          return 0;
        }
    }
    if (debug) printf("docheckdvdfile r1 - isofilename: %s, strlen(isofilename) = %u, dvdfilename: %s, strlen(dvdfilename) = %u%s", isofilename, (unsigned int) strlen(isofilename), dvdfilename, (unsigned int) strlen(dvdfilename), newline);
  return 1;  // isofilename does not end with .xxx, it could be a device like /dev/cdrom so we won't make a .dvd file
}

long long getfilesize(FILE *fp) {
    // store starting position so we can reset it
    long long startoffset = (long long) ftello(fp);
    if (startoffset == -1) {
        color(red);
        printf("ERROR: ftello returned -1! (%s) Failed to get filesize!%s", strerror(errno), newline);
        color(normal);
      return -1;
    }
    // seek to the end and store the offset
    if (fseeko(fp, 0, SEEK_END) != 0) {
        color(red);
        printf("ERROR: Failed to seek to new file position! (%s) Failed to get filesize!%s", strerror(errno), newline);
        color(normal);
      return -1;
    }
    long long lastoffset = (long long) ftello(fp);
    if (lastoffset == -1) {
        color(red);
        printf("ERROR: ftello returned -1! (%s) Failed to get filesize!%s", strerror(errno), newline);
        color(normal);
      return -1;
    }
    // reset position
    if (fseeko(fp, startoffset, SEEK_SET) != 0) {
        color(red);
        printf("ERROR: Failed to seek back to original file position! (%s) Failed to get filesize!%s", strerror(errno), newline);
        color(normal);
      return -1;
    }
  return lastoffset;
}

// returns a random number between x and y
int randomnumber(int x, int y) {
    // initialize random number generator
    srand(time(NULL));
  return (rand() % (y - x + 1) + x);
}

void deletestealthfile(char *stealthfilename, char *localdir, bool videofile) {
    #ifdef WIN32
        if (videofile && strlen(installdirvideofilepath)) {
            // we just opened a video file from the old installdir/StealthFiles and now we need to delete it
            if (debug) printf("deleting video file '%s'%s", installdirvideofilepath, newline);
            remove(installdirvideofilepath);
            memset(installdirvideofilepath, 0, 2048);
          return;
        }
    #endif
    char fullpath[2048];
    memset(fullpath, 0, 2048);
    if (!homeless) {
        strcat(fullpath, homedir); strcat(fullpath, abgxdir); strcat(fullpath, localdir);
    }
    strcat(fullpath, stealthfilename);
    if (debug) {
        if (videofile) printf("deleting video file '%s' from localdir '%s' (fullpath: '%s')%s", stealthfilename, localdir, fullpath, newline);
        else printf("deleting stealth file '%s' from localdir '%s' (fullpath: '%s')%s", stealthfilename, localdir, fullpath, newline);
    }
    remove(fullpath);
  return;
}

int openinifromxexini() {
    int i;
    bool invalidline;
    int num_sscrcsfromxexini = 0;
    unsigned long sscrcsfromxexini[20];
    char line[11];  // 8 chars in crc + up to 2 newline chars and terminating null
    memset(line, 0, 11);
    // get a random SS crc out of the ini
    while (fgets(line, 11, xexinifile) != NULL && num_sscrcsfromxexini < 20) {
        if (debug) printf("openinifromxexini - xex line: %s%s", line, newline);
        // valid characters are 0-9, A-F, a-f
        invalidline = false;
        for (i=0;i<8;i++) if (line[i] < 0x30 || (line[i] > 0x39 && line[i] < 0x41) || (line[i] > 0x46 && line[i] < 0x61) || line[i] > 0x66) invalidline = true;
        if (invalidline) {
            if (debug) printf("invalidline = true%s", newline);
          continue;
        }
        if (line[8] != 0x0A && line[8] != 0x0D) {
            if (debug) printf("line[8] = 0x%02X%s", line[8], newline);
          continue;  // should have ended with a newline character
        }
        sscrcsfromxexini[num_sscrcsfromxexini] = strtoul(line, NULL, 16);
        if (debug) printf("openinifromxexini - found ss crc - sscrcsfromxexini[%d]: %08lX%s", num_sscrcsfromxexini, sscrcsfromxexini[num_sscrcsfromxexini], newline);
        num_sscrcsfromxexini++;
    }
    if (debug) printf("num_sscrcsfromxexini = %d%s", num_sscrcsfromxexini, newline);
    if (num_sscrcsfromxexini == 0) {
        color(yellow);
        printf("ERROR: Failed to find a valid SS CRC in '%s'%s", xexinifilename, newline);
        color(normal);
        // delete the xex ini
        fclose(xexinifile);
        deletestealthfile(xexinifilename, stealthdir, false);
      return 1;
    }
    int randomxexinicrc = randomnumber(0, num_sscrcsfromxexini - 1);
    if (num_sscrcsfromxexini > 1 && verbose) {
        printf("%s'%s' contains %d SS CRCs, randomly picked %s%d%s", sp5, xexinifilename, num_sscrcsfromxexini, numbersign, randomxexinicrc + 1, newline);
    }
    fix_ss_crc32 = sscrcsfromxexini[randomxexinicrc];
    memset(inifilename, 0, 24);
    sprintf(inifilename, "%08lX%08lX.ini", fix_ss_crc32, xex_crc32);
    inifile = NULL;
    inifile = openstealthfile(inifilename, stealthdir, webinidir, SSXEX_INI_FROM_XEX_INI, "the online verified database");
    if (inifile == NULL && num_sscrcsfromxexini > 1) {
        // get a different ss crc out of the ini
        for (i=0;i<num_sscrcsfromxexini;i++) {
            if (i == randomxexinicrc) continue;
            if (verbose) printf("%sFailed to find or open '%s', trying SS CRC %s%d...%s", sp5, inifilename, numbersign, i + 1, newline);
            fix_ss_crc32 = sscrcsfromxexini[i];
            memset(inifilename, 0, 24);
            sprintf(inifilename, "%08lX%08lX.ini", fix_ss_crc32, xex_crc32);
            inifile = openstealthfile(inifilename, stealthdir, webinidir, SSXEX_INI_FROM_XEX_INI, "the online verified database");
            if (inifile != NULL) return 0;
        }
    }
    if (inifile == NULL) {
        color(yellow);
        printf("Failed to find or open a verified Xex/SS ini file%s", newline);
        color(normal);
        // delete the xex ini
        fclose(xexinifile);
        deletestealthfile(xexinifilename, stealthdir, false);
      return 1;
    }
  return 0;
}

#define MAX_INI_LINES 60

void parseini() {
    int i;
    long long inifilesize = getfilesize(inifile);
    char line[200];
    int linesread = 0;
    ini_dmi_count = 0; ini_ss = 0; ini_pfi = 0; ini_video = 0; ini_rawss = 0; ini_v0 = 0; ini_v1 = 0; ini_game = 0; ini_xexhash = 0;
    for (i=0;i<30;i++) {
        ini_dmi[i] = 0;
    }
    if (verbose) {
        printf("%s%sUsing %s (%"LL"d bytes)", newline, sp5, inifilename, inifilesize);
    }
    if (extraverbose) {
        printf(":%s%s", newline, newline);
        color(blue);
    }
    else if (verbose) printf("%s", newline);
    memset(line, 0, 200);
    while (fgets(line, 200, inifile) != NULL && linesread < MAX_INI_LINES) {
        if (extraverbose) {
            printf("%s", line);
            if (html) printf("<br>");
        }
        for (i=2;i<12;i++) {  // shortest field is 'SS=', longest field is 'RegionFlags='
            if (line[i] == '=') {
                if (memcmp(line, "DMI", i) == 0 && ini_dmi_count < 30) {
                    ini_dmi[ini_dmi_count] = strtoul(line+i+1, NULL, 16);
                    ini_dmi_count++;
                }
                else if (memcmp(line, "SS", i) == 0) ini_ss = strtoul(line+i+1, NULL, 16);
                else if (memcmp(line, "PFI", i) == 0) ini_pfi = strtoul(line+i+1, NULL, 16);
                else if (memcmp(line, "Video", i) == 0) ini_video = strtoul(line+i+1, NULL, 16);
                else if (memcmp(line, "RawSS", i) == 0) ini_rawss = strtoul(line+i+1, NULL, 16);
                else if (memcmp(line, "V0", i) == 0) ini_v0 = strtoul(line+i+1, NULL, 16);
                else if (memcmp(line, "V1", i) == 0) ini_v1 = strtoul(line+i+1, NULL, 16);
                else if (memcmp(line, "Game", i) == 0) ini_game = strtoul(line+i+1, NULL, 16);
                else if (memcmp(line, "XexHash", i) == 0) ini_xexhash = strtoul(line+i+1, NULL, 16);
/*              else if (memcmp(line, "RegionFlags", i) == 0) memcpy(ini_regionflags, line+i+1, 8);
                else if (memcmp(line, "MediaID", i) == 0) memcpy(ini_mediaid, line+i+1, 33); */
            }
        }
        memset(line, 0, 200);
        linesread++;
    }
    if (extraverbose) {
        printf("%s", newline);
        color(normal);
    }
    if (debug && linesread >= MAX_INI_LINES && fgets(line, 200, inifile) != NULL) {
        color(yellow);
        printf("Warning: %s was longer than %d lines, is it a valid ini?%s", inifilename, MAX_INI_LINES, newline);
        color(normal);
    }
  return;
}

FILE *openstealthfile(char *stealthfilename, char *localdir, char *webdir, int type, char *location) {
    memset(installdirvideofilepath, 0, 2048);
    FILE *stealthfile = NULL;
    char fullpath[2048];
    if (type == STEALTH_FILE || type == SMALL_VIDEO_FILE || type == GIANT_VIDEO_FILE || localonly || stayoffline) goto checklocally;
    else goto checkonline;  // check for updated inis, ss files or ap25 files
    checklocally:
    // look for the stealth file locally
    #ifdef WIN32
        if (!useinstalldir && (type == SMALL_VIDEO_FILE || type == GIANT_VIDEO_FILE)) {
            // check in the old installdir/StealthFiles folder first
            // load the abgx360 install directory from the windows registry (written by the installer)
            memset(fullpath, 0, 2048);
            HKEY hkResult;
            DWORD pcbData = sizeof(fullpath);
            longreturnvalue = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "SOFTWARE\\abgx360", 0, KEY_READ, &hkResult);
            if (debug) printf("RegOpenKeyEx returned %ld, pcbData = %ld%s", longreturnvalue, pcbData, newline);
            if (longreturnvalue == ERROR_SUCCESS) {
                longreturnvalue = RegQueryValueEx(hkResult, "Install_Dir", NULL, NULL, fullpath, &pcbData);
                if (debug) printf("ReqQueryValueEx returned %ld, pcbData = %ld%s", longreturnvalue, pcbData, newline);
                if (longreturnvalue == ERROR_SUCCESS && strlen(fullpath)) {
                    if (fullpath[strlen(fullpath) - 1] != '\\' && fullpath[strlen(fullpath) - 1] != '/') strcat(fullpath, "\\");
                    strcat(fullpath, localdir);
                    strcat(fullpath, stealthfilename);
                    stealthfile = fopen(fullpath, "rb");
                    if (stealthfile != NULL) {
                        strncpy(installdirvideofilepath, fullpath, 2047);  // necessary if we need to delete the file if it's bad
                        return stealthfile;
                    }
                }
            }
        }
    #endif
    memset(fullpath, 0, 2048);
    if (!homeless) {
        strcat(fullpath, homedir); strcat(fullpath, abgxdir); strcat(fullpath, localdir);
    }
    strcat(fullpath, stealthfilename);
    stealthfile = fopen(fullpath, "rb");
    if (type == GIANT_VIDEO_FILE || localonly || stayoffline) return stealthfile;
    checkonline:
    if (stealthfile == NULL && !stayoffline) {
        // try to find the stealth file online, this will also check locally whether the file is downloaded or not
        printf("%sLooking for %s in %s%s", sp5, stealthfilename, location, newline);
        memset(curlerrorbuffer, 0, CURL_ERROR_SIZE+1);
        char fullurl[strlen(webdir) + strlen(stealthfilename) + 1];
        sprintf(fullurl, "%s%s", webdir, stealthfilename);
        char progressdata[13 + strlen(stealthfilename)];
        sprintf(progressdata, "Downloading %s", stealthfilename);
        curl_easy_reset(curl);
        if (type == SS_FILE || type == STEALTH_FILE || type == SMALL_VIDEO_FILE || type == AP25_BIN_FILE) {
            // gzip actually increases bandwidth usage for very small files like xex inis or ap25 hash files, and not really worth it for ss/xex inis due to small bandwidth savings and increased latency
            curl_easy_setopt(curl, CURLOPT_ENCODING, "");  // If a zero-length string is set, then an Accept-Encoding: header containing all supported encodings is sent.
        }
        curl_easy_setopt(curl, CURLOPT_USERAGENT, curluseragent);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlerrorbuffer);
        curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 0);  // refuse redirects (account is probably suspended and we don't want to retrieve the error page as a file)
        curl_easy_setopt(curl, CURLOPT_URL, fullurl);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connectiontimeout);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, (curl_progress_callback) curlprogress);
        curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, &progressdata);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
        if (extraverbose) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        memset(fullpath, 0, 2048);
        if (!homeless) {
            strcat(fullpath, homedir); strcat(fullpath, abgxdir); strcat(fullpath, localdir);
        }
        strcat(fullpath, stealthfilename);
        struct MyCurlFile curlstealthfile = {fullpath, NULL};
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &curlstealthfile);
        if (stat(fullpath, &buf) == 0) {
            curl_easy_setopt(curl, CURLOPT_TIMECONDITION, 1);
            curl_easy_setopt(curl, CURLOPT_TIMEVALUE, buf.st_mtime);
            if (debug) printf("%s: buf.st_mtime = %ld%s", fullpath, (long) buf.st_mtime, newline);
        }
        else {
            curl_easy_setopt(curl, CURLOPT_TIMECONDITION, 0);
            if (debug) printf("stat failed for %s (%s)%s", fullpath, strerror(errno), newline);
        }
        curlheaderprinted = false;
        if (extraverbose) fprintf(stderr, "\n");
        printstderr = true;
        color(blue);
        curlprogressstartmsecs = getmsecs();
        res = curl_easy_perform(curl);
        color(normal);
        printstderr = false;
        if (extraverbose || curlheaderprinted) fprintf(stderr, "\n");
        if (res != CURLE_OK) {  // error occurred
            if (res == CURLE_HTTP_RETURNED_ERROR) {
                if (strstr(curlerrorbuffer, "404") != NULL) {
                    if (type == XEX_INI) {  // verified xex.ini that may or may not exist
                        color(yellow);
                        printf("%sThere is no verified rip of Xex CRC %08lX in the online database%s", sp5, xex_crc32, newline);
                    }
                    else if (type == SSXEX_INI) {  // verified ss+xex.ini that may or may not exist
                        color(normal);
                        printf("%sThere is no verified rip of this Xex/SS combination in the online database%s", sp5, newline);
                    }
                    else if (type == SSXEX_INI_FROM_XEX_INI) {  // verifed ss+xex.ini where the ss is taken from an existing xex.ini and the ss+xex.ini should definitely exist unless the user is doing stupid things or we screwed up the db somehow
                        color(yellow);
                        printf("%sThe verified ini for this Xex/SS combination is missing from the online db%s", sp5, newline);
                    }
                    else if (type == UNVERIFIED_INI) {  // means we're about to do an autoupload and we're looking at the existing unverified inis to see if any of them are an exact match to ours so we don't waste our time entering details for an upload that will be rejected
                        color(normal);
                        printf("%sThere are no duplicate uploads to worry about%s", sp5, newline);
                    }
                    else if (type == SMALL_VIDEO_FILE || type == SS_FILE || type == STEALTH_FILE || type == AP25_BIN_FILE) {  // video.iso or pfi/dmi/ss/ap25.bin - should be there unless the ini we're using (and/or the stealth files it references) have been purged from the db for some reason... or maybe the user is putting his own inis in our StealthFiles folder
                        color(yellow);
                        printf("%s%s is missing from the online database%s", sp5, stealthfilename, newline);
                    }
                    else if (type == AP25_HASH_FILE) {  // ap25.sha1 file that is looked for when a game has the ap25 media flag set or media id matches the list updatable through abgx360.dat
                        color(yellow);
                        printf("%sThere is no AP25 replay sector for this game in the online database yet%s", sp5, newline);
                    }
                }
                else if ((strstr(curlerrorbuffer, "403") != NULL) || (strstr(curlerrorbuffer, "401") != NULL)) {
                    color(yellow);
                    printf("%sThe server is denying access to %s, try again later%s", sp5, stealthfilename, newline);
                }
                else {
                    color(yellow);
                    printf("%sERROR: %s%s", sp5, curlerrorbuffer, newline);
                }
            }
            else {
                stayoffline = true;
                color(yellow);
                printf("%sERROR: %s%s", sp5, curlerrorbuffer, newline);
                printf("There seems to be a problem with the db so online functions have been disabled%s"
                       "Try again later...%s", newline, newline);
            }
        }
        else {
            color(normal);
            printcurlinfo(curl, stealthfilename);
        }
        if (curlstealthfile.stream != NULL) fclose(curlstealthfile.stream);
        if (extraverbose) {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);  // reset to avoid annoying "Closing Connection ..." atexit
        }
        color(normal);
        
        stealthfile = fopen(fullpath, "rb");
    }
  return stealthfile;
}

void resetstderr() {
    int i;
    for(i=0;i<charsprinted;i++) fprintf(stderr, "\b");
  return;
}

void clearstderr() {
    int i;
    for(i=0;i<charsprinted;i++) fprintf(stderr, "\b");
    for(i=0;i<charsprinted;i++) fprintf(stderr, " ");
    for(i=0;i<charsprinted;i++) fprintf(stderr, "\b");
  return;
}

void closestderr() {
    int i;
    for(i=0;i<charsprinted;i++) fprintf(stderr, "\b");
    for(i=0;i<charsprinted;i++) fprintf(stderr, " ");
    for(i=0;i<charsprinted;i++) fprintf(stderr, "\b");
    color(normal);
    printstderr = false;
  return;
}

int trytoreadstealthfile(void *ptr, size_t size, size_t nmemb, FILE *stream, char *filename, long long offset) {
    int i;
    if (fseeko(stream, offset, SEEK_SET) != 0) {
        printseekerror(filename, "Reading stealth file");
      return 1;
    }
    if (fread(ptr, size, nmemb, stream) < nmemb) {
        charsprinted = 0;
        readerrorstotal = 0;
        printstderr = true;
        color(yellow);
        for (i=0;i<readretries;i++) {
            readerrorstotal++;
            resetstderr();
            charsprinted = fprintf(stderr, "ERROR: Read error while reading %s [%lu retries]", filename, readerrorstotal);
            if (fseeko(stream, offset, SEEK_SET) != 0) {
                printseekerror(filename, "Reading stealth file");
              return 1;
            }
            if (fread(ptr, size, nmemb, stream) == nmemb) {
                closestderr();
                printf("%s read error recovered [%lu retries]%s", filename, readerrorstotal, newline);
              return 0;
            }
        }
        // unrecovered error
        closestderr();
        color(red);
        printf("ERROR: Unrecoverable read error while reading %s! [%lu retries]%s", filename, readerrorstotal, newline);
        color(normal);
      return 1;
    }
    else return 0;
}

int trytowritestealthfile(const void *ptr, size_t size, size_t nmemb, FILE *stream, char *filename, long long offset) {
    int i;
    if (fseeko(stream, offset, SEEK_SET) != 0) {
        printseekerror(filename, "Writing stealth file");
      return 1;
    }
    if (fwrite(ptr, size, nmemb, stream) < nmemb) {
        charsprinted = 0;
        writeerrorstotal = 0;
        printstderr = true; color(yellow);
        for (i=0;i<readretries;i++) {
            writeerrorstotal++;
            resetstderr();
            charsprinted = fprintf(stderr, "ERROR: Write error while writing %s [%lu retries]", filename, writeerrorstotal);
            if (fseeko(stream, offset, SEEK_SET) != 0) {
                printseekerror(filename, "Writing stealth file");
              return 1;
            }
            if (fwrite(ptr, size, nmemb, stream) == nmemb) {
                closestderr();
                printf("%s write error recovered [%lu retries]%s", filename, writeerrorstotal, newline);
              return 0;
            }
        }
        // unrecovered error
        closestderr();
        color(red);
        printf("ERROR: Unrecoverable write error while writing %s! [%lu retries]%s", filename, writeerrorstotal, newline);
        color(normal);
      return 1;
    }
    else return 0;
}

void initcheckread() {
    charsprinted = 0;
    readerrorstotal = 0; readerrorsrecovered = 0;
  return;
}

void initcheckwrite() {
    charsprinted = 0;
    writeerrorstotal = 0; writeerrorsrecovered = 0;
  return;
}

void donecheckread(char *name) {
    if (readerrorstotal > 0) {
        closestderr();
        if (filecount > 1) {
            if (readerrorstotal == 1) printf("%s read error was recovered [%lu recovered / 1 retry]%s",
                                              name, readerrorsrecovered, newline);
            else printf("%s read errors were recovered [%lu recovered / %lu retries]%s",
                         name, readerrorsrecovered, readerrorstotal, newline);
        }
        else {
            if (readerrorstotal == 1) printf("A read error was recovered [%lu recovered / 1 retry]%s",
                                              readerrorsrecovered, newline);
            else printf("Read errors were recovered [%lu recovered / %lu retries]%s",
                         readerrorsrecovered, readerrorstotal, newline);
        }
    }
  return;
}

void donecheckwrite(char *name) {
    if (writeerrorstotal > 0) {
        closestderr();
        if (filecount > 1) {
            if (writeerrorstotal == 1) printf("%s write error was recovered [%lu recovered / 1 retry]%s",
                                               name, writeerrorsrecovered, newline);
            else printf("%s write errors were recovered [%lu recovered / %lu retries]%s",
                         name, writeerrorsrecovered, writeerrorstotal, newline);
        }
        else {
            if (writeerrorstotal == 1) printf("A write error was recovered [%lu recovered / 1 retry]%s",
                                               writeerrorsrecovered, newline);
            else printf("Write errors were recovered [%lu recovered / %lu retries]%s",
                         writeerrorsrecovered, writeerrorstotal, newline);
        }
    }
  return;
}

int checkreadandprinterrors(void *ptr, size_t size, size_t nmemb, FILE *stream, unsigned long loop, unsigned long long startoffset,
                            char *name, char *action) {
    //if ((dvdarg || riparg) && stream == NULL) {
    if (dvdarg && stream == NULL) {
        #ifdef WIN32
            if (size != 1) {
                color(red);
                printf("ERROR: size for checkreadandprinterrors is not equal to 1!%s", newline);
                color(normal);
              return 1;
            }
            unsigned long long readoffset = (unsigned long long) loop * nmemb + startoffset;
            if (readoffset % 2048) {  // should try and handle this but it's not necessary right now
                color(red);
                printf("ERROR: readoffset (0x%"LL"X) for checkreadandprinterrors does not fall on the start of a sector%s",
                       readoffset, newline);
                color(normal);
              return 1;
            }
            unsigned char cdb[10] = {0x0};
            unsigned long LBA = (unsigned long) (readoffset / 2048);
            unsigned long transferlength = 0;
            unsigned long transferlengthremainder = 0;
            if (nmemb >= 2048) {
                transferlength = (unsigned long) (nmemb / 2048);
                transferlengthremainder = (unsigned long) (nmemb % 2048);
            }
            if (transferlength > 16 || (transferlength == 16 && transferlengthremainder)) {  // might not be safe so we'll only read 16 sectors max at a time
                if (debug) {
                    color(cyan);
                    printf("checkreadandprinterrors: tranferlength > 16, LBA = 0x%lX (%lu), transferlength = %lu%s",
                            LBA, LBA, transferlength, newline);
                    color(normal);
                }
                unsigned long safereads = transferlength / 16;
                unsigned long safereadsremainder = transferlength % 16;
                unsigned long m;
                for (m=0;m<safereads;m++) {
                    if (checkreadandprinterrors(ptr+m*32768, size, 32768, stream, m, startoffset+loop*nmemb, name, action))
                  return 1;
                }
                if (safereadsremainder) {
                    if (checkreadandprinterrors(ptr+safereads*32768, size, safereadsremainder*2048, stream, 0,
                                                startoffset+loop*nmemb+safereads*32768, name, action))
                  return 1;
                }
                if (transferlengthremainder) {
                    if (checkreadandprinterrors(ptr+safereads*32768+safereadsremainder*2048, size, transferlengthremainder,
                                                stream, 0, startoffset+loop*nmemb+safereads*32768+safereadsremainder*2048,
                                                name, action))
                  return 1;
                }
            }
            else {
                void *newptr = ptr;
                if (nmemb < 2048 || transferlengthremainder) {
                    transferlength++;
                    newptr = calloc(transferlength*2048, sizeof(char));  // will be 32 KB max
                    if (newptr == NULL) {
                        color(red);
                        printf("ERROR: Memory allocation for partial sector read from DVD failed! Game over man... Game over!%s", newline);
                        color(normal);
                      exit(1);
                    }
                }
                // build read cdb command
                cdb[0] = 0x28;  // READ (10)
                // 2-5 = LBA MSB-LSB
                cdb[2] = (unsigned char) (LBA >> 24);
                cdb[3] = (unsigned char) ((LBA & 0x00FF0000L) >> 16);
                cdb[4] = (unsigned char) ((LBA & 0x0000FF00L) >> 8);
                cdb[5] = (unsigned char) (LBA & 0x000000FFL);
                // 7-8 = transfer length (blocks) MSB-LSB
                cdb[7] = (unsigned char) (transferlength >> 8);
                cdb[8] = (unsigned char) (transferlength & 0x000000FFL);
                if (sendcdb(DATA_IN, newptr, transferlength*2048, cdb, 10, true)) {
                    if (readerrorstotal == 0) {
                        printstderr = true;
                        color(yellow);
                    }
                    int i;
                    for (i=0;i<readretries;i++) {
                        readerrorstotal++;
                        resetstderr();
                        charsprinted = fprintf(stderr, "ERROR: Error reading DVD [%lu recovered / %lu retries]",
                                                        readerrorsrecovered, readerrorstotal);
                        if (sendcdb(DATA_IN, newptr, transferlength*2048, cdb, 10, true) == 0) {
                            // recovered error
                            readerrorsrecovered++;
                            resetstderr();
                            charsprinted = fprintf(stderr, "ERROR: Error reading DVD [%lu recovered / %lu retries]",
                                                            readerrorsrecovered, readerrorstotal);
                            if (nmemb < 2048 || transferlengthremainder) {
                                memcpy(ptr, newptr, nmemb);
                                free(newptr);
                            }
                          return 0;
                        }
                    }
                    // unrecovered error
                    closestderr();
                    color(red);
                    printf("ERROR: Unrecoverable read error while reading %s (%s), %s failed!%s",
                            name, cdberror(sense), action, newline);
                    color(normal);
                  return 1;
                }
                if (nmemb < 2048 || transferlengthremainder) {
                    memcpy(ptr, newptr, nmemb);
                    free(newptr);
                }
            }
          return 0;
        #else
            if (dvdarg) {
                color(red);
                printf("ERROR: WTF? WIN32 not defined but somehow dvdarg was set?%s", newline);
                color(normal);
                if (debug) printf("dvdarg = %d%s", dvdarg, newline);
              return 1;
            }
            // todo: unix sector ripping
            
          return 1;
        #endif
    }
    else {
        if (fread(ptr, size, nmemb, stream) == nmemb) return 0;
        else {
            if (readerrorstotal == 0) {
                printstderr = true;
                color(yellow);
            }
            if (feof(stream)) {  // EOF
                closestderr();
                color(red);
                printf("ERROR: End of File reached while reading %s, %s failed!%s", name, action, newline);
                color(normal);
              return 1;
            }
            unsigned long long readoffset = (unsigned long long) loop * nmemb + startoffset;
            int i;
            for (i=0;i<readretries;i++) {
                readerrorstotal++;
                resetstderr();
                charsprinted = fprintf(stderr, "ERROR: Error reading Input [%lu recovered / %lu retries]",
                                                readerrorsrecovered, readerrorstotal);
                if (fseeko(stream, readoffset, SEEK_SET) != 0) {
                    closestderr();
                    printseekerror(name, action);
                  return 1;
                }
                if (fread(ptr, size, nmemb, stream) == nmemb) {
                    // recovered error
                    readerrorsrecovered++;
                    resetstderr();
                    charsprinted = fprintf(stderr, "ERROR: Error reading Input [%lu recovered / %lu retries]",
                                                    readerrorsrecovered, readerrorstotal);
                  return 0;
                }
            }
            // unrecovered error
            closestderr();
            color(red);
            printf("ERROR: Unrecoverable read error while reading %s, %s failed!%s",
                    name, action, newline);
            color(normal);
          return 1;
        }
    }
}

int checkwriteandprinterrors(const void *ptr, size_t size, size_t nmemb, FILE *stream, unsigned long loop, unsigned long long startoffset,
                             char *name, char *action) {
    if (fwrite(ptr, size, nmemb, stream) == nmemb) return 0;
    else {
        if (writeerrorstotal == 0) {
            printstderr = true;
            color(yellow);
        }
        if (feof(stream)) {  // EOF
            closestderr();
            color(red);
            printf("ERROR: End of File reached while writing %s, %s failed!%s", name, action, newline);
            color(normal);
          return 1;
        }
        unsigned long long writeoffset = (unsigned long long) loop * nmemb + startoffset;
        int i;
        for (i=0;i<readretries;i++) {
            writeerrorstotal++;
            resetstderr();
            charsprinted = fprintf(stderr, "ERROR: Error writing %s [%lu recovered / %lu retries]", name, writeerrorsrecovered, writeerrorstotal);
            if (fseeko(stream, writeoffset, SEEK_SET) != 0) {
                closestderr();
                printseekerror(name, action);
              return 1;
            }
            if (fwrite(ptr, size, nmemb, stream) == nmemb) {  // recovered error
                writeerrorsrecovered++;
                resetstderr();
                charsprinted = fprintf(stderr, "ERROR: Error writing %s [%lu recovered / %lu retries]",
                                                name, writeerrorsrecovered, writeerrorstotal);
              return 0;
            }
        }
        // unrecovered error
        closestderr();
        color(red);
        printf("ERROR: Unrecoverable write error while writing %s (%s), %s failed!%s", name, strerror(errno), action, newline);
        color(normal);
      return 1;
    }
}

int padzeros(FILE *stream, char *filename, long long startoffset, long long endoffset) {
    unsigned long m;
    if (startoffset == -1) return 1;  // seek error
    if (!writefile) {
        color(yellow);
        printf("Aborting Padding because writing is disabled%s", newline);
        color(normal);
      return 1;
    }
    if (endoffset - startoffset < BIGBUF_SIZE) {
        memset(bigbuffer, 0, endoffset - startoffset);
        if (fseeko(stream, startoffset, SEEK_SET) != 0) {
            printseekerror(filename, "Padding");
          return 1;
        }
        initcheckwrite();
        if (checkwriteandprinterrors(bigbuffer, 1, endoffset - startoffset, stream, 0, startoffset, filename, "padding") != 0) return 1;
        donecheckwrite(filename);
      return 0;
    }
    else {
        memset(bigbuffer, 0, BIGBUF_SIZE);
        if (fseeko(stream, startoffset, SEEK_SET) != 0) {
            printseekerror(filename, "Padding");
          return 1;
        }
        if (debug) {
            printf("startoffset = %"LL"d, endoffset = %"LL"d\n", startoffset, endoffset);
            printf("(endoffset - startoffset) / BIGBUF_SIZE = %"LL"d\n", (endoffset - startoffset) / BIGBUF_SIZE);
            printf("BIGBUF_SIZE = %d\n", BIGBUF_SIZE);                
        }
        initcheckwrite();
        for (m=0; m < (unsigned long) ((endoffset - startoffset) / BIGBUF_SIZE); m++) {
            if (checkwriteandprinterrors(bigbuffer, 1, BIGBUF_SIZE, stream, m, startoffset, filename, "padding") != 0) return 1;
        }
        if ((endoffset - startoffset) % BIGBUF_SIZE != 0) {
            if (checkwriteandprinterrors(bigbuffer, 1, (endoffset - startoffset) % BIGBUF_SIZE, stream, 0, ftello(stream), filename, "padding") != 0) return 1;
        }
        donecheckwrite(filename);
      return 0;
    }
}

void printseekerror(char *filename, char *action) {
    color(red);
    printf("ERROR: Failed to seek to new file position in %s%s%s! (%s) %s failed!%s",
            quotation, filename, quotation, strerror(errno), action, newline);
    color(normal);
  return;
}

int rebuildiso(char *filename) {
    int i;
    unsigned long m;
    if (debug) printf("rebuildiso - filename: %s%s", filename, newline);
    if (isotoosmall) {
        fprintf(stderr, "\n");
        color(red);
        printf("ERROR: ISO filesize is too small, Rebuilding was aborted!%s", newline);
        color(normal);
      return 1;
    }
    if (BIGBUF_SIZE < 32768) {
        fprintf(stderr, "\n");
        color(red);
        printf("ERROR: BIGBUF_SIZE was less than 32 KB... oops! Rebuilding was aborted!%s", newline);
        color(normal);
        if (debug) printf("BIGBUF_SIZE = %u%s", BIGBUF_SIZE, newline);
      return 1;
    }
    if (!writefile) {
        color(yellow);
        printf("Rebuilding was aborted because writing is disabled%s", newline);
        color(normal);
      return 1;
    }
    #ifdef WIN32
        unsigned long startmsecs = 0;
        unsigned long endmsecs;
    #endif
    long long freespace, freespacerequired;
    if (rebuildlowspace) {  // minimal disk space usage method
        long long startingfilesize = getfilesize(fp);
        if (startingfilesize == -1) return 1;  // seek error
        freespacerequired = 7572881408LL - startingfilesize;
        if (debug) printf("freespacerequired: %"LL"d Bytes, startingfilesize = %"LL"d Bytes%s", freespacerequired, startingfilesize, newline);
        if (freespacerequired > 0) {
            // check that we have enough space
            freespace = freediskspace(filename);
            if (debug) printf("freespace: %"LL"d Bytes%s", freespace, newline);
            if (freespace < freespacerequired) {
                color(red);
                printf("ERROR: Not enough free disk space! You need to free at least %"LL"d MB "
                       "on the partition your ISO is located. Rebuilding was aborted!%s",
                        (freespacerequired - freespace) / 1048576, newline);
                color(normal);
              return 1;
            }
        }
        // extend or truncate filesize to 7572881408 bytes
        if (dotruncate(filename, startingfilesize, 7572881408LL, true) != 0) return 1;
        // start shifting all of the data down 265879552 bytes to make room for the video partition
        #ifdef WIN32
            if (debug || testing) {
                startmsecs = GetTickCount();
            }
        #endif
        initcheckread();
        initcheckwrite();
        memset(bigbuffer, 0, BIGBUF_SIZE);
        long long blockreadoffset = 7307001856LL;
        // 7307001856 / 265879552 = 27.482
        for (i=0;i<27;i++) {
            blockreadoffset -= 265879552;
            // 265879552 / 32768 = 8114
            for (m=0;m<8114;m++) {
                if (fseeko(fp, blockreadoffset + (m * 32768), SEEK_SET) != 0) {
                    printf("%s", newline);
                    printseekerror(filename, "Rebuilding");
                  return 1;
                }
                if (checkreadandprinterrors(bigbuffer, 1, 32768, fp, 0, blockreadoffset + (m * 32768), filename, "rebuilding") != 0) return 1;
                // 265879552 - 32768 = 265846784
                if (fseeko(fp, 265846784, SEEK_CUR) != 0) {
                    printf("%s", newline);
                    printseekerror(filename, "Rebuilding");
                  return 1;
                }
                if (checkwriteandprinterrors(bigbuffer, 1, 32768, fp, 0, blockreadoffset + (m * 32768) + 265846784, filename, "rebuilding") != 0) return 1;
                // 7307001856 / 32768 = 222992
                if (((i * 8114) + m) % 2230 == 0) {
                    resetstderr();
                    charsprinted = fprintf(stderr, "%lu%% done", ((i * 8114) + m) * 100 / 222992);
                }
            }
        }
        // 7307001856 - (27 * 265879552) = 128253952
        // 128253952 / 32768 = 3914
        for (m=0;m<3914;m++) {
            if (fseeko(fp, m * 32768, SEEK_SET) != 0) {
                printf("%s", newline);
                printseekerror(filename, "Rebuilding");
              return 1;
            }
            if (checkreadandprinterrors(bigbuffer, 1, 32768, fp, 0, m * 32768, filename, "rebuilding") != 0) return 1;
            if (fseeko(fp, 265846784, SEEK_CUR) != 0) {
                printf("%s", newline);
                printseekerror(filename, "Rebuilding");
              return 1;
            }
            if (checkwriteandprinterrors(bigbuffer, 1, 32768, fp, 0, m * 32768 + 265846784, filename, "rebuilding") != 0) return 1;
            if (((27 * 8114) + m) % 2230 == 0) {
                resetstderr();
                charsprinted = fprintf(stderr, "%lu%% done", ((27 * 8114) + m) * 100 / 222992);
            }
        }
        clearstderr();
        charsprinted = fprintf(stderr, "Done\n");
        donecheckread(filename);
        donecheckwrite(filename);
        video = 0xFD90000LL;
        #ifdef WIN32
            if (debug || testing) {
                endmsecs = GetTickCount();
                printf("%lu seconds elapsed%s", (endmsecs - startmsecs) / 1000, newline);
            }
        #endif
        fprintf(stderr, "Padding Video... ");
        // erase everything except the game data
        if (padzeros(fp, filename, 0, 0xFD90000LL) != 0) return 1;
        fprintf(stderr, "Done\n");
      return 0;
    }
    else {  // default method
        // check that we have at least 7572881408 bytes of free space
        freespace = freediskspace(filename);
        if (freespace < 7572881408LL) {
            color(red);
            printf("ERROR: Not enough free disk space! You need to free at least %"LL"d MB "
                   "on the partition your ISO is located. Rebuilding was aborted!%s",
                    (7572881408LL - freespace) / 1048576, newline);
            color(normal);
          return 1;
        }
        FILE *rebuiltisofile;
        char randomext[16];
        // generate a filename for the rebuilt iso file that can't be opened for reading
        // (hopefully that means it doesn't exist but we should probably check errno to make sure)
        char rebuiltisofilename[strlen(filename) + 16];
        memset(rebuiltisofilename, 0, strlen(filename) + 16);
        strcpy(rebuiltisofilename, filename);
        // check for *.xxx
        if (memcmp(rebuiltisofilename+(strlen(filename) - 4), ".", 1) == 0) {
            // case insensitive check for *.iso
            if (strncasecmp(rebuiltisofilename+(strlen(filename) - 4), ".iso", 4) == 0) {
                // original iso named *.iso, rebuilt iso named *.[smallest integer possible].iso until a unique filename is found
                i = 1;
                while (i < 4001) {
                    memset(randomext, 0, 16);
                    sprintf(randomext, ".%d.iso", i);
                    memcpy(rebuiltisofilename+(strlen(filename) - 4), randomext, strlen(randomext));
                    rebuiltisofile = fopen(rebuiltisofilename, "rb");
                    if (rebuiltisofile == NULL) break;
                    i++;
                }
                if (i == 4001) {
                    color(red);
                    printf("ERROR: Failed to find a unique filename for the rebuilt ISO! (4,000 filenames tried) Perhaps you should try using a different Rebuilding Method...%s", newline);
                    color(normal);
                  return 1;
                }
            }
            else {
                // original iso named *.???, rebuilt iso named *.iso (preferred) or *.[smallest integer possible].iso until a unique filename is found
                memcpy(rebuiltisofilename+(strlen(filename) - 4), ".iso", 4);
                rebuiltisofile = fopen(rebuiltisofilename, "rb");
                i = 1;
                while (rebuiltisofile != NULL && i < 4001) {
                    memset(randomext, 0, 16);
                    sprintf(randomext, ".%d.iso", i);
                    memcpy(rebuiltisofilename+(strlen(filename) - 4), randomext, strlen(randomext));
                    rebuiltisofile = fopen(rebuiltisofilename, "rb");
                    if (rebuiltisofile == NULL) break;
                    i++;
                }
                if (i == 4001) {
                    color(red);
                    printf("ERROR: Failed to find a unique filename for the rebuilt ISO! (4,000 filenames tried) Perhaps you should try using a different Rebuilding Method...%s", newline);
                    color(normal);
                  return 1;
                }
            }
        }
        else {
            // original iso named *, rebuilt iso named *.iso (preferred) or *.[smallest integer possible].iso until a unique filename is found
            strcat(rebuiltisofilename, ".iso");
            rebuiltisofile = fopen(rebuiltisofilename, "rb");
            i = 1;
            while (rebuiltisofile != NULL && i < 4001) {
                memset(randomext, 0, 16);
                sprintf(randomext, ".%d.iso", i);
                memcpy(rebuiltisofilename+(strlen(filename)), randomext, strlen(randomext));
                rebuiltisofile = fopen(rebuiltisofilename, "rb");
                if (rebuiltisofile == NULL) break;
                i++;
            }
            if (i == 4001) {
                color(red);
                printf("ERROR: Failed to find a unique filename for the rebuilt ISO! (4,000 filenames tried) Perhaps you should try using a different Rebuilding Method...%s", newline);
                color(normal);
              return 1;
            }
        }
        
        rebuiltisofile = fopen(rebuiltisofilename, "wb+");
        if (rebuiltisofile == NULL) {
            color(red);
            printf("ERROR: Failed to create a new file for the rebuilt ISO! (%s)%s", strerror(errno), newline);
            color(normal);
          return 1;
        }
        if (debug) printf("rebuiltisofilename: %s%s", rebuiltisofilename, newline);
        #ifdef WIN32
            if (debug || testing) {
                startmsecs = GetTickCount();
            }
        #endif
        memset(bigbuffer, 0, BIGBUF_SIZE);
        
        // extend rebuilt iso filesize to 265879553 bytes
        if (dotruncate(rebuiltisofilename, 0, 265879553LL, true) != 0) return 1;
        
        // seek to 265879552 (0xFD90000) in rebuiltisofile
        if (fseeko(rebuiltisofile, 265879552LL, SEEK_SET) != 0) {
            printf("%s", newline);
            printseekerror(rebuiltisofilename, "Rebuilding");
          return 1;
        }
        
        // seek to 0 in input file
        if (fseeko(fp, 0, SEEK_SET) != 0) {
            printf("%s", newline);
            printseekerror(filename, "Rebuilding");
          return 1;
        }
        
        // copy first 7307001856 bytes from input file to rebuiltisofile (make sure we checked to see if input file is too small)
        // 7307001856 / 32768 = 222992
        initcheckread(); initcheckwrite();
        for (m=0;m<222992;m++) {
            if (checkreadandprinterrors(bigbuffer, 1, 32768, fp, m, 0, filename, "rebuilding") != 0) return 1;
            if (checkwriteandprinterrors(bigbuffer, 1, 32768, rebuiltisofile, m, 0xFD90000LL, rebuiltisofilename, "rebuilding") != 0) return 1;
            if (m % 2230 == 0) {
                resetstderr();
                charsprinted = fprintf(stderr, "%lu%% done", (m) * 100 / 222992);
            }
        }
        clearstderr();
        charsprinted = fprintf(stderr, "Done\n");
        donecheckread(filename); donecheckwrite(filename);
        video = 0xFD90000LL;
        #ifdef WIN32
            if (debug || testing) {
                endmsecs = GetTickCount();
                printf("%lu seconds elapsed%s", (endmsecs - startmsecs) / 1000, newline);
            }
        #endif
        fprintf(stderr, "Padding Video... ");
        // erase everything except the game data
        if (padzeros(rebuiltisofile, rebuiltisofilename, 0, 0xFD90000LL) != 0) return 1;
        fprintf(stderr, "Done\n");
        
        void *unusedptr;
        if (!keeporiginaliso) {
            // close original iso file and delete it
            if (fp != NULL) fclose(fp);
            if (remove(filename)) {
                color(yellow);
                printf("ERROR: Removing %s%s%s failed! (%s) However, your ISO has been successfully rebuilt to %s%s%s%s",
                       quotation, filename, quotation, strerror(errno), quotation, rebuiltisofilename, quotation, newline);
                color(normal);
                unusedptr = realloc(isofilename, (strlen(rebuiltisofilename) + 1) * sizeof(char));
                if (isofilename == NULL) {
                    color(red); printf("ERROR: Failed to reallocate memory for isofilename! Game over man... Game over!%s", newline); color(normal);
                  exit(1);
                }
                strcpy(isofilename, rebuiltisofilename);
                if (rebuiltisofile != NULL) fclose(rebuiltisofile);
                fp = fopen(rebuiltisofilename, "rb+");
                if (fp == NULL) {
                    color(red);
                    printf("ERROR: Failed to reopen %s%s%s for writing! (%s) Unable to patch stealth files!%s",
                           quotation, rebuiltisofilename, quotation, strerror(errno), newline);
                    color(normal);
                    fp = fopen(rebuiltisofilename, "rb");
                    if (fp == NULL) {
                        color(red);
                        printf("ERROR: Failed to reopen %s%s%s for reading! (%s) Game over man... Game over!%s", 
                               quotation, rebuiltisofilename, quotation, strerror(errno), newline);
                        color(normal);
                      exit(1);
                    }
                  return 1;
                }
              return 0;
            }
            // close rebuilt iso file
            if (rebuiltisofile != NULL) fclose(rebuiltisofile);
            // rename it to original iso filename
            if (rename(rebuiltisofilename, filename)) {
                color(yellow);
                printf("ERROR: Renaming %s%s%s to %s%s%s failed! (%s) However, your ISO has been successfully rebuilt to %s%s%s%s",
                       quotation, rebuiltisofilename, quotation, quotation, filename, quotation, strerror(errno), quotation, rebuiltisofilename, quotation, newline);
                color(normal);
                unusedptr = realloc(isofilename, (strlen(rebuiltisofilename) + 1) * sizeof(char));
                if (isofilename == NULL) {
                    color(red);
                    printf("ERROR: Failed to reallocate memory for isofilename! Game over man... Game over!%s", newline);
                    color(normal);
                  exit(1);
                }
                strcpy(isofilename, rebuiltisofilename);
                fp = fopen(rebuiltisofilename, "rb+");
                if (fp == NULL) {
                    color(red);
                    printf("ERROR: Failed to reopen %s%s%s for writing! (%s) Unable to patch stealth files!%s",
                           quotation, rebuiltisofilename, quotation, strerror(errno), newline);
                    color(normal);
                    fp = fopen(rebuiltisofilename, "rb");
                    if (fp == NULL) {
                        color(red);
                        printf("ERROR: Failed to reopen %s%s%s for reading! (%s) Game over man... Game over!%s", 
                               quotation, rebuiltisofilename, quotation, strerror(errno), newline);
                        color(normal);
                      exit(1);
                    }
                  return 1;
                }
              return 0;
            }
            // reopen it as fp
            fp = fopen(filename, "rb+");
            if (fp == NULL) {
                color(red);
                printf("ERROR: Failed to reopen %s%s%s for writing! (%s) Unable to patch stealth files!%s",
                       quotation, filename, quotation, strerror(errno), newline);
                color(normal);
                fp = fopen(filename, "rb");
                if (fp == NULL) {
                    color(red);
                    printf("ERROR: Failed to reopen %s%s%s for reading! (%s) Game over man... Game over!%s", 
                           quotation, filename, quotation, strerror(errno), newline);
                    color(normal);
                  exit(1);
                }
              return 1;
            }
            color(green);
            printf("%s%s%s was successfully rebuilt!%s", quotation, filename, quotation, newline);
            color(normal);
        }
        else {
            // keep original iso, just change isofilename and fp to the rebuilt iso
            if (fp != NULL) fclose(fp);
            if (rebuiltisofile != NULL) fclose(rebuiltisofile);
            unusedptr = realloc(isofilename, (strlen(rebuiltisofilename) + 1) * sizeof(char));
            if (isofilename == NULL) {
                color(red);
                printf("ERROR: Failed to reallocate memory for isofilename! Game over man... Game over!%s", newline);
                color(normal);
              exit(1);
            }
            strcpy(isofilename, rebuiltisofilename);
            fp = fopen(rebuiltisofilename, "rb+");
            if (fp == NULL) {
                color(red);
                printf("ERROR: Failed to reopen %s%s%s for writing! (%s) Unable to patch stealth files!%s",
                       quotation, rebuiltisofilename, quotation, strerror(errno), newline);
                color(normal);
                fp = fopen(rebuiltisofilename, "rb");
                if (fp == NULL) {
                    color(red);
                    printf("ERROR: Failed to reopen %s%s%s for reading! (%s) Game over man... Game over!%s", 
                           quotation, rebuiltisofilename, quotation, strerror(errno), newline);
                    color(normal);
                  exit(1);
                }
              return 1;
            }
            color(green);
            printf("Your ISO was successfully rebuilt to %s (the original still exists)%s",
                   rebuiltisofilename, newline);
            color(normal);
        }
        if (debug) printf("rebuildiso r0 - isofilename: %s, strlen(isofilename) = %u, filename: %s, strlen(filename) = %u%s", isofilename, (unsigned int) strlen(isofilename), filename, (unsigned int) strlen(filename), newline);
      return 0;
    }
}

int doautoupload(char *argv[]) {
    int i, a;
    printf("%sStarting AutoUpload%s", newline, newline);
    if (xex_crc32 == 0 || ss_crc32 == 0 || ss_rawcrc32 == 0 || pfi_crc32 == 0 || dmi_crc32 == 0 || video_crc32 == 0 || videoL0_crc32 == 0 || videoL1_crc32 == 0) {
        color(yellow);
        printf("One or more CRC requirements for AutoUpload were not met, cannot continue%s", newline);
        color(normal);
      return 1;
    }
    if (!xex_foundmediaid || !foundregioncode) {
        color(yellow);
        printf("Xex media id and/or region code were not found, AutoUpload cannot continue%s", newline);
        color(normal);
      return 1;
    }
    
    if (game_crc32 == 0 && !checkgamecrcnever) {
        if (docheckgamecrc() == 0) {
            if (corruptionoffsetcount == 0) {
                color(green);
                if (verbose) printf("%s", sp5);
                printf("AnyDVD style corruption was not detected%s", newline);
                color(normal);
            }
            if (verbose) printf("%sGame CRC = %08lX%s%s", sp5, game_crc32, newline, newline);
        }
        else {
            checkgamecrcnever = true;
            gamecrcfailed = true;
        }
    }
    if (game_crc32 == 0) {
        printf("Game partition CRC is required for AutoUpload, cannot continue%s", newline);
      return 1;
    }

    // check for unverified inis with matching xex_crc32, ss_crc32, ss_rawcrc32, pfi_crc32, dmi_crc32, video_crc32, videoL0_crc32, videoL1_crc32, game_crc32
    // if found, there is no point in bothering the user to upload a duplicate ini that will be rejected
    printf("%sChecking to see if this upload is a duplicate and would be rejected%s", sp5, newline);
    if (unverifiediniexists()) {
        printf("An exact match already exists and is waiting to be verified, upload was aborted%s", newline);
      return 1;
    }
    
    #ifndef WIN32
        close_keyboard();
    #endif
    
    // get these values from the user
    char ini_discsource[14] = {0}, ini_gamename[151] = {0}, ini_gamertag[151] = {0}, ini_drivename[9] = {0}, ini_drivefw[30] = {0}, ini_notes[151] = {0}, ini_temp[4];
    
    printstderr = true;
    color(white);
    fprintf(stderr, "\nPlease enter values for the ini file. Press enter for the [default value].\n");
    color(normal);
    printstderr = false;
    
    fprintf(stderr, "Enter Nickname [%s]: ", argv[autouploaduserarg]);
    readstdin(ini_gamertag, 151);
    if (!strlen(ini_gamertag)) strncpy(ini_gamertag, argv[autouploaduserarg], 150);

    if (foundgamename) {
        fprintf(stderr, "Enter Game Name for %s [%s]: ", isofilename, gamename);
        readstdin(ini_gamename, 151);
        if (!strlen(ini_gamename)) strcpy(ini_gamename, gamename);
    }
    else while (!strlen(ini_gamename)) {
        fprintf(stderr, "Enter Game Name for %s: ", isofilename);
        readstdin(ini_gamename, 151);
    }
    
    i = 0;
    memset(ini_temp, 0, 4);
    while (i != 1 && i != 2 && i != 3) {
        fprintf(stderr, "Enter Disc Source (1=Scene Release, 2=Other Release, 3=Retail Disc): ");
        readstdin(ini_temp, 4);
        i = (int) strtol(ini_temp, NULL, 10);
    }

    if (i == 1) strcpy(ini_discsource, "Scene Release");
    else if (i == 2) strcpy(ini_discsource, "Other Release");
    else if (i == 3) strcpy(ini_discsource, "Retail Disc");
    
    if (i == 1 || i == 2) {
        strcpy(ini_drivename, "Unknown");
        strcpy(ini_drivefw, "Unknown");
    }
    else {
        i = 0;
        a = 0;
        memset(ini_temp, 0, 4);
        fprintf(stderr, "Enter Ripping Drive (1=Unknown, 2=SH-D162C, 3=SH-D162D, 4=SH-D163A, 5=SH-D163B,\n"
                        "6=TS-H943A, 7=GDR3120L, 8=VAD6038, 9=DG-16D2S, 0=Other) [1]: ");
        readstdin(ini_temp, 4);
        i = (int) strtol(ini_temp, NULL, 10);
        memset(ini_temp, 0, 4);
        if (i == 2) {
            strcpy(ini_drivename, "SH-D162C");
            fprintf(stderr, "Enter Firmware Version (1=Unknown, 2=0.5, 3=0.60, 4=0.80, 5=0.81, 6=1.00,\n"
                            "7=Other) [1]: ");
            readstdin(ini_temp, 4);
            a = (int) strtol(ini_temp, NULL, 10);
            if      (a == 2) strcpy(ini_drivefw, "0.5");
            else if (a == 3) strcpy(ini_drivefw, "0.60");
            else if (a == 4) strcpy(ini_drivefw, "0.80");
            else if (a == 5) strcpy(ini_drivefw, "0.81");
            else if (a == 6) strcpy(ini_drivefw, "1.00");
            else if (a == 7) strcpy(ini_drivefw, "Other");
            else strcpy(ini_drivefw, "Unknown");
        }
        else if (i == 3) {
            strcpy(ini_drivename, "SH-D162D");
            fprintf(stderr, "Enter Firmware Version (1=Unknown, 2=1.00, 3=Other) [1]: ");
            readstdin(ini_temp, 4);
            a = (int) strtol(ini_temp, NULL, 10);
            if (a == 2) strcpy(ini_drivefw, "1.00");
            else if (a == 3) strcpy(ini_drivefw, "Other");
            else strcpy(ini_drivefw, "Unknown");
        }
        else if (i == 4) {
            strcpy(ini_drivename, "SH-D163A");
            fprintf(stderr, "Enter Firmware Version (1=Unknown, 2=0.80, 3=1.00, 4=Other) [1]: ");
            readstdin(ini_temp, 4);
            a = (int) strtol(ini_temp, NULL, 10);
            if (a == 2) strcpy(ini_drivefw, "0.80");
            else if (a == 3) strcpy(ini_drivefw, "1.00");
            else if (a == 4) strcpy(ini_drivefw, "Other");
            else strcpy(ini_drivefw, "Unknown");
        }
        else if (i == 5) {
            strcpy(ini_drivename, "SH-D163B");
            fprintf(stderr, "Enter Firmware Version (1=Unknown, 2=1.00, 3=Other) [1]: ");
            readstdin(ini_temp, 4);
            a = (int) strtol(ini_temp, NULL, 10);
            if (a == 2) strcpy(ini_drivefw, "1.00");
            else if (a == 3) strcpy(ini_drivefw, "Other");
            else strcpy(ini_drivefw, "Unknown");
        }
        else if (i == 6) {
            strcpy(ini_drivename, "TS-H943A");
            fprintf(stderr, "Enter Firmware Version (1=Unknown, 2=Xtreme 4.x, 3=Xtreme 5.x, 4=iXtreme v1.6,\n"
                            "5=iXtreme v1.6 0800 standalone, 6=iXtreme v1.61, 7=Other) [1]: ");
            readstdin(ini_temp, 4);
            a = (int) strtol(ini_temp, NULL, 10);
            if (a == 2) strcpy(ini_drivefw, "4.x");
            else if (a == 3) strcpy(ini_drivefw, "5.x");
            else if (a == 4) strcpy(ini_drivefw, "iXtreme v1.6");
            else if (a == 5) strcpy(ini_drivefw, "iXtreme v1.6 0800 standalone");
            else if (a == 6) strcpy(ini_drivefw, "iXtreme v1.61");
            else if (a == 7) strcpy(ini_drivefw, "Other");
            else strcpy(ini_drivefw, "Unknown");
        }
        else if (i == 7) {
            strcpy(ini_drivename, "GDR3120L");
            strcpy(ini_drivefw, "Unknown");
        }
        else if (i == 8) {
            strcpy(ini_drivename, "VAD6038");
            fprintf(stderr, "Enter Firmware Version (1=Unknown, 2=iXtreme v1.41 0800 (Benq0800.bin),\n"
                            "3=iXtreme v1.6, 4=iXtreme v1.6 0800 standalone, 5=iXtreme v1.61,\n"
                            "6=Other) [1]: ");
            readstdin(ini_temp, 4);
            a = (int) strtol(ini_temp, NULL, 10);
            if (a == 2) strcpy(ini_drivefw, "iXtreme v1.41 0800");
            else if (a == 3) strcpy(ini_drivefw, "iXtreme v1.6");
            else if (a == 4) strcpy(ini_drivefw, "iXtreme v1.6 0800 standalone");
            else if (a == 5) strcpy(ini_drivefw, "iXtreme v1.61");
            else if (a == 6) strcpy(ini_drivefw, "Other");
            else strcpy(ini_drivefw, "Unknown");
        }
        else if (i == 9) {
            strcpy(ini_drivename, "DG-16D2S");
            fprintf(stderr, "Enter Firmware Version (1=Unknown, 2=iXtreme v1.6 (74850C),\n"
                            "3=iXtreme v1.6 (83850C), 4=iXtreme v1.6 0800 standalone, 5=Other) [1]: ");
            readstdin(ini_temp, 4);
            a = (int) strtol(ini_temp, NULL, 10);
            if (a == 2) strcpy(ini_drivefw, "iXtreme v1.6 (74850C)");
            else if (a == 3) strcpy(ini_drivefw, "iXtreme v1.6 (83850C)");
            else if (a == 4) strcpy(ini_drivefw, "iXtreme v1.6 0800 standalone");
            else if (a == 5) strcpy(ini_drivefw, "Other");
            else strcpy(ini_drivefw, "Unknown");
        }
        else if (i == 0) {
            strcpy(ini_drivename, "Other");
            strcpy(ini_drivefw, "Unknown");
        }
        else {
            strcpy(ini_drivename, "Unknown");
            strcpy(ini_drivefw, "Unknown");
        }
    }
    
    fprintf(stderr, "Enter additional Notes [abgx360 %s]: ", headerversion);
    readstdin(ini_notes, 151 - 11 - strlen(headerversion));  // leave enough room for abgx360 version stamp
    if (strlen(ini_notes)) strcat(ini_notes, " ");
    strcat(ini_notes, "[abgx360 ");
    strcat(ini_notes, headerversion);
    strcat(ini_notes, "]");
    
    // write ini file
    int userstealthpathlength = 0;
    if (!homeless) userstealthpathlength = strlen(homedir) + strlen(abgxdir) + strlen(userstealthdir);
    char autouploadinifilename[userstealthpathlength + 21];  // ini is 20 chars + 1 for null byte
    memset(autouploadinifilename, 0, userstealthpathlength + 21);
    if (!homeless) sprintf(autouploadinifilename, "%s%s%s%08lX%08lX.ini", homedir, abgxdir, userstealthdir, ss_crc32, xex_crc32);
    else sprintf(autouploadinifilename, "%08lX%08lX.ini", ss_crc32, xex_crc32);
    if (writeini(autouploadinifilename, ini_discsource, ini_gamename, ini_gamertag, ini_drivename, ini_drivefw, ini_notes) != 0) {
        color(yellow);
        printf("AutoUpload Aborted%s", newline);
        color(normal);
      return 1;
    }
    // extract pfi
    char autouploadpfifilename[userstealthpathlength + 17];  // 16 + 1
    memset(autouploadpfifilename, 0, userstealthpathlength + 17);
    if (!homeless) sprintf(autouploadpfifilename, "%s%s%sPFI_%08lX.bin", homedir, abgxdir, userstealthdir, pfi_crc32);
    else sprintf(autouploadpfifilename, "PFI_%08lX.bin", pfi_crc32);
    if (extractstealthfile(fp, isofilename, 0xFD8E800LL, "PFI", autouploadpfifilename) != 0) {
        color(yellow);
        printf("AutoUpload Aborted%s", newline);
        color(normal);
      return 1;
    }
    // extract dmi
    char autouploaddmifilename[userstealthpathlength + 17];  // 16 + 1
    memset(autouploaddmifilename, 0, userstealthpathlength + 17);
    if (!homeless) sprintf(autouploaddmifilename, "%s%s%sDMI_%08lX.bin", homedir, abgxdir, userstealthdir, dmi_crc32);
    else sprintf(autouploaddmifilename, "DMI_%08lX.bin", dmi_crc32);
    if (extractstealthfile(fp, isofilename, 0xFD8F000LL, "DMI", autouploaddmifilename) != 0) {
        color(yellow);
        printf("AutoUpload Aborted%s", newline);
        color(normal);
      return 1;
    }
    // extract ss
    char autouploadssfilename[userstealthpathlength + 16];  // 15 + 1
    memset(autouploadssfilename, 0, userstealthpathlength + 16);
    if (!homeless) sprintf(autouploadssfilename, "%s%s%sSS_%08lX.bin", homedir, abgxdir, userstealthdir, ss_crc32);
    else sprintf(autouploadssfilename, "SS_%08lX.bin", ss_crc32);
    if (extractstealthfile(fp, isofilename, 0xFD8F800LL, "SS", autouploadssfilename) != 0) {
        color(yellow);
        printf("AutoUpload Aborted%s", newline);
        color(normal);
      return 1;
    }
    // do autoupload
    struct curl_httppost *formpost = NULL;
    struct curl_httppost *lastptr = NULL;
    
    char curloutputfilename[strlen(homedir) + strlen(abgxdir) + 8 + 1];
    memset(curloutputfilename, 0, strlen(homedir) + strlen(abgxdir) + 8 + 1);
    if (!homeless) {
        strcat(curloutputfilename, homedir);
        strcat(curloutputfilename, abgxdir);
    }
    strcat(curloutputfilename, "curl.txt");
    if (debug) printf("curloutputfilename = %s%s", curloutputfilename, newline);
    
    fprintf(stderr, "Doing AutoUpload...\n");
    if (extraverbose) fprintf(stderr, "\n");
    printstderr = true;
    color(blue);
    memset(curlerrorbuffer, 0, CURL_ERROR_SIZE+1);
    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, curluseragent);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curlerrorbuffer);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(curl, CURLOPT_URL, autouploadwebaddress);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connectiontimeout);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, (curl_progress_callback) curlprogress);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, (char*) "Uploading stealth files");
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
    if (extraverbose) curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
    curl_easy_setopt(curl, CURLOPT_TIMECONDITION, 0);
    struct MyCurlFile curloutputfile = {curloutputfilename, NULL};
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *) &curloutputfile);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "username", CURLFORM_COPYCONTENTS, argv[autouploaduserarg], CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "password", CURLFORM_COPYCONTENTS, argv[autouploadpassarg], CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "uploadedfile", CURLFORM_FILE, autouploadinifilename, CURLFORM_CONTENTTYPE, "application/octet-stream", CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "uploadedss", CURLFORM_FILE, autouploadssfilename, CURLFORM_CONTENTTYPE, "application/octet-stream", CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "uploadedpfi", CURLFORM_FILE, autouploadpfifilename, CURLFORM_CONTENTTYPE, "application/octet-stream", CURLFORM_END);
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "uploadeddmi", CURLFORM_FILE, autouploaddmifilename, CURLFORM_CONTENTTYPE, "application/octet-stream", CURLFORM_END);
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
    curlheaderprinted = false;
    curlprogressstartmsecs = getmsecs();
    res = curl_easy_perform(curl);
    if (curlheaderprinted && !extraverbose) fprintf(stderr, "\n");
    if (res != CURLE_OK && !extraverbose) {  // error occurred and hasn't already been displayed
        color(yellow);
        fprintf(stderr, "ERROR: %s", curlerrorbuffer);
    }
    if (curloutputfile.stream != NULL) fclose(curloutputfile.stream);
    if (res != CURLE_OK && res != CURLE_HTTP_RETURNED_ERROR) {  // 404 is ok
        stayoffline = true;
    }
    if (extraverbose) {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);  // reset to avoid annoying "Closing Connection ..." atexit
    }
    curl_easy_setopt(curl, CURLOPT_HTTPGET, 1);  // reset in case any more files will be downloaded
    curl_formfree(formpost);  // free data copied by formadd
    fprintf(stderr, "\n");
    color(normal);
    printstderr = false;
    
    // parse output code
    FILE *curloutput = fopen(curloutputfilename, "rb");
    if (curloutput == NULL) {
        color(yellow);
        printf("ERROR: Failed to open cURL output file (%s), result of AutoUpload is unknown%s", strerror(errno), newline);
        color(normal);
      return 1;
    }
    else {
        memset(buffer, 0, 2048);
        dontcare = fread(buffer, 1, 2047, curloutput);
        fclose(curloutput);
        remove(curloutputfilename);
        if (debug) printf("curloutput: %s%s", buffer, newline);
        if (strlen(buffer) > 0) {
            if (memcmp(buffer, "ffffff", 6) == 0) {
                color(yellow);
                printf("Server replied: An unspecified error occurred!%s", newline);
                color(normal);
              return 1;
            }
            else {
                if (strlen(buffer) != 4) {
                    color(yellow);
                    printf("ERROR: Server reply is invalid, result of AutoUpload is unknown%s", newline);
                    color(normal);
                    if (extraverbose) printf("Server reply: %s%s", buffer, newline);
                  return 1;
                }
                if      (buffer[0] == '1') { color(green);  printf("Server replied: DMI uploaded perfectly%s", newline);                                         color(normal); }
                else if (buffer[0] == '2') { color(yellow); printf("Server replied: DMI file has a different checksum to that in the ini, rejected%s", newline); color(normal); }
                else if (buffer[0] == '3')                  printf("Server replied: DMI file already exists, ignored%s", newline);
                else if (buffer[0] == '4') { color(yellow); printf("Server replied: Unspecified error in DMI upload%s", newline);                                color(normal); }
                if      (buffer[1] == '1') { color(green);  printf("Server replied: PFI uploaded perfectly%s", newline);                                         color(normal); }
                else if (buffer[1] == '2') { color(yellow); printf("Server replied: PFI file has a different checksum to that in the ini, rejected%s", newline); color(normal); }
                else if (buffer[1] == '3')                  printf("Server replied: PFI file already exists, ignored%s", newline);
                else if (buffer[1] == '4') { color(yellow); printf("Server replied: Unspecified error in PFI upload%s", newline);                                color(normal); }
                if      (buffer[2] == '1') { color(green);  printf("Server replied: SS uploaded perfectly%s", newline);                                          color(normal); }
                else if (buffer[2] == '2') { color(yellow); printf("Server replied: SS file has a different checksum to that in the ini, rejected%s", newline);  color(normal); }
                else if (buffer[2] == '3')                  printf("Server replied: SS file already exists, ignored%s", newline);
                else if (buffer[2] == '4') { color(yellow); printf("Server replied: Unspecified error in SS upload%s", newline);                                 color(normal); }
                if      (buffer[3] == '0') { color(yellow); printf("Server replied: Ini file too large!%s", newline);                                            color(normal); }
                else if (buffer[3] == '1') { color(green);  printf("Server replied: New DMI found and added to database%s", newline);                            color(normal); }
                else if (buffer[3] == '2') { color(green);  printf("Server replied: Ini already exists and has been verified, nothing to do here%s", newline);   color(normal); }
                else if (buffer[3] == '3') { color(yellow); printf("Server replied: Files do not match! Verification unsuccessful%s", newline);                  color(normal); }
                else if (buffer[3] == '4') { color(green);  printf("Server replied: Files match! Verification completed successfully!%s", newline);              color(normal); }
                else if (buffer[3] == '5')                  printf("Server replied: Ini added to unverified folder, awaiting verification upload%s", newline);
                else if (buffer[3] == '6')                  printf("Server replied: Raw SS matches. File may be duplicate, rejected%s", newline);
                else if (buffer[3] == '7') { color(yellow); printf("Server replied: Game data checksum is invalid! Perhaps you forgot to check it%s", newline);  color(normal); }
                else if (buffer[3] == '8') { color(yellow); printf("Server replied: Ini file is invalid or could not be loaded%s", newline);                     color(normal); }
            }
        }
        else {
            color(yellow);
            printf("ERROR: cURL output file was empty, result of AutoUpload is unknown%s", newline);
            color(normal);
          return 1;
        }
    }
  return 0;
}

int writeini(char *inifilename, char *ini_discsource, char *ini_gamename, char *ini_gamertag, char *ini_drivename,
             char *ini_drivefw, char *ini_notes) {
    unsigned int u;
    printf("Writing ini to %s%s%s%s", quotation, inifilename, quotation, newline);
    FILE *inifile = fopen(inifilename, "wb");
    if (inifile == NULL) {
        color(red);
        printf("ERROR: Failed to open %s%s%s for writing! (%s)%s", quotation, inifilename, quotation, strerror(errno), newline);
        color(normal);
      return 1;
    }
    memset(buffer, 0, 2048);
    sprintf(buffer, "[%08lX%08lX]\r\nSS=%08lX\r\nRawSS=%08lX\r\nPFI=%08lX\r\nDMI=%08lX\r\nRegionFlags=%02X%02X%02X%02X\r\n"
                    "V0=%08lX\r\nV1=%08lX\r\nVideo=%08lX\r\nGame=%08lX\r\nXexHash=%08lX\r\n"
                    "MediaID=%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X-%02X%02X%02X%02X\r\n"
                    "DiscSource=%s\r\nGameName=%s\r\nGamerTag=%s\r\nDriveName=%s\r\nDriveFW=%s\r\nNotes=%s\r\n",
                    ss_crc32, xex_crc32, ss_crc32, ss_rawcrc32, pfi_crc32, dmi_crc32, regioncode[0], regioncode[1], regioncode[2], regioncode[3],
                    videoL0_crc32, videoL1_crc32, video_crc32, game_crc32, xex_crc32,
                    xex_mediaid[0], xex_mediaid[1], xex_mediaid[2], xex_mediaid[3], xex_mediaid[4], xex_mediaid[5], xex_mediaid[6], xex_mediaid[7],
                    xex_mediaid[8], xex_mediaid[9], xex_mediaid[10], xex_mediaid[11], xex_mediaid[12], xex_mediaid[13], xex_mediaid[14], xex_mediaid[15],
                    ini_discsource, ini_gamename, ini_gamertag, ini_drivename, ini_drivefw, ini_notes);
    printf("%s", newline);
    if (extraverbose) {
        // print ini
        color(blue);
        if (html) {
            for (u=0;u<strlen(buffer);u++) {
                if (buffer[u] == '\r') {
                    printf("%s", newline);
                    u++;
                }
                else printf("%c", buffer[u]);
            }
        }
        else printf("%s", buffer);
        color(normal);
        printf("%s", newline);
    }
    
    initcheckwrite();
    if (checkwriteandprinterrors(buffer, 1, strlen(buffer), inifile, 0, 0, inifilename, "writing ini") != 0) {
        fclose(inifile);
        remove(inifilename);
      return 1;
    }
    donecheckwrite(inifilename);
    fclose(inifile);
  return 0;
}

int extractstealthfile(FILE *isofile, char *isofilename, long long offset, char *name, char *stealthfilename) {
    printf("Extracting %s to %s%s%s%s", name, quotation, stealthfilename, quotation, newline);
    FILE *extractstealthfile = fopen(stealthfilename, "wb");
    if (extractstealthfile == NULL) {
        color(red);
        printf("ERROR: Failed to open %s%s%s for writing! (%s) Extraction was aborted!%s", quotation, stealthfilename, quotation, strerror(errno), newline);
        color(normal);
      return 1;
    }
    memset(buffer, 0, 2048);
    if (fseeko(isofile, offset, SEEK_SET) != 0) {
        printseekerror(isofilename, "Extracting stealth file");
      return 1;
    }
    initcheckread();
    if (checkreadandprinterrors(buffer, 1, 2048, isofile, 0, offset, isofilename, "Extracting stealth file") != 0) return 1;
    donecheckread(isofilename);
    if (trytowritestealthfile(buffer, 1, 2048, extractstealthfile, stealthfilename, 0) != 0) return 1;
    color(green);
    printf("Extraction was successful%s", newline);
    color(normal);
    fclose(extractstealthfile);
  return 0;
}

bool unverifiediniexists() {
    // make sure that all crcs have already been checked and are non-zero before this function is executed
    int i;
    bool dmi_matches;
    int loop;
    for (loop=1;loop<100;loop++) {
        dmi_matches = false;
        memset(inifilename, 0, 24);
        sprintf(inifilename, "%08lX%08lX_%d.ini", ss_crc32, xex_crc32, loop);
        if (debug) printf("unverifiediniexists - about to open inifilename: %s%s", inifilename, newline);
        inifile = openstealthfile(inifilename, stealthdir, webunverifiedinidir, UNVERIFIED_INI, "the online unverified dir");
        if (inifile == NULL) {
            if (debug) printf("unverifiediniexists - inifile was NULL, infilename: %s%s", inifilename, newline);
          return false;
        }
        if (debug) printf("unverifiediniexists - about to parse inifilename: %s%s", inifilename, newline);
        parseini();
        // delete it
        fclose(inifile);
        deletestealthfile(inifilename, stealthdir, false);
        if (ini_rawss == 0 || ini_dmi[0] == 0 || ini_pfi == 0 || ini_video == 0 || ini_v0 == 0 || ini_v1 == 0 || ini_game == 0) {
            if (debug) printf("unverifiediniexists - one or more ini values were zero: ini_rawss = %lu, ini_dmi[0] = %lu, ini_pfi = %lu, ini_video = %lu, ini_v0 = %lu, ini_v1 = %lu, ini_game = %lu%s",
                               ini_rawss, ini_dmi[0], ini_pfi, ini_video, ini_v0, ini_v1, ini_game, newline);
            continue;
        }
        for (i=0;i<ini_dmi_count;i++) {
            if (ini_dmi[i] == dmi_crc32) dmi_matches = true;
        }
        if (dmi_matches && ini_rawss == ss_rawcrc32 && ini_pfi == pfi_crc32 && ini_game == game_crc32 &&
            ini_video == video_crc32 && ini_v0 == videoL0_crc32 && ini_v1 == videoL1_crc32) {
          return true;
        }
    }
  return false;
}

int doverify() {
    verify_found_bad_pfi_or_video = false;
    int i;
    bool printextraline = true;
    bool verified_dmi = false;
    printf("%sStarting Verification%s", newline, newline);
    if ((stayoffline || localonly) && !offlinewarningprinted) {
        color(yellow);
        //if (verbose) printf("%s", newline);
        printf("You need to enable online functions (assuming your connection works and the db%s"
               "is up) if you want to check for the latest files in the online database.%s", newline, newline);
        color(normal);
        offlinewarningprinted = true;
    }
    if (xex_crc32 == 0 || ss_crc32 == 0 || ss_rawcrc32 == 0 || pfi_crc32 == 0 || dmi_crc32 == 0 || video_crc32 == 0 || videoL0_crc32 == 0 || videoL1_crc32 == 0) {
        color(yellow);
        printf("One or more CRC requirements for Verification were not met, cannot continue%s", newline);
        color(normal);
      return 1;
    }
    memset(inifilename, 0, 24);
    sprintf(inifilename, "%08lX%08lX.ini", ss_crc32, xex_crc32);
    inifile = openstealthfile(inifilename, stealthdir, webinidir, SSXEX_INI, "the online verified database");
    if (inifile == NULL) {
        printf("Failed to find a verified ini file for this Xex/SS combination%s", newline);
        if (dvdarg || !writefile || !autofixalways || !autofixuncertain) {
            // if we won't be autofixing we'll try to find any ini for the xex and verify pfi, video and game data
            printf("Attempting to at least verify the PFI, Video and game data%s", newline);
            memset(xexinifilename, 0, 17);
            sprintf(xexinifilename, "Xex_%08lX.ini", xex_crc32);
            xexinifile = openstealthfile(xexinifilename, stealthdir, webinidir, XEX_INI, "the online verified database");
            if (xexinifile == NULL) {
                printf("Failed to find a verified ini file for this Xex%s", newline);
                noxexiniavailable = true;
              return 1;
            }
            if (openinifromxexini() != 0) return 1;
            parseini();
            if (ini_pfi == 0) {
                color(red);
                printf("ERROR: Failed to find a PFI CRC in '%s', deleting it%s", inifilename, newline);
                color(normal);
                // delete it
                fclose(inifile);
                deletestealthfile(inifilename, stealthdir, false);
              return 1;
            }
            if (ini_video == 0) {
                color(red);
                printf("ERROR: Failed to find a Video CRC in '%s', deleting it%s", inifilename, newline);
                color(normal);
                // delete it
                fclose(inifile);
                deletestealthfile(inifilename, stealthdir, false);
              return 1;
            }
            if (ini_v0 == 0) {
                color(red);
                printf("ERROR: Failed to find a V0 CRC in '%s', deleting it%s", inifilename, newline);
                color(normal);
                // delete it
                fclose(inifile);
                deletestealthfile(inifilename, stealthdir, false);
              return 1;
            }
            if (ini_v1 == 0) {
                color(red);
                printf("ERROR: Failed to find a V1 CRC in '%s', deleting it%s", inifilename, newline);
                color(normal);
                // delete it
                fclose(inifile);
                deletestealthfile(inifilename, stealthdir, false);
              return 1;
            }
            if (ini_game == 0 && !checkgamecrcnever) {
                color(red);
                printf("ERROR: Failed to find a Game CRC in '%s', deleting it%s", inifilename, newline);
                color(normal);
                // delete it
                fclose(inifile);
                deletestealthfile(inifilename, stealthdir, false);
              return 1;
            }
            // check that video matches
            if (ini_video == video_crc32) {
                if (verbose) {
                    color(green);
                    printf("%sVideo CRC matches%s", sp5, newline);
                    color(normal);
                }
            }
            else {
                color(red);
                printf("%sVideo CRC does not match!%s", sp5, newline);
                color(normal);
                verify_found_bad_pfi_or_video = true;
                autoupload = false;
            }
            // check that v0 matches
            if (ini_v0 == videoL0_crc32) {
                if (verbose) {
                    color(green);
                    printf("%sV0 %s CRC matches%s", sp5, sp2, newline);
                    color(normal);
                }
            }
            else {
                color(red);
                printf("%sV0 %s CRC does not match!%s", sp5, sp2, newline);
                color(normal);
                verify_found_bad_pfi_or_video = true;
                autoupload = false;
            }
            // check that v1 matches
            if (ini_v1 == videoL1_crc32) {
                if (verbose) {
                    color(green);
                    printf("%sV1 %s CRC matches%s", sp5, sp2, newline);
                    color(normal);
                }
            }
            else {
                color(red);
                printf("%sV1 %s CRC does not match!%s", sp5, sp2, newline);
                color(normal);
                verify_found_bad_pfi_or_video = true;
                autoupload = false;
            }
            // check that pfi matches
            if (ini_pfi == pfi_crc32) {
                if (verbose) {
                    color(green);
                    printf("%sPFI %s CRC matches%s", sp5, sp1, newline);
                    color(normal);
                }
            }
            else {
                color(red);
                printf("%sPFI %s CRC does not match!%s", sp5, sp1, newline);
                color(normal);
                verify_found_bad_pfi_or_video = true;
                autoupload = false;
            }
            // make the splitvid check possible
            if (ini_video == video_crc32 && ini_v0 == videoL0_crc32 && ini_v1 == videoL1_crc32) {
                video_stealthuncertain = false;
            }
            if (ini_pfi == pfi_crc32) {
                pfi_stealthuncertain = false;
            }
            color(yellow);
            printf("%sDMI is unverified%s", sp5, newline);
            printf("%sSS %sis unverified%s", sp5, sp1, newline);
            color(normal);
            // of course xex matches because we're using <(a different)ss_crc32><xex_crc32>.ini
            if (verbose) {
                color(green);
                printf("%sXex %s CRC matches%s", sp5, sp1, newline);
                color(normal);
            }
            if (game_crc32 != 0) {
                if (ini_game != game_crc32) {
                    printbadgamecrcerror();
                    autoupload = false;
                  return 1;
                }
                if (verbose) {
                    color(green);
                    printf("%sGame%s CRC matches%s%s", sp5, sp1, newline, newline);
                    color(normal);
                }
              return 3;
            }
            else if (!checkgamecrcnever) {
                if (docheckgamecrc() == 0) {
                    if (corruptionoffsetcount == 0) {
                        color(green);
                        if (verbose) printf("%s", sp5);
                        printf("AnyDVD style corruption was not detected%s", newline);
                        color(normal);
                    }
                    if (verbose) {
                        printf("%sGame CRC = %08lX", sp5, game_crc32);
                        if (ini_game == game_crc32) {
                            color(green);
                            printf(" (matches)%s%s", newline, newline);
                            color(normal);
                        }
                        else {
                            printf("%s%s", newline, newline);
                        }
                    }
                    if (ini_game != game_crc32) {
                        printbadgamecrcerror();
                        autoupload = false;
                      return 1;
                    }
                  return 3;
                }
                else {
                    if (usercancelledgamecrc) {
                        if (verbose) printf("%s", newline);
                        color(yellow);
                        printf("Game CRC was skipped%s", newline);
                    }
                    else {
                        if (verbose) printf("%s", newline);
                        color(red);
                        printf("Error occurred while checking the Game CRC%s", newline);
                        gamecrcfailed = true;
                    }
                    color(normal);
                    checkgamecrcnever = true;
                  return 1;
                }
            }
            else {
                // game crc was never checked
                if (verbose) printf("%s", newline);
                color(yellow);
                printf("Game CRC was not checked%s", newline);
                color(normal);
              return 1;
            }
        }
      return 1;
    }
    // verify using the ini (xex and ss are already verified)
    parseini();
    if (ini_dmi[0] == 0) {
        color(yellow);
        printf("ERROR: Failed to find a DMI CRC in '%s'%s", inifilename, newline);
        color(normal);
        // delete it
        fclose(inifile);
        deletestealthfile(inifilename, stealthdir, false);
      return 1;
    }
    if (ini_pfi == 0) {
        color(yellow);
        printf("ERROR: Failed to find a PFI CRC in '%s'%s", inifilename, newline);
        color(normal);
        // delete it
        fclose(inifile);
        deletestealthfile(inifilename, stealthdir, false);
      return 1;
    }
    if (ini_video == 0) {
        color(yellow);
        printf("ERROR: Failed to find a Video CRC in '%s'%s", inifilename, newline);
        color(normal);
        // delete it
        fclose(inifile);
        deletestealthfile(inifilename, stealthdir, false);
      return 1;
    }
    if (ini_v0 == 0) {
        color(yellow);
        printf("ERROR: Failed to find a V0 CRC in '%s'%s", inifilename, newline);
        color(normal);
        // delete it
        fclose(inifile);
        deletestealthfile(inifilename, stealthdir, false);
      return 1;
    }
    if (ini_v1 == 0) {
        color(yellow);
        printf("ERROR: Failed to find a V1 CRC in '%s'%s", inifilename, newline);
        color(normal);
        // delete it
        fclose(inifile);
        deletestealthfile(inifilename, stealthdir, false);
      return 1;
    }
    if (ini_game == 0 && !checkgamecrcnever) {
        color(yellow);
        printf("ERROR: Failed to find a Game CRC in '%s'%s", inifilename, newline);
        color(normal);
        // delete it
        fclose(inifile);
        deletestealthfile(inifilename, stealthdir, false);
      return 1;
    }
    // check for any matching dmi
    for (i=0;i<ini_dmi_count;i++) {
        if (ini_dmi[i] == dmi_crc32) verified_dmi = true;
    }
    // check that video matches
    if (ini_video == video_crc32) {
        if (verbose) {
            color(green);
            printf("%sVideo CRC matches%s", sp5, newline);
            color(normal);
        }
    }
    else {
        color(red);
        printf("%sVideo CRC does not match!%s", sp5, newline);
        color(normal);
        verify_found_bad_pfi_or_video = true;
        autoupload = false;
    }
    // check that v0 matches
    if (ini_v0 == videoL0_crc32) {
        if (verbose) {
            color(green);
            printf("%sV0 %s CRC matches%s", sp5, sp2, newline);
            color(normal);
        }
    }
    else {
        color(red);
        printf("%sV0 %s CRC does not match!%s", sp5, sp2, newline);
        color(normal);
        verify_found_bad_pfi_or_video = true;
        autoupload = false;
    }
    // check that v1 matches
    if (ini_v1 == videoL1_crc32) {
        if (verbose) {
            color(green);
            printf("%sV1 %s CRC matches%s", sp5, sp2, newline);
            color(normal);
        }
    }
    else {
        color(red);
        printf("%sV1 %s CRC does not match!%s", sp5, sp2, newline);
        color(normal);
        verify_found_bad_pfi_or_video = true;
        autoupload = false;
    }
    // check that pfi matches
    if (ini_pfi == pfi_crc32) {
        if (verbose) {
            color(green);
            printf("%sPFI %s CRC matches%s", sp5, sp1, newline);
            color(normal);
        }
    }
    else {
        color(red);
        printf("%sPFI %s CRC does not match!%s", sp5, sp1, newline);
        color(normal);
        verify_found_bad_pfi_or_video = true;
        autoupload = false;
    }
    // display results of our check for any matching dmi
    if (verified_dmi) {
        if (verbose) {
            color(green);
            printf("%sDMI %s CRC matches%s", sp5, sp1, newline);
            color(normal);
        }
    }
    else {
        color(yellow);
        printf("%sDMI is unverified%s", sp5, newline);
        color(normal);
    }
    // of course ss and xex match because we're using <ss_crc32><xex_crc32>.ini
    if (verbose) {
        color(green);
        printf("%sSS %s CRC matches%s"
               "%sXex %s CRC matches%s", sp5, sp2, newline, sp5, sp1, newline);
        color(normal);
    }
    if (game_crc32 != 0) {
        if (ini_game != game_crc32) {
            printbadgamecrcerror();
            autoupload = false;
        }
        else if (verbose) {
            color(green);
            printf("%sGame%s CRC matches%s", sp5, sp1, newline);
            color(normal);
        }
    }
    else if (!checkgamecrcnever) {
        if (docheckgamecrc() == 0) {
            if (corruptionoffsetcount == 0) {
                color(green);
                if (verbose) printf("%s", sp5);
                printf("AnyDVD style corruption was not detected%s", newline);
                color(normal);
            }
            if (verbose) {
                printf("%sGame CRC = %08lX", sp5, game_crc32);
                if (ini_game == game_crc32) {
                    color(green);
                    printf(" (matches)%s%s", newline, newline);
                    color(normal);
                }
                else {
                    printf("%s%s", newline, newline);
                }
                printextraline = false;
            }
            if (ini_game != game_crc32) {
                printbadgamecrcerror();
                autoupload = false;
            }
        }
        else {
            checkgamecrcnever = true;
            gamecrcfailed = true;
        }
    }
    if (verbose && printextraline) printf("%s", newline);
    if (verified_dmi && ini_pfi == pfi_crc32 && ini_video == video_crc32 && ini_v0 == videoL0_crc32 && ini_v1 == videoL1_crc32) {
        if (game_crc32 == 0) {  // stealth crcs are good but never got the game crc for whatever reason
            color(green);
            printf("All Stealth CRCs match");
            if (usercancelledgamecrc) {
                // all good (skipped game crc)
                color(yellow);
                printf(" (Game CRC was skipped)%s", newline);
                color(normal);
              return 2;
            }
            else if (gamecrcfailed) {
                // there was an error checking the game crc
                color(red);
                printf(" (Error occurred while checking the Game CRC)%s", newline);
                color(normal);
              return 2;
            }
            else {
                // game crc was never checked
                color(yellow);
                printf(" (Game CRC was not checked)%s", newline);
                color(normal);
              return 2;
            }
        }
        else {  // successfully got the game crc
            if (ini_game == game_crc32) {
                // all good
                color(green);
                printf("All CRCs match%s", newline);
                color(normal);
              return 0;
            }
            else {
                // game crc is bad (and it's the only thing that's bad) so autofix can't do shit
                autofix = false;
                autofixalways = false;
              return 2;
            }
        }
    }
  return 1;
}

int doautofix() {
    int i;
    unsigned long m;
    long long videofilesize, stealthfilesize;
    unsigned char autofix_dmi[2048], autofix_pfi[2048];
    char pfifilename[17], dmifilename[17], ssfilename[16], videofilename[19];
    FILE *pfifile = NULL, *dmifile = NULL, *ssfile = NULL, *videofile = NULL;
    bool fixss = false, fixpfi = false, fixvideo = false, keepdmi = false;
    if (xex_crc32 == 0) {
        color(yellow);
        printf("%sMinimum requirement for AutoFix (Xex CRC) not met, cannot continue%s", newline, newline);
        color(normal);
      return 1;
    }
    if (noxexiniavailable) {
        color(yellow);
        printf("%sAborting AutoFix because there is no Xex ini available%s", newline, newline);
        color(normal);
      return 1;
    }
    if (!writefile) {
        color(yellow);
        printf("%sAborting AutoFix because writing is disabled%s", newline, newline);
        color(normal);
      return 1;
    }
    int randomdmicrc = 0;
    printf("%sStarting AutoFix%s", newline, newline);
    if ((stayoffline || localonly) && !offlinewarningprinted) {
        color(yellow);
        //if (verbose) printf("%s", newline);
        printf("You need to enable online functions (assuming your connection works and the db%s"
               "is up) if you want to check for the latest files in the online database.%s", newline, newline);
        color(normal);
        offlinewarningprinted = true;
    }
    // try to autofix using an ss crc out of the Xex_<xex_crc32>.ini
    memset(xexinifilename, 0, 17);
    sprintf(xexinifilename, "Xex_%08lX.ini", xex_crc32);
    xexinifile = openstealthfile(xexinifilename, stealthdir, webinidir, XEX_INI, "the online verified database");
    if (xexinifile == NULL) {
        color(yellow);
        printf("Failed to find a verified ini file for this Xex%s", newline);
        color(normal);
      return 1;
    }
    else {
        if (openinifromxexini() != 0) return 1;  // failed to find or open a (valid) verified ini
    }
    // autofix using the ini
    parseini();
    // get/open the required stealth files
    if (ini_ss != ss_crc32 || drtfucked) {
        fixss = true;
        if (ini_ss == 0) {
            printf("ERROR: Failed to find an SS CRC in '%s'%s", inifilename, newline);
          return 1;
        }
        memset(ssfilename, 0, 16);
        sprintf(ssfilename, "SS_%08lX.bin", ini_ss);
        ssfile = openstealthfile(ssfilename, stealthdir, webstealthdir, SS_FILE, "the online verified database");
        if (ssfile == NULL) {
            printf("ERROR: Failed to find or open '%s' (%s)%s", ssfilename, strerror(errno), newline);
          return 1;
        }
        memset(ss, 0, 2048);
        // make sure that ssfile is 2048 bytes before reading it
        stealthfilesize = getfilesize(ssfile);
        if (stealthfilesize == -1) {  // seek error
            fclose(ssfile);
          return 1;
        }
        if (stealthfilesize != 2048) {
            color(red);
            printf("ERROR: %s is %"LL"d bytes! (should have been exactly 2048) AutoFix was aborted!%s", ssfilename, stealthfilesize, newline);
            color(normal);
            fclose(ssfile);
            deletestealthfile(ssfilename, stealthdir, false);
            return 1;
        }
        if (trytoreadstealthfile(ss, 1, 2048, ssfile, ssfilename, 0) != 0) {
            fclose(ssfile);
            deletestealthfile(ssfilename, stealthdir, false);
          return 1;
        }
        fclose(ssfile);
        // check to see if autofix ss is valid for this game
        printf("%sVerifying %s is valid before using it for AutoFix%s", sp5, ssfilename, newline);
        if (getzeros(ss, 0, 2047) == 2048) {
            color(red);
            printf("ERROR: %s is blank! AutoFix was aborted!%s", ssfilename, newline);
            color(normal);
            deletestealthfile(ssfilename, stealthdir, false);
          return 1;
        }
        if (verbose) printf("%s", newline);
        checkss();
        if (verbose) printf("%s", newline);
        if (ss_stealthfailed || ss_stealthuncertain) {
            // ss failed stealth check against the xex
            color(red);
            printf("ERROR: %s appears to be invalid! AutoFix was aborted!%s", ssfilename, newline);
            color(normal);
            deletestealthfile(ssfilename, stealthdir, false);
          return 1;
        }
        // check to see if autofix ss matches ini crc
        if (ini_ss != ss_crc32) {
            color(red);
            printf("ERROR: %s has an incorrect CRC! (it was %08lX and should have been %08lX) AutoFix was aborted!%s", ssfilename, ss_crc32, ini_ss, newline);
            color(normal);
            deletestealthfile(ssfilename, stealthdir, false);
          return 1;
        }
    }
    for (i=0;i<ini_dmi_count;i++) {  // keep the current dmi if it matches any of the verified dmis for this xex/ss
        if (ini_dmi[i] == dmi_crc32) keepdmi = true;
    }
    if (!keepdmi) {
        if (ini_dmi_count == 0 || ini_dmi[0] == 0) {
            printf("ERROR: Failed to find a DMI CRC in '%s'%s", inifilename, newline);
          return 1;
        }
        else if (ini_dmi_count > 1) {
            randomdmicrc = randomnumber(0, ini_dmi_count - 1);
            if (ini_dmi[randomdmicrc] == 0) {
                for(i=0;i<ini_dmi_count;i++) {
                    if (ini_dmi[i] != 0) randomdmicrc = i;
                }
                if (ini_dmi[randomdmicrc] == 0) {
                    printf("ERROR: Failed to find a valid DMI CRC in '%s'%s", inifilename, newline);
                  return 1;
                }
            }
            if (verbose) printf("%s'%s' contains %d DMI crcs, randomly picked %s%d%s",
                                 sp5, inifilename, ini_dmi_count, numbersign, randomdmicrc + 1, newline);
        }
        memset(dmifilename, 0, 17);
        sprintf(dmifilename, "DMI_%08lX.bin", ini_dmi[randomdmicrc]);
        dmifile = openstealthfile(dmifilename, stealthdir, webstealthdir, STEALTH_FILE, "the online verified database");
        if (dmifile == NULL) {
            printf("ERROR: Failed to find or open '%s' (%s)%s", dmifilename, strerror(errno), newline);
            // try to open another dmi file?
          return 1;
        }
        memset(autofix_dmi, 0, 2048);
        // make sure that dmifile is 2048 bytes before reading it
        stealthfilesize = getfilesize(dmifile);
        if (stealthfilesize == -1) {  // seek error
            fclose(dmifile);
          return 1;
        }
        if (stealthfilesize != 2048) {
            color(red);
            printf("ERROR: %s is %"LL"d bytes! (should have been exactly 2048) AutoFix was aborted!%s", dmifilename, stealthfilesize, newline);
            color(normal);
            fclose(dmifile);
            deletestealthfile(dmifilename, stealthdir, false);
            return 1;
        }
        if (trytoreadstealthfile(autofix_dmi, 1, 2048, dmifile, dmifilename, 0) != 0) {
            fclose(dmifile);
            deletestealthfile(dmifilename, stealthdir, false);
          return 1;
        }
        fclose(dmifile);
        // check to see if autofix dmi is valid for this game
        printf("%sVerifying %s is valid before using it for AutoFix%s", sp5, dmifilename, newline);
        if (getzeros(autofix_dmi, 0, 2047) == 2048) {
            color(red);
            printf("ERROR: %s is blank! AutoFix was aborted!%s", dmifilename, newline);
            color(normal);
            deletestealthfile(dmifilename, stealthdir, false);
          return 1;
        }
        checkdmi(autofix_dmi);
        if (verbose) printf("%s", newline);
        if (dmi_stealthfailed || dmi_stealthuncertain) {
            // dmi failed stealth check against the xex/ss
            color(red);
            printf("ERROR: %s appears to be invalid! AutoFix was aborted!%s", dmifilename, newline);
            color(normal);
            deletestealthfile(dmifilename, stealthdir, false);
          return 1;
        }
        // check to see if autofix dmi matches ini crc
        if (ini_dmi[randomdmicrc] != dmi_crc32) {
            color(red);
            printf("ERROR: %s has an incorrect CRC! (it was %08lX and should have been %08lX) AutoFix was aborted!%s", dmifilename, dmi_crc32, ini_dmi[randomdmicrc], newline);
            color(normal);
            deletestealthfile(dmifilename, stealthdir, false);
          return 1;
        }
    }
    if (ini_pfi != pfi_crc32) {
        fixpfi = true;
        if (ini_pfi == 0) {
            printf("ERROR: Failed to find a PFI CRC in '%s'%s", inifilename, newline);
          return 1;
        }
        memset(pfifilename, 0, 17);
        sprintf(pfifilename, "PFI_%08lX.bin", ini_pfi);
        pfifile = openstealthfile(pfifilename, stealthdir, webstealthdir, STEALTH_FILE, "the online verified database");
        if (pfifile == NULL) {
            printf("ERROR: Failed to find or open '%s' (%s)%s", pfifilename, strerror(errno), newline);
          return 1;
        }
        memset(autofix_pfi, 0, 2048);
        // make sure that pfifile is 2048 bytes before reading it
        stealthfilesize = getfilesize(pfifile);
        if (stealthfilesize == -1) {  // seek error
            fclose(pfifile);
          return 1;
        }
        if (stealthfilesize != 2048) {
            color(red);
            printf("ERROR: %s is %"LL"d bytes! (should have been exactly 2048) AutoFix was aborted!%s", pfifilename, stealthfilesize, newline);
            color(normal);
            fclose(pfifile);
            deletestealthfile(pfifilename, stealthdir, false);
            return 1;
        }
        if (trytoreadstealthfile(autofix_pfi, 1, 2048, pfifile, pfifilename, 0) != 0) {
            fclose(pfifile);
            deletestealthfile(pfifilename, stealthdir, false);
          return 1;
        }
        fclose(pfifile);
        // check to see if autofix pfi is valid for this game
        printf("%sVerifying %s is valid before using it for AutoFix%s", sp5, pfifilename, newline);
        if (getzeros(autofix_pfi, 0, 2047) == 2048) {
            color(red);
            printf("ERROR: %s is blank! AutoFix was aborted!%s", pfifilename, newline);
            color(normal);
            deletestealthfile(pfifilename, stealthdir, false);
          return 1;
        }
        checkpfi(autofix_pfi);
        if (verbose) printf("%s", newline);
        if (pfi_stealthfailed || pfi_stealthuncertain) {
            // pfi failed stealth check
            color(red);
            printf("ERROR: %s appears to be invalid! AutoFix was aborted!%s", pfifilename, newline);
            color(normal);
            deletestealthfile(pfifilename, stealthdir, false);
          return 1;
        }
        // check to see if autofix pfi matches ini crc
        if (ini_pfi != pfi_crc32) {
            color(red);
            printf("ERROR: %s has an incorrect CRC! (it was %08lX and should have been %08lX) AutoFix was aborted!%s", pfifilename, pfi_crc32, ini_pfi, newline);
            color(normal);
            deletestealthfile(pfifilename, stealthdir, false);
          return 1;
        }
    }
    if (ini_video != video_crc32) {
        fixvideo = true;
        if (ini_video == 0) {
            printf("ERROR: Failed to find a Video CRC in '%s'%s", inifilename, newline);
            fclose(inifile);
            deletestealthfile(inifilename, stealthdir, false);
          return 1;
        }
        memset(videofilename, 0, 19);
        sprintf(videofilename, "Video_%08lX.iso", ini_video);
        bool video_file_is_hosted = false;
        for (m=0;m<num_videoentries;m++) {
            if (ini_video == mostrecentvideoentries[m].crc) {
                if (mostrecentvideoentries[m].hosted) video_file_is_hosted = true;
              break;
            }
        }
        if (video_file_is_hosted) videofile = openstealthfile(videofilename, stealthdir, webstealthdir, SMALL_VIDEO_FILE, "the online verified database");
        else videofile = openstealthfile(videofilename, stealthdir, webstealthdir, GIANT_VIDEO_FILE, "the online verified database");
        if (videofile == NULL) {
            color(yellow);
            if (!video_file_is_hosted) printf("Failed to find or open '%s' (%s)%s"
                "This is a very large video partition that isn't hosted on the database...%s"
                "You will have to find it elsewhere and put it in your StealthFiles folder - %s"
                "Check the Download page on abgx360.net%s",
                videofilename, strerror(errno), newline, newline, newline, newline);
            else printf("ERROR: Failed to find or open '%s' (%s)%s", videofilename, strerror(errno), newline);
            color(normal);
          return 1;
        }
        // check to see if autofix video is valid for this game
        printf("%sVerifying %s is valid before using it for AutoFix%s", sp5, videofilename, newline);
        if (verbose) printf("%s", newline);
        checkvideo(videofilename, videofile, false, false);
        if (verbose) printf("%s", newline);
        if (video_stealthfailed || video_stealthuncertain) {
            // video failed stealth check
            color(red);
            printf("ERROR: %s appears to be invalid! AutoFix was aborted!%s", videofilename, newline);
            color(normal);
            fclose(videofile);
            deletestealthfile(videofilename, stealthdir, true);
          return 1;
        }
        // check to see if autofix video matches ini crc
        if (ini_video != video_crc32) {
            color(red);
            printf("ERROR: %s has an incorrect CRC! (it was %08lX and should have been %08lX) AutoFix was aborted!%s", videofilename, video_crc32, ini_video, newline);
            color(normal);
            fclose(videofile);
            deletestealthfile(videofilename, stealthdir, true);
          return 1;
        }
    }
    printf("%sAutomatically patching stealth files...%s", sp5, newline);
    // reopen iso file for reading and writing
    fp = freopen(isofilename, "rb+", fp);
    if (fp == NULL) {
        color(yellow);
        printf("ERROR: Failed to reopen %s for writing. (%s)%s", isofilename, strerror(errno), newline);
        color(normal);
        fp = fopen(isofilename, "rb");
        if (fp == NULL) {
            color(red);
            printf("ERROR: Failed to reopen %s for reading! (%s) Game over man... Game over!%s", isofilename, strerror(errno), newline);
            color(normal);
          exit(1);
        }
      return 1;
    }
    if (video == 0) {
        // just a game partition (iso needs to be rebuilt)
        if (norebuild) {
            color(yellow);
            printf("ERROR: You have chosen not to rebuild ISOs, but this one needs to be rebuilt!%s", newline);
            color(normal);
          return 1;
        }
        else if (rebuildfailed) {
            color(yellow);
            printf("ERROR: This ISO needs to be rebuilt but a previous attempt failed!%s", newline);
            color(normal);
          return 1;
        }
        fprintf(stderr, "Image is just a game partition, rebuilding... ");
        if (rebuildiso(isofilename) != 0) {
            rebuildfailed = true;
            if (rebuildlowspace) {
                color(red);
                printf("Rebuilding Failed! Your ISO is now probably corrupt!%s", newline);
            }
            else {
                color(yellow);
                printf("Rebuilding Failed!%s", newline);
            }
            color(normal);
          return 1;
        }
        else if (checkdvdfile) {
            if (debug) printf("docheckdvdfile isofilename: %s%s", isofilename, newline);
            printf("%s", newline);
            docheckdvdfile();
        }
    }
    if (fixvideo) {
        // patch video
        if (verbose) printf("%sPatching Video from %s%s", sp5, videofilename, newline);
        videofilesize = getfilesize(videofile);
        if (videofilesize == -1) {  // seek error
            fclose(videofile);
          return 1;
        }
        sizeoverbuffer = videofilesize / BIGBUF_SIZE;
        bufferremainder = videofilesize % BIGBUF_SIZE;
        if (fseeko(fp, 0, SEEK_SET) != 0) {
            printseekerror(isofilename, "Patching video file");
            fclose(videofile);
          return 1;
        }
        if (fseeko(videofile, 0, SEEK_SET) != 0) {
            printseekerror(videofilename, "Patching video file");
            fclose(videofile);
          return 1;
        }
        initcheckread();
        initcheckwrite();
        for (m=0;m<sizeoverbuffer;m++) {
            if (checkreadandprinterrors(bigbuffer, 1, BIGBUF_SIZE, videofile, m, 0, videofilename, "patching") != 0) {
                fclose(videofile);
              return 1;
            }
            if (checkwriteandprinterrors(bigbuffer, 1, BIGBUF_SIZE, fp, m, 0, isofilename, "patching") != 0) {
                fclose(videofile);
              return 1;
            }
        }
        if (bufferremainder) {
            if (checkreadandprinterrors(bigbuffer, 1, bufferremainder, videofile, 0, videofilesize - bufferremainder, videofilename, "patching") != 0) {
                fclose(videofile);
              return 1;
            }
            if (checkwriteandprinterrors(bigbuffer, 1, bufferremainder, fp, 0, videofilesize - bufferremainder, isofilename, "patching") !=0) {
                fclose(videofile);
              return 1;
            }
        }
        donecheckread(videofilename);
        donecheckwrite(isofilename);
        fclose(videofile);
        if (verbose) fprintf(stderr, "     Padding Video... ");
        if (padzeros(fp, isofilename, videofilesize, (long long) total_sectors_available_for_video_data*2048) != 0) return 1;
        if (verbose) fprintf(stderr, "Done\n");
    }
    if (fixpfi) {  // patch pfi
        if (verbose) printf("%sPatching PFI from %s%s", sp5, pfifilename, newline);
        if (trytowritestealthfile(autofix_pfi, 1, 2048, fp, isofilename, 0xFD8E800LL) != 0) return 1;
    }
    if (!keepdmi) {  // patch dmi
        if (verbose) printf("%sPatching DMI from %s%s", sp5, dmifilename, newline);
        if (trytowritestealthfile(autofix_dmi, 1, 2048, fp, isofilename, 0xFD8F000LL) != 0) return 1;
    }
    if (fixss) {  // patch ss
        if (verbose) printf("%sPatching SS from %s%s", sp5, ssfilename, newline);
        if (trytowritestealthfile(ss, 1, 2048, fp, isofilename, 0xFD8F800LL) != 0) return 1;
    }
    
    color(green);
    printf("Stealth files patched successfully!%s", newline);
    color(normal);
    
    if (ini_game != 0 && game_crc32 != 0 && ini_game == game_crc32) {
        color(green);
        printf("Game partition CRC matches the verified ini!%s", newline);
        color(normal);
    }
    else {
        if (ini_game == 0 && !checkgamecrcnever) {
            color(yellow);
            printf("ERROR: Failed to find a Game CRC in '%s', unable to verify game data!%s", inifilename, newline);
            color(normal);
          return 2;
        }
        if (game_crc32 == 0 && !checkgamecrcnever) {
            if (docheckgamecrc() == 0) {
                if (corruptionoffsetcount == 0) {
                    color(green);
                    if (verbose) printf("%s", sp5);
                    printf("AnyDVD style corruption was not detected%s", newline);
                    color(normal);
                }
                if (verbose) printf("%sGame CRC = %08lX%s%s", sp5, game_crc32, newline, newline);
                if (ini_game == game_crc32) {
                    color(green);
                    printf("Game partition CRC matches the verified ini!%s", newline);
                    color(normal);
                }
                else {
                    printbadgamecrcerror();
                  return 3;
                }
            }
            else {
                checkgamecrcnever = true;
                gamecrcfailed = true;
            }
        }
        if (game_crc32 == 0) {
            if (usercancelledgamecrc) {
                // user skipped game crc
                color(yellow);
                printf("Stealth now matches the verified ini but checking the Game CRC was cancelled%s", newline);
                color(normal);
            }
            else if (gamecrcfailed) {
                // there was an error checking the game crc
                color(red);
                printf("Stealth now matches the verified ini but an error occurred while checking the Game CRC%s", newline);
                color(normal);
            }
            else {
                // game crc was never checked
                color(yellow);
                printf("Stealth now matches the verified ini but the Game CRC was not checked%s", newline);
                color(normal);
            }
          return 2;
        }
        if (game_crc32 != 0 && ini_game != game_crc32) {
            printbadgamecrcerror();
          return 3;
        }
    }
  return 0;
}

int doaddsplitvid() {
    int i;
    unsigned long m;
    bool openforwriting = false;
    unsigned long VideoL1onL0_crc32 = 0, VideoL1onL1_crc32 = 0;
    if (video == 0) {
        color(yellow);
        printf("Image is just a game partition, SplitVid check aborted%s", newline);
        color(normal);
      return 1;
    }
    if (video_crc32 == 0 || pfi_crc32 == 0 || !pfi_foundsectorstotal || pfi_stealthfailed || pfi_stealthuncertain || video_stealthfailed || video_stealthuncertain) {
        color(yellow);
        printf("Cannot check SplitVid due to invalid or uncertain PFI/Video%s", newline);
        color(normal);
      return 1;
    }
    // check to see if data is already valid first
    // filesize should be at least [pfi_offsetL1 + pfi_sectorsL1 * 2048]
    if ((unsigned long long) fpfilesize < pfi_offsetL1 + pfi_sectorsL1 * 2048) {
        if (!writefile) {
            color(yellow);
            printf("Size is too small for SplitVid but writing is disabled, cannot fix!%s", newline);
            color(normal);
          return 1;
        }
        printf("Size is too small for SplitVid, extending...%s", newline);
        if (dotruncate(isofilename, fpfilesize, pfi_offsetL1 + pfi_sectorsL1 * 2048, false) != 0) return 1;
        // get the new filesize
        fpfilesize = getfilesize(fp);
        if (fpfilesize == -1) return 1;  // seek error
        goto patchsplitvid;  // file had to be extended so we know splitvid is invalid
    }
    else if ((unsigned long long) fpfilesize > pfi_offsetL1 + pfi_sectorsL1 * 2048) {
        // this really doesn't matter unless the image is too big to burn
        if (writefile) {
            printf("Size is too large for SplitVid, truncating...%s", newline);
            if (dotruncate(isofilename, fpfilesize, pfi_offsetL1 + pfi_sectorsL1 * 2048, false) != 0) return 1;
            // get the new filesize
            fpfilesize = getfilesize(fp);
            if (fpfilesize == -1) return 1;  // seek error
        }
        else if (!dvdarg) {
            color(yellow);
            printf("Size is larger than necessary for SplitVid but would have been truncated if writing was enabled%s", newline);
            color(normal);
        }
    }
    // compare crc of [pfi_sectorsL1] sectors at [pfi_offsetL0end + 1] to [pfi_sectorsL1] sectors at [pfi_offsetL1]
    fprintf(stderr, "Comparing L1 Video on L0 to L1 Video on L1... ");
    if (fseeko(fp, pfi_offsetL0end + 1, SEEK_SET) != 0) {
        printseekerror(isofilename, "Checking SplitVid");
      return 1;
    }
    memset(bigbuffer, 0, BIGBUF_SIZE);
    sizeoverbuffer = pfi_sectorsL1 * 2048 / BIGBUF_SIZE;
    bufferremainder = pfi_sectorsL1 * 2048 % BIGBUF_SIZE;
    initcheckread();
    for (m=0; m<sizeoverbuffer; m++) {
        if (sizeoverbuffer >= 50 && m && (m % (2 * sizeoverbuffer / 100) == 0) && (m / (2 * sizeoverbuffer / 100) <= 100)) {
            for (i=0;i<charsprinted;i++) fprintf(stderr, "\b");
            charsprinted = fprintf(stderr, "%2lu%% ", m / (2 * sizeoverbuffer / 100));
        }
        if (checkreadandprinterrors(bigbuffer, 1, BIGBUF_SIZE, fp, m, pfi_offsetL0end + 1, "L1 Video on L0", "Checking SplitVid") != 0) return 1;
        VideoL1onL0_crc32 = crc32(VideoL1onL0_crc32, bigbuffer, BIGBUF_SIZE);
    }
    if (bufferremainder) {
        if (checkreadandprinterrors(bigbuffer, 1, bufferremainder, fp, 0, pfi_offsetL0end + 1 + (pfi_sectorsL1 * 2048) - bufferremainder, "L1 Video on L0", "Checking SplitVid") != 0) return 1;
        VideoL1onL0_crc32 = crc32(VideoL1onL0_crc32, bigbuffer, bufferremainder);
    }
    for (i=0;i<charsprinted;i++) fprintf(stderr, "\b");
    donecheckread(isofilename);
    if (fseeko(fp, pfi_offsetL1, SEEK_SET) != 0) {
        printseekerror(isofilename, "Checking SplitVid");
      return 1;
    }
    initcheckread();
    for (m=0; m<sizeoverbuffer; m++) {
        if (sizeoverbuffer >= 50 && m && (m % (2 * sizeoverbuffer / 100) == 0) && (m / (2 * sizeoverbuffer / 100) <= 100)) {
            for (i=0;i<charsprinted;i++) fprintf(stderr, "\b");
            charsprinted = fprintf(stderr, "%2lu%% ", m / (2 * sizeoverbuffer / 100) + 50);
        }
        if (checkreadandprinterrors(bigbuffer, 1, BIGBUF_SIZE, fp, m, pfi_offsetL1, "L1 Video on L1", "Checking SplitVid") != 0) return 1;
        VideoL1onL1_crc32 = crc32(VideoL1onL1_crc32, bigbuffer, BIGBUF_SIZE);
    }
    if (bufferremainder) {
        if (checkreadandprinterrors(bigbuffer, 1, bufferremainder, fp, 0, pfi_offsetL1 + (pfi_sectorsL1 * 2048) - bufferremainder, "L1 Video on L1", "Checking SplitVid") != 0) return 1;
        VideoL1onL1_crc32 = crc32(VideoL1onL1_crc32, bigbuffer, bufferremainder);
    }
    for (i=0;i<charsprinted;i++) fprintf(stderr, "\b");
    for (i=0;i<charsprinted;i++) fprintf(stderr, " ");
    for (i=0;i<charsprinted;i++) fprintf(stderr, "\b");
    donecheckread(isofilename);
    if (VideoL1onL0_crc32 == VideoL1onL1_crc32) {
        color(green);
        printf("SplitVid is valid%s", newline);
        color(normal);
      goto checkL1padding;
    }
    
    patchsplitvid:
    // copy [pfi_sectorsL1] sectors from [pfi_offsetL0end + 1] to [pfi_offsetL1]
    if (!writefile) {
        color(yellow); printf("SplitVid is invalid but writing is disabled, unable to fix!%s", newline); color(normal);
      return 1;
    }
    printf("SplitVid is invalid, fixing...%s", newline);
    if (!openforwriting) {
        fp = freopen(isofilename, "rb+", fp);
        if (fp == NULL) {
            color(yellow); printf("ERROR: Failed to reopen %s for writing! (%s) Unable to fix SplitVid!%s", isofilename, strerror(errno), newline); color(normal);
            fp = fopen(isofilename, "rb");
            if (fp == NULL) {
                color(red); printf("ERROR: Failed to reopen %s for reading! (%s) Game over man... Game over!%s", isofilename, strerror(errno), newline); color(normal);
              exit(1);
            }
          return 1;
        }
        openforwriting = true;
    }
    initcheckread(); initcheckwrite();
    for (m=0;m<pfi_sectorsL1;m++) {
        if (fseeko(fp, pfi_offsetL0end + 1 + (m * 2048), SEEK_SET) != 0) {
            printseekerror(isofilename, "Fixing SplitVid");
          return 1;
        }
        if (checkreadandprinterrors(ubuffer, 1, 2048, fp, 0, pfi_offsetL0end + 1 + (m * 2048), "L1 Video on L0", "Fixing SplitVid") != 0) return 1;
        if (fseeko(fp, pfi_offsetL1 + (m * 2048), SEEK_SET) != 0) {
            printseekerror(isofilename, "Fixing SplitVid");
          return 1;
        }
        if (checkwriteandprinterrors(ubuffer, 1, 2048, fp, 0, pfi_offsetL1 + (m * 2048), "L1 Video on L1", "Fixing SplitVid") != 0) return 1;
    }
    donecheckread(isofilename); donecheckwrite(isofilename);
    color(green); printf("SplitVid was added successfully!%s", newline); color(normal);
    
    checkL1padding:
    if (checkpadding) {
        // ((pfi_offsetL1 - 7572881408) / 2048) sectors should be blank starting at offset 7572881408
        if (fseeko(fp, 7572881408LL, SEEK_SET) != 0) {
            printseekerror(isofilename, "Checking zero padding");
          return 1;
        }
        memset(bigbuffer, 0, BIGBUF_SIZE);
        sizeoverbuffer = (unsigned long) (pfi_offsetL1 - 7572881408LL) / BIGBUF_SIZE;
        bufferremainder = (unsigned long) (pfi_offsetL1 - 7572881408LL) % BIGBUF_SIZE;
        initcheckread();
        bool videoL1zeropadding = true;
        long dataloop = 0;
        if (debug) printf("size = %lu, sizeoverbuffer = %lu, bufferremainder = %lu%s", (unsigned long) (pfi_offsetL1 - 7572881408LL), sizeoverbuffer, bufferremainder, newline);
        fprintf(stderr, "Checking L1 Video padding... ");
        for (m=0;m<sizeoverbuffer;m++) {
            if (sizeoverbuffer >= 100 && m && (m % (sizeoverbuffer / 100) == 0) && (m / (sizeoverbuffer / 100) <= 100)) {
                for (i=0;i<charsprinted;i++) fprintf(stderr, "\b");
                charsprinted = fprintf(stderr, "%2lu%% ", m / (sizeoverbuffer / 100));
            }
            if (checkreadandprinterrors(bigbuffer, 1, BIGBUF_SIZE, fp, m, 7572881408LL, "L1 Video Padding", "checking zero padding") != 0) return 1;
            if (getzeros(bigbuffer, 0, BIGBUF_SIZE - 1) != BIGBUF_SIZE) {
                videoL1zeropadding = false;
                dataloop = m;
                if (debug) printf("data found at 0x%"LL"X, current offset: 0x%"LL"X, dataloop = %ld%s",
                                  (unsigned long long) m*BIGBUF_SIZE + 7572881408LL, (unsigned long long) ftello(fp), dataloop, newline);
              goto skipL1remainder;
            }
        }
        if (bufferremainder) {
            if (checkreadandprinterrors(bigbuffer, 1, bufferremainder, fp, 0, pfi_offsetL1 - bufferremainder, "L1 Video Padding", "checking zero padding") != 0) return 1;
            if (getzeros(bigbuffer, 0, bufferremainder - 1) != bufferremainder) {
                videoL1zeropadding = false;
                dataloop = -1;  // identify that data was found in the bufferremainder
                if (debug) printf("data found at 0x%"LL"X, current offset: 0x%"LL"X, dataloop = %ld%s",
                                   pfi_offsetL1 - bufferremainder, (unsigned long long) ftello(fp), dataloop, newline);
              goto skipL1remainder;
            }
        }
        skipL1remainder:
        for (i=0;i<charsprinted;i++) fprintf(stderr, "\b");
        for (i=0;i<charsprinted;i++) fprintf(stderr, " ");
        for (i=0;i<charsprinted;i++) fprintf(stderr, "\b");
        donecheckread("L1 Video padding");

        if (videoL1zeropadding) {
            color(green);
            printf("L1 Video is zero padded%s", newline);
            color(normal);
        }
        else {
            if (!writefile) {
                color(yellow); printf("L1 Video padding contains bad data but writing is disabled, unable to fix!%s", newline); color(normal);
              return 1;
            }
            printf("L1 Video padding contains bad data, fixing...%s", newline);
            if (extraverbose) {
                if (dataloop == -1) memset(bigbuffer+bufferremainder, 0, BIGBUF_SIZE - bufferremainder);
                unsigned long sectoroffset = 0;
                for (i=0;i<(BIGBUF_SIZE/2048);i++) {
                    if (debug) printf("%d: %lu zeros%s", i, getzeros(bigbuffer, (unsigned long) i*2048, (unsigned long) i*2048+2047), newline);
                    if (getzeros(bigbuffer, (unsigned long) i*2048, (unsigned long) i*2048+2047) != 2048) {
                        sectoroffset = (unsigned long) i*2048;
                      break;
                    }
                }
                printf("Showing first sector of bad data (0x%"LL"X) in hex and ascii:%s", dataloop == -1 ? (pfi_offsetL1 - bufferremainder + sectoroffset) : (7572881408LL + (unsigned long long) dataloop*BIGBUF_SIZE + (unsigned long long) sectoroffset), newline);
                hexdump(bigbuffer+sectoroffset, 0, 2048);
                printf("%s", newline);
            }
            if (!openforwriting) {
                fp = freopen(isofilename, "rb+", fp);
                if (fp == NULL) {
                    color(red);
                    printf("ERROR: Failed to reopen %s for writing! (%s) Fixing zero padding failed!%s", isofilename, strerror(errno), newline);
                    color(normal);
                    fp = fopen(isofilename, "rb");
                    if (fp == NULL) {
                        color(red);
                        printf("ERROR: Failed to reopen %s for reading! (%s) Game over man... Game over!%s", isofilename, strerror(errno), newline);
                        color(normal);
                      exit(1);
                    }
                  return 1;
                }
                openforwriting = true;
            }
            if (dataloop == -1) {
                if (fseeko(fp, pfi_offsetL1 - bufferremainder, SEEK_SET) != 0) {
                    printseekerror(isofilename, "Fixing zero padding");
                  return 1;
                }
            }
            else if (fseeko(fp, 7572881408LL + (unsigned long long) dataloop*BIGBUF_SIZE, SEEK_SET) != 0) {
                printseekerror(isofilename, "Fixing zero padding");
              return 1;
            }
            if (debug) {
                printf("Current offset: 0x%"LL"X%s", (unsigned long long) ftello(fp), newline);
            }
            initcheckwrite();
            memset(bigbuffer, 0, BIGBUF_SIZE);
            if (dataloop == -1) {
                if (checkwriteandprinterrors(bigbuffer, 1, bufferremainder, fp, 0, pfi_offsetL1 - bufferremainder, "L1 Video padding", "fixing zero padding") != 0) return 1;
            }
            else {
                for (m=0;m<sizeoverbuffer - dataloop;m++) {
                    if (checkwriteandprinterrors(bigbuffer, 1, BIGBUF_SIZE, fp, m, 7572881408LL + (unsigned long long) dataloop*BIGBUF_SIZE, "L1 Video padding", "fixing zero padding") != 0) return 1;
                }
                if (bufferremainder) {
                    if (checkwriteandprinterrors(bigbuffer, 1, bufferremainder, fp, 0, pfi_offsetL1 - bufferremainder, "L1 Video padding", "fixing zero padding") != 0) return 1;
                }
            }
            donecheckwrite("L1 Video padding");
            color(green);
            printf("L1 Video padding fixed successfully%s", newline);
            color(normal);
        }
    }
  return 0;
}

void printhtmltop(int argc, char *argv[]) {
    int i;
    if (!script) {
        printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\" \"http://www.w3.org/TR/html4/loose.dtd\">\n"
               "<html>\n"
               "<head>\n"
               "<meta http-equiv=\"Content-Type\" content=\"text/html;charset=windows-1252\">\n"
               "<title>");
        // use entire cmd line as title
        for (i=0;i<argc;i++) {
            if (i && strcasecmp(argv[i-1], "--pass") == 0) printf("password ");  // OMG the password is "password"!
            else printf("%s ", argv[i]);
        }
        printf("</title>\n"
               "<style type=\"text/css\">\n"
                   "<!--\n"
                   "body{font-family: Terminal, monospace; font-size: 14px; line-height: 12px; color: #CCCCCC;}\n"
                   ".sp{font-family: monospace;}\n"
                   ".green{color: #00FF00;}\n"
                   ".yellow{color: #FFFF00;}\n"
                   ".red{color: #FF0000;}\n"
                   ".cyan{color: #00FFFF;}\n"
                   ".blue{color: #0000FF;}\n"
                   ".darkblue{color: #000099;}\n"
                   ".white{color: #FFFFFF;}\n"
                   ".arrow{color: #0000FF;}\n"
                   ".box{color: #0000FF;}\n"
                   ".normal{color: #CCCCCC;}\n"
                   ".brown{color: #999900;}\n"
                   ".wtfhex{color: #FF0000;}\n"
                   ".wtfchar{color: #FFFFFF; background-color: #CC0000;}\n"
                   ".hexoffset{color: #808080;}\n"
                   ".darkgray{color: #808080;}\n"
                   ".filename{color: #CCCCCC; background-color: #0000FF;}\n"
                   ".normal_u{color: #CCCCCC; font-family: Code2000, "                      // 51,239 chars, 61,864 glyphs, 20,924 CJK, v1.16
                                                          "\"Arial Unicode MS\", "          // 38,917 chars, 50,377 glyphs, 20,902 CJK, v1.00
                                                          "\"TITUS Cyberbit Unicode\", "    // 36,161 chars,      ? glyphs,      ? CJK, v4.0
                                                          "\"Bitstream Cyberbit\", "        // 32,910 chars, 29,934 glyphs, 20,902 CJK, v2.0 beta
                                                          "\"GNU Unifont\", "               // 63,446 chars, 63,446 glyphs, 16,887 CJK, v5.1.20080914
                                                          "\"Y.OzFontN\", "                 // 21,360 chars, 59,678 glyphs,  9,829 CJK, v9.41
                                                          "\"Lucida Grande\", "  // (MAC OS X)  2,245 chars,  2,826 glyphs,      0 CJK, v5.0d8e1 rev 1.002
                                                          "\"Microsoft Sans Serif\", "      //  2,301 chars,  2,257 glyphs,      0 CJK, v1.41
                                                          "\"Lucida Sans Unicode\", "       //  1,765 chars,  1,776 glyphs,      0 CJK, v2.00
                                                          "monospace; font-size: 12px;}\n"
                   ".white_u{color: #FFFFFF; font-family: Code2000, \"Arial Unicode MS\", \"TITUS Cyberbit Unicode\", \"Bitstream Cyberbit\", \"GNU Unifont\", "
                                                         "\"Y.OzFontN\", \"Lucida Grande\", \"Microsoft Sans Serif\", \"Lucida Sans Unicode\", monospace; font-size: 12px;}\n"
                   ".darkgray_u{color: #808080; font-family: Code2000, \"Arial Unicode MS\", \"TITUS Cyberbit Unicode\", \"Bitstream Cyberbit\", \"GNU Unifont\", "
                                                          "\"Y.OzFontN\", \"Lucida Grande\", \"Microsoft Sans Serif\", \"Lucida Sans Unicode\", monospace; font-size: 12px;}\n"
                   ".achtitle{color: #FFFFFF; font-family: Code2000, \"Arial Unicode MS\", \"TITUS Cyberbit Unicode\", \"Bitstream Cyberbit\", \"GNU Unifont\", "
                                                         "\"Y.OzFontN\", \"Lucida Grande\", \"Microsoft Sans Serif\", \"Lucida Sans Unicode\", monospace; font-size: 16px; font-weight:bold; line-height: 16px;}\n"
                   "//-->\n"
               "</style>\n"
               "<script language=\"JavaScript\" type=\"text/javascript\">\n"
               "<!--\n"
               "function sh(unachievedid,achievedid,rowid,checkid) {\n"
               "	var unachievedtext = document.getElementById(unachievedid);\n"
               "	var achievedtext = document.getElementById(achievedid);\n"
               "	var tablerow = document.getElementById(rowid);\n"
               "	var checkedpng = document.getElementById(checkid);\n"
               "	if(unachievedtext.style.display == 'none') {\n"
               "		unachievedtext.style.display = 'inline';\n"
               "		achievedtext.style.display = 'none';\n"
               "		tablerow.style.opacity = '1.0';\n"
               "		tablerow.style.filter = 'alpha(opacity=100)';\n"  // for IE
               "		checkedpng.style.visibility = 'hidden';\n"
               "	}\n"
               "	else {\n"
               "		unachievedtext.style.display = 'none';\n"
               "		achievedtext.style.display = 'inline';\n"
               "		tablerow.style.opacity = '0.4';\n"
               "		tablerow.style.filter = 'alpha(opacity=40)';\n"  // for IE
               "		checkedpng.style.visibility = 'visible';\n"
               "	}\n"
               "}\n"
               "//-->\n"
               "</script>\n"
               "</head>\n"
               "<body bgcolor=\"#000000\">\n"
               "<span class=normal>");
    }
  return;
}

void printhtmlbottom() {
    if (!script) printf("</span></body></html>");
  return;
}

void printheader() {
    int i;
    if (!noheader) {
        color(blue);
        if (terminal) printf("ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ%s", newline);
        else printf("-------------------------------------------------------------------------------%s", newline);
        color(darkblue); printf("\\\\//\\\\//\\\\//\\\\//");
        if (terminal) { printf("\\\\/"); color(normal); printf("Û"); color(darkblue); printf("\\\\//\\ "); }
        else { color(white); printf("%s_ |_%s_ ", sp2, sp2); }
        color(green); printf("\\ \\/ /"); color(white);
        if (terminal) printf("ÚÄ¿ÚÄ¿ÚÄ¿");
        else printf("_%s_%s_%s", sp2, sp2, sp2);
        color(darkblue); printf("\\\\//\\\\//\\\\//\\\\//\\\\//\\\\//\\\\//\\\\//\\\\//\\\\%s//\\\\//\\\\//\\\\//\\", newline);
        if (!terminal) printf("\\");
        if (terminal) { color(normal); printf("ÛßÛ"); color(darkblue); printf("\\"); color(normal); printf("ÛßÛ");
                        color(darkblue); printf("\\"); color(normal); printf("ÛßÛ"); }
        else { color(white); printf(" (_||_)(_|"); }
        color(green); printf("/ /\\ \\"); color(white);
        if (terminal && html) printf(" Ä´ÃÄ¿| |");
        else if (terminal) printf(" Ä´ÃÄ¿³ ³");
        else printf("_||_ | | ");
        color(darkblue); printf("//\\\\//\\\\//\\\\//\\\\//\\\\//\\\\//\\\\//\\\\//\\\\//%s", newline); color(blue);
        if (terminal) { printf("ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ"); color(normal); printf("ßßß"); color(blue); printf("Ä");
                        color(normal); printf("ßßß"); color(blue); printf("Ä"); color(normal); printf("ßßÛ"); }
        else { printf("------------------------"); color(white); printf("_|"); }
        color(blue);
        if (terminal) printf("ÄÄÄÄÄÄ");
        else printf("------");
        color(white);
        if (terminal) printf("ÀÄÙÀÄÙÀÄÙ");
        else printf("_||_||_|"); color(blue);
        if (terminal) printf("Ä");
        else printf("--");
        color(normal); printf("%s", headerversion); color(blue);
        for (i=0;i<13 - (int) strlen(headerversion);i++) {
            if (terminal) printf("Ä");
            else printf("-");
        }
        color(normal); printf("[http://abgx360.net]"); color(blue);
        if (terminal) printf("ÄÄÄÄ");
        else printf("----");
        printf("%s", newline);
        color(normal);
        if (terminal) printf("%s%s%s ßßß", sp10, sp10, sp2);
        printf("%s", newline);
    }
  return;
}

void startunicode() {
    if (html) {
        unicode = true;
        printf("</span><span class=normal_u>");
    }
  return;
}

void endunicode() {
    if (html) {
        unicode = false;
        printf("</span><span class=normal>");
    }
  return;
}

void color(char *color) {
    if (stripcolors) {
      return;
    }
    #ifdef WIN32
        if (!html) {
            if (strcmp(color, normal) == 0) {
                if (printstderr) SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 7);
                else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
            }
            else if (strcmp(color, white) == 0) {
                if (printstderr) SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 15);
                else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 15);
            }
            else if (strcmp(color, green) == 0) {
                if (printstderr) SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 10);
                else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10);
            }
            else if (strcmp(color, blue) == 0) {
                if (printstderr) SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 9);
                else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 9);
            }
            else if (strcmp(color, darkblue) == 0) {
                if (printstderr) SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 1);
                else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 1);
            }
            else if (strcmp(color, yellow) == 0) {
                if (printstderr) SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 14);
                else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 14);
            }
            else if (strcmp(color, red) == 0) {
                if (printstderr) SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 12);
                else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
            }
            else if (strcmp(color, darkgray) == 0) {
                if (printstderr) SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 8);
                else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 8);
            }
            else if (strcmp(color, arrow) == 0) {
                if (printstderr) SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 9);  // blue
                else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 9);
            }
            else if (strcmp(color, box) == 0) {
                if (printstderr) SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 9);  // blue
                else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 9);
            }
            else if (strcmp(color, wtfhexcolor) == 0) {
                if (printstderr) SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 12);  // red
                else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12);
            }
            else if (strcmp(color, wtfcharcolor) == 0) {
                if (printstderr) SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 207);  // white on red background
                else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 207);
            }
            else if (strcmp(color, hexoffsetcolor) == 0) {
                if (printstderr) SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 8); // dark gray
                else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 8);
            }
            else if (strcmp(color, cyan) == 0) {
                if (printstderr) SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 11);
                else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 11);
            }
            else if (strcmp(color, brown) == 0) {
                if (printstderr) SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 6);
                else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 6);
            }
            else if (strcmp(color, filename) == 0) {
                if (printstderr) SetConsoleTextAttribute(GetStdHandle(STD_ERROR_HANDLE), 151);  // light gray on blue
                else SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 151);
            }
        }
        else {
            if (unicode) {
                if      (strcmp(color, normal) == 0)   printf("</span><span class=normal_u>");
                else if (strcmp(color, white) == 0)    printf("</span><span class=white_u>");
                else if (strcmp(color, darkgray) == 0) printf("</span><span class=darkgray_u>");
                else printf("</span>\n\n<!-- BUG in color()! -->\n\n<span class=normal_u>");  // shouldn't get here... you either forgot to use endunicode() or you used another color without adding it directly above this line
            }
            else printf("%s", color);
        }
    #else
        if (!html) {
            if (printstderr) fprintf(stderr, "%s", color);
            else printf("%s", color);
        }
        else {
            if (unicode) {
                if      (strcmp(color, normal) == 0)   printf("</span><span class=normal_u>");
                else if (strcmp(color, white) == 0)    printf("</span><span class=white_u>");
                else if (strcmp(color, darkgray) == 0) printf("</span><span class=darkgray_u>");
                else printf("</span>\n\n<!-- BUG in color()! -->\n\n<span class=normal_u>");  // shouldn't get here... you either forgot to use endunicode() or you used another color without adding it directly above this line
            }
            else printf("%s", color);
        }
    #endif
  return;
}

unsigned long getint(char* ptr) {
    unsigned long ret;
    ret =  (*(ptr+3) & 0xFF) << 24;
    ret |= (*(ptr+2) & 0xFF) << 16;
    ret |= (*(ptr+1) & 0xFF) << 8;
    ret |=  *(ptr+0) & 0xFF;
  return ret & 0xFFFFFFFF;
}

unsigned long getuint(unsigned char* ptr) {
    unsigned long ret;
    ret =  (*(ptr+3) & 0xFF) << 24;
    ret |= (*(ptr+2) & 0xFF) << 16;
    ret |= (*(ptr+1) & 0xFF) << 8;
    ret |=  *(ptr+0) & 0xFF;
  return ret & 0xFFFFFFFF;
}

unsigned long getuintmsb(unsigned char* ptr) {
    unsigned long ret;
    ret =  (*(ptr+0) & 0xFF) << 24;
    ret |= (*(ptr+1) & 0xFF) << 16;
    ret |= (*(ptr+2) & 0xFF) << 8;
    ret |=  *(ptr+3) & 0xFF;
  return ret & 0xFFFFFFFF;
}

unsigned long long getuint64(unsigned char* ptr) {
    unsigned long long ret;
    ret =  (unsigned long long) (*(ptr+7) & 0xFF) << 56;
    ret |= (unsigned long long) (*(ptr+6) & 0xFF) << 48;
    ret |= (unsigned long long) (*(ptr+5) & 0xFF) << 40;
    ret |= (unsigned long long) (*(ptr+4) & 0xFF) << 32;
    ret |= (unsigned long long) (*(ptr+3) & 0xFF) << 24;
    ret |= (unsigned long long) (*(ptr+2) & 0xFF) << 16;
    ret |= (unsigned long long) (*(ptr+1) & 0xFF) << 8;
    ret |= (unsigned long long) (*(ptr+0) & 0xFF);
  return ret & 0xFFFFFFFFFFFFFFFFLL;
}

// this function is used to count zeros (blank data) within an array
unsigned long getzeros(unsigned char* ptr, unsigned long firstbyte, unsigned long lastbyte) {
    unsigned long zeros = 0;
    unsigned long pos;
    for (pos=firstbyte; pos<=lastbyte; pos++) {
        if (ptr[pos] == 0) zeros++;
    }
  return zeros;
}

char *readstdin(char *dest, int size) {
    if (size <= 0) {
        printstderr = true;
        color(red);
        fprintf(stderr, "ERROR: size for readstdin is %d!\n", size);
        color(normal);
        printstderr = false;
        dest[0] = 0x0;
      return dest;
    }
    if (size == 1) {
        // just a friendly reminder in case i forget
        printstderr = true;
        color(red);
        fprintf(stderr, "hey dumbass programmer! readstdin doesn't work when size == 1!\n");
        color(normal);
        printstderr = false;
        dest[0] = 0x0;
      return dest;
    }
    int readchar = 0;
    int charsread = 1;
    readchar = fgetc(stdin);
    while (readchar != EOF && readchar != '\n') {
        if (charsread < size) dest[charsread-1] = readchar;
        readchar = fgetc(stdin);
        charsread++;
    }
    dest[size-1] = 0x0;  // terminating null
  return dest;
}

void printmediaid(unsigned char* mediaid) {
    int i;
    for (i=0; i<12; i++) printf("%02X", mediaid[i]);
    printf("-");
    for (i=12; i<16; i++) printf("%02X", mediaid[i]);
  return;
}

#define MAX_DATFILE_SIZE 32768

void makedat() {
    int i, j;
    unsigned short u;
    unsigned char datfilebuffer[MAX_DATFILE_SIZE];
    unsigned long m, n;
    static unsigned long datentries = 8;  // update this if adding more
    FILE *datfile = fopen("abgx360.dat", "wb");
    if (datfile != NULL) {
        memset(datfilebuffer, 0, MAX_DATFILE_SIZE);
        // save current version as latest
        datfilebuffer[0] = (unsigned char)  (currentversion & 0xFFL);
        datfilebuffer[1] = (unsigned char) ((currentversion & 0xFF00L) >> 8);
        datfilebuffer[2] = (unsigned char) ((currentversion & 0xFF0000L) >> 16);
        memcpy(datfilebuffer+0x10, headerversion, strlen(headerversion));
        memcpy(datfilebuffer+0x20, "latestversion", 13);
        // lastknown2ndwave for the benefit of v0.9.4 only
        memcpy(datfilebuffer+0x50, "\x00\x80\xD9\x5F\x6D\x77\xC9\x01", 8);
        memcpy(datfilebuffer+0x60, "2009/01/16", 10);
        memcpy(datfilebuffer+0x70, "lastknown2ndwave", 16);
        // validation string
        memcpy(datfilebuffer+0xA0, "youmoms says hi", 15);
        // put new entries in a table - using MSB now
        // number of entries
        datfilebuffer[0xC0] = (unsigned char) ((datentries & 0xFF000000L) >> 24);
        datfilebuffer[0xC1] = (unsigned char) ((datentries & 0x00FF0000L) >> 16);
        datfilebuffer[0xC2] = (unsigned char) ((datentries & 0x0000FF00L) >> 8);
        datfilebuffer[0xC3] = (unsigned char)  (datentries & 0x000000FFL);
        // 0xC4 - 0xC7 reserved
        m = 0xC8;
        n = m + (datentries * 8);
        // lastknown3rdwave - actual date is LSB - for the benefit of v1.0.0 and v1.0.1
        unsigned long long lastknown3rdwave = 0x01CA2B6050724000LL;  // 2009-09-02 - before the earliest 4th wave (Lips: Number One Hits)
        memcpy(datfilebuffer+m, "\x03\x03\x03\x03", 4);  // identify this entry as lastknown3rdwave
        datfilebuffer[m+4] = (unsigned char) ((n & 0xFF000000L) >> 24);  // give the address where it will be stored in the dat file (n)
        datfilebuffer[m+5] = (unsigned char) ((n & 0x00FF0000L) >> 16);
        datfilebuffer[m+6] = (unsigned char) ((n & 0x0000FF00L) >> 8);
        datfilebuffer[m+7] = (unsigned char)  (n & 0x000000FFL);
        datfilebuffer[n] =   (unsigned char)  (lastknown3rdwave & 0xFFLL);  // save the actual lastknown3rdwave date at n (8 bytes)
        datfilebuffer[n+1] = (unsigned char) ((lastknown3rdwave & 0xFF00LL) >> 8);
        datfilebuffer[n+2] = (unsigned char) ((lastknown3rdwave & 0xFF0000LL) >> 16);
        datfilebuffer[n+3] = (unsigned char) ((lastknown3rdwave & 0xFF000000LL) >> 24);
        datfilebuffer[n+4] = (unsigned char) ((lastknown3rdwave & 0xFF00000000LL) >> 32);
        datfilebuffer[n+5] = (unsigned char) ((lastknown3rdwave & 0xFF0000000000LL) >> 40);
        datfilebuffer[n+6] = (unsigned char) ((lastknown3rdwave & 0xFF000000000000LL) >> 48);
        datfilebuffer[n+7] = (unsigned char) ((lastknown3rdwave & 0xFF00000000000000LL) >> 56);
        n += 8;  // don't forget to increment n with data size
        m += 8;  // ready for next entry
        // lastknown4thwave - actual date is LSB - for the benefit of v1.0.2
        unsigned long long lastknown4thwave = 0x01CA3FCE9F2FC000LL;  // 2009-09-28
        memcpy(datfilebuffer+m, "\x04\x04\x04\x04", 4);  // identify this entry as lastknown4thwave
        datfilebuffer[m+4] = (unsigned char) ((n & 0xFF000000L) >> 24);  // give the address where it will be stored in the dat file (n)
        datfilebuffer[m+5] = (unsigned char) ((n & 0x00FF0000L) >> 16);
        datfilebuffer[m+6] = (unsigned char) ((n & 0x0000FF00L) >> 8);
        datfilebuffer[m+7] = (unsigned char)  (n & 0x000000FFL);
        datfilebuffer[n] =   (unsigned char)  (lastknown4thwave & 0xFFLL);  // save the actual lastknown4thwave date at n (8 bytes)
        datfilebuffer[n+1] = (unsigned char) ((lastknown4thwave & 0xFF00LL) >> 8);
        datfilebuffer[n+2] = (unsigned char) ((lastknown4thwave & 0xFF0000LL) >> 16);
        datfilebuffer[n+3] = (unsigned char) ((lastknown4thwave & 0xFF000000LL) >> 24);
        datfilebuffer[n+4] = (unsigned char) ((lastknown4thwave & 0xFF00000000LL) >> 32);
        datfilebuffer[n+5] = (unsigned char) ((lastknown4thwave & 0xFF0000000000LL) >> 40);
        datfilebuffer[n+6] = (unsigned char) ((lastknown4thwave & 0xFF000000000000LL) >> 48);
        datfilebuffer[n+7] = (unsigned char) ((lastknown4thwave & 0xFF00000000000000LL) >> 56);
        n += 8;  // don't forget to increment n with data size
        m += 8;  // ready for next entry
        // lastknownwave - actual date is LSB
        memcpy(datfilebuffer+m, "\x00LKW", 4);  // identify this entry as lastknownwave
        datfilebuffer[m+4] = (unsigned char) ((n & 0xFF000000L) >> 24);  // give the address where it will be stored in the dat file (n)
        datfilebuffer[m+5] = (unsigned char) ((n & 0x00FF0000L) >> 16);
        datfilebuffer[m+6] = (unsigned char) ((n & 0x0000FF00L) >> 8);
        datfilebuffer[m+7] = (unsigned char)  (n & 0x000000FFL);
        datfilebuffer[n] =   (unsigned char)  (lastknownwave & 0xFFLL);  // save the actual lastknownwave date at n (8 bytes)
        datfilebuffer[n+1] = (unsigned char) ((lastknownwave & 0xFF00LL) >> 8);
        datfilebuffer[n+2] = (unsigned char) ((lastknownwave & 0xFF0000LL) >> 16);
        datfilebuffer[n+3] = (unsigned char) ((lastknownwave & 0xFF000000LL) >> 24);
        datfilebuffer[n+4] = (unsigned char) ((lastknownwave & 0xFF00000000LL) >> 32);
        datfilebuffer[n+5] = (unsigned char) ((lastknownwave & 0xFF0000000000LL) >> 40);
        datfilebuffer[n+6] = (unsigned char) ((lastknownwave & 0xFF000000000000LL) >> 48);
        datfilebuffer[n+7] = (unsigned char) ((lastknownwave & 0xFF00000000000000LL) >> 56);
        n += 8;  // don't forget to increment n with data size
        m += 8;  // ready for next entry
        // total_sectors_available_for_video_data - LSB
        memcpy(datfilebuffer+m, "TSAV", 4);  // identify this entry as total_sectors_available_for_video_data
        datfilebuffer[m+4] = (unsigned char) ((n & 0xFF000000L) >> 24);  // give the address where it will be stored in the dat file (n)
        datfilebuffer[m+5] = (unsigned char) ((n & 0x00FF0000L) >> 16);
        datfilebuffer[m+6] = (unsigned char) ((n & 0x0000FF00L) >> 8);
        datfilebuffer[m+7] = (unsigned char)  (n & 0x000000FFL);
        datfilebuffer[n] =   (unsigned char)  (total_sectors_available_for_video_data & 0xFFL);  // save the actual total_sectors_available_for_video_data at n (4 bytes)
        datfilebuffer[n+1] = (unsigned char) ((total_sectors_available_for_video_data & 0xFF00L) >> 8);
        datfilebuffer[n+2] = (unsigned char) ((total_sectors_available_for_video_data & 0xFF0000L) >> 16);
        datfilebuffer[n+3] = (unsigned char) ((total_sectors_available_for_video_data & 0xFF000000L) >> 24);
        n += 4;  // don't forget to increment n with data size
        m += 8;  // ready for next entry
        // media ids for AP25 games with no AP25 flag in the xex
        memcpy(datfilebuffer+m, "APMI", 4);  // identify this entry as AP25 media ids
        datfilebuffer[m+4] = (unsigned char) ((n & 0xFF000000L) >> 24);  // give the address where it will be stored in the dat file (n)
        datfilebuffer[m+5] = (unsigned char) ((n & 0x00FF0000L) >> 16);
        datfilebuffer[m+6] = (unsigned char) ((n & 0x0000FF00L) >> 8);
        datfilebuffer[m+7] = (unsigned char)  (n & 0x000000FFL);
        datfilebuffer[n] =   (unsigned char) ((NUM_CURRENTAP25MEDIAIDS & 0xFF000000) >> 24);  // save number of AP25 media ids
        datfilebuffer[n+1] = (unsigned char) ((NUM_CURRENTAP25MEDIAIDS & 0x00FF0000) >> 16);
        datfilebuffer[n+2] = (unsigned char) ((NUM_CURRENTAP25MEDIAIDS & 0x0000FF00) >> 8);
        datfilebuffer[n+3] = (unsigned char)  (NUM_CURRENTAP25MEDIAIDS & 0x000000FF);
        n += 4;
        for (i=0;i<NUM_CURRENTAP25MEDIAIDS;i++) {  // save each media id starting at n (4 bytes each)
            for (j=0;j<4;j++) datfilebuffer[i*4+n+j] = currentap25mediaids[i].mediaid[j];  // save the mediaid
        }
        n += NUM_CURRENTAP25MEDIAIDS * 4;  // don't forget to increment n with data size
        m += 8;  // ready for next entry
        // pfi/video exceptions - for the benefit of v1.0.1 and v1.0.2
        struct pfiexception {unsigned char mediaid[16]; unsigned long long authored; unsigned int wave;};
        #define NUM_CURRENTPFIEXCEPTIONS 8  // update this when adding new exceptions
        struct pfiexception currentpfiexceptions[NUM_CURRENTPFIEXCEPTIONS] = {
            { "\x7E\xB0\x30\x33\xA2\xDC\x2E\x8C\xFE\xF8\x49\x19\x39\x7A\x8D\x12", 0x01C70C36D2568000LL, 2 }, // DDR Universe, 2006/11/20
            { "\xC8\x6C\x31\x6F\xB4\x39\xFA\xF3\xE1\xF7\x51\x89\x17\x2E\xD3\x96", 0x01C7045B2A350000LL, 2 }, // Microsoft Xbox 360 Ping 17 Test Disc v.01 Nov 2006, 2006/11/10
            { "\xDE\xBE\x7B\x11\xF4\x41\xCA\x49\xFC\x46\x69\x4F\x25\xC0\xF5\x65", 0x01C97B5B33EA4000LL, 3 }, // Disney Sing It: High School Musical 3 - Senior Year, 2009/01/21
            { "\xFF\x9C\x6C\x8C\xAE\x98\x78\x2F\x6F\xDA\xDB\xFB\x03\x25\xE4\xED", 0x01C98A4959C38000LL, 2 }, // Stoked (USA), 2009/02/09
            { "\x63\xDE\x26\x86\x0C\xF1\x26\x84\x1A\x31\x1A\xC6\x4E\xFD\x55\xC4", 0x01C98A4959C38000LL, 2 }, // Afro Samurai (PAL), 2009/02/09
            { "\x03\x12\xE3\x9D\x72\x33\xF5\xB3\xFE\xB4\xC8\x54\x68\x3C\x17\x81", 0x01C98BDBAE970000LL, 2 }, // Monsters vs. Aliens, 2009/02/11
            { "\xAF\xE5\x7B\xCF\x9F\xB8\x25\xC1\x49\x31\xAE\x68\x39\xB3\xE0\xD0", 0x01CA2C297ADC0000LL, 4 }, // Lips: Number One Hits, 2009/09/03
            { "\xF6\xE3\x63\x7A\x0D\x32\xB7\x05\x68\x21\x8C\x97\x39\xB3\xE0\xD1", 0x01CA2C297ADC0000LL, 4 }  // Lips: Deutsche Partyhits, 2009/09/03
            // don't forget to update NUM_CURRENTPFIEXCEPTIONS if adding a new exception here
            // lastknown2ndwave in makedat() needs to be before the earliest 3rd wave game (whether it's an exception or not)
            // lastknown3rdwave in makedat() needs to be before the earliest 4th wave game (whether it's an exception or not)
        };
        memcpy(datfilebuffer+m, "\xEE\xEE\xEE\xEE", 4);  // identify this entry as exceptions
        datfilebuffer[m+4] = (unsigned char) ((n & 0xFF000000L) >> 24);  // give the address where it will be stored in the dat file (n)
        datfilebuffer[m+5] = (unsigned char) ((n & 0x00FF0000L) >> 16);
        datfilebuffer[m+6] = (unsigned char) ((n & 0x0000FF00L) >> 8);
        datfilebuffer[m+7] = (unsigned char)  (n & 0x000000FFL);
        datfilebuffer[n] =   (unsigned char) ((NUM_CURRENTPFIEXCEPTIONS & 0xFF000000) >> 24);  // save number of exceptions
        datfilebuffer[n+1] = (unsigned char) ((NUM_CURRENTPFIEXCEPTIONS & 0x00FF0000) >> 16);
        datfilebuffer[n+2] = (unsigned char) ((NUM_CURRENTPFIEXCEPTIONS & 0x0000FF00) >> 8);
        datfilebuffer[n+3] = (unsigned char)  (NUM_CURRENTPFIEXCEPTIONS & 0x000000FF);
        n += 4;
        for (i=0;i<NUM_CURRENTPFIEXCEPTIONS;i++) {  // save each exception starting at n (32 bytes each [28 + 4 reserved])
            for (j=0;j<16;j++) datfilebuffer[i*32+n+j] = currentpfiexceptions[i].mediaid[j];  // save the mediaid
            datfilebuffer[i*32+n+16] = (unsigned char)  (currentpfiexceptions[i].authored & 0xFFLL);  // save the authoring date
            datfilebuffer[i*32+n+17] = (unsigned char) ((currentpfiexceptions[i].authored & 0xFF00LL) >> 8);
            datfilebuffer[i*32+n+18] = (unsigned char) ((currentpfiexceptions[i].authored & 0xFF0000LL) >> 16);
            datfilebuffer[i*32+n+19] = (unsigned char) ((currentpfiexceptions[i].authored & 0xFF000000LL) >> 24);
            datfilebuffer[i*32+n+20] = (unsigned char) ((currentpfiexceptions[i].authored & 0xFF00000000LL) >> 32);
            datfilebuffer[i*32+n+21] = (unsigned char) ((currentpfiexceptions[i].authored & 0xFF0000000000LL) >> 40);
            datfilebuffer[i*32+n+22] = (unsigned char) ((currentpfiexceptions[i].authored & 0xFF000000000000LL) >> 48);
            datfilebuffer[i*32+n+23] = (unsigned char) ((currentpfiexceptions[i].authored & 0xFF00000000000000LL) >> 56);
            datfilebuffer[i*32+n+24] = (unsigned char) ((currentpfiexceptions[i].wave & 0xFF000000) >> 24);  // save the wave number
            datfilebuffer[i*32+n+25] = (unsigned char) ((currentpfiexceptions[i].wave & 0x00FF0000) >> 16);
            datfilebuffer[i*32+n+26] = (unsigned char) ((currentpfiexceptions[i].wave & 0x0000FF00) >> 8);
            datfilebuffer[i*32+n+27] = (unsigned char)  (currentpfiexceptions[i].wave & 0x000000FF);
        }
        n += NUM_CURRENTPFIEXCEPTIONS * 32;  // don't forget to increment n with data size
        m += 8;  // ready for next entry
        // PFI hashes/descriptions
        memcpy(datfilebuffer+m, "\x00PFI", 4);  // identify this entry as PFI entries
        datfilebuffer[m+4] = (unsigned char) ((n & 0xFF000000L) >> 24);  // give the address where it will be stored in the dat file (n)
        datfilebuffer[m+5] = (unsigned char) ((n & 0x00FF0000L) >> 16);
        datfilebuffer[m+6] = (unsigned char) ((n & 0x0000FF00L) >> 8);
        datfilebuffer[m+7] = (unsigned char)  (n & 0x000000FFL);
        datfilebuffer[n] =   (unsigned char) ((NUM_CURRENTPFIENTRIES & 0xFF000000) >> 24);  // save number of entries
        datfilebuffer[n+1] = (unsigned char) ((NUM_CURRENTPFIENTRIES & 0x00FF0000) >> 16);
        datfilebuffer[n+2] = (unsigned char) ((NUM_CURRENTPFIENTRIES & 0x0000FF00) >> 8);
        datfilebuffer[n+3] = (unsigned char)  (NUM_CURRENTPFIENTRIES & 0x000000FF);
        n += 4;
        for (i=0;i<NUM_CURRENTPFIENTRIES;i++) {  // save each exception starting at n (1 byte for flags (currently none), 4 bytes for crc, 20 bytes for sha1, 2 bytes for description length and strlen(currentpfientries[i].description) bytes for description
            datfilebuffer[n] = 0x00;
            datfilebuffer[n+1] = (unsigned char) ((currentpfientries[i].crc & 0xFF000000) >> 24);  // save the crc
            datfilebuffer[n+2] = (unsigned char) ((currentpfientries[i].crc & 0x00FF0000) >> 16);
            datfilebuffer[n+3] = (unsigned char) ((currentpfientries[i].crc & 0x0000FF00) >> 8);
            datfilebuffer[n+4] = (unsigned char)  (currentpfientries[i].crc & 0x000000FF);
            for (j=0;j<20;j++) datfilebuffer[n+5+j] = currentpfientries[i].sha1[j]; // save the sha1 hash
            u = (unsigned short) strlen(currentpfientries[i].description);
            datfilebuffer[n+25] = (unsigned char) ((u & 0xFF00) >> 8);
            datfilebuffer[n+26] = (unsigned char)  (u & 0x00FF);
            memcpy(datfilebuffer+n+27, currentpfientries[i].description, (size_t) u);
            n += 27 + u;
        }
        m += 8;  // ready for next entry
        // Video hashes/descriptions
        memcpy(datfilebuffer+m, "\x00VID", 4);  // identify this entry as Video entries
        datfilebuffer[m+4] = (unsigned char) ((n & 0xFF000000L) >> 24);  // give the address where it will be stored in the dat file (n)
        datfilebuffer[m+5] = (unsigned char) ((n & 0x00FF0000L) >> 16);
        datfilebuffer[m+6] = (unsigned char) ((n & 0x0000FF00L) >> 8);
        datfilebuffer[m+7] = (unsigned char)  (n & 0x000000FFL);
        datfilebuffer[n] =   (unsigned char) ((NUM_CURRENTVIDEOENTRIES & 0xFF000000) >> 24);  // save number of entries
        datfilebuffer[n+1] = (unsigned char) ((NUM_CURRENTVIDEOENTRIES & 0x00FF0000) >> 16);
        datfilebuffer[n+2] = (unsigned char) ((NUM_CURRENTVIDEOENTRIES & 0x0000FF00) >> 8);
        datfilebuffer[n+3] = (unsigned char)  (NUM_CURRENTVIDEOENTRIES & 0x000000FF);
        n += 4;
        for (i=0;i<NUM_CURRENTVIDEOENTRIES;i++) {  // save each exception starting at n (1 byte for flags (currently only a hosted flag), 4 bytes for crc, 20 bytes for sha1, 2 bytes for description length and strlen(currentvideoentries[i].description) bytes for description
            if (currentvideoentries[i].hosted) datfilebuffer[n] = 0x01;
            else datfilebuffer[n] = 0x00;
            datfilebuffer[n+1] = (unsigned char) ((currentvideoentries[i].crc & 0xFF000000) >> 24);  // save the crc
            datfilebuffer[n+2] = (unsigned char) ((currentvideoentries[i].crc & 0x00FF0000) >> 16);
            datfilebuffer[n+3] = (unsigned char) ((currentvideoentries[i].crc & 0x0000FF00) >> 8);
            datfilebuffer[n+4] = (unsigned char)  (currentvideoentries[i].crc & 0x000000FF);
            for (j=0;j<20;j++) datfilebuffer[n+5+j] = currentvideoentries[i].sha1[j]; // save the sha1 hash
            u = (unsigned short) strlen(currentvideoentries[i].description);
            datfilebuffer[n+25] = (unsigned char) ((u & 0xFF00) >> 8);
            datfilebuffer[n+26] = (unsigned char)  (u & 0x00FF);
            memcpy(datfilebuffer+n+27, currentvideoentries[i].description, (size_t) u);
            n += 27 + u;
        }
        m += 8;  // ready for next entry
        // next entry here (don't forget to update datentries at the top of this function)
        
        // store crc of everything at end of file
        unsigned long datfile_crc32 = crc32(0, datfilebuffer, n);
        datfilebuffer[n] =   (unsigned char) ((datfile_crc32 & 0xFF000000) >> 24);
        datfilebuffer[n+1] = (unsigned char) ((datfile_crc32 & 0x00FF0000) >> 16);
        datfilebuffer[n+2] = (unsigned char) ((datfile_crc32 & 0x0000FF00) >> 8);
        datfilebuffer[n+3] = (unsigned char)  (datfile_crc32 & 0x000000FF);
        n += 4;
        if (n > MAX_DATFILE_SIZE) {
            // just in case we're retarded
            color(red);
            printf("ERROR: data size (%lu) is greater than MAX_DATFILE_SIZE (%d)! Writing abgx360.dat aborted!%s", n, MAX_DATFILE_SIZE, newline);
            color(normal);
            fclose(datfile);
          return;
        }
        if (verbose) {
            printf("Writing abgx360.dat (data size = %lu):%s", n, newline);
            hexdump(datfilebuffer, 0, n);
        }
        initcheckwrite();
        if (checkwriteandprinterrors(datfilebuffer, 1, n, datfile, 0, 0, "abgx360.dat", "writing") != 0) {
            fclose(datfile);
            return;
        }
        donecheckwrite("abgx360.dat");
        fclose(datfile);
        color(green);
        printf("Successfully wrote abgx360.dat%s", newline);
        color(normal);
    }
    else {
        color(red);
        printf("ERROR: Failed to open abgx360.dat for writing%s", newline);
        color(normal);
    }
  return;
}

void checkdat() {
    int i;
    unsigned long m, n;
    unsigned short u;
    char *s;
    char datpathbuffer[2048];
    memset(datpathbuffer, 0, 2048);
    if (!homeless) {
        strcat(datpathbuffer, homedir);
        strcat(datpathbuffer, abgxdir);
    }
    strcat(datpathbuffer, "abgx360.dat");
    FILE *datfile = fopen(datpathbuffer, "rb");
    if (datfile != NULL) {
        long long lldatfilesize = getfilesize(datfile);
        if (debug) printf("lldatfilesize = %"LL"d%s", lldatfilesize, newline);
        if (lldatfilesize == -1) return;  // seek error
        if (lldatfilesize > WOW_THATS_A_LOT_OF_RAM || lldatfilesize < 200) return;  // should never be this big or small
        unsigned long datfilesize = (unsigned long) lldatfilesize;  // should easily fit - this helps for comparison to unsigned long offsets
        if (debug) printf("datfilesize = %lu%s", datfilesize, newline);
        unsigned char datfilebuffer[datfilesize];
        memset(datfilebuffer, 0, datfilesize);
        initcheckread();
        if (checkreadandprinterrors(datfilebuffer, 1, datfilesize, datfile, 0, 0, "abgx360.dat", "reading abgx360.dat") != 0) return;
        donecheckread("abgx360.dat");
        fclose(datfile);
        // see if the file is valid before pulling values from it
        if (memcmp(datfilebuffer+0xA0, "youmoms says hi", 15) != 0) {
            if (debug) {
                printf("required string not found in abgx360.dat, file is invalid:%s", newline);
                hexdump(datfilebuffer, 0, datfilesize);
            }
            // delete invalid abgx360.dat
            remove(datpathbuffer);
          return;
        }
        if (crc32(0, datfilebuffer, datfilesize - 4) != getuintmsb(datfilebuffer+datfilesize-4)) { // embedded crc check
            if (debug) {
                printf("crc32 is bad for abgx360.dat, file is invalid:%s", newline);
                hexdump(datfilebuffer, 0, datfilesize);
            }
            // delete invalid abgx360.dat
            remove(datpathbuffer);
          return;
        }
        latestversion = getuint(datfilebuffer);
        if (latestversion > currentversion) {
            color(yellow);
            printf("An updated version of abgx360 is available: v%u.%u.%u%s%s",
                   datfilebuffer[2], datfilebuffer[1], datfilebuffer[0], newline, newline);
            color(normal);
        }
        if (latestversion < currentversion) {
            // don't process an old dat file
            if (debug) printf("%s: (appears to be old - not processing)%slatestversion = 0x%06lX, currentversion = 0x%06lX%s",
                               datpathbuffer, newline, latestversion, currentversion, newline);
            // delete it
            remove(datpathbuffer);
          return;
        }
        unsigned long datentries = getuintmsb(datfilebuffer+0xC0);
        unsigned long lastknownwave_offset = 0, pfientries_offset = 0, videoentries_offset = 0,
                      tsav_offset = 0, ap25mediaids_offset = 0;
        if (debug) printf("%s:%slatestversion = 0x%06lX, datentries = %lu%s", datpathbuffer, newline, latestversion, datentries, newline);
        if (datentries) {
            for (m=0;m<datentries;m++) {
                if (0xC8+(m*8)+8 >= datfilesize) break;
                if (getuintmsb(datfilebuffer+0xC8+(m*8)) == 0x004C4B57) lastknownwave_offset = getuintmsb(datfilebuffer+0xCC+(m*8));
                if (getuintmsb(datfilebuffer+0xC8+(m*8)) == 0x00504649) pfientries_offset =    getuintmsb(datfilebuffer+0xCC+(m*8));
                if (getuintmsb(datfilebuffer+0xC8+(m*8)) == 0x00564944) videoentries_offset =  getuintmsb(datfilebuffer+0xCC+(m*8));
                if (getuintmsb(datfilebuffer+0xC8+(m*8)) == 0x54534156) tsav_offset =          getuintmsb(datfilebuffer+0xCC+(m*8));
                if (getuintmsb(datfilebuffer+0xC8+(m*8)) == 0x41504D49) ap25mediaids_offset =  getuintmsb(datfilebuffer+0xCC+(m*8));
            }
        }
        if (lastknownwave_offset && (lastknownwave_offset + 8) <= datfilesize) {
            lastknownwave = getuint64(datfilebuffer+lastknownwave_offset);
            if (debug) {
                printf("lastknownwave = 0x%016"LL"X = ", lastknownwave);
                printwin32filetime(lastknownwave);
                printf("%s", newline);
            }
        }
        if (tsav_offset && (tsav_offset + 4) <= datfilesize) {
            total_sectors_available_for_video_data = getuint(datfilebuffer+tsav_offset);
            if (debug) printf("total_sectors_available_for_video_data = %lu%s", total_sectors_available_for_video_data, newline);
        }
        if (ap25mediaids_offset && (ap25mediaids_offset + 4) <= datfilesize) {
            num_ap25mediaids = getuintmsb(datfilebuffer+ap25mediaids_offset);
            datfileap25mediaids = calloc(num_ap25mediaids, sizeof(struct mediaidshort));
            if (datfileap25mediaids == NULL) {
                if (debug) {
                    color(red);
                    printf("ERROR: Memory allocation for datfileap25mediaids failed!%s", newline);
                    color(normal);
                }    
                num_ap25mediaids = NUM_CURRENTAP25MEDIAIDS;
            }
            else {
                n = ap25mediaids_offset + 4;
                for (m=0;m<num_ap25mediaids;m++) {
                    if (n + 4 > datfilesize) {
                        num_ap25mediaids = NUM_CURRENTAP25MEDIAIDS;
                      return;
                    }
                    for (i=0;i<4;i++) datfileap25mediaids[m].mediaid[i] = datfilebuffer[n+i];
                    if (debug) {
                        printf("datfileap25mediaids[%lu].mediaid = ", m);
                        for (i=0;i<4;i++) printf("%02X", datfileap25mediaids[m].mediaid[i]);
                        printf("%s", newline);
                    }
                    n += 4;
                }
                mostrecentap25mediaids = datfileap25mediaids;
            }
        }
        if (pfientries_offset && (pfientries_offset + 4) <= datfilesize) {
            num_pfientries = getuintmsb(datfilebuffer+pfientries_offset);
            datfilepfientries = calloc(num_pfientries, sizeof(struct waveentry));
            if (datfilepfientries == NULL) {
                if (debug) {
                    color(red);
                    printf("ERROR: Memory allocation for datfilepfientries failed!%s", newline);
                    color(normal);
                }    
                num_pfientries = NUM_CURRENTPFIENTRIES;
            }
            else {
                n = pfientries_offset + 4;
                for (m=0;m<num_pfientries;m++) {
                    if (n + 27 > datfilesize) {
                        num_pfientries = NUM_CURRENTPFIENTRIES;
                      return;
                    }
                    datfilepfientries[m].hosted = true;  // pfi will always be hosted
                    datfilepfientries[m].crc = getuintmsb(datfilebuffer+n+1);
                    for (i=0;i<20;i++) datfilepfientries[m].sha1[i] = datfilebuffer[n+5+i];
                    u = getwordmsb(datfilebuffer+n+25);
                    if (n + 27 + u > datfilesize) {
                        num_pfientries = NUM_CURRENTPFIENTRIES;
                      return;
                    }
                    if ( (s = (char *) calloc(u+1, sizeof(char))) == NULL ) {
                        if (debug) {
                            color(red);
                            printf("ERROR: Memory allocation for datfilepfientries[%lu].description failed!%s", m, newline);
                            color(normal);
                        }
                        num_pfientries = NUM_CURRENTPFIENTRIES;
                      return;
                    }
                    memcpy(s, datfilebuffer+n+27, u);
                    datfilepfientries[m].description = s;
                    if (debug) {
                        printf("datfilepfientries[%lu].hosted %s = %s%s", m, sp4, datfilepfientries[m].hosted ? "true" : "false", newline);
                        printf("datfilepfientries[%lu].crc %s = %08lX%s", m, sp7, datfilepfientries[m].crc, newline);
                        printf("datfilepfientries[%lu].sha1 %s = ", m, sp6);
                        for (i=0;i<20;i++) printf("%02X", datfilepfientries[m].sha1[i]);
                        printf("%sdatfilepfientries[%lu].description = %s%s%s", newline, m, datfilepfientries[m].description, newline, newline);
                    }
                    n += 27 + u;
                }
                mostrecentpfientries = datfilepfientries;
            }
        }
        if (videoentries_offset && (videoentries_offset + 4) <= datfilesize) {
            num_videoentries = getuintmsb(datfilebuffer+videoentries_offset);
            datfilevideoentries = calloc(num_videoentries, sizeof(struct waveentry));
            if (datfilevideoentries == NULL) {
                if (debug) {
                    color(red);
                    printf("ERROR: Memory allocation for datfilevideoentries failed!%s", newline);
                    color(normal);
                }    
                num_videoentries = NUM_CURRENTVIDEOENTRIES;
            }
            else {
                n = videoentries_offset + 4;
                for (m=0;m<num_videoentries;m++) {
                    if (n + 27 > datfilesize) {
                        num_videoentries = NUM_CURRENTVIDEOENTRIES;
                      return;
                    }
                    if (datfilebuffer[n] & 0x01) datfilevideoentries[m].hosted = true;
                    else datfilevideoentries[m].hosted = false;
                    datfilevideoentries[m].crc = getuintmsb(datfilebuffer+n+1);
                    for (i=0;i<20;i++) datfilevideoentries[m].sha1[i] = datfilebuffer[n+5+i];
                    u = getwordmsb(datfilebuffer+n+25);
                    if (n + 27 + u > datfilesize) {
                        num_videoentries = NUM_CURRENTVIDEOENTRIES;
                      return;
                    }
                    if ( (s = (char *) calloc(u+1, sizeof(char))) == NULL ) {
                        if (debug) {
                            color(red);
                            printf("ERROR: Memory allocation for datfilevideoentries[%lu].description failed!%s", m, newline);
                            color(normal);
                        }
                        num_videoentries = NUM_CURRENTVIDEOENTRIES;
                      return;
                    }
                    memcpy(s, datfilebuffer+n+27, u);
                    datfilevideoentries[m].description = s;
                    if (debug) {
                        printf("datfilevideoentries[%lu].hosted %s = %s%s", m, sp4, datfilevideoentries[m].hosted ? "true" : "false", newline);
                        printf("datfilevideoentries[%lu].crc %s = %08lX%s", m, sp7, datfilevideoentries[m].crc, newline);
                        printf("datfilevideoentries[%lu].sha1 %s = ", m, sp6);
                        for (i=0;i<20;i++) printf("%02X", datfilevideoentries[m].sha1[i]);
                        printf("%sdatfilevideoentries[%lu].description = %s%s%s", newline, m, datfilevideoentries[m].description, newline, newline);
                    }
                    n += 27 + u;
                }
                mostrecentvideoentries = datfilevideoentries;
            }
        }
        
    }
  return;
}

void getinivalue(char *buffer, int bufferstart, char *nameofvalue, int value, char *defaultvalue) {
    int i, a, b;
    if (debug) {
        printf("getinivalue - %s - buffer: %s", nameofvalue, buffer);
        if (html) printf("<br>");
        printf("getinivalue - %s - bufferstart: %d%s", nameofvalue, bufferstart, newline);
        printf("getinivalue - %s - value: %d%s", nameofvalue, value, newline);
        printf("getinivalue - %s - defaultvalue: %s%s", nameofvalue, defaultvalue, newline);
    }
    a = 0;
    for (i=bufferstart;i<bufferstart+21;i++) {  // allow up to 20 spaces after the colon
        if (buffer[i] != 0x20) {
            a = i;
            break;
        }
    }
    if (a) {
        b = 0;
        for (i=a;i<2047;i++) {
            if (buffer[i] == 0x0D || buffer[i] == 0x0A) {
                b = i;
                break;
            }
        }
        if (b) {
            if (debug) printf("getinivalue - %s - a = %d, b = %d, b - a = %d%s", nameofvalue, a, b, b - a, newline);
            if (value == WEB_INIDIR) {
                webinidir = calloc(b - a + 1, sizeof(char));
                if (webinidir == NULL) {
                    webinidir = defaultvalue;
                    if (debug) printf("getinivalue - %s - calloc failed, %s: %s%s", nameofvalue, nameofvalue, webinidir, newline);
                }
                else {
                    strncpy(webinidir, buffer+a, b - a);
                    if (debug) printf("getinivalue - %s - %s: %s, strlen(%s) = %u%s%s", nameofvalue, nameofvalue, webinidir, nameofvalue, (unsigned int) strlen(webinidir), newline, newline);
                }
            }
            else if (value == WEB_CSV) {
                webcsv = calloc(b - a + 1, sizeof(char));
                if (webcsv == NULL) {
                    webcsv = defaultvalue;
                    if (debug) printf("getinivalue - %s - calloc failed, %s: %s%s", nameofvalue, nameofvalue, webcsv, newline);
                }
                else {
                    strncpy(webcsv, buffer+a, b - a);
                    if (debug) printf("getinivalue - %s - %s: %s, strlen(%s) = %u%s%s", nameofvalue, nameofvalue, webcsv, nameofvalue, (unsigned int) strlen(webcsv), newline, newline);
                }
            }
            else if (value == WEB_DAT) {
                webdat = calloc(b - a + 1, sizeof(char));
                if (webdat == NULL) {
                    webdat = defaultvalue;
                    if (debug) printf("getinivalue - %s - calloc failed, %s: %s%s", nameofvalue, nameofvalue, webdat, newline);
                }
                else {
                    strncpy(webdat, buffer+a, b - a);
                    if (debug) printf("getinivalue - %s - %s: %s, strlen(%s) = %u%s%s", nameofvalue, nameofvalue, webdat, nameofvalue, (unsigned int) strlen(webdat), newline, newline);
                }
            }
            else if (value == WEB_STEALTHDIR) {
                webstealthdir = calloc(b - a + 1, sizeof(char));
                if (webstealthdir == NULL) {
                    webstealthdir = defaultvalue;
                    if (debug) printf("getinivalue - %s - calloc failed, %s: %s%s", nameofvalue, nameofvalue, webstealthdir, newline);
                }
                else {
                    strncpy(webstealthdir, buffer+a, b - a);
                    if (debug) printf("getinivalue - %s - %s: %s, strlen(%s) = %u%s%s", nameofvalue, nameofvalue, webstealthdir, nameofvalue, (unsigned int) strlen(webstealthdir), newline, newline);
                }
            }
            else if (value == WEB_AUTOUPLOAD) {
                autouploadwebaddress = calloc(b - a + 1, sizeof(char));
                if (autouploadwebaddress == NULL) {
                    autouploadwebaddress = defaultvalue;
                    if (debug) printf("getinivalue - %s - calloc failed, %s: %s%s", nameofvalue, nameofvalue, autouploadwebaddress, newline);
                }
                else {
                    strncpy(autouploadwebaddress, buffer+a, b - a);
                    if (debug) printf("getinivalue - %s - %s: %s, strlen(%s) = %u%s%s", nameofvalue, nameofvalue, autouploadwebaddress, nameofvalue, (unsigned int) strlen(autouploadwebaddress), newline, newline);
                }
            }
            else if (value == WEB_UNVERIFIEDINIDIR) {
                webunverifiedinidir = calloc(b - a + 1, sizeof(char));
                if (webunverifiedinidir == NULL) {
                    webunverifiedinidir = defaultvalue;
                    if (debug) printf("getinivalue - %s - calloc failed, %s: %s%s", nameofvalue, nameofvalue, webunverifiedinidir, newline);
                }
                else {
                    strncpy(webunverifiedinidir, buffer+a, b - a);
                    if (debug) printf("getinivalue - %s - %s: %s, strlen(%s) = %u%s%s", nameofvalue, nameofvalue, webunverifiedinidir, nameofvalue, (unsigned int) strlen(webunverifiedinidir), newline, newline);
                }
            }
        }
    }
  return;
}

void checkini() {
    memset(buffer, 0, 2048);
    if (!homeless) { strcat(buffer, homedir); strcat(buffer, abgxdir); }
    strcat(buffer, "abgx360.ini");
    FILE *inifile = fopen(buffer, "rb");
    if (inifile != NULL) {
        unsigned long m = 0;
        memset(buffer, 0, 2048);
        while (fgets(buffer, 2048, inifile) != NULL && m < 2000) {  // 2000 lines limit
            if (memcmp(buffer, "web_inidir:", 11) == 0) {
                getinivalue(buffer, 11, "webinidir", WEB_INIDIR, "http://abgx360.net/Apps/verified/");
            }
            else if (memcmp(buffer, "web_stealthdir:", 15) == 0) {
                getinivalue(buffer, 15, "webstealthdir", WEB_STEALTHDIR, "http://abgx360.net/Apps/StealthFiles/");
            }
            else if (memcmp(buffer, "web_csv:", 8) == 0) {
                getinivalue(buffer, 8, "webcsv", WEB_CSV, "http://abgx360.net/Apps/Stealth360/GameNameLookup.csv");
            }
            else if (memcmp(buffer, "web_dat:", 8) == 0) {
                getinivalue(buffer, 8, "webdat", WEB_DAT, "http://abgx360.net/Apps/Stealth360/abgx360.dat");
            }
            else if (memcmp(buffer, "web_autoupload:", 15) == 0) {
                getinivalue(buffer, 15, "autouploadwebaddress", WEB_AUTOUPLOAD, "http://abgx360.net/Apps/Control/AutoUpload.php");
            }
            else if (memcmp(buffer, "web_unverifiedinidir:", 21) == 0) {
                getinivalue(buffer, 21, "webunverifiedinidir", WEB_UNVERIFIEDINIDIR, "http://abgx360.net/Apps/unverified/");
            }
            memset(buffer, 0, 2048);
            m++;
        }
    }
  return;
}

void checkcsv(unsigned char* mediaid) {
    int i, a, c;
    memset(buffer, 0, 2048);
    memset(gamename, 0, 151);
    if (!homeless) { strcat(buffer, homedir); strcat(buffer, abgxdir); }
    strcat(buffer, "GameNameLookup.csv");
    csvfile = fopen(buffer, "rb");
    if (csvfile == NULL) {
        if (debug) printf("checkcsv - failed to open %s%s%s for reading%s", quotation, buffer, quotation, newline);
      return;
    }
    // convert 16 byte media id into 32 byte ascii hex representation for comparison to csv values
    unsigned char mediaidhex[33], cmp;
    memset(mediaidhex, 0, 33);
    for (i=0; i<32; i++) {
        if (i%2 == 0) cmp =  mediaid[i/2] & 0xF0;
        else          cmp = (mediaid[i/2] & 0x0F) << 4;
        if      (cmp == 0xF0) mediaidhex[i] = 'F';
        else if (cmp == 0xE0) mediaidhex[i] = 'E';
        else if (cmp == 0xD0) mediaidhex[i] = 'D';
        else if (cmp == 0xC0) mediaidhex[i] = 'C';
        else if (cmp == 0xB0) mediaidhex[i] = 'B';
        else if (cmp == 0xA0) mediaidhex[i] = 'A';
        else if (cmp == 0x90) mediaidhex[i] = '9';
        else if (cmp == 0x80) mediaidhex[i] = '8';
        else if (cmp == 0x70) mediaidhex[i] = '7';
        else if (cmp == 0x60) mediaidhex[i] = '6';
        else if (cmp == 0x50) mediaidhex[i] = '5';
        else if (cmp == 0x40) mediaidhex[i] = '4';
        else if (cmp == 0x30) mediaidhex[i] = '3';
        else if (cmp == 0x20) mediaidhex[i] = '2';
        else if (cmp == 0x10) mediaidhex[i] = '1';
        else                  mediaidhex[i] = '0';
    }
    // check each line of the csv for a matching media id and print the name of the game if found
    char line[2048];
    memset(line, 0, 2048);
    while (fgets(line, 2048, csvfile) != NULL) {
        for (i=0;i<2016;i++) {
            if (line[i] == 0x2C && memcmp(mediaidhex, line+i+1, 32) == 0) {    // 0x2C = comma
                if (debug) {
                    printf("checkcsv found match in line: %s%s", line, newline);
                    printf("comma at %d%s", i, newline);
                    printf("mediaid: %s ", sp2);
                    for (c=0;c<16;c++) printf("%02X", mediaid[c]);
                    printf("%s", newline);
                    printf("mediaidhex: %s%s", mediaidhex, newline);
                }
                a = 0;
                if (verbose) printf("%s", newline);
                color(white);
                while (line[a] != 0x2C && a < 79) {
                    printf("%c", line[a]);
                    gamename[a] = line[a];
                    a++;
                }
                foundgamename = true;
                printf("%s", newline); color(normal);
                if (!verbose || checkssbin || justastealthfile) printf("%s", newline);
                fclose(csvfile);
              return;
            }
        }
        memset(line, 0, 2048);
    }
    fclose(csvfile);
  return;
}

void printwin32filetime(unsigned long long win32filetime) {
    int month, leap, year = 1601;
    unsigned long long seconds = win32filetime / 10000000;
    unsigned long long minutes = seconds / 60;
    unsigned long long hours = minutes / 60;
    long days = (long) (hours / 24);
    while (days > 364) { 
      if (year % 400 == 0) days -= 366;
      else if (year % 100 == 0) days -= 365;
      else if (year % 4 == 0) days -= 366;
      else days -= 365;
      year++;
    }
    // days == -1 means the while loop executed during a leap year with days=365
    if (days == -1) printf("%u/12/31 %02"LL"u:%02"LL"u:%02"LL"u", year-1, hours % 24, minutes % 60, seconds % 60);
    else {
        // days++ makes it more natural (days=1 is now Jan 1 and Feb 1 is days=32)
        days++;
        // check if year is a leap year
        if (year % 400 == 0) leap = 1;
        else if (year % 100 == 0) leap = 0;
        else if (year % 4 == 0) leap = 1;
        else leap = 0;
        // find out what month it is and subtract days from the previous months
        if      (days > 334+leap) {days -= 334+leap; month=12;}
        else if (days > 304+leap) {days -= 304+leap; month=11;}
        else if (days > 273+leap) {days -= 273+leap; month=10;}
        else if (days > 243+leap) {days -= 243+leap; month=9;}
        else if (days > 212+leap) {days -= 212+leap; month=8;}
        else if (days > 181+leap) {days -= 181+leap; month=7;}
        else if (days > 151+leap) {days -= 151+leap; month=6;}
        else if (days > 120+leap) {days -= 120+leap; month=5;}
        else if (days > 90+leap)  {days -= 90+leap;  month=4;}
        else if (days > 59+leap)  {days -= 59+leap;  month=3;}
        else if (days > 31)       {days -= 31;       month=2;}
        else month=1;
        printf("%u/%02i/%02lu %02"LL"u:%02"LL"u:%02"LL"u", year, month, days, hours % 24, minutes % 60, seconds % 60);
    }
  return;
}

/*	month		days		start	(leap)
1 	January 	31          1
2 	February 	28 or 29    32
3 	March 		31          60      61
4 	April 		30          91      92
5 	May 		31          121     122
6 	June 		30          152     153
7 	July 		31          182     183
8 	August 		31          213     214
9 	September 	30          244     245
10 	October 	31          274     275
11 	November 	30          305     306
12 	December 	31          335     336
*/

void printunixfiletime(unsigned long seconds) {
    int month, leap, year = 1970;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    long days = (long) (hours / 24);
    while (days > 364) { 
      if (year % 400 == 0) days -= 366;
      else if (year % 100 == 0) days -= 365;
      else if (year % 4 == 0) days -= 366;
      else days -= 365;
      year++;
    }

    // days == -1 means the while loop executed during a leap year with days=365
    if (days == -1) printf("%u/12/31 %02lu:%02lu:%02lu", year-1, hours % 24, minutes % 60, seconds % 60);
    else {
        // days++ makes it more natural (days=1 is now Jan 1 and Feb 1 is days=32)
        days++;
        // check if year is a leap year
        if (year % 400 == 0) leap = 1;
        else if (year % 100 == 0) leap = 0;
        else if (year % 4 == 0) leap = 1;
        else leap = 0;
        // find out what month it is and subtract days from the previous months
        if      (days > 334+leap) {days -= 334+leap; month=12;}
        else if (days > 304+leap) {days -= 304+leap; month=11;}
        else if (days > 273+leap) {days -= 273+leap; month=10;}
        else if (days > 243+leap) {days -= 243+leap; month=9;}
        else if (days > 212+leap) {days -= 212+leap; month=8;}
        else if (days > 181+leap) {days -= 181+leap; month=7;}
        else if (days > 151+leap) {days -= 151+leap; month=6;}
        else if (days > 120+leap) {days -= 120+leap; month=5;}
        else if (days > 90+leap)  {days -= 90+leap;  month=4;}
        else if (days > 59+leap)  {days -= 59+leap;  month=3;}
        else if (days > 31)       {days -= 31;       month=2;}
        else month=1;
        printf("%u/%02i/%02lu %02lu:%02lu:%02lu", year, month, days, hours % 24, minutes % 60, seconds % 60);
    }
  return;
}

void printwtfhexcolor() {
    color(wtfhexcolor);
    wtfhex = true;
  return;
}

void printwtfcharcolor() {
    color(wtfcharcolor);
    wtfhex = true;
  return;
}

// prints the specified number of bytes in hex and ascii
void hexdump(unsigned char* ptr, int stealthtype, int bytes) {
    int a, b, c;
    unsigned char thisline[16], lastline[16];
    bool pfihex = false, dmihex = false, foundusbcorruption = false;
    if (stealthtype == PFI_HEX && bytes == 2048) pfihex = true;
    else if (stealthtype == DMI_HEX && bytes == 2048) {
        dmihex = true;
        if (memcmp(ptr+0x7FC, "\xFF\xFF\xFF\xFF", 4) == 0) foundusbcorruption = true;
    }
    // print header
    color(hexoffsetcolor);
    printf("%s%sOFFSET%s 0%s1%s2%s3%s4%s5%s6%s7%s8%s9%sA%sB%sC%sD%sE%sF",
            newline, sp5, sp2, sp2, sp2, sp2, sp2, sp2, sp2, sp2, sp2, sp2, sp2, sp2, sp2, sp2, sp2, sp2);
    color(normal);
    int rows = bytes / 16;
    int rowremainder = bytes % 16;
    for(a=0; a<rows; a++) {
        // store this line so we can compare it to the last line (and print a "*" instead of a bunch of duplicate lines)
        memcpy(thisline, ptr+a*16, 16);
        // if this isn't the first loop, check to see if this line matches the last line
        if ((a > 0) && (memcmp(thisline, lastline, 16) == 0)) {
            // it does, but don't print another "*" if we just did
            if (!matchedbefore) {
                color(hexoffsetcolor); printf("%s%s*", newline, sp5); color(normal);
                matchedbefore = true;
            }
            continue;
        }
        else {
            // not a duplicate line
            matchedbefore = false;
            // start a new row and print the offset (in hex)
            color(hexoffsetcolor);
            printf("%s%s0x%04X%s", newline, sp5, a*16, sp2);
            color(normal);
            // print the next 16 characters in hex
            for(b=0; b<16; b++) {
                c = a*16+b;
                if (pfihex) {
                    // should have zeros at 0x4, 0xC, 0x6 - 0x8
                    if (ptr[c] != 0x00 && (c == 0x4 || c == 0xC || (c >= 0x6 && c <= 0x8))) printwtfhexcolor();
                    // should have 0x03 at 0x5
                    if (c == 5 && ptr[c] != 0x03) printwtfhexcolor();
                    // should have zeros from 0x11 - 0x7FF
                    if (ptr[c] != 0x00 && (c >= 0x11 && c <= 0x7FF)) printwtfhexcolor();
                }
                else if (dmihex) {
                    // should have zeros from 0x50 to 0x633
                    if (ptr[c] != 0 && (c <= 0x633 && c >= 0x50)) printwtfhexcolor();
                    // should have "XBOX" at 0x7E8
                    if (c == 0x7E8 && ptr[c] != 'X') printwtfhexcolor();
                    if (c == 0x7E9 && ptr[c] != 'B') printwtfhexcolor();
                    if (c == 0x7EA && ptr[c] != 'O') printwtfhexcolor();
                    if (c == 0x7EB && ptr[c] != 'X') printwtfhexcolor();
                    // sata->usb related corruption
                    if (foundusbcorruption && c >= 0x7FC) printwtfhexcolor();
                }
                printf("%02X ", ptr[c]);
                if (wtfhex) {
                    color(normal);
                    wtfhex = false;
                }
            }  
            // print them again in ascii
            printf("%s", sp1);
            for(b=0; b<16; b++) {
                c = a*16+b;
                if (pfihex) {
                    // should have zeros at 0x4, 0xC, 0x6 - 0x8
                    if (ptr[c] != 0x00 && (c == 0x4 || c == 0xC || (c >= 0x6 && c <= 0x8))) printwtfcharcolor();
                    // should have 0x03 at 0x5
                    if (c == 5 && ptr[c] != 0x03) printwtfcharcolor();
                    // should have zeros from 0x11 - 0x7FF
                    if (ptr[c] != 0x00 && (c >= 0x11 && c <= 0x7FF)) printwtfcharcolor();
                }
                else if (dmihex) {
                    // should have zeros from 0x50 to 0x633
                    if (ptr[c] != 0 && (c <= 0x633 && c >= 0x50)) printwtfcharcolor();
                    // should have "XBOX" at 0x7E8
                    if (c == 0x7E8 && ptr[c] != 'X') printwtfcharcolor();
                    if (c == 0x7E9 && ptr[c] != 'B') printwtfcharcolor();
                    if (c == 0x7EA && ptr[c] != 'O') printwtfcharcolor();
                    if (c == 0x7EB && ptr[c] != 'X') printwtfcharcolor();
                    // sata->usb related corruption
                    if (foundusbcorruption && c >= 0x7FC) printwtfcharcolor();
                }
                // print zeros as dots
                if(ptr[c] == 0) printf (".");
                // don't print control codes! (print a space instead)
                else if (ptr[c] < 0x20 || (ptr[c] > 0x7E && ptr[c] < 0xA1)) printf("%s", sp1);
                else {
                    if (html) printf("&#%d;", ptr[c]);
                    else printf("%c", ptr[c]);
                }
                if (wtfhex) {
                    color(normal);
                    wtfhex = false;
                }
            }
        }
        memcpy(lastline, thisline, 16);
    }
    if (rowremainder) {
        // start a new row and print the offset (in hex)
        color(hexoffsetcolor); printf("%s%s0x%04X%s", newline, sp5, rows*16, sp2); color(normal);
        // print the last remaining characters in hex
        for(b=0; b<rowremainder; b++) {
            c = rows*16+b;
            printf("%02X ", ptr[c]);
        }
        // pad with spaces up to ascii area
        for (b=0; b<(16-rowremainder); b++) {
            printf("%s", sp3);
        }
        // print remaining characters again in ascii
        printf("%s", sp1);
        for(b=0; b<rowremainder; b++) {
            c = rows*16+b;
            // print zeros as dots
            if(ptr[c] == 0) printf (".");
            // don't print control codes! (print a space instead)
            else if ((ptr[c] < 0x20) || ((ptr[c] > 0x7E) && (ptr[c] < 0xA1))) printf("%s", sp1);
            else {
                if (html) printf("&#%d;", ptr[c]);
                else printf("%c", ptr[c]);
            }
        }
    }
    printf("%s", newline);
  return;
}

char *lzxstrerror(int errnum) {
    switch(errnum) {
        case MSPACK_ERR_OK:         return "no error";
        case MSPACK_ERR_ARGS:       return "bad arguments to method";
        case MSPACK_ERR_OPEN:       return "error opening file";
        case MSPACK_ERR_READ:       return "error reading file";
        case MSPACK_ERR_WRITE:      return "error writing file";
        case MSPACK_ERR_SEEK:       return "seek error";
        case MSPACK_ERR_NOMEMORY:   return "out of memory";
        case MSPACK_ERR_SIGNATURE:  return "bad \"magic id\" in file";
        case MSPACK_ERR_DATAFORMAT: return "bad or corrupt file format";
        case MSPACK_ERR_CHECKSUM:   return "bad checksum or CRC";
        case MSPACK_ERR_CRUNCH:     return "error during compression";
        case MSPACK_ERR_DECRUNCH:   return "error during decompression";
        default:                    return "unknown error";
    }
}

int checkgame() {
    unsigned long m, rootsize, defaultxexsize = 0;
    unsigned long long rootsector, rootaddress, defaultxexaddress, defaultxexsector = 0;
    if (verbose) printf("Checking Game%s", newline);
    // seek to sector 32
    if (fseeko(fp, 32*2048+video, SEEK_SET) != 0) {
        printseekerror(isofilename, "Checking sector 32");
      return 1;
    }
    memset(ubuffer, 0, 2048);
    initcheckread();
    if (checkreadandprinterrors(ubuffer, 1, 2048, fp, 0, 32*2048+video, isofilename, "checking sector 32") != 0) return 1;
    donecheckread(isofilename);
    rootsector = getuint(ubuffer+20);
    rootaddress = rootsector*2048+video;
    rootsize = getuint(ubuffer+24);
    if (verbose || L0capacity != -1) {
        if (verbose) printf("%sISO: %s%s%s%s", sp5, quotation, isofilename, quotation, newline);
        if (L0capacity != -1) {
            if (L0capacity == 0) {
                color(yellow);
                if (verbose) printf("%s", sp5);
                printf("Layerbreak: Unknown%s", newline);
            }
            else {
                if (L0capacity == 1913760) color(green);
                else color(red);
                if (verbose) printf("%s", sp5);
                printf("Layerbreak: %"LL"d%s", L0capacity, newline);
            }
            color(normal);
        }
        if (verbose) printf("%sSize: %"LL"d bytes", sp5, fpfilesize);
    }
    if (video == 0) {  // just a game partition
        if (fpfilesize < 7307001856LL) {
            color(red);
            if (verbose) printf(" (filesize too small!)%s", newline);
            printf("Warning: This ISO is short %"LL"d bytes! Part of the game data is missing!%s"
                   "%s(should have been exactly %"LL"d bytes)%s",
                   7307001856LL - fpfilesize, newline, sp9, 7307001856LL, newline);
            color(normal);
            isotoosmall = true;
        }
        else if (verbose) printf("%s", newline);
    }
    else if (fpfilesize < 7307001856LL + (long long) video) {
        color(red);
        if (verbose) printf(" (filesize too small!)%s", newline);
        printf("Warning: This ISO is short %"LL"d bytes! Part of the game data is missing!%s"
               "%s(should have been at least %"LL"d bytes)%s",
               7307001856LL + video - fpfilesize, newline, sp9, 7307001856LL + video, newline);
        color(normal);
        isotoosmall = true;
    }
    else if (verbose) printf("%s", newline);
    else if (isotoosmall) printf("%s", newline);
    if (extraverbose) {
        printf("%sGame partition offset: 0x%"LL"X%s", sp5, video, newline);
        printf("%sRoot sector: %"LL"u (0x%"LL"X), %lu bytes%s", sp5, rootsector, rootaddress, rootsize, newline);
    }
    // seek to the rootsector
    if (fseeko(fp, rootaddress, SEEK_SET) != 0) {
        printseekerror(isofilename, "Checking the root sector");
      return 1;
    }
    // create an array for the rootsector with the correct size (max 5000 sectors)
    if (rootsize > 10240000) {
        rootsize = 10240000;
        color(yellow);
        printf("Warning: The root directory appears to be extremely large!%s"
               "(Only the first 5000 sectors will be checked for default.xex)%s", newline, newline);
        color(normal);
    }
    if (rootsize < 13) {
        color(red);
        printf("ERROR: The root directory is too small!%s", newline);
        color(normal);
      return 1;
    }
    char *rootbuffer = malloc(rootsize * sizeof(char));
    if (rootbuffer == NULL) {
        color(red);
        printf("ERROR: Memory allocation for rootbuffer failed! Game over man... Game over!%s", newline);
        color(normal);
      exit(1);
    }
    initcheckread();
    if (checkreadandprinterrors(rootbuffer, 1, rootsize, fp, 0, rootaddress, isofilename, "checking the root sector") != 0) {
        free(rootbuffer);
      return 1;
    }
    donecheckread(isofilename);
    if (debug) {
        printf("1st sector of rootsector:%s", newline);
        hexdump((unsigned char*) rootbuffer, 0, 2048);
    }
    if (getzeros((unsigned char*) rootbuffer, 0, rootsize - 1) == rootsize) {
        color(red);
        printf("ERROR: The root sector is blank!%s", newline);
        color(normal);
        free(rootbuffer);
      return 1;
    }
    // case insensitive search for "default.xex" (0x80 tells us it's a file and 0x0B = 11 chars in filename)
    char defaultxex[13] = {0x80,0x0B,'d','e','f','a','u','l','t','.','x','e','x'};
    for (m=0; m<rootsize-12; m++) {
        if (strncasecmp(rootbuffer+m, defaultxex, 13) == 0) {
            defaultxexsector = getint(rootbuffer+m-8);
            defaultxexsize = getint(rootbuffer+m-4);
          break;
        }
    }
    free(rootbuffer);
    if (defaultxexsector == 0) {
        // default.xex was not found in our search
        color(red);
        printf("ERROR: default.xex was not found in the root sector of this game!%s", newline);
        color(normal);
      return 1;
    }
    defaultxexaddress = defaultxexsector * 2048 + video;
    if (extraverbose) printf("%sdefault.xex sector: %"LL"u (0x%"LL"X), %lu bytes%s",
                              sp5, defaultxexsector, defaultxexaddress, defaultxexsize, newline);
    if (!dontparsefs) {
        if (converttosectors(rootsize) > MAX_DIR_SECTORS) {
            if (verbose) printf("%s", sp5);
            color(yellow);
            printf("Unable to parse filesystem because the root directory is %ld sectors long!%s"
                   "Unable to check random padding!%s",
                    converttosectors(rootsize), newline, newline);
            if (debug) printf("MAX_DIR_SECTORS = %d%s", MAX_DIR_SECTORS, newline);
            color(normal);
          goto skipparsingfs;
        }
        bufferlevels = MIN_DIR_LEVELS;
        buffermaxdir = converttosectors(rootsize) > MIN_DIR_SECTORS ? converttosectors(rootsize) : MIN_DIR_SECTORS;
        fsbuffer = (char *) calloc(bufferlevels*buffermaxdir*2048, sizeof(char));
        if (fsbuffer == NULL) {
            color(red);
            printf("ERROR: Memory allocation for fsbuffer failed! Game over man... Game over!%s", newline);
            color(normal);
          exit(1);
        }
        memset(dirprefix, 0, 2048);
        // read rootsector
        if (readblock(isofilename, "Parsing filesystem and checking random padding", fp, rootsector, (unsigned char*) fsbuffer, converttosectors(rootsize))) {
            free(fsbuffer);
          goto skipparsingfs;
        }
        while (1) {
            // see if filesystem can be parsed with current buffer size or if it needs to be increased (don't display files)
            if (parse(isofilename, "Parsing filesystem and checking random padding", 0, 0, 0, fp, true, false, dirprefix) == 2) {
                if (debugfs) {
                    color(yellow);
                    printf("parse returned 2: buffermaxdir=%d, bufferlevels=%d%s", buffermaxdir, bufferlevels, newline);
                    color(normal);
                }
                if (buffermaxdir > MAX_DIR_SECTORS) {
                    parsingfsfailed = true;
                    if (verbose) printf("%s", sp5);
                    color(yellow);
                    printf("Unable to parse filesystem because a directory is %d sectors long!%s"
                           "Unable to check random padding!%s", buffermaxdir, newline, newline);
                    if (debug) printf("MAX_DIR_SECTORS = %d%s", MAX_DIR_SECTORS, newline);
                    color(normal);
                  break;
                }
                if (bufferlevels >= MAX_DIR_LEVELS) {
                    parsingfsfailed = true;
                    if (verbose) printf("%s", sp5);
                    color(yellow);
                    printf("Unable to parse filesystem because a directory is %d levels deep!%s"
                           "Unable to check random padding!%s", bufferlevels, newline, newline);
                    if (debug) printf("MAX_DIR_LEVELS = %d%s", MAX_DIR_LEVELS, newline);
                    color(normal);
                  break;
                }
                fsbuffer = realloc(fsbuffer, bufferlevels*buffermaxdir*2048*sizeof(char));
                if (fsbuffer == NULL) {
                    color(red);
                    printf("ERROR: Memory reallocation for fsbuffer failed! Game over man... Game over!%s", newline);
                    color(normal);
                  exit(1);
                }
                totalfiles = 0;
                totaldirectories = 0;
                totalbytes = 0;
                memset(fsbuffer, 0, bufferlevels*buffermaxdir*2048);
                memset(dirprefix, 0, 2048);
                // read rootsector again
                if (readblock(isofilename, "Parsing filesystem and checking random padding", fp, rootsector,
                              (unsigned char*) fsbuffer, converttosectors(rootsize))) {
                    free(fsbuffer);
                    goto skipparsingfs;
                }
            }
            else break;
        }
        if (showfiles && !parsingfsfailed) {
            totalfiles = 0;
            totaldirectories = 0;
            totalbytes = 0;
            color(blue);
            printf("%sDisplaying Filesystem%s", sp5, newline);
            printf("%sSector %s Bytes %s Filename%s", sp5, sp7, sp1, newline);
            printf("%s--------------------------------------------------------------------------%s", sp5, newline);
            // display files
            parse(isofilename, "Parsing filesystem", 0, 0, 0, fp, false, false, dirprefix);
            printf("%s--------------------------------------------------------------------------%s", sp5, newline);
            color(normal);
        }
        if (verbose && !parsingfsfailed) {
            printf("%sFiles in ISO: %lu, Folders in ISO: %lu%s", sp5, totalfiles, totaldirectories, newline);
            printf("%sTotal bytes used: %"LL"u (%.2f%%)%s", sp5, totalbytes, (float) totalbytes/72986092, newline);
        }
        if (!parsingfsfailed) {
            // check for random padding
            bool randompadding = false;
            int sectorschecked = 0;
            unsigned long holecount = 0L;
            // struct filesys { unsigned long datasector, datalength; } *filesystem, *holes;
            filesystem = calloc(totalfiles + totaldirectories + 4, sizeof(struct filesys));
            if (filesystem != NULL) {
                totalfiles = 0;
                totaldirectories = 0;
                totalbytes = 0;
                memset(fsbuffer, 0, bufferlevels*buffermaxdir*2048);
                memset(dirprefix, 0, 2048);
                if (readblock(isofilename, "Parsing filesystem and checking random padding", fp, rootsector,
                              (unsigned char*) fsbuffer, converttosectors(rootsize))) {
                    free(fsbuffer);
                  goto skipparsingfs;
                }
                // dont' display files but add to filesystem structure
                parse(isofilename, "Parsing filesystem and checking random padding", 0, 0, 0, fp, true, true, dirprefix);
                filesystem[totalfiles + totaldirectories].datasector = 32L;  // sector 32 has data (it tells us where the rootsector is)
                filesystem[totalfiles + totaldirectories].datalength = 2L;   // as well as sector 33
                filesystem[totalfiles + totaldirectories + 1].datasector = rootsector;  // don't forget to add the rootsector
                filesystem[totalfiles + totaldirectories + 1].datalength = converttosectors(rootsize);
                filesystem[totalfiles + totaldirectories + 2].datasector = 3567872L;  // add the very last sector that probably doesn't even exist unless image is splitvid (doesn't matter, this is just for hole calculation)
                filesystem[totalfiles + totaldirectories + 2].datalength = 0L;
                filesystem[totalfiles + totaldirectories + 3].datasector = 48L;  // sectors 48 - 4143 are reserved for some kind of data hash authentication
                filesystem[totalfiles + totaldirectories + 3].datalength = 4096L;
                if (debugfs) {
                    printf("filesystem:%s", newline);
                    for (m=0;m<totalfiles + totaldirectories + 4;m++) {
                        printf("%lu: %lu, %lu%s", m, filesystem[m].datasector, filesystem[m].datalength, newline);
                    }
                }
                // sort datasectors
                qsort(filesystem, totalfiles + totaldirectories + 4, sizeof(struct filesys), compfilesys);
                if (debugfs) {
                    printf("filesystem (sorted):%s", newline);
                    for (m=0;m<totalfiles + totaldirectories + 4;m++) {
                        printf("%lu: %lu, %lu%s", m, filesystem[m].datasector, filesystem[m].datalength, newline);
                    }
                }
                holes = calloc(totalfiles + totaldirectories + 4, sizeof(struct filesys));
                if (holes != NULL) {
                    unsigned long holesector = 0L;
                    for (m=0;m<totalfiles + totaldirectories + 4;m++) {
                        if (filesystem[m].datasector > holesector) {
                            holes[holecount].datasector = holesector;
                            holes[holecount].datalength = filesystem[m].datasector - holesector;
                            holecount++;
                        }
                        holesector = filesystem[m].datasector + filesystem[m].datalength;
                    }
                    if (debugfs) {
                        printf("holes:%s", newline);
                        for (m=0;m<holecount;m++) printf("%lu: %lu, %lu, 0x%"LL"X%s", m, holes[m].datasector, holes[m].datalength,
                                     (unsigned long long) (holes[m].datasector + holes[m].datalength - 1) * 2048 + video, newline);
                    }
                    // search the first and last sector of all holes up to a maximum of 20 holes for random padding
                    for (m=0;m<(holecount <= 20 ? holecount : 20);m++) {
                        if (fseeko(fp, (unsigned long long) holes[m].datasector * 2048 + video, SEEK_SET) != 0) {
                            printseekerror(isofilename, "Random padding check");
                          break;
                        }
                        initcheckread();
                        if (checkreadandprinterrors(ubuffer, 1, 2048, fp, 0, (unsigned long long) holes[m].datasector * 2048 + video,
                                                    isofilename, "random padding check") != 0) break;
                        donecheckread(isofilename);
                        sectorschecked++;
                        if (debugfs) {
                            printf("first hole sector (0x%"LL"X, %lu zeros):%s",
                                    (unsigned long long) holes[m].datasector * 2048 + video, getzeros(ubuffer, 0, 2047), newline);
                            hexdump(ubuffer, 0, 2048);
                        }
                        if (getzeros(ubuffer, 0, 2047) != 2048) {
                            randompadding = true;
                          break;
                        }
                        
                        if (holes[m].datalength > 1) {
                            if (fseeko(fp, (unsigned long long)
                                       (holes[m].datasector + holes[m].datalength - 1) * 2048 + video, SEEK_SET) != 0) {
                                printseekerror(isofilename, "Random padding check");
                              break;
                            }
                            initcheckread();
                            if (checkreadandprinterrors(ubuffer, 1, 2048, fp, 0, (unsigned long long)
                                                        (holes[m].datasector + holes[m].datalength - 1) * 2048 + video,
                                                        isofilename, "random padding check") != 0) break;
                            donecheckread(isofilename);
                            sectorschecked++;
                            if (debugfs) {
                                printf("last hole sector (0x%"LL"X, %lu zeros):%s", (unsigned long long)
                                        (holes[m].datasector + holes[m].datalength - 1) * 2048 + video,
                                        getzeros(ubuffer, 0, 2047), newline);
                                hexdump(ubuffer, 0, 2048);
                            }
                            if (getzeros(ubuffer, 0, 2047) != 2048) {
                                randompadding = true;
                              break;
                            }
                        }
                    }
                    free(holes);
                }
                else {
                    color(red);
                    printf("ERROR: Memory allocation for filesystem holes failed! Game over man... Game over!%s", newline);
                    color(normal);
                  exit(1);
                }
                free(filesystem);
            }
            else {
                color(red);
                printf("ERROR: Memory allocation for filesystem failed! Game over man... Game over!%s", newline);
                color(normal);
              exit(1);
            }
            if (holecount) {
                if (randompadding) {
                    color(green);
                    printf("Game appears to have random padding%s", newline);
                    color(normal);
                    if (debugfs) printf("%d sectors checked, %lu total filesystem holes%s", sectorschecked, holecount, newline);
                }
                else {
                    color(red);
                    printf("Failed to find random padding! (%d sectors checked, %lu total filesystem holes)%s"
                           "This game probably won't boot even if stealth passes!%s",
                            sectorschecked, holecount, newline, newline);
                    color(normal);
                }
                if (!verbose) printf("%s", newline);
            }
        }
        free(fsbuffer);
    }
    skipparsingfs:
    if (verbose) printf("%s", newline);
    // seek to the default.xex
    if (fseeko(fp, defaultxexaddress, SEEK_SET) != 0) {
        printseekerror(isofilename, "Checking the default.xex");
      return 1;
    }
    // allocate a buffer for it
    if (defaultxexsize < 24) {
        // definitely way too small
        color(red);
        printf("ERROR: The default.xex is way too small! (%lu bytes) It will not be checked!%s", defaultxexsize, newline);
        color(normal);
      return 1;
    }
    if (defaultxexsize > WOW_THATS_A_LOT_OF_RAM) {
        if (debug) {
            color(yellow);
            printf("defaultxexsize (%lu) is a lot of ram!%s", defaultxexsize, newline);
            color(normal);
        }
        fprintf(stderr, "Warning: Checking the default.xex will require %.1f MBs of RAM...\n", (float) defaultxexsize/1048576);
        char response[4];
        memset(response, 0, 4);
        while (response[0] != 'y' && response[0] != 'n' && response[0] != 'Y' && response[0] != 'N') {
            fprintf(stderr, "Do you want to continue? (y/n) ");
            readstdin(response, 4);
            if (debug) printf("response[0] = %c (0x%02X)%s", response[0], response[0], newline);
        }
        if (response[0] == 'n' || response[0] == 'N') {
            printf("default.xex check was aborted as requested%s", newline);
          return 1;
        }
    }
    unsigned char *defaultxexbuffer = malloc(defaultxexsize * sizeof(char));
    if (defaultxexbuffer == NULL) {
        color(red);
        printf("ERROR: Memory allocation for defaultxexbuffer failed! Game over man... Game over!%s", newline);
        color(normal);
      exit(1);
    }
    // read the default.xex into the buffer (already seeked to it)
    initcheckread();
    if (checkreadandprinterrors(defaultxexbuffer, 1, defaultxexsize, fp, 0, defaultxexaddress,
                                isofilename, "checking the default.xex") != 0) {
        free(defaultxexbuffer);
      return 1;
    }
    donecheckread(isofilename);
    if (debug) {
        printf("default.xex 1st sector:%s", newline);
        hexdump(defaultxexbuffer, 0, 2048);
    }
    // look for XEX magic bytes
    if (memcmp(defaultxexbuffer, "XEX2", 4) != 0) {
        color(red);
        printf("ERROR: %sXEX2%s magic was not found at the start of default.xex!%s", quotation, quotation, newline);
        color(normal);
        free(defaultxexbuffer);
      return 1;
    }
    // check the default.xex
    if (verbose) printf("Checking default.xex%s", newline);
    if (checkdefaultxex(defaultxexbuffer, defaultxexsize) != 0) {
        free(defaultxexbuffer);
      return 1;
    }
    free(defaultxexbuffer);
  return 0;
}

// this function is taken from zlib's uncompr.c
// i changed one line (inflateInit to inflateInit2) in order to enable decoding gzip headers as well as zlib
int myuncompress (dest, destLen, source, sourceLen)
    Bytef *dest;
    uLongf *destLen;
    const Bytef *source;
    uLong sourceLen;
{
    z_stream stream;
    int err;

    stream.next_in = (Bytef*)source;
    stream.avail_in = (uInt)sourceLen;
    /* Check for source > 64K on 16-bit machine: */
    if ((uLong)stream.avail_in != sourceLen) return Z_BUF_ERROR;

    stream.next_out = dest;
    stream.avail_out = (uInt)*destLen;
    if ((uLong)stream.avail_out != *destLen) return Z_BUF_ERROR;

    stream.zalloc = (alloc_func)0;
    stream.zfree = (free_func)0;

    err = inflateInit2(&stream, 47);  // 15 (max window size) + 32 to enable zlib and gzip decoding with automatic header detection
    if (err != Z_OK) return err;

    err = inflate(&stream, Z_FINISH);
    if (err != Z_STREAM_END) {
        inflateEnd(&stream);
        if (err == Z_NEED_DICT || (err == Z_BUF_ERROR && stream.avail_in == 0))
            return Z_DATA_ERROR;
        return err;
    }
    *destLen = stream.total_out;

    err = inflateEnd(&stream);
    return err;
}

int printcodepoint(unsigned long codepoint, bool justcount) {
    // print unicode code point and return the number of chars printed
    if (html) {
        if (!justcount) printf("&#%lu;", codepoint);
      return 1;  // not an error
    }
    else {
        if (codepoint < 0x2000) {  // includes latin, greek, cyrillic
            if (codepoint == 0x00A0) {
                // no-break space
                if (justcount) return 1;
                else return printf(" ");
            }
            else if (codepoint == 0x00A9) {
                // copyright sign
                if (terminal) {
                    if (justcount) return 3;
                    else return printf("(C)");
                }
                else {
                    if (justcount) return 1;
                    else return printf("©");
                }
            }
            else if (codepoint == 0x00AE) {
                // registered sign
                if (terminal) {
                    if (justcount) return 3;
                    else return printf("(R)");
                }
                else {
                    if (justcount) return 1;
                    else return printf("®");
                }
            }
            else if (codepoint == 0x00B0 || codepoint == 0x00BA) {
                // degree sign / ordinal indicator
                if (justcount) return 1;
                else if (terminal) return printf("%c", 0xF8);
                else {
                    if (codepoint == 0x00B0) return printf("°");
                    else return printf("º");
                }
            }
            else if (codepoint == 0x00B2) {
                // superscript 2
                if (justcount) return 1;
                else if (terminal) return printf("%c", 0xFD);
                else return printf("²");
            }
            else if (codepoint == 0x00B3) {
                // superscript 3
                if (terminal) {
                    if (justcount) return 2;
                    else return printf("^3");
                }
                else {
                    if (justcount) return 1;
                    else return printf("³");
                }
            }
            else if (codepoint == 0x00B4) {
                // acute accent
                if (justcount) return 1;
                else if (terminal) return printf("'");
                else return printf("´");
            }
            else if (codepoint == 0x00B9) {
                // superscript 1
                if (terminal) {
                    if (justcount) return 2;
                    else return printf("^1");
                }
                else {
                    if (justcount) return 1;
                    else return printf("¹");
                }
            }
            else if (codepoint == 0x00BC) {
                // 1/4
                if (justcount) return 1;
                else if (terminal) return printf("%c", 0xAC);
                else return printf("¼");
            }
            else if (codepoint == 0x00BD) {
                // 1/2
                if (justcount) return 1;
                else if (terminal) return printf("%c", 0xAB);
                else return printf("½");
            }
            else if (codepoint == 0x00BE) {
                // 3/4
                if (terminal) {
                    if (justcount) return 3;
                    else return printf("3/4");
                }
                else {
                    if (justcount) return 1;
                    else return printf("¾");
                }
            }
            else if (codepoint == 0x00BF) {
                // inverted question mark
                if (justcount) return 1;
                else if (terminal) return printf("%c", 0xA8);
                else return printf("¿");
            }
            else if ((codepoint >= 0x00C0 && codepoint <= 0x00C5) || codepoint == 0x0100 || codepoint == 0x0102 || codepoint == 0x0104 ||
                     codepoint == 0x01CD || codepoint == 0x01DE || codepoint == 0x01E0 || codepoint == 0x01FA || codepoint == 0x0200 ||
                     codepoint == 0x0202 || codepoint == 0x0386 || codepoint == 0x0391 || codepoint == 0x0410 || codepoint == 0x04D0 ||
                     codepoint == 0x04D2) {
                // latin or cyrillic capital A's or greek capital Alphas
                if (justcount) return 1;
                if (terminal) {
                    if (codepoint == 0x00C4 || codepoint == 0x04D2) return printf("%c", 0x8E);  // capital A with Diaeresis
                    else if (codepoint == 0x00C5) return printf("%c", 0x8F);  // capital A with ring above
                }
                else {
                    if      (codepoint == 0x00C0) return printf("À");  // capital A with Grave
                    else if (codepoint == 0x00C1) return printf("Á");  // capital A with Acute
                    else if (codepoint == 0x00C2) return printf("Â");  // capital A with Circumflex
                    else if (codepoint == 0x00C3) return printf("Ã");  // capital A with Tilde
                    else if (codepoint == 0x00C4 || codepoint == 0x04D2) return printf("Ä");  // latin/cyrillic capital A with Diaeresis
                    else if (codepoint == 0x00C5) return printf("Å");  // capital A with ring above
                }
                return printf("A");
            }
            else if ((codepoint >= 0x00E0 && codepoint <= 0x00E5) || codepoint == 0x0101 || codepoint == 0x0103 || codepoint == 0x0105 ||
                     codepoint == 0x01CE || codepoint == 0x01DF || codepoint == 0x01E1 || codepoint == 0x01FB || codepoint == 0x0201 ||
                     codepoint == 0x0203 || codepoint == 0x0430 || codepoint == 0x04D1 || codepoint == 0x04D3) {
                // latin or cyrillic lowercase a's
                if (justcount) return 1;
                if (terminal) {
                    if      (codepoint == 0x00E0) return printf("%c", 0x85);  // lowercase a with Grave
                    else if (codepoint == 0x00E1) return printf("%c", 0xA0);  // lowercase a with Acute
                    else if (codepoint == 0x00E2) return printf("%c", 0x83);  // lowercase a with Circumflex
                    else if (codepoint == 0x00E4 || codepoint == 0x04D3) return printf("%c", 0x84);  // latin/cyrillic lowercase a with Diaeresis
                    else if (codepoint == 0x00E5) return printf("%c", 0x86);  // lowercase a with ring above
                }
                else {
                    if      (codepoint == 0x00E0) return printf("à");  // lowercase a with Grave
                    else if (codepoint == 0x00E1) return printf("á");  // lowercase a with Acute
                    else if (codepoint == 0x00E2) return printf("â");  // lowercase a with Circumflex
                    else if (codepoint == 0x00E3) return printf("ã");  // lowercase a with Tilde
                    else if (codepoint == 0x00E4 || codepoint == 0x04D3) return printf("ä");  // latin/cyrillic lowercase a with Diaeresis
                    else if (codepoint == 0x00E5) return printf("å");  // lowercase a with ring above
                }
                return printf("a");
            }
            else if (codepoint == 0x00C6 || codepoint == 0x01E2 || codepoint == 0x01FC || codepoint == 0x04D4) {
                // latin or cyrillic capital AE's
                if (justcount) return 1;
                if (terminal) return printf("%c", 0x92);
                return printf("Æ");
            }
            else if (codepoint == 0x00E6 || codepoint == 0x01E3 || codepoint == 0x01FD || codepoint == 0x04D5) {
                // latin or cyrillic lowercase ae's
                if (justcount) return 1;
                if (terminal) return printf("%c", 0x91);
                return printf("æ");
            }
            else if ((codepoint >= 0x00C8 && codepoint <= 0x00CB) || codepoint == 0x0112 || codepoint == 0x0114 || codepoint == 0x0116 ||
                     codepoint == 0x0118 || codepoint == 0x011A || codepoint == 0x0204 || codepoint == 0x0206 || codepoint == 0x0388 ||
                     codepoint == 0x0395 || codepoint == 0x0401 || codepoint == 0x0415 || codepoint == 0x04D6) {
                // latin capital E's, cyrillic capital Ie/Io's or greek capital Epsilons
                if (justcount) return 1;
                if (terminal) {
                    if (codepoint == 0x00C9) return printf("%c", 0x90);  // capital E with Acute
                }
                else {
                    if      (codepoint == 0x00C8) return printf("È");  // capital E with Grave
                    else if (codepoint == 0x00C9) return printf("É");  // capital E with Acute
                    else if (codepoint == 0x00CA) return printf("Ê");  // capital E with Circumflex
                    else if (codepoint == 0x00CB || codepoint == 0x0401) return printf("Ë");  // capital E with Diaeresis or cyrillic capital Io
                }
                return printf("E");
            }
            else if ((codepoint >= 0x00E8 && codepoint <= 0x00EB) || codepoint == 0x0113 || codepoint == 0x0115 || codepoint == 0x0117 ||
                     codepoint == 0x0119 || codepoint == 0x011B || codepoint == 0x0205 || codepoint == 0x0207 || codepoint == 0x0435 ||
                     codepoint == 0x0451 || codepoint == 0x04D7) {
                // latin lowercase e's or cyrillic lowercase Ie/Io's
                if (justcount) return 1;
                if (terminal) {
                    if      (codepoint == 0x00E8) return printf("%c", 0x8A);  // lowercase e with Grave
                    else if (codepoint == 0x00E9) return printf("%c", 0x82);  // lowercase e with Acute
                    else if (codepoint == 0x00EA) return printf("%c", 0x88);  // lowercase e with Circumflex
                    else if (codepoint == 0x00EB || codepoint == 0x0451) return printf("%c", 0x89);  // lowercase e with Diaeresis or cyrillic lowercase Io
                }
                else {
                    if      (codepoint == 0x00E8) return printf("è");  // lowercase e with Grave
                    else if (codepoint == 0x00E9) return printf("é");  // lowercase e with Acute
                    else if (codepoint == 0x00EA) return printf("ê");  // lowercase e with Circumflex
                    else if (codepoint == 0x00EB || codepoint == 0x0451) return printf("ë");  // lowercase e with Diaeresis or cyrillic lowercase Io
                }
                return printf("e");
            }
            else if ((codepoint >= 0x00CC && codepoint <= 0x00CF) || codepoint == 0x0128 || codepoint == 0x012A || codepoint == 0x012C ||
                     codepoint == 0x012E || codepoint == 0x0130 || codepoint == 0x01CF || codepoint == 0x0208 || codepoint == 0x020A ||
                     codepoint == 0x038A || codepoint == 0x0399 || codepoint == 0x03AA || codepoint == 0x0406 || codepoint == 0x0407 ||
                     codepoint == 0x04C0) {
                // latin capital I's, greek capital Iotas or cyrillic capital I's/Yi's/Palochkas
                if (justcount) return 1;
                if (!terminal) {
                    if      (codepoint == 0x00CC) return printf("Ì");  // capital I with Grave
                    else if (codepoint == 0x00CD) return printf("Í");  // capital I with Acute
                    else if (codepoint == 0x00CE) return printf("Î");  // capital I with Circumflex
                    else if (codepoint == 0x00CF || codepoint == 0x03AA || codepoint == 0x0407)
                        return printf("Ï");  // latin capital I with Diaeresis, greek capital Iota with Dialytika or cyrillic capital Yi
                }
                return printf("I");
            }
            else if ((codepoint >= 0x00EC && codepoint <= 0x00EF) || codepoint == 0x0129 || codepoint == 0x012B || codepoint == 0x012D ||
                     codepoint == 0x012F || codepoint == 0x0131 || codepoint == 0x01D0 || codepoint == 0x0209 || codepoint == 0x020B ||
                     codepoint == 0x0390 || codepoint == 0x03AF || codepoint == 0x03B9 || codepoint == 0x03CA || codepoint == 0x0456 ||
                     codepoint == 0x0457) {
                // latin lowercase i's, greek lowercase iota's or cyrillic lowercase i's/yi's
                if (justcount) return 1;
                if (terminal) {
                    if      (codepoint == 0x00EC) return printf("%c", 0x8D);  // lowercase i with Grave
                    else if (codepoint == 0x00ED) return printf("%c", 0xA1);  // lowercase i with Acute
                    else if (codepoint == 0x00EE) return printf("%c", 0x8C);  // lowercase i with Circumflex
                    else if (codepoint == 0x00EF || codepoint == 0x03CA || codepoint == 0x0457)
                        return printf("%c", 0x8B);  // latin lowercase i with Diaeresis, greek lowercase iota with Dialytika or cyrillic lowercase yi
                }
                else {
                    if      (codepoint == 0x00EC) return printf("ì");  // lowercase i with Grave
                    else if (codepoint == 0x00ED) return printf("í");  // lowercase i with Acute
                    else if (codepoint == 0x00EE) return printf("î");  // lowercase i with Circumflex
                    else if (codepoint == 0x00EF || codepoint == 0x03CA || codepoint == 0x0457)
                        return printf("ï");  // latin lowercase i with Diaeresis, greek lowercase iota with Dialytika or cyrillic lowercase yi
                }
                return printf("i");
            }
            else if (codepoint == 0x00D1 || codepoint == 0x0143 || codepoint == 0x0145 || codepoint == 0x0147 || codepoint == 0x019D ||
                     codepoint == 0x0274 || codepoint == 0x039D) {
                // latin capital N's or greek capital Nu's
                if (justcount) return 1;
                if (codepoint == 0x00D1) {
                    // latin capital N with Tilde
                    if (terminal) return printf("%c", 0xA5);
                    else return printf("Ñ");
                }
                return printf("N");
            }
            else if (codepoint == 0x00F1 || codepoint == 0x0144 || codepoint == 0x0146 || codepoint == 0x0148 || codepoint == 0x0149 ||
                     codepoint == 0x019E || codepoint == 0x0272 || codepoint == 0x0273) {
                // latin lowercase n's
                if (justcount) return 1;
                if (codepoint == 0x00F1) {
                    // latin lowercase n with Tilde
                    if (terminal) return printf("%c", 0xA4);
                    else return printf("ñ");
                }
                return printf("n");
            }
            else if ((codepoint >= 0x00D2 && codepoint <= 0x00D6) || codepoint == 0x00D8 || codepoint == 0x014C || codepoint == 0x014E ||
                     codepoint == 0x0150 || codepoint == 0x01A0 || codepoint == 0x01D1 || codepoint == 0x01EA || codepoint == 0x01EC ||
                     codepoint == 0x01FE || codepoint == 0x020C || codepoint == 0x020E || codepoint == 0x038C || codepoint == 0x039F ||
                     codepoint == 0x041E || codepoint == 0x04E6 || codepoint == 0x04E8 || codepoint == 0x04EA) {
                // latin or cyrillic capital O's or greek capital Omicrons
                if (justcount) return 1;
                if (terminal) {
                    if (codepoint == 0x00D6 || codepoint == 0x04E6) return printf("%c", 0x99);  // latin/cyrillic capital O with Diaeresis
                }
                else {
                    if      (codepoint == 0x00D2) return printf("Ò");  // capital O with Grave
                    else if (codepoint == 0x00D3) return printf("Ó");  // capital O with Acute
                    else if (codepoint == 0x00D4) return printf("Ô");  // capital O with Circumflex
                    else if (codepoint == 0x00D5) return printf("Õ");  // capital O with Tilde
                    else if (codepoint == 0x00D6 || codepoint == 0x04E6) return printf("Ö");  // latin/cyrillic capital O with Diaeresis
                    else if (codepoint == 0x00D8) return printf("Ø");  // capital O with Stroke
                }
                return printf("O");
            }
            else if ((codepoint >= 0x00F2 && codepoint <= 0x00F6) || codepoint == 0x00F8 || codepoint == 0x014D || codepoint == 0x014F ||
                     codepoint == 0x0151 || codepoint == 0x01A1 || codepoint == 0x01D2 || codepoint == 0x01EB || codepoint == 0x01ED ||
                     codepoint == 0x01FF || codepoint == 0x020D || codepoint == 0x020F || codepoint == 0x0275 || codepoint == 0x03BF ||
                     codepoint == 0x03CC || codepoint == 0x043E || codepoint == 0x04E7 || codepoint == 0x04E9 || codepoint == 0x04EB) {
                // latin or cyrillic lowercase o's or greek lowercase omicrons
                if (justcount) return 1;
                if (terminal) {
                    if      (codepoint == 0x00F2) return printf("%c", 0x95);  // lowercase o with Grave
                    else if (codepoint == 0x00F3) return printf("%c", 0xA2);  // lowercase o with Acute
                    else if (codepoint == 0x00F4) return printf("%c", 0x93);  // lowercase o with Circumflex
                    else if (codepoint == 0x00F6 || codepoint == 0x04E7) return printf("%c", 0x94);  // latin/cyrillic lowercase o with Diaeresis
                }
                else {
                    if      (codepoint == 0x00F2) return printf("ò");  // lowercase o with Grave
                    else if (codepoint == 0x00F3) return printf("ó");  // lowercase o with Acute
                    else if (codepoint == 0x00F4) return printf("ô");  // lowercase o with Circumflex
                    else if (codepoint == 0x00F5) return printf("õ");  // lowercase o with Tilde
                    else if (codepoint == 0x00F6 || codepoint == 0x04E7) return printf("ö");  // latin/cyrillic lowercase o with Diaeresis
                    else if (codepoint == 0x00F8) return printf("ø");  // lowercase o with Stroke
                }
                return printf("o");
            }
            else if ((codepoint >= 0x00D9 && codepoint <= 0x00DC) || codepoint == 0x0168 || codepoint == 0x016A || codepoint == 0x016C ||
                     codepoint == 0x016E || codepoint == 0x0170 || codepoint == 0x0172 || codepoint == 0x01AF || codepoint == 0x01D3 ||
                     codepoint == 0x01D5 || codepoint == 0x01D7 || codepoint == 0x01D9 || codepoint == 0x01DB || codepoint == 0x0214 ||
                     codepoint == 0x0216) {
                // latin capital U's
                if (justcount) return 1;
                if (terminal) {
                    if (codepoint == 0x00DC) return printf("%c", 0x9A);  // capital U with Diaeresis
                }
                else {
                    if      (codepoint == 0x00D9) return printf("Ù");  // capital U with Grave
                    else if (codepoint == 0x00DA) return printf("Ú");  // capital U with Acute
                    else if (codepoint == 0x00DB) return printf("Û");  // capital U with Circumflex
                    else if (codepoint == 0x00DC) return printf("Ü");  // capital U with Diaeresis
                }
                return printf("U");
            }
            else if ((codepoint >= 0x00F9 && codepoint <= 0x00FC) || codepoint == 0x0169 || codepoint == 0x016B || codepoint == 0x016D ||
                     codepoint == 0x016F || codepoint == 0x0171 || codepoint == 0x0173 || codepoint == 0x01B0 || codepoint == 0x01D4 ||
                     codepoint == 0x01D6 || codepoint == 0x01D8 || codepoint == 0x01DA || codepoint == 0x01DC || codepoint == 0x0215 ||
                     codepoint == 0x0217 || codepoint == 0x03B0 || codepoint == 0x03C5 || codepoint == 0x03CB || codepoint == 0x03CD) {
                // latin lowercase u's or greek lowercase upsilons
                if (justcount) return 1;
                if (terminal) {
                    if      (codepoint == 0x00F9) return printf("%c", 0x97);  // lowercase u with Grave
                    else if (codepoint == 0x00FA) return printf("%c", 0xA3);  // lowercase u with Acute
                    else if (codepoint == 0x00FB) return printf("%c", 0x96);  // lowercase u with Circumflex
                    else if (codepoint == 0x00FC || codepoint == 0x03CB) return printf("%c", 0x81);  // latin lowercase u with Diaeresis or greek lowercase upsilon with Dialytika
                }
                else {
                    if      (codepoint == 0x00F9) return printf("ù");  // lowercase u with Grave
                    else if (codepoint == 0x00FA) return printf("ú");  // lowercase u with Acute
                    else if (codepoint == 0x00FB) return printf("û");  // lowercase u with Circumflex
                    else if (codepoint == 0x00FC || codepoint == 0x03CB) return printf("ü");  // latin lowercase u with Diaeresis or greek lowercase upsilon with Dialytika
                }
                return printf("u");
            }
        }
        else {
            // codepoint >= 0x2000
            if ((codepoint >= 0x2010 && codepoint <= 0x2015) || codepoint == 0x2027) {
                // hyphens/dashes
                if (justcount) return 1;
                else return printf("-");
            }
            else if (codepoint == 0x2026) {
                // horizontal ellipsis
                if (justcount) return 3;
                else return printf("...");
            }
            else if (codepoint == 0x2025) {
                // two dot leader
                if (justcount) return 2;
                else return printf("..");
            }
            else if (codepoint == 0x2024) {
                // one dot leader
                if (justcount) return 1;
                else return printf(".");
            }
            else if (codepoint == 0x2022) {
                // bullet
                if (justcount) return 1;
                else if (terminal) return printf("%c", 0xF9);
                else return printf("·");
            }
            else if (codepoint == 0x2018 || codepoint == 0x2019 || codepoint == 0x201B || codepoint == 0x2032 || codepoint == 0x2035) {
                // single quotes/primes
                if (justcount) return 1;
                else return printf("'");
            }
            else if (codepoint == 0x201C || codepoint == 0x201D || codepoint == 0x201F || codepoint == 0x2033 || codepoint == 0x2036) {
                // double quotes/primes
                if (justcount) return 1;
                else return printf("\"");
            }
            else if (codepoint == 0x2034 || codepoint == 0x2037) {
                // triple primes
                if (justcount) return 3;
                else return printf("'''");
            }
            else if (codepoint == 0x2122) {
                // trademark sign
                if (justcount) return 4;
                else return printf("(TM)");
            }
            else if (codepoint == 0x221E) {
                // infinity
                if (terminal) {
                    if (justcount) return 1;
                    else return printf("%c", 0xEC);
                }
                else {
                    if (justcount) return 8;
                    else return printf("infinity");
                }
            }
            else if (codepoint == 0x24B8 || codepoint == 0x24D2) {
                // circled letter C/c
                if (terminal) {
                    if (justcount) return 3;
                    else return printf("(C)");
                }
                else {
                    if (justcount) return 1;
                    else return printf("©");
                }
            }
        }
    }
    if (justcount) return 1;
    else return printf("?");
}

int printunicodestring(unsigned char *string, int length, bool allownewlines, char *indent) {
    // be sure to update findunicodedelimiter() if you make changes to this function
    int charsprinted = 0, i = 0;
    unsigned long codepoint;
    while (i < length) {
        if (string[i] == 0x00) break;
        if (string[i] > 0x7F) {
            if (string[i] < 0xC0) {
                // 0x80 - 0xBF: Second, third, or fourth byte of a multi-byte sequence (shouldn't happen here)
                charsprinted += printf("?");
                i++;
            }
            else if (string[i] < 0xC2) {
                // 0xC0 - 0xC1: Overlong encoding: start of a 2-byte sequence, but code point <= 127 (bad)
                charsprinted += printf("?");
                i++;
            }
            else if (string[i] < 0xE0) {
                // 0xC2 - 0xDF: Start of a 2-byte sequence
                if ((i+1) < length && string[i+1] > 0x7F && string[i+1] < 0xC0) {
                    // valid sequence
                    codepoint = 0L;
                    codepoint |= ((unsigned long) (string[i] & 0x1F) << 6);
                    codepoint |= (unsigned long) (string[i+1] & 0x3F);
                    charsprinted += printcodepoint(codepoint, false);
                    i += 2;
                }
                else {
                    // invalid sequence
                    charsprinted += printf("?");
                    i++;
                }
            }
            else if (string[i] < 0xF0) {
                // 0xE0 - 0xEF: Start of a 3-byte sequence
                if ((i+2) < length && string[i+1] > 0x7F && string[i+1] < 0xC0 &&
                                      string[i+2] > 0x7F && string[i+2] < 0xC0) {
                    // valid sequence
                    codepoint = 0L;
                    codepoint |= ((unsigned long) (string[i] & 0x0F) << 12);
                    codepoint |= ((unsigned long) (string[i+1] & 0x3F) << 6);
                    codepoint |= (unsigned long) (string[i+2] & 0x3F);
                    charsprinted += printcodepoint(codepoint, false);
                    i += 3;
                }
                else {
                    // invalid sequence
                    charsprinted += printf("?");
                    i++;
                }
            }
            else if (string[i] < 0xF5) {
                // 0xF0 - 0xF4: Start of a 4-byte sequence
                if ((i+3) < length && string[i+1] > 0x7F && string[i+1] < 0xC0 &&
                                      string[i+2] > 0x7F && string[i+2] < 0xC0 &&
                                      string[i+3] > 0x7F && string[i+3] < 0xC0) {
                    // valid sequence
                    codepoint = 0L;
                    codepoint |= ((unsigned long) (string[i] & 0x07) << 18);
                    codepoint |= ((unsigned long) (string[i+1] & 0x3F) << 12);
                    codepoint |= (unsigned long) (string[i+2] & 0x3F << 6);
                    codepoint |= (unsigned long) (string[i+3] & 0x3F);
                    charsprinted += printcodepoint(codepoint, false);
                    i += 4;
                }
                else {
                    // invalid sequence
                    charsprinted += printf("?");
                    i++;
                }
            }
            else {
                // 0xF5 - 0xF7: Restricted by RFC 3629: start of 4-byte sequence for codepoint above 10FFFF
                // 0xF8 - 0xFB: Restricted by RFC 3629: start of 5-byte sequence
                // 0xFC - 0xFD: Restricted by RFC 3629: start of 6-byte sequence
                // 0xFE - 0xFF: Invalid: not defined by original UTF-8 specification
                charsprinted += printf("?");
                i++;
            }
        }
        else if (string[i] > 0x1F) {
            // 0x20 - 0x7F: US-ASCII (single byte)
            if (html) {
                if ((string[i] >= 'a' && string[i] <= 'z') ||
                     string[i] == ' ' ||
                     string[i] == '.' ||
                    (string[i] >= 'A' && string[i] <= 'Z') ||
                    (string[i] >= '0' && string[i] <= '9')) printf("%c", string[i]);
                else printf("&#%d;", (int) string[i]);
                charsprinted += 1;
            }
            else charsprinted += printf("%c", string[i]);
            i++;
        }
        else {
            // 0x00 - 0x1F: control codes, newlines, etc
            if (allownewlines) {
                if (string[i] == 0x0D) {
                    if ((i+1) < length && string[i+1] == 0x0A) {
                        printf("%s%s", newline, indent);
                        charsprinted = 0;
                        i+=2;
                    }
                    else {
                        printf("%s%s", newline, indent);
                        charsprinted = 0;
                        i++;
                    }
                }
                else if (string[i] == 0x0A) {
                    printf("%s%s", newline, indent);
                    charsprinted = 0;
                    i++;
                }
            }
            else i++;
        }    
    }
  return charsprinted;
}

int findunicodedelimiter(unsigned char *string, int length, int maxlength, bool allownewlines) {
    // this function needs to be updated if you make changes to printunicodestring()
    int i = 0, j = 0, a, b, delim = length;
    if (html || maxlength <= 0) return length;
    unsigned long codepoint;
    struct {unsigned char x; int i;} chars[maxlength + 20];  // +20 to be safe
    while (i < length && j <= maxlength) {
        if (string[i] == 0x00) break;
        if (string[i] > 0x7F) {
            if (string[i] < 0xC0) {
                // 0x80 - 0xBF: Second, third, or fourth byte of a multi-byte sequence (shouldn't happen here)
                chars[j].x = '?';
                chars[j].i = i;
                i++;
                j++;
            }
            else if (string[i] < 0xC2) {
                // 0xC0 - 0xC1: Overlong encoding: start of a 2-byte sequence, but code point <= 127 (bad)
                chars[j].x = '?';
                chars[j].i = i;
                i++;
                j++;
            }
            else if (string[i] < 0xE0) {
                // 0xC2 - 0xDF: Start of a 2-byte sequence
                if ((i+1) < length && string[i+1] > 0x7F && string[i+1] < 0xC0) {
                    // valid sequence
                    codepoint = 0L;
                    codepoint |= ((unsigned long) (string[i] & 0x1F) << 6);
                    codepoint |= (unsigned long) (string[i+1] & 0x3F);
                    b = printcodepoint(codepoint, true);  // just count
                    for (a=0;a<b;a++) {
                        chars[j].x = '?';
                        chars[j].i = i;
                        j++;
                    }
                    i += 2;
                }
                else {
                    // invalid sequence
                    chars[j].x = '?';
                    chars[j].i = i;
                    i++;
                    j++;
                }
            }
            else if (string[i] < 0xF0) {
                // 0xE0 - 0xEF: Start of a 3-byte sequence
                if ((i+2) < length && string[i+1] > 0x7F && string[i+1] < 0xC0 &&
                                      string[i+2] > 0x7F && string[i+2] < 0xC0) {
                    // valid sequence
                    codepoint = 0L;
                    codepoint |= ((unsigned long) (string[i] & 0x0F) << 12);
                    codepoint |= ((unsigned long) (string[i+1] & 0x3F) << 6);
                    codepoint |= (unsigned long) (string[i+2] & 0x3F);
                    b = printcodepoint(codepoint, true);  // just count
                    for (a=0;a<b;a++) {
                        chars[j].x = '?';
                        chars[j].i = i;
                        j++;
                    }
                    i += 3;
                }
                else {
                    // invalid sequence
                    chars[j].x = '?';
                    chars[j].i = i;
                    i++;
                    j++;
                }
            }
            else if (string[i] < 0xF5) {
                // 0xF0 - 0xF4: Start of a 4-byte sequence
                if ((i+3) < length && string[i+1] > 0x7F && string[i+1] < 0xC0 &&
                                      string[i+2] > 0x7F && string[i+2] < 0xC0 &&
                                      string[i+3] > 0x7F && string[i+3] < 0xC0) {
                    // valid sequence
                    codepoint = 0L;
                    codepoint |= ((unsigned long) (string[i] & 0x07) << 18);
                    codepoint |= ((unsigned long) (string[i+1] & 0x3F) << 12);
                    codepoint |= (unsigned long) (string[i+2] & 0x3F << 6);
                    codepoint |= (unsigned long) (string[i+3] & 0x3F);
                    b = printcodepoint(codepoint, true);  // just count
                    for (a=0;a<b;a++) {
                        chars[j].x = '?';
                        chars[j].i = i;
                        j++;
                    }
                    i += 4;
                }
                else {
                    // invalid sequence
                    chars[j].x = '?';
                    chars[j].i = i;
                    i++;
                    j++;
                }
            }
            else {
                // 0xF5 - 0xF7: Restricted by RFC 3629: start of 4-byte sequence for codepoint above 10FFFF
                // 0xF8 - 0xFB: Restricted by RFC 3629: start of 5-byte sequence
                // 0xFC - 0xFD: Restricted by RFC 3629: start of 6-byte sequence
                // 0xFE - 0xFF: Invalid: not defined by original UTF-8 specification
                chars[j].x = '?';
                chars[j].i = i;
                i++;
                j++;
            }
        }
        else if (string[i] > 0x1F) {
            // 0x20 - 0x7F: US-ASCII (single byte)
            chars[j].x = string[i];
            chars[j].i = i;
            i++;
            j++;
        }
        else {
            // 0x01 - 0x1F: control codes, newlines, etc
            if (allownewlines) {
                if (string[i] == 0x0D) {
                    if ((i+1) < length && string[i+1] == 0x0A) {
                        i+=2;
                        j=0;
                    }
                    else {
                        i++;
                        j=0;
                    }
                }
                else if (string[i] == 0x0A) {
                    i++;
                    j=0;
                }
            }
            else i++;
        }
    }
    if (debug) {
        printf("%schars a-j:%s", newline, newline);
        for (a=0;a<j;a++) printf("%c", chars[a].x);
        printf("%s", newline);
    }
    if (j > maxlength) {
        delim = maxlength + 1;
        while (delim) {
            if (chars[delim].x == ' ') {
                if (debug) printf("delim = %d%s", delim, newline);
                return chars[delim].i;
            }
            delim--;
        }
    }
  return length;
}

int printlongunicodestring(unsigned char *string, int length, int maxlength, bool allownewlines, char *indent) {
    int delimitedlength, currentlength = length, currentchar = 0;
    if (html) return printunicodestring(string, length, allownewlines, indent);
    while (currentlength > 0) {
        delimitedlength = findunicodedelimiter(string+currentchar, currentlength, maxlength, allownewlines);
        if (delimitedlength >= currentlength) {
            return printunicodestring(string+currentchar, currentlength, allownewlines, indent);
        }
        else {
            printunicodestring(string+currentchar, delimitedlength, allownewlines, indent);
            printf("%s%s", newline, indent);
            currentchar += delimitedlength + 1;
            currentlength -= delimitedlength + 1;
        }
    }
  return 0;
}

#define SIZE_OF_EMBEDDEDIMAGEBUFFER 43700  // 32 KB * 4 / 3 + 10 for padding and safety 

#define RESOURCE_HEADERTYPE_SECTION 0x0001
#define RESOURCE_HEADERTYPE_IMAGE   0x0002
#define RESOURCE_HEADERTYPE_LANG    0x0003

#define RESOURCE_STRINGID_INIT      0xFFFF

#define RESOURCE_LANG_INIT                 0xFFFFFFFFL
#define RESOURCE_LANG_ENGLISH              1L
#define RESOURCE_LANG_JAPANESE             2L
#define RESOURCE_LANG_GERMAN               3L
#define RESOURCE_LANG_FRENCH               4L
#define RESOURCE_LANG_SPANISH              5L
#define RESOURCE_LANG_ITALIAN              6L
#define RESOURCE_LANG_KOREAN               7L
#define RESOURCE_LANG_CHINESE_TRADITIONAL  8L
#define RESOURCE_LANG_PORTUGUESE           9L
#define RESOURCE_LANG_CHINESE_SIMPLIFIED   10L
#define RESOURCE_LANG_POLISH               11L
#define RESOURCE_LANG_RUSSIAN              12L

#define RESOURCE_TITLETYPE_INIT     0xFFFFFFFFL
#define RESOURCE_TITLETYPE_SYSTEM   0L  // Non-game title, released by Microsoft
#define RESOURCE_TITLETYPE_FULL     1L  // Game that ships on a DVD disc and is sold stand-alone in retail stores
#define RESOURCE_TITLETYPE_DEMO     2L  // Demo game or a bonus disc
#define RESOURCE_TITLETYPE_DOWNLOAD 3L  // Xbox Live Arcade game

bool isanonunicodelanguage(unsigned long langid) {
    if (langid == RESOURCE_LANG_ENGLISH ||
        langid == RESOURCE_LANG_GERMAN ||
        langid == RESOURCE_LANG_FRENCH ||
        langid == RESOURCE_LANG_SPANISH ||
        langid == RESOURCE_LANG_ITALIAN ||
        langid == RESOURCE_LANG_PORTUGUESE ||
        langid == RESOURCE_LANG_POLISH)
        return true;
    else return false;
}

void base64encode(unsigned char *data, unsigned long size, unsigned char *output) {
    static unsigned char base64lookup[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned long m, blocks, remainder, outputsize = 0;
    unsigned char remainderblock[3] = {0};
    memset(output, 0, SIZE_OF_EMBEDDEDIMAGEBUFFER);
    blocks = size / 3;
    remainder = size % 3;
    for (m=0;m<blocks;m++) {
        output[outputsize] = base64lookup[data[m*3] >> 2];
        output[outputsize+1] = base64lookup[((data[m*3] & 0x03) << 4) | ((data[m*3+1] & 0xF0) >> 4)];
        output[outputsize+2] = base64lookup[((data[m*3+1] & 0x0F) << 2) | ((data[m*3+2] & 0xC0) >> 6)];
        output[outputsize+3] = base64lookup[data[m*3+2] & 0x3F];
        outputsize += 4;
    }
    if (remainder) {
        memcpy(remainderblock, data+m*3, remainder);
        output[outputsize] = base64lookup[remainderblock[0] >> 2];
        output[outputsize+1] = base64lookup[((remainderblock[0] & 0x03) << 4) | ((remainderblock[1] & 0xF0) >> 4)];
        if (remainder == 2) output[outputsize+2] = base64lookup[(remainderblock[1] & 0x0F) << 2];
        else output[outputsize+2] = '=';
        output[outputsize+3] = '=';
    }
}

void parsetitleidresource(unsigned char *resourcebuffer, unsigned long resourcesize, unsigned char *titleid) {
    int i, j, ret;
    char *spx;
    unsigned long m, n, x;
    unsigned short h, type;
    long l;
    unsigned char embeddedimagebuffer[SIZE_OF_EMBEDDEDIMAGEBUFFER];
    // check that size isn't way too small
    if (resourcesize < 24) {
        if (debug || testing) {
            color(red);
            printf("PARSING ERROR: resourcesize (%lu) is less than 24!%s", resourcesize, newline);
            color(normal);
        }
      return;
    }
    else if (debug || testing) printf("resourcesize = %lu%s", resourcesize, newline);
    if (debug) {
        FILE *defaultspa = NULL;
        char defaultspapath[2048];
        memset(defaultspapath, 0, 2048);
        if (!homeless) {
            strcat(defaultspapath, homedir); strcat(defaultspapath, abgxdir);
        }
        strcat(defaultspapath, "default.spa");
        defaultspa = fopen(defaultspapath, "wb");
        if (defaultspa == NULL) {
            printf("ERROR: Failed to open %s%s%s for writing! (%s)%s", quotation, defaultspapath, quotation, strerror(errno), newline);
        }
        else {
            // should use proper error checking but this for debug so doesn't matter that much
            dontcare = fwrite(resourcebuffer, 1, resourcesize, defaultspa);
            fclose(defaultspa);
        }
    }
    struct imagestruct {
        unsigned long imageid,
        offset,
        size;
    };
    struct languagestruct {
        unsigned long langid,
        offset,
        size;
    };
    struct achievementstruct {
        unsigned short achievementid,
        nameid,
        achievedid,
        unachievedid,
        gamerscore,
        type;
        unsigned long imageid;
    };
    struct avatarawardstruct {
        unsigned short avatarawardid,
        nameid,
        achievedid,
        unachievedid;
        unsigned long imageid;
    };
    struct titleresourcestruct {
        bool foundfeatures,
        camera, coop, customsoundtrack, dolby51, harddriveenhanced, harddriverequired, liveaware,
        liveclans, livecoop, livedownloads, livefriends, livemessaging, livemultiplayer, livescoreboard,
        liveserviceproxy, livestats, livetournaments, livevoice, livevoicemessaging, memoryunit, multiplayerversus,
        peripherals, premiumoffers, systemlink, usercreatablecontent, xbox2support, xboxsupport;
        unsigned short publisherstringid,
        developerstringid,
        selltextstringid,
        genretextstringid,
        offlineplayersmax,
        systemlinkplayersmax,
        liveplayersmax;
        unsigned long headerentries,
        bodyoffset,
        titletype,
        defaultlanguage,
        num_images,
        num_embedable,
        num_languages,
        num_achievements,
        num_avatarawards,
        totalgamerscore,
        xlastgz_offset, xlastgz_size,
        xlastsize,
        XACH_offset, XACH_size,
        XSTC_offset, XSTC_size,
        XTHD_offset, XTHD_size,
        XSRC_offset, XSRC_size,
        XGAA_offset, XGAA_size;
    };
    struct titleresourcestruct resource = {
        false,                                            // found features
        false, false, false, false, false, false, false,  // misc supported features
        false, false, false, false, false, false, false,  // misc supported features
        false, false, false, false, false, false, false,  // misc supported features
        false, false, false, false, false, false,         // misc supported features
        RESOURCE_STRINGID_INIT,    // publisherstringid
        RESOURCE_STRINGID_INIT,    // developerstringid
        RESOURCE_STRINGID_INIT,    // selltextstringid
        RESOURCE_STRINGID_INIT,    // genretextstringid
        0,                         // offlineplayersmax
        0,                         // systemlinkplayersmax
        0,                         // liveplayersmax
        getuintmsb(resourcebuffer+12),                                               // header entries
        getuintmsb(resourcebuffer+8) * 18 + getuintmsb(resourcebuffer+16) * 8 + 24,  // body offset
        RESOURCE_TITLETYPE_INIT,    // title type
        RESOURCE_LANG_INIT,         // default language
        0L,                         // number of images
        0L,                         // number of images suitable for embedding
        0L,                         // number of supported languages
        0L,                         // number of achievements
        0L,                         // number of avatar awards
        0L,                         // total gamerscore
        0L, 0L,                     // gzipped xlast source file offset/size
        0L,                         // uncompressed xlast source file size
        0L, 0L,                     // XACH offset/size
        0L, 0L,                     // XSTC offset/size
        0L, 0L,                     // XTHD offset/size
        0L, 0L,                     // XSRC offset/size
        0L, 0L                      // XGAA offset/size
    };
    if ((resource.headerentries * 18 + 24) > resourcesize) {
        if (debug || testing) {
            color(red);
            printf("PARSING ERROR: too many header entries (%lu)%s", resource.headerentries, newline);
            color(normal);
        }
      return;
    }
    if (debug) {
        // display header
        if (memcmp(resourcebuffer, "XDBF", 4)) color(red);  // should never happen because this should have been checked before this function was ever called
        printf("%.4s %08lX %08lX %08lX %08lX %08lX%s", resourcebuffer, getuintmsb(resourcebuffer+4), getuintmsb(resourcebuffer+8),
                getuintmsb(resourcebuffer+12), getuintmsb(resourcebuffer+16), getuintmsb(resourcebuffer+20), newline);
        for (m=0; m<resource.headerentries; m++) {
            n = m * 18 + 24;
            type = getwordmsb(resourcebuffer+n);
            if (type == RESOURCE_HEADERTYPE_SECTION) {
                if (!memcmp(resourcebuffer+n+6, "XACH", 4) || !memcmp(resourcebuffer+n+6, "XCXT", 4) || !memcmp(resourcebuffer+n+6, "XITB", 4) ||
                    !memcmp(resourcebuffer+n+6, "XMAT", 4) || !memcmp(resourcebuffer+n+6, "XPBM", 4) || !memcmp(resourcebuffer+n+6, "XPRP", 4) ||
                    !memcmp(resourcebuffer+n+6, "XRPT", 4) || !memcmp(resourcebuffer+n+6, "XSRC", 4) || !memcmp(resourcebuffer+n+6, "XSTC", 4) ||
                    !memcmp(resourcebuffer+n+6, "XTHD", 4) || !memcmp(resourcebuffer+n+6, "XVC2", 4) || !memcmp(resourcebuffer+n+6, "XGAA", 4)) color(normal);
                else color(yellow);
                printf("%04X %08lX %.4s %s %08lX %08lX%s", type, getuintmsb(resourcebuffer+n+2), resourcebuffer+n+6,
                        sp3, getuintmsb(resourcebuffer+n+10), getuintmsb(resourcebuffer+n+14), newline);
            }
            else {
                if (type == RESOURCE_HEADERTYPE_IMAGE || type == RESOURCE_HEADERTYPE_LANG) color(normal);
                else color(yellow);
                printf("%04X %08lX %08lX %08lX %08lX%s", type, getuintmsb(resourcebuffer+n+2), getuintmsb(resourcebuffer+n+6),
                        getuintmsb(resourcebuffer+n+10), getuintmsb(resourcebuffer+n+14), newline);
            }
        }
        color(normal);
        // body offset
        printf("body offset: 0x%lX%s", resource.bodyoffset, newline);
    }
    // parse header
    for (m=0; m<resource.headerentries; m++) {
        n = m * 18 + 24;
        type = getwordmsb(resourcebuffer+n);
        if (type == RESOURCE_HEADERTYPE_SECTION) {
            if (memcmp(resourcebuffer+n+6, "XACH", 4) == 0) {
                // XACH contains the achievements
                resource.XACH_offset = resource.bodyoffset + getuintmsb(resourcebuffer+n+10);
                resource.XACH_size = getuintmsb(resourcebuffer+n+14);
                if ((resource.XACH_offset + resource.XACH_size) > resourcesize || resource.XACH_size < 14 || memcmp(resourcebuffer+resource.XACH_offset, "XACH", 4) != 0) {
                    if (debug || testing) {
                        color(red);
                        printf("PARSING ERROR: invalid XACH entry: offset: 0x%08lX, size: %08lu, string: %.4s%s",
                                resource.XACH_offset, resource.XACH_size, resourcebuffer+resource.XACH_offset, newline);
                        color(normal);
                    }
                    resource.XACH_offset = 0L;
                    resource.XACH_size = 0L;
                }
                else {
                    resource.num_achievements = (unsigned long) getwordmsb(resourcebuffer+resource.XACH_offset+12);
                    if ((resource.num_achievements * 36 + 14) > resource.XACH_size) {
                        if (debug || testing) {
                            color(red);
                            printf("PARSING ERROR: invalid XACH entry: offset: 0x%08lX, size: %08lu, string: %.4s, num_achievements: %lu%s",
                                    resource.XACH_offset, resource.XACH_size, resourcebuffer+resource.XACH_offset, resource.num_achievements, newline);
                            color(normal);
                        }
                        resource.XACH_offset = 0L;
                        resource.XACH_size = 0L;
                        resource.num_achievements = 0L;
                    }
                    else if (debug || testing) printf("found XACH: number of achievements: %lu%s", resource.num_achievements, newline);
                }
            }
            else if (memcmp(resourcebuffer+n+6, "XGAA", 4) == 0) {
                // XGAA contains the avatar awards
                resource.XGAA_offset = resource.bodyoffset + getuintmsb(resourcebuffer+n+10);
                resource.XGAA_size = getuintmsb(resourcebuffer+n+14);
                if ((resource.XGAA_offset + resource.XGAA_size) > resourcesize || resource.XGAA_size < 14 || memcmp(resourcebuffer+resource.XGAA_offset, "XGAA", 4) != 0) {
                    if (debug || testing) {
                        color(red);
                        printf("PARSING ERROR: invalid XGAA entry: offset: 0x%08lX, size: %08lu, string: %.4s%s",
                                resource.XGAA_offset, resource.XGAA_size, resourcebuffer+resource.XGAA_offset, newline);
                        color(normal);
                    }
                    resource.XGAA_offset = 0L;
                    resource.XGAA_size = 0L;
                }
                else {
                    resource.num_avatarawards = (unsigned long) getwordmsb(resourcebuffer+resource.XGAA_offset+12);
                    if ((resource.num_avatarawards * 36 + 14) > resource.XGAA_size) {
                        if (debug || testing) {
                            color(red);
                            printf("PARSING ERROR: invalid XGAA entry: offset: 0x%08lX, size: %08lu, string: %.4s, num_avatarawards: %lu%s",
                                    resource.XGAA_offset, resource.XGAA_size, resourcebuffer+resource.XGAA_offset, resource.num_avatarawards, newline);
                            color(normal);
                        }
                        resource.XGAA_offset = 0L;
                        resource.XGAA_size = 0L;
                        resource.num_avatarawards = 0L;
                    }
                    else if (debug || testing) printf("found XGAA: number of avatar awards: %lu%s", resource.num_avatarawards, newline);
                }
            }
            else if (memcmp(resourcebuffer+n+6, "XSTC", 4) == 0) {
                // XSTC contains the default language
                resource.XSTC_offset = resource.bodyoffset + getuintmsb(resourcebuffer+n+10);
                resource.XSTC_size = getuintmsb(resourcebuffer+n+14);
                if ((resource.XSTC_offset + resource.XSTC_size) > resourcesize || resource.XSTC_size < 16 || memcmp(resourcebuffer+resource.XSTC_offset, "XSTC", 4) != 0) {
                    if (debug || testing) {
                        color(red);
                        printf("PARSING ERROR: invalid XSTC entry: offset: 0x%08lX, size: %08lu, string: %.4s%s",
                                resource.XSTC_offset, resource.XSTC_size, resourcebuffer+resource.XSTC_offset, newline);
                        color(normal);
                    }
                    resource.XSTC_offset = 0L;
                    resource.XSTC_size = 0L;
                }
                else {
                    resource.defaultlanguage = getuintmsb(resourcebuffer+resource.XSTC_offset+12);
                    if (debug || testing) printf("found XSTC: default language: %02lu%s", resource.defaultlanguage, newline);
                }
            }
            else if (memcmp(resourcebuffer+n+6, "XTHD", 4) == 0) {
                // XTHD contains the title id and title type
                resource.XTHD_offset = resource.bodyoffset + getuintmsb(resourcebuffer+n+10);
                resource.XTHD_size = getuintmsb(resourcebuffer+n+14);
                if ((resource.XTHD_offset + resource.XTHD_size) > resourcesize || resource.XTHD_size < 20 || memcmp(resourcebuffer+resource.XTHD_offset, "XTHD", 4) != 0) {
                    if (debug || testing) {
                        color(red);
                        printf("PARSING ERROR: invalid XTHD entry: offset: 0x%08lX, size: %08lu, string: %.4s%s",
                                resource.XTHD_offset, resource.XTHD_size, resourcebuffer+resource.XTHD_offset, newline);
                        color(normal);
                    }
                    resource.XTHD_offset = 0L;
                    resource.XTHD_size = 0L;
                }
                else {
                    resource.titletype = getuintmsb(resourcebuffer+resource.XTHD_offset+16);
                    if (titleid == NULL) {
                        // this should only happen when checking a .spa file because checkdefaultxex() would have had to find a title id before getting to the point where it could call this function
                        titleid = resourcebuffer+resource.XTHD_offset+12;
                        if (debug || testing) printf("null titleid was replaced with XTHD titleid: %02X%02X%02X%02X%s", titleid[0], titleid[1], titleid[2], titleid[3], newline);
                    }
                    else if (debug || testing) printf("titleid passed to parsetitleidresource (or multiple XTHD entries): %02X%02X%02X%02X%s", titleid[0], titleid[1], titleid[2], titleid[3], newline);
                    if (debug || testing) printf("found XTHD: titleid: %02X%02X%02X%02X (%c%c-%u), titletype: %lu%s", resourcebuffer[resource.XTHD_offset+12], resourcebuffer[resource.XTHD_offset+13],
                                                  resourcebuffer[resource.XTHD_offset+14], resourcebuffer[resource.XTHD_offset+15], resourcebuffer[resource.XTHD_offset+12],
                                                  resourcebuffer[resource.XTHD_offset+13], getwordmsb(resourcebuffer+resource.XTHD_offset+14), resource.titletype, newline);
                }
            }
            else if (memcmp(resourcebuffer+n+6, "XSRC", 4) == 0) {
                // XSRC contains the original xlast source xml (gzipped)
                resource.XSRC_offset = resource.bodyoffset + getuintmsb(resourcebuffer+n+10);
                resource.XSRC_size = getuintmsb(resourcebuffer+n+14);
                if ((resource.XSRC_offset + resource.XSRC_size) > resourcesize || resource.XSRC_size < 20 || memcmp(resourcebuffer+resource.XSRC_offset, "XSRC", 4) != 0) {
                    if (debug || testing) {
                        color(red);
                        printf("PARSING ERROR: invalid XSRC entry: offset: 0x%08lX, size: %08lu, string: %.4s%s",
                                resource.XSRC_offset, resource.XSRC_size, resourcebuffer+resource.XSRC_offset, newline);
                        color(normal);
                    }
                    resource.XSRC_offset = 0L;
                    resource.XSRC_size = 0L;
                }
                else {
                    x = getuintmsb(resourcebuffer+resource.XSRC_offset+12);  // length of original xlast filename
                    resource.xlastgz_offset = resource.XSRC_offset + 24 + x;
                    if (resource.xlastgz_offset > (resource.XSRC_offset + resource.XSRC_size)) {
                        if (debug || testing) {
                            color(red);
                            printf("PARSING ERROR: invalid XSRC entry: offset: 0x%08lX, size: %08lu, string: %.4s, filename length: %lu, xlastgz_offset: %lu%s",
                                    resource.XSRC_offset, resource.XSRC_size, resourcebuffer+resource.XSRC_offset, x, resource.xlastgz_offset, newline);
                            color(normal);
                        }
                        resource.XSRC_offset = 0L;
                        resource.XSRC_size = 0L;
                        resource.xlastgz_offset = 0L;
                    }
                    else {
                        resource.xlastgz_size = getuintmsb(resourcebuffer+resource.XSRC_offset+20+x);
                        if ((resource.xlastgz_offset + resource.xlastgz_size) > (resource.XSRC_offset + resource.XSRC_size)) {
                            if (debug || testing) {
                                color(red);
                                printf("PARSING ERROR: invalid XSRC entry: offset: 0x%08lX, size: %08lu, string: %.4s, filename length: %lu, xlastgz_offset: %lu, xlastgz_size: %lu%s",
                                        resource.XSRC_offset, resource.XSRC_size, resourcebuffer+resource.XSRC_offset, x, resource.xlastgz_offset, resource.xlastgz_size, newline);
                                color(normal);
                            }
                            resource.XSRC_offset = 0L;
                            resource.XSRC_size = 0L;
                            resource.xlastgz_offset = 0L;
                            resource.xlastgz_size = 0L;
                        }
                        else {
                            resource.xlastsize = getuintmsb(resourcebuffer+resource.XSRC_offset+16+x);
                            if (debug || testing) printf("found XSRC: xlast gz offset: %lX, gz size: %lu, original size: %lu, original name: %.*s%s",
                                                          resource.xlastgz_offset, resource.xlastgz_size, resource.xlastsize, (int) x, resourcebuffer+resource.XSRC_offset+16, newline);
                        }
                    }
                }
            }
            
            
        }
        else if (type == RESOURCE_HEADERTYPE_IMAGE) {
            resource.num_images++;
        }
        else if (type == RESOURCE_HEADERTYPE_LANG) {
            resource.num_languages++;
        }
        else if (debug || testing) {
            // unknown entry type
            color(yellow);
            printf("PARSING ERROR: unrecognized header entry type (%04X)%s", type, newline);
            color(normal);
        }
    }
    char imagedirpath[2048] = {0};
    struct imagestruct embedable_images[resource.num_images];
    if (extractimages) {
        if (imagedirmissing || homeless) {
            color(yellow);
            printf("ERROR: Unable to extract images because the Image directory is missing or can't be found%s", newline);
            color(normal);
            extractimages = false;
        }
        else if (titleid == NULL) {
            color(yellow);
            printf("ERROR: Unable to extract images because the Title ID was not found%s", newline);
            color(normal);
            extractimages = false;
        }
        else if (resource.num_images) {
            if (embedimages) {
                x = 0;
                if (showachievements) x = resource.num_achievements;
                if (showavatarawards) x += resource.num_avatarawards;
                if (x > 500) {
                    // this might make the html source huge
                    color(yellow);
                    printf("ERROR: Unwilling to embed images because there are over 500 achievements/avatar awards%s", newline);
                    color(normal);
                    embedimages = false;
                }
            }
            char fullimagename[2048];
            unsigned char pngheader[8] = {0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A};
            unsigned long imageid, imageoffset, imagesize;
            FILE *imagefile;
            strcat(imagedirpath, homedir);
            strcat(imagedirpath, abgxdir);
            strcat(imagedirpath, imagedir);
            // do the extraction
            for (m=0; m<resource.headerentries; m++) {
                n = m * 18 + 24;
                if (getwordmsb(resourcebuffer+n) == RESOURCE_HEADERTYPE_IMAGE) {
                    imageid = getuintmsb(resourcebuffer+n+6);
                    imageoffset = resource.bodyoffset + getuintmsb(resourcebuffer+n+10);
                    imagesize = getuintmsb(resourcebuffer+n+14);
                    if ((imageoffset + imagesize) > resourcesize || imagesize < 8 || memcmp(resourcebuffer+imageoffset, pngheader, 8) != 0) {
                        if (debug || testing) {
                            color(red);
                            printf("PARSING ERROR: invalid image: imageid: %08lX, offset: 0x%08lX, size: %08lu%s", imageid, imageoffset, imagesize, newline);
                            color(normal);
                            if (imageoffset + imagesize <= resourcesize && imagesize > 0) {
                                printf("first 2048 bytes of the image:%s", newline);
                                hexdump(resourcebuffer+imageoffset, 0, imagesize > 2048 ? 2048 : imagesize);
                            }
                        }
                    }
                    else {
                        sprintf(fullimagename, "%s%02X%02X%02X%02X-%08lX.png", imagedirpath, titleid[0], titleid[1], titleid[2], titleid[3], imageid);
                        imagefile = fopen(fullimagename, "wb");
                        if (imagefile == NULL) {
                            color(red);
                            printf("ERROR: Failed to open %s%s%s for writing! (%s) Failed to extract images!%s",
                                    quotation, fullimagename, quotation, strerror(errno), newline);
                            color(normal);
                            extractimages = false;
                          break;
                        }
                        initcheckwrite();
                        if (checkwriteandprinterrors(resourcebuffer+imageoffset, 1, imagesize, imagefile, 0, 0, fullimagename, "extracting images") != 0) {
                            fclose(imagefile);
                            extractimages = false;
                          break;
                        }
                        donecheckwrite(fullimagename);
                        fclose(imagefile);
                        if (debug) printf("successfully extracted: %s%s", fullimagename, newline);
                        if (embedimages && imagesize <= 32768) {
                            // Max 32 KB is important for browser compatibility and SIZE_OF_EMBEDDEDIMAGEBUFFER
                            embedable_images[resource.num_embedable].imageid = imageid;
                            embedable_images[resource.num_embedable].offset = imageoffset;
                            embedable_images[resource.num_embedable].size = imagesize;
                            resource.num_embedable++;
                        }
                    }
                }
            }
        }
        else extractimages = false;  // this is so we don't add a bad imagedirpath to html source or something else stupid
    }
    
    if (resource.xlastgz_offset && resource.xlastgz_size && resource.xlastsize) {
        // found xlast source (gzipped)
        if (debug) {
            printf("1st 2048 bytes of xlast gz:%s", newline);
            hexdump(resourcebuffer+resource.xlastgz_offset, 0, resource.xlastgz_size > 2048 ? 2048 : resource.xlastgz_size);
        }
        if (resource.xlastsize > WOW_THATS_A_LOT_OF_RAM) {
            if (debug) {
                color(yellow);
                printf("resource.xlastsize (%lu) is greater than WOW_THATS_A_LOT_OF_RAM (%d)%s", resource.xlastsize, WOW_THATS_A_LOT_OF_RAM, newline);
                color(normal);
            }
            fprintf(stderr, "Warning: Decompressing the xlast file will require %.1f MBs of RAM...\n",
                            (float) resource.xlastsize/1048576);
            char response[4];
            memset(response, 0, 4);
            while (response[0] != 'y' && response[0] != 'n' && response[0] != 'Y' && response[0] != 'N') {
                fprintf(stderr, "Do you want to continue? (y/n) ");
                readstdin(response, 4);
                if (debug) printf("response[0] = %c (0x%02X)%s", response[0], response[0], newline);
            }
            if (response[0] == 'n' || response[0] == 'N') {
                printf("Decompressing the xlast file was aborted as requested%s", newline);
              goto skipxlastdecompression;
            }
        }
        // do the decompression
        unsigned char *xlastbuffer = malloc(resource.xlastsize);
        uLongf realxlastsize = (uLongf) resource.xlastsize;
        ret = myuncompress((Bytef*)xlastbuffer, &realxlastsize, (const Bytef*) resourcebuffer+resource.xlastgz_offset, (uLongf) resource.xlastgz_size);
        if (ret != Z_OK) {
            if (debug || testing) {
                color(red);
                printf("PARSING ERROR: decompressing xlast gz failed: ");
                if (ret == Z_MEM_ERROR) printf("not enough memory%s", newline);
                else if (ret == Z_BUF_ERROR) printf("not enough room in the output buffer%s", newline);
                else if (ret == Z_DATA_ERROR) printf("the input data is corrupt%s", newline);
                else printf("unknown error%s", newline);
                color(normal);
            }
        }
        else {
            if (debug || testing) {
                printf("decompressing xlast gz was successful: resource.xlastsize: %lu, realxlastsize: %lu%s", resource.xlastsize, (unsigned long) realxlastsize, newline);
                if (debug) {
                    printf("1st 2048 bytes of xlast source:%s", newline);
                    hexdump(xlastbuffer, 0, realxlastsize > 2048 ? 2048 : realxlastsize);
                    FILE *xlast = NULL;
                    char xlastpath[2048];
                    memset(xlastpath, 0, 2048);
                    if (!homeless) {
                        strcat(xlastpath, homedir); strcat(xlastpath, abgxdir);
                    }
                    strcat(xlastpath, "xlast.xml");
                    xlast = fopen(xlastpath, "wb");
                    if (xlast == NULL) {
                        printf("ERROR: Failed to open %s%s%s for writing! (%s)%s", quotation, xlastpath, quotation, strerror(errno), newline);
                    }
                    else {
                        // should use proper error checking but this for debug so doesn't matter that much
                        dontcare = fwrite(xlastbuffer, 1, realxlastsize, xlast);
                        fclose(xlast);
                    }
                }
            }
            if (realxlastsize > 60) {
                // parse xlast xml (should be UTF-16LE)
                unsigned char xmlutf16leheader[10] = {0x3C,0x00,0x3F,0x00,0x78,0x00,0x6D,0x00,0x6C,0x00};
                if (memcmp(xlastbuffer, xmlutf16leheader, 10) != 0) {
                    if (debug || testing) {
                        color(red);
                        printf("PARSING ERROR: did not find \"%s?xml\" (UTF-16LE) at the start xlast source%s", lessthan, newline);
                        color(normal);
                    }
                }
                else {
                    char asciivalue[6];
                    static unsigned char greaterthansign[2] =        {0x3E, 0x00};
                    static unsigned char quotemark[2] =              {0x22, 0x00};
                    static unsigned char ProductInformation[40] =    {0x3C, 0x00, 0x50, 0x00, 0x72, 0x00, 0x6F, 0x00, 0x64, 0x00, 0x75, 0x00, 0x63, 0x00, 0x74, 0x00,
                                                                      0x49, 0x00, 0x6E, 0x00, 0x66, 0x00, 0x6F, 0x00, 0x72, 0x00, 0x6D, 0x00, 0x61, 0x00, 0x74, 0x00,
                                                                      0x69, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x20, 0x00};
                    static unsigned char offlinePlayersMax[38] =     {0x6F, 0x00, 0x66, 0x00, 0x66, 0x00, 0x6C, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x65, 0x00, 0x50, 0x00,
	                                                                  0x6C, 0x00, 0x61, 0x00, 0x79, 0x00, 0x65, 0x00, 0x72, 0x00, 0x73, 0x00, 0x4D, 0x00, 0x61, 0x00,
	                                                                  0x78, 0x00, 0x3D, 0x00, 0x22, 0x00};
	                static unsigned char systemLinkPlayersMax[44] =  {0x73, 0x00, 0x79, 0x00, 0x73, 0x00, 0x74, 0x00, 0x65, 0x00, 0x6D, 0x00, 0x4C, 0x00, 0x69, 0x00,
                                                                      0x6E, 0x00, 0x6B, 0x00, 0x50, 0x00, 0x6C, 0x00, 0x61, 0x00, 0x79, 0x00, 0x65, 0x00, 0x72, 0x00,
                                                                      0x73, 0x00, 0x4D, 0x00, 0x61, 0x00, 0x78, 0x00, 0x3D, 0x00, 0x22, 0x00};
                    static unsigned char livePlayersMax[32] =        {0x6C, 0x00, 0x69, 0x00, 0x76, 0x00, 0x65, 0x00, 0x50, 0x00, 0x6C, 0x00, 0x61, 0x00, 0x79, 0x00,
                                                                      0x65, 0x00, 0x72, 0x00, 0x73, 0x00, 0x4D, 0x00, 0x61, 0x00, 0x78, 0x00, 0x3D, 0x00, 0x22, 0x00};
                    static unsigned char publisherStringId[38] =     {0x70, 0x00, 0x75, 0x00, 0x62, 0x00, 0x6C, 0x00, 0x69, 0x00, 0x73, 0x00, 0x68, 0x00, 0x65, 0x00,
                                                                      0x72, 0x00, 0x53, 0x00, 0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x67, 0x00, 0x49, 0x00,
                                                                      0x64, 0x00, 0x3D, 0x00, 0x22, 0x00};
                    static unsigned char developerStringId[38] =     {0x64, 0x00, 0x65, 0x00, 0x76, 0x00, 0x65, 0x00, 0x6C, 0x00, 0x6F, 0x00, 0x70, 0x00, 0x65, 0x00,
                                                                      0x72, 0x00, 0x53, 0x00, 0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x67, 0x00, 0x49, 0x00,
                                                                      0x64, 0x00, 0x3D, 0x00, 0x22, 0x00};
                    static unsigned char sellTextStringId[36] =      {0x73, 0x00, 0x65, 0x00, 0x6C, 0x00, 0x6C, 0x00, 0x54, 0x00, 0x65, 0x00, 0x78, 0x00, 0x74, 0x00,
                                                                      0x53, 0x00, 0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x67, 0x00, 0x49, 0x00, 0x64, 0x00,
                                                                      0x3D, 0x00, 0x22, 0x00};
                    static unsigned char genreTextStringId[38] =     {0x67, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x72, 0x00, 0x65, 0x00, 0x54, 0x00, 0x65, 0x00, 0x78, 0x00,
                                                                      0x74, 0x00, 0x53, 0x00, 0x74, 0x00, 0x72, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x67, 0x00, 0x49, 0x00,
                                                                      0x64, 0x00, 0x3D, 0x00, 0x22, 0x00};
                    static unsigned char Featurename[30] =           {0x3C, 0x00, 0x46, 0x00, 0x65, 0x00, 0x61, 0x00, 0x74, 0x00, 0x75, 0x00, 0x72, 0x00, 0x65, 0x00, 
                                                                      0x20, 0x00, 0x6E, 0x00, 0x61, 0x00, 0x6D, 0x00, 0x65, 0x00, 0x3D, 0x00, 0x22, 0x00};
                    static unsigned char enabledtrue[34] =           {0x20, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x61, 0x00, 0x62, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x64, 0x00, 
                                                                      0x3D, 0x00, 0x22, 0x00, 0x74, 0x00, 0x72, 0x00, 0x75, 0x00, 0x65, 0x00, 0x22, 0x00, 0x2F, 0x00, 
                                                                      0x3E, 0x00};
                    static unsigned char EndProductInformation[42] = {0x3C, 0x00, 0x2F, 0x00, 0x50, 0x00, 0x72, 0x00, 0x6F, 0x00, 0x64, 0x00, 0x75, 0x00, 0x63, 0x00, 
                                                                      0x74, 0x00, 0x49, 0x00, 0x6E, 0x00, 0x66, 0x00, 0x6F, 0x00, 0x72, 0x00, 0x6D, 0x00, 0x61, 0x00, 
                                                                      0x74, 0x00, 0x69, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x3E, 0x00};
                    static unsigned char camera[14] =                {0x63, 0x00, 0x61, 0x00, 0x6D, 0x00, 0x65, 0x00, 0x72, 0x00, 0x61, 0x00, 0x22, 0x00};
                    static unsigned char coop[10] =                  {0x63, 0x00, 0x6F, 0x00, 0x4F, 0x00, 0x70, 0x00, 0x22, 0x00};
                    static unsigned char customsoundtrack[34] =      {0x63, 0x00, 0x75, 0x00, 0x73, 0x00, 0x74, 0x00, 0x6F, 0x00, 0x6D, 0x00, 0x53, 0x00, 0x6F, 0x00, 
                                                                      0x75, 0x00, 0x6E, 0x00, 0x64, 0x00, 0x74, 0x00, 0x72, 0x00, 0x61, 0x00, 0x63, 0x00, 0x6B, 0x00, 
                                                                      0x22, 0x00};
                    static unsigned char dolby51[16] =               {0x64, 0x00, 0x6F, 0x00, 0x6C, 0x00, 0x62, 0x00, 0x79, 0x00, 0x35, 0x00, 0x31, 0x00, 0x22, 0x00};
                    static unsigned char harddriveenhanced[36] =     {0x68, 0x00, 0x61, 0x00, 0x72, 0x00, 0x64, 0x00, 0x44, 0x00, 0x72, 0x00, 0x69, 0x00, 0x76, 0x00, 
                                                                      0x65, 0x00, 0x45, 0x00, 0x6E, 0x00, 0x68, 0x00, 0x61, 0x00, 0x6E, 0x00, 0x63, 0x00, 0x65, 0x00, 
                                                                      0x64, 0x00, 0x22, 0x00};
                    static unsigned char harddriverequired[36] =     {0x68, 0x00, 0x61, 0x00, 0x72, 0x00, 0x64, 0x00, 0x44, 0x00, 0x72, 0x00, 0x69, 0x00, 0x76, 0x00, 
                                                                      0x65, 0x00, 0x52, 0x00, 0x65, 0x00, 0x71, 0x00, 0x75, 0x00, 0x69, 0x00, 0x72, 0x00, 0x65, 0x00, 
                                                                      0x64, 0x00, 0x22, 0x00};
                    static unsigned char liveaware[20] =             {0x6C, 0x00, 0x69, 0x00, 0x76, 0x00, 0x65, 0x00, 0x41, 0x00, 0x77, 0x00, 0x61, 0x00, 0x72, 0x00, 
                                                                      0x65, 0x00, 0x22, 0x00};
                    static unsigned char liveclans[20] =             {0x6C, 0x00, 0x69, 0x00, 0x76, 0x00, 0x65, 0x00, 0x43, 0x00, 0x6C, 0x00, 0x61, 0x00, 0x6E, 0x00, 
                                                                      0x73, 0x00, 0x22, 0x00};
                    static unsigned char livecoop[18] =              {0x6C, 0x00, 0x69, 0x00, 0x76, 0x00, 0x65, 0x00, 0x43, 0x00, 0x6F, 0x00, 0x4F, 0x00, 0x70, 0x00, 
                                                                      0x22, 0x00};
                    static unsigned char livedownloads[28] =         {0x6C, 0x00, 0x69, 0x00, 0x76, 0x00, 0x65, 0x00, 0x44, 0x00, 0x6F, 0x00, 0x77, 0x00, 0x6E, 0x00, 
                                                                      0x6C, 0x00, 0x6F, 0x00, 0x61, 0x00, 0x64, 0x00, 0x73, 0x00, 0x22, 0x00};
                    static unsigned char livefriends[24] =           {0x6C, 0x00, 0x69, 0x00, 0x76, 0x00, 0x65, 0x00, 0x46, 0x00, 0x72, 0x00, 0x69, 0x00, 0x65, 0x00, 
                                                                      0x6E, 0x00, 0x64, 0x00, 0x73, 0x00, 0x22, 0x00};
                    static unsigned char livemessaging[28] =         {0x6C, 0x00, 0x69, 0x00, 0x76, 0x00, 0x65, 0x00, 0x4D, 0x00, 0x65, 0x00, 0x73, 0x00, 0x73, 0x00, 
                                                                      0x61, 0x00, 0x67, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x67, 0x00, 0x22, 0x00};
                    static unsigned char livemultiplayer[32] =       {0x6C, 0x00, 0x69, 0x00, 0x76, 0x00, 0x65, 0x00, 0x4D, 0x00, 0x75, 0x00, 0x6C, 0x00, 0x74, 0x00, 
                                                                      0x69, 0x00, 0x70, 0x00, 0x6C, 0x00, 0x61, 0x00, 0x79, 0x00, 0x65, 0x00, 0x72, 0x00, 0x22, 0x00};
                    static unsigned char livescoreboard[30] =        {0x6C, 0x00, 0x69, 0x00, 0x76, 0x00, 0x65, 0x00, 0x53, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x72, 0x00, 
                                                                      0x65, 0x00, 0x62, 0x00, 0x6F, 0x00, 0x61, 0x00, 0x72, 0x00, 0x64, 0x00, 0x22, 0x00};
                    static unsigned char liveserviceproxy[34] =      {0x6C, 0x00, 0x69, 0x00, 0x76, 0x00, 0x65, 0x00, 0x53, 0x00, 0x65, 0x00, 0x72, 0x00, 0x76, 0x00, 
                                                                      0x69, 0x00, 0x63, 0x00, 0x65, 0x00, 0x50, 0x00, 0x72, 0x00, 0x6F, 0x00, 0x78, 0x00, 0x79, 0x00, 
                                                                      0x22, 0x00};
                    static unsigned char livestats[20] =             {0x6C, 0x00, 0x69, 0x00, 0x76, 0x00, 0x65, 0x00, 0x53, 0x00, 0x74, 0x00, 0x61, 0x00, 0x74, 0x00, 
                                                                      0x73, 0x00, 0x22, 0x00};
                    static unsigned char livetournaments[32] =       {0x6C, 0x00, 0x69, 0x00, 0x76, 0x00, 0x65, 0x00, 0x54, 0x00, 0x6F, 0x00, 0x75, 0x00, 0x72, 0x00, 
                                                                      0x6E, 0x00, 0x61, 0x00, 0x6D, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x73, 0x00, 0x22, 0x00};
                    static unsigned char livevoice[20] =             {0x6C, 0x00, 0x69, 0x00, 0x76, 0x00, 0x65, 0x00, 0x56, 0x00, 0x6F, 0x00, 0x69, 0x00, 0x63, 0x00, 
                                                                      0x65, 0x00, 0x22, 0x00};
                    static unsigned char livevoicemessaging[38] =    {0x6C, 0x00, 0x69, 0x00, 0x76, 0x00, 0x65, 0x00, 0x56, 0x00, 0x6F, 0x00, 0x69, 0x00, 0x63, 0x00, 
                                                                      0x65, 0x00, 0x4D, 0x00, 0x65, 0x00, 0x73, 0x00, 0x73, 0x00, 0x61, 0x00, 0x67, 0x00, 0x69, 0x00, 
                                                                      0x6E, 0x00, 0x67, 0x00, 0x22, 0x00};
                    static unsigned char memoryunit[22] =            {0x6D, 0x00, 0x65, 0x00, 0x6D, 0x00, 0x6F, 0x00, 0x72, 0x00, 0x79, 0x00, 0x55, 0x00, 0x6E, 0x00, 
                                                                      0x69, 0x00, 0x74, 0x00, 0x22, 0x00};
                    static unsigned char multiplayerversus[36] =     {0x6D, 0x00, 0x75, 0x00, 0x6C, 0x00, 0x74, 0x00, 0x69, 0x00, 0x70, 0x00, 0x6C, 0x00, 0x61, 0x00, 
                                                                      0x79, 0x00, 0x65, 0x00, 0x72, 0x00, 0x56, 0x00, 0x65, 0x00, 0x72, 0x00, 0x73, 0x00, 0x75, 0x00, 
                                                                      0x73, 0x00, 0x22, 0x00};
                    static unsigned char peripherals[24] =           {0x70, 0x00, 0x65, 0x00, 0x72, 0x00, 0x69, 0x00, 0x70, 0x00, 0x68, 0x00, 0x65, 0x00, 0x72, 0x00, 
                                                                      0x61, 0x00, 0x6C, 0x00, 0x73, 0x00, 0x22, 0x00};
                    static unsigned char premiumoffers[28] =         {0x70, 0x00, 0x72, 0x00, 0x65, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x75, 0x00, 0x6D, 0x00, 0x4F, 0x00, 
                                                                      0x66, 0x00, 0x66, 0x00, 0x65, 0x00, 0x72, 0x00, 0x73, 0x00, 0x22, 0x00};
                    static unsigned char systemlink[22] =            {0x73, 0x00, 0x79, 0x00, 0x73, 0x00, 0x74, 0x00, 0x65, 0x00, 0x6D, 0x00, 0x4C, 0x00, 0x69, 0x00, 
                                                                      0x6E, 0x00, 0x6B, 0x00, 0x22, 0x00};
                    static unsigned char usercreatablecontent[42] =  {0x75, 0x00, 0x73, 0x00, 0x65, 0x00, 0x72, 0x00, 0x43, 0x00, 0x72, 0x00, 0x65, 0x00, 0x61, 0x00, 
                                                                      0x74, 0x00, 0x61, 0x00, 0x62, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x43, 0x00, 0x6F, 0x00, 0x6E, 0x00, 
                                                                      0x74, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x22, 0x00};
                    static unsigned char xbox2support[26] =          {0x78, 0x00, 0x62, 0x00, 0x6F, 0x00, 0x78, 0x00, 0x32, 0x00, 0x53, 0x00, 0x75, 0x00, 0x70, 0x00, 
                                                                      0x70, 0x00, 0x6F, 0x00, 0x72, 0x00, 0x74, 0x00, 0x22, 0x00};
                    static unsigned char xboxsupport[24] =           {0x78, 0x00, 0x62, 0x00, 0x6F, 0x00, 0x78, 0x00, 0x53, 0x00, 0x75, 0x00, 0x70, 0x00, 0x70, 0x00, 
                                                                      0x6F, 0x00, 0x72, 0x00, 0x74, 0x00, 0x22, 0x00};
                    for (m=10;m<realxlastsize-50;m+=2) {
                        if (memcmp(xlastbuffer+m, ProductInformation, 4) == 0) {  // small optimization.. check the first 4 bytes "<P" to see if we need to check the full 40
                            if (memcmp(xlastbuffer+m, ProductInformation, 40) == 0) {
                                for(n=m+40;n<realxlastsize-2;n+=2) {
                                    if (memcmp(xlastbuffer+n, greaterthansign, 2) == 0) break;
                                }
                                for (m+=40;m<n;m+=2) {
                                    if (memcmp(xlastbuffer+m, offlinePlayersMax, 4) == 0) {
                                        if (memcmp(xlastbuffer+m, offlinePlayersMax, 38) == 0) {
                                            memset(asciivalue, 0, 6);
                                            i = 0;
                                            for (m+=38;m<n;m+=2) {
                                                if (memcmp(xlastbuffer+m, quotemark, 2) == 0) {
                                                    if (i == 0) {
                                                        // no chars (i.e. offlinePlayersMax="")
                                                        if (debug || testing) {
                                                            color(red);
                                                            printf("PARSING ERROR: no chars for offlinePlayersMax%s", newline);
                                                            color(normal);
                                                        }
                                                    }
                                                    else {
                                                        l = strtol(asciivalue, NULL, 10);
                                                        if (l >= 0 && l <= 65535) {
                                                            // save value
                                                            resource.offlineplayersmax = (unsigned short) l;
                                                            if (resource.offlineplayersmax) resource.foundfeatures = true;
                                                        }
                                                        else if (debug || testing) {
                                                            color(red);
                                                            printf("PARSING ERROR: offlinePlayersMax (%ld) is not 0-65535%s", l, newline);
                                                            color(normal);
                                                        }
                                                    }
                                                  break;
                                                }
                                                if (i > 4) {
                                                    // max 5 chars (i=0-4) for 2 byte value (0-65535)
                                                    if (debug || testing) {
                                                        color(red);
                                                        printf("PARSING ERROR: too many chars for offlinePlayersMax asciivalue (%s) next char(s): %s%s", asciivalue, xlastbuffer+m, newline);
                                                        color(normal);
                                                    }
                                                  break;
                                                }
                                                if (xlastbuffer[m] >= '0' && xlastbuffer[m] <= '9' && xlastbuffer[m+1] == 0x00) {
                                                    asciivalue[i] = (char) xlastbuffer[m];
                                                    i++;
                                                }
                                                else {
                                                    if (debug || testing) {
                                                        color(red);
                                                        printf("PARSING ERROR: offlinePlayersMax contains a value other than 0-9 (%s)%s", xlastbuffer+m, newline);
                                                        color(normal);
                                                    }
                                                  break;
                                                }
                                            }
                                        }
                                    }
                                    else if (memcmp(xlastbuffer+m, systemLinkPlayersMax, 4) == 0) {
                                        if (memcmp(xlastbuffer+m, systemLinkPlayersMax, 44) == 0) {
                                            memset(asciivalue, 0, 6);
                                            i = 0;
                                            for (m+=44;m<n;m+=2) {
                                                if (memcmp(xlastbuffer+m, quotemark, 2) == 0) {
                                                    if (i == 0) {
                                                        // no chars (i.e. systemLinkPlayersMax="")
                                                        if (debug || testing) {
                                                            color(red);
                                                            printf("PARSING ERROR: no chars for systemLinkPlayersMax%s", newline);
                                                            color(normal);
                                                        }
                                                    }
                                                    else {
                                                        l = strtol(asciivalue, NULL, 10);
                                                        if (l >= 0 && l <= 65535) {
                                                            // save value
                                                            resource.systemlinkplayersmax = (unsigned short) l;
                                                        }
                                                        else if (debug || testing) {
                                                            color(red);
                                                            printf("PARSING ERROR: systemLinkPlayersMax (%ld) is not 0-65535%s", l, newline);
                                                            color(normal);
                                                        }
                                                    }
                                                  break;
                                                }
                                                if (i > 4) {
                                                    // max 5 chars (i=0-4) for 2 byte value (0-65535)
                                                    if (debug || testing) {
                                                        color(red);
                                                        printf("PARSING ERROR: too many chars for systemLinkPlayersMax asciivalue (%s) next char(s): %s%s", asciivalue, xlastbuffer+m, newline);
                                                        color(normal);
                                                    }
                                                  break;
                                                }
                                                if (xlastbuffer[m] >= '0' && xlastbuffer[m] <= '9' && xlastbuffer[m+1] == 0x00) {
                                                    asciivalue[i] = (char) xlastbuffer[m];
                                                    i++;
                                                }
                                                else {
                                                    if (debug || testing) {
                                                        color(red);
                                                        printf("PARSING ERROR: systemLinkPlayersMax contains a value other than 0-9 (%s)%s", xlastbuffer+m, newline);
                                                        color(normal);
                                                    }
                                                  break;
                                                }
                                            }
                                        }
                                    }
                                    else if (memcmp(xlastbuffer+m, livePlayersMax, 4) == 0) {
                                        if (memcmp(xlastbuffer+m, livePlayersMax, 32) == 0) {
                                            memset(asciivalue, 0, 6);
                                            i = 0;
                                            for (m+=32;m<n;m+=2) {
                                                if (memcmp(xlastbuffer+m, quotemark, 2) == 0) {
                                                    if (i == 0) {
                                                        // no chars (i.e. livePlayersMax="")
                                                        if (debug || testing) {
                                                            color(red);
                                                            printf("PARSING ERROR: no chars for livePlayersMax%s", newline);
                                                            color(normal);
                                                        }
                                                    }
                                                    else {
                                                        l = strtol(asciivalue, NULL, 10);
                                                        if (l >= 0 && l <= 65535) {
                                                            // save value
                                                            resource.liveplayersmax = (unsigned short) l;
                                                        }
                                                        else if (debug || testing) {
                                                            color(red);
                                                            printf("PARSING ERROR: livePlayersMax (%ld) is not 0-65535%s", l, newline);
                                                            color(normal);
                                                        }
                                                    }
                                                  break;
                                                }
                                                if (i > 4) {
                                                    // max 5 chars (i=0-4) for 2 byte value (0-65535)
                                                    if (debug || testing) {
                                                        color(red);
                                                        printf("PARSING ERROR: too many chars for livePlayersMax asciivalue (%s) next char(s): %s%s", asciivalue, xlastbuffer+m, newline);
                                                        color(normal);
                                                    }
                                                  break;
                                                }
                                                if (xlastbuffer[m] >= '0' && xlastbuffer[m] <= '9' && xlastbuffer[m+1] == 0x00) {
                                                    asciivalue[i] = (char) xlastbuffer[m];
                                                    i++;
                                                }
                                                else {
                                                    if (debug || testing) {
                                                        color(red);
                                                        printf("PARSING ERROR: livePlayersMax contains a value other than 0-9 (%s)%s", xlastbuffer+m, newline);
                                                        color(normal);
                                                    }
                                                  break;
                                                }
                                            }
                                        }
                                    }
                                    else if (memcmp(xlastbuffer+m, publisherStringId, 4) == 0) {
                                        if (memcmp(xlastbuffer+m, publisherStringId, 38) == 0) {
                                            memset(asciivalue, 0, 6);
                                            i = 0;
                                            for (m+=38;m<n;m+=2) {
                                                if (memcmp(xlastbuffer+m, quotemark, 2) == 0) {
                                                    if (i == 0) {
                                                        // no chars (i.e. publisherStringId="")
                                                        if (debug || testing) {
                                                            color(red);
                                                            printf("PARSING ERROR: no chars for publisherStringId%s", newline);
                                                            color(normal);
                                                        }
                                                    }
                                                    else {
                                                        l = strtol(asciivalue, NULL, 10);
                                                        if (l >= 0 && l <= 65535) {
                                                            // save value
                                                            resource.publisherstringid = (unsigned short) l;
                                                        }
                                                        else if (debug || testing) {
                                                            color(red);
                                                            printf("PARSING ERROR: publisherStringId (%ld) is not 0-65535%s", l, newline);
                                                            color(normal);
                                                        }
                                                    }
                                                  break;
                                                }
                                                if (i > 4) {
                                                    // max 5 chars (i=0-4) for 2 byte stringid (0-65535)
                                                    if (debug || testing) {
                                                        color(red);
                                                        printf("PARSING ERROR: too many chars for publisherStringId asciivalue (%s) next char(s): %s%s", asciivalue, xlastbuffer+m, newline);
                                                        color(normal);
                                                    }
                                                  break;
                                                }
                                                if (xlastbuffer[m] >= '0' && xlastbuffer[m] <= '9' && xlastbuffer[m+1] == 0x00) {
                                                    asciivalue[i] = (char) xlastbuffer[m];
                                                    i++;
                                                }
                                                else {
                                                    if (debug || testing) {
                                                        color(red);
                                                        printf("PARSING ERROR: publisherStringId contains a value other than 0-9 (%s)%s", xlastbuffer+m, newline);
                                                        color(normal);
                                                    }
                                                  break;
                                                }
                                            }
                                        }
                                    }
                                    else if (memcmp(xlastbuffer+m, developerStringId, 4) == 0) {
                                        if (memcmp(xlastbuffer+m, developerStringId, 38) == 0) {
                                            memset(asciivalue, 0, 6);
                                            i = 0;
                                            for (m+=38;m<n;m+=2) {
                                                if (memcmp(xlastbuffer+m, quotemark, 2) == 0) {
                                                    if (i == 0) {
                                                        // no chars (i.e. developerStringId="")
                                                        if (debug || testing) {
                                                            color(red);
                                                            printf("PARSING ERROR: no chars for developerStringId%s", newline);
                                                            color(normal);
                                                        }
                                                    }
                                                    else {
                                                        l = strtol(asciivalue, NULL, 10);
                                                        if (l >= 0 && l <= 65535) {
                                                            // save value
                                                            resource.developerstringid = (unsigned short) l;
                                                        }
                                                        else if (debug || testing) {
                                                            color(red);
                                                            printf("PARSING ERROR: developerStringId (%ld) is not 0-65535%s", l, newline);
                                                            color(normal);
                                                        }
                                                    }
                                                  break;
                                                }
                                                if (i > 4) {
                                                    // max 5 chars (i=0-4) for 2 byte stringid (0-65535)
                                                    if (debug || testing) {
                                                        color(red);
                                                        printf("PARSING ERROR: too many chars for developerStringId asciivalue (%s) next char(s): %s%s", asciivalue, xlastbuffer+m, newline);
                                                        color(normal);
                                                    }
                                                  break;
                                                }
                                                if (xlastbuffer[m] >= '0' && xlastbuffer[m] <= '9' && xlastbuffer[m+1] == 0x00) {
                                                    asciivalue[i] = (char) xlastbuffer[m];
                                                    i++;
                                                }
                                                else {
                                                    if (debug || testing) {
                                                        color(red);
                                                        printf("PARSING ERROR: developerStringId contains a value other than 0-9 (%s)%s", xlastbuffer+m, newline);
                                                        color(normal);
                                                    }
                                                  break;
                                                }
                                            }
                                        }
                                    }
                                    else if (memcmp(xlastbuffer+m, sellTextStringId, 4) == 0) {
                                        if (memcmp(xlastbuffer+m, sellTextStringId, 36) == 0) {
                                            memset(asciivalue, 0, 6);
                                            i = 0;
                                            for (m+=36;m<n;m+=2) {
                                                if (memcmp(xlastbuffer+m, quotemark, 2) == 0) {
                                                    if (i == 0) {
                                                        // no chars (i.e. sellTextStringId="")
                                                        if (debug || testing) {
                                                            color(red);
                                                            printf("PARSING ERROR: no chars for sellTextStringId%s", newline);
                                                            color(normal);
                                                        }
                                                    }
                                                    else {
                                                        l = strtol(asciivalue, NULL, 10);
                                                        if (l >= 0 && l <= 65535) {
                                                            // save value
                                                            resource.selltextstringid = (unsigned short) l;
                                                        }
                                                        else if (debug || testing) {
                                                            color(red);
                                                            printf("PARSING ERROR: sellTextStringId (%ld) is not 0-65535%s", l, newline);
                                                            color(normal);
                                                        }
                                                    }
                                                  break;
                                                }
                                                if (i > 4) {
                                                    // max 5 chars (i=0-4) for 2 byte stringid (0-65535)
                                                    if (debug || testing) {
                                                        color(red);
                                                        printf("PARSING ERROR: too many chars for sellTextStringId asciivalue (%s) next char(s): %s%s", asciivalue, xlastbuffer+m, newline);
                                                        color(normal);
                                                    }
                                                  break;
                                                }
                                                if (xlastbuffer[m] >= '0' && xlastbuffer[m] <= '9' && xlastbuffer[m+1] == 0x00) {
                                                    asciivalue[i] = (char) xlastbuffer[m];
                                                    i++;
                                                }
                                                else {
                                                    if (debug || testing) {
                                                        color(red);
                                                        printf("PARSING ERROR: sellTextStringId contains a value other than 0-9 (%s)%s", xlastbuffer+m, newline);
                                                        color(normal);
                                                    }
                                                  break;
                                                }
                                            }
                                        }
                                    }
                                    else if (memcmp(xlastbuffer+m, genreTextStringId, 4) == 0) {
                                        if (memcmp(xlastbuffer+m, genreTextStringId, 38) == 0) {
                                            memset(asciivalue, 0, 6);
                                            i = 0;
                                            for (m+=38;m<n;m+=2) {
                                                if (memcmp(xlastbuffer+m, quotemark, 2) == 0) {
                                                    if (i == 0) {
                                                        // no chars (i.e. genreTextStringId="")
                                                        if (debug || testing) {
                                                            color(red);
                                                            printf("PARSING ERROR: no chars for genreTextStringId%s", newline);
                                                            color(normal);
                                                        }
                                                    }
                                                    else {
                                                        l = strtol(asciivalue, NULL, 10);
                                                        if (l >= 0 && l <= 65535) {
                                                            // save value
                                                            resource.genretextstringid = (unsigned short) l;
                                                        }
                                                        else if (debug || testing) {
                                                            color(red);
                                                            printf("PARSING ERROR: genreTextStringId (%ld) is not 0-65535%s", l, newline);
                                                            color(normal);
                                                        }
                                                    }
                                                  break;
                                                }
                                                if (i > 4) {
                                                    // max 5 chars (i=0-4) for 2 byte stringid (0-65535)
                                                    if (debug || testing) {
                                                        color(red);
                                                        printf("PARSING ERROR: too many chars for genreTextStringId asciivalue (%s) next char(s): %s%s", asciivalue, xlastbuffer+m, newline);
                                                        color(normal);
                                                    }
                                                  break;
                                                }
                                                if (xlastbuffer[m] >= '0' && xlastbuffer[m] <= '9' && xlastbuffer[m+1] == 0x00) {
                                                    asciivalue[i] = (char) xlastbuffer[m];
                                                    i++;
                                                }
                                                else {
                                                    if (debug || testing) {
                                                        color(red);
                                                        printf("PARSING ERROR: genreTextStringId contains a value other than 0-9 (%s)%s", xlastbuffer+m, newline);
                                                        color(normal);
                                                    }
                                                  break;
                                                }
                                            }
                                        }
                                    }
                                }
                                for (n+=2;n<realxlastsize-42;n+=2) {
                                    if (memcmp(xlastbuffer+n, EndProductInformation, 4) == 0) {
                                        if (memcmp(xlastbuffer+n, EndProductInformation, 42) == 0) break;
                                    }
                                }
                                for (m+=2;m<n;m+=2) {
                                    if (memcmp(xlastbuffer+m, Featurename, 30) == 0) {
                                        m+=30;
                                        if (!resource.camera && memcmp(xlastbuffer+m, camera, 14) == 0) {
                                            if (memcmp(xlastbuffer+m+14, enabledtrue, 34) == 0) {
                                                resource.camera = true;
                                                resource.foundfeatures = true;
                                                m+=48;
                                            }
                                        }
                                        else if (!resource.coop && memcmp(xlastbuffer+m, coop, 10) == 0) {
                                            if (memcmp(xlastbuffer+m+10, enabledtrue, 34) == 0) {
                                                resource.coop = true;
                                                resource.foundfeatures = true;
                                                m+=44;
                                            }
                                        }
                                        else if (!resource.customsoundtrack && memcmp(xlastbuffer+m, customsoundtrack, 34) == 0) {
                                            if (memcmp(xlastbuffer+m+34, enabledtrue, 34) == 0) {
                                                resource.customsoundtrack = true;
                                                resource.foundfeatures = true;
                                                m+=68;
                                            }
                                        }
                                        else if (!resource.dolby51 && memcmp(xlastbuffer+m, dolby51, 16) == 0) {
                                            if (memcmp(xlastbuffer+m+16, enabledtrue, 34) == 0) {
                                                resource.dolby51 = true;
                                                resource.foundfeatures = true;
                                                m+=50;
                                            }
                                        }
                                        else if (!resource.harddriveenhanced && memcmp(xlastbuffer+m, harddriveenhanced, 36) == 0) {
                                            if (memcmp(xlastbuffer+m+36, enabledtrue, 34) == 0) {
                                                resource.harddriveenhanced = true;
                                                resource.foundfeatures = true;
                                                m+=70;
                                            }
                                        }
                                        else if (!resource.harddriverequired && memcmp(xlastbuffer+m, harddriverequired, 36) == 0) {
                                            if (memcmp(xlastbuffer+m+36, enabledtrue, 34) == 0) {
                                                resource.harddriverequired = true;
                                                resource.foundfeatures = true;
                                                m+=70;
                                            }
                                        }
                                        else if (!resource.liveaware && memcmp(xlastbuffer+m, liveaware, 20) == 0) {
                                            if (memcmp(xlastbuffer+m+20, enabledtrue, 34) == 0) {
                                                resource.liveaware = true;
                                                m+=54;
                                            }
                                        }
                                        else if (!resource.liveclans && memcmp(xlastbuffer+m, liveclans, 20) == 0) {
                                            if (memcmp(xlastbuffer+m+20, enabledtrue, 34) == 0) {
                                                resource.liveclans = true;
                                                resource.foundfeatures = true;
                                                m+=54;
                                            }
                                        }
                                        else if (!resource.livecoop && memcmp(xlastbuffer+m, livecoop, 18) == 0) {
                                            if (memcmp(xlastbuffer+m+18, enabledtrue, 34) == 0) {
                                                resource.livecoop = true;
                                                resource.foundfeatures = true;
                                                m+=52;
                                            }
                                        }
                                        else if (!resource.livedownloads && memcmp(xlastbuffer+m, livedownloads, 28) == 0) {
                                            if (memcmp(xlastbuffer+m+28, enabledtrue, 34) == 0) {
                                                resource.livedownloads = true;
                                                resource.foundfeatures = true;
                                                m+=62;
                                            }
                                        }
                                        else if (!resource.livefriends && memcmp(xlastbuffer+m, livefriends, 24) == 0) {
                                            if (memcmp(xlastbuffer+m+24, enabledtrue, 34) == 0) {
                                                resource.livefriends = true;
                                                m+=58;
                                            }
                                        }
                                        else if (!resource.livemessaging && memcmp(xlastbuffer+m, livemessaging, 28) == 0) {
                                            if (memcmp(xlastbuffer+m+28, enabledtrue, 34) == 0) {
                                                resource.livemessaging = true;
                                                m+=62;
                                            }
                                        }
                                        else if (!resource.livemultiplayer && memcmp(xlastbuffer+m, livemultiplayer, 32) == 0) {
                                            if (memcmp(xlastbuffer+m+32, enabledtrue, 34) == 0) {
                                                resource.livemultiplayer = true;
                                                resource.foundfeatures = true;
                                                m+=66;
                                            }
                                        }
                                        else if (!resource.livescoreboard && memcmp(xlastbuffer+m, livescoreboard, 30) == 0) {
                                            if (memcmp(xlastbuffer+m+30, enabledtrue, 34) == 0) {
                                                resource.livescoreboard = true;
                                                resource.foundfeatures = true;
                                                m+=64;
                                            }
                                        }
                                        else if (!resource.liveserviceproxy && memcmp(xlastbuffer+m, liveserviceproxy, 34) == 0) {
                                            if (memcmp(xlastbuffer+m+34, enabledtrue, 34) == 0) {
                                                resource.liveserviceproxy = true;
                                                resource.foundfeatures = true;
                                                m+=68;
                                            }
                                        }
                                        else if (!resource.livestats && memcmp(xlastbuffer+m, livestats, 20) == 0) {
                                            if (memcmp(xlastbuffer+m+20, enabledtrue, 34) == 0) {
                                                resource.livestats = true;
                                                resource.foundfeatures = true;
                                                m+=54;
                                            }
                                        }
                                        else if (!resource.livetournaments && memcmp(xlastbuffer+m, livetournaments, 32) == 0) {
                                            if (memcmp(xlastbuffer+m+32, enabledtrue, 34) == 0) {
                                                resource.livetournaments = true;
                                                resource.foundfeatures = true;
                                                m+=66;
                                            }
                                        }
                                        else if (!resource.livevoice && memcmp(xlastbuffer+m, livevoice, 20) == 0) {
                                            if (memcmp(xlastbuffer+m+20, enabledtrue, 34) == 0) {
                                                resource.livevoice = true;
                                                resource.foundfeatures = true;
                                                m+=54;
                                            }
                                        }
                                        else if (!resource.livevoicemessaging && memcmp(xlastbuffer+m, livevoicemessaging, 38) == 0) {
                                            if (memcmp(xlastbuffer+m+38, enabledtrue, 34) == 0) {
                                                resource.livevoicemessaging = true;
                                                m+=72;
                                            }
                                        }
                                        else if (!resource.memoryunit && memcmp(xlastbuffer+m, memoryunit, 22) == 0) {
                                            if (memcmp(xlastbuffer+m+22, enabledtrue, 34) == 0) {
                                                resource.memoryunit = true;
                                                m+=56;
                                            }
                                        }
                                        else if (!resource.multiplayerversus && memcmp(xlastbuffer+m, multiplayerversus, 36) == 0) {
                                            if (memcmp(xlastbuffer+m+36, enabledtrue, 34) == 0) {
                                                resource.multiplayerversus = true;
                                                resource.foundfeatures = true;
                                                m+=70;
                                            }
                                        }
                                        else if (!resource.peripherals && memcmp(xlastbuffer+m, peripherals, 24) == 0) {
                                            if (memcmp(xlastbuffer+m+24, enabledtrue, 34) == 0) {
                                                resource.peripherals = true;
                                                resource.foundfeatures = true;
                                                m+=58;
                                            }
                                        }
                                        else if (!resource.premiumoffers && memcmp(xlastbuffer+m, premiumoffers, 28) == 0) {
                                            if (memcmp(xlastbuffer+m+28, enabledtrue, 34) == 0) {
                                                resource.premiumoffers = true;
                                                resource.foundfeatures = true;
                                                m+=62;
                                            }
                                        }
                                        else if (!resource.systemlink && memcmp(xlastbuffer+m, systemlink, 22) == 0) {
                                            if (memcmp(xlastbuffer+m+22, enabledtrue, 34) == 0) {
                                                resource.systemlink = true;
                                                resource.foundfeatures = true;
                                                m+=56;
                                            }
                                        }
                                        else if (!resource.usercreatablecontent && memcmp(xlastbuffer+m, usercreatablecontent, 42) == 0) {
                                            if (memcmp(xlastbuffer+m+42, enabledtrue, 34) == 0) {
                                                resource.usercreatablecontent = true;
                                                resource.foundfeatures = true;
                                                m+=76;
                                            }
                                        }
                                        else if (!resource.xbox2support && memcmp(xlastbuffer+m, xbox2support, 26) == 0) {
                                            if (memcmp(xlastbuffer+m+26, enabledtrue, 34) == 0) {
                                                resource.xbox2support = true;
                                                m+=60;
                                            }
                                        }
                                        else if (!resource.xboxsupport && memcmp(xlastbuffer+m, xboxsupport, 24) == 0) {
                                            if (memcmp(xlastbuffer+m+24, enabledtrue, 34) == 0) {
                                                resource.xboxsupport = true;
                                                resource.foundfeatures = true;
                                                m+=58;
                                            }
                                        }
                                    }
                                }
                              break;
                            }
                        }
                    }
                }
            }
            else if (debug || testing) {
                color(red);
                printf("PARSING ERROR: realxlastsize (%lu) is less than 61%s", (unsigned long) realxlastsize, newline);
                color(normal);
            }
        }
        free(xlastbuffer);
    }
    skipxlastdecompression:
    
    if (extraverbose && resource.titletype != RESOURCE_TITLETYPE_INIT) {
        printf("%sTitle Type: %s ", sp5, sp10);
        if (resource.titletype == RESOURCE_TITLETYPE_SYSTEM)        printf("System Title (non-game title released by Microsoft)%s", newline);
        else if (resource.titletype == RESOURCE_TITLETYPE_FULL)     printf("Full Game Title%s", newline);
        else if (resource.titletype == RESOURCE_TITLETYPE_DEMO)     printf("Demo Title (demo game or a bonus disc)%s", newline);
        else if (resource.titletype == RESOURCE_TITLETYPE_DOWNLOAD) printf("Downloadable Game Title (Xbox Live Arcade game)%s", newline);
        else printf("Unknown (%lu)%s", resource.titletype, newline);
    }
    
    long long englishlanguageindex = -1LL, defaultlanguageindex = -1LL, userlanguageindex = -1LL;
    bool found_nonunicodelanguage = false;
    struct languagestruct language[resource.num_languages ? resource.num_languages : 1];  // don't declare a 0 size array
    if (resource.num_languages) {
        // save language entries and make sure they're valid before incrementing what will be the real number of languages (which is what really saves them)
        // also save indices for english and the default language as well as the user's preferred language if they entered one
        x = 0;
        for (m=0; m<resource.headerentries; m++) {
            n = m * 18 + 24;
            if (getwordmsb(resourcebuffer+n) == RESOURCE_HEADERTYPE_LANG) {
                language[x].langid = getuintmsb(resourcebuffer+n+6);
                language[x].offset = resource.bodyoffset + getuintmsb(resourcebuffer+n+10);
                language[x].size = getuintmsb(resourcebuffer+n+14);
                if ((language[x].offset + language[x].size) > resourcesize || language[x].size < 14 || memcmp(resourcebuffer+language[x].offset, "XSTR", 4) != 0) {
                    if (debug || testing) {
                        color(red);
                        printf("PARSING ERROR: invalid language entry: langid: %02lu, offset: 0x%08lX, size: %08lu%s", language[x].langid, language[x].offset, language[x].size, newline);
                        color(normal);
                    }
                }
                else {
                    if (userlangarg && language[x].langid == (unsigned long) userlang)
                        userlanguageindex = (long long) x;
                    if (language[x].langid == RESOURCE_LANG_ENGLISH)
                        englishlanguageindex = (long long) x;
                    else if (resource.defaultlanguage != RESOURCE_LANG_INIT && resource.defaultlanguage == language[x].langid)
                        defaultlanguageindex = (long long) x;
                    else if (!found_nonunicodelanguage && isanonunicodelanguage(language[x].langid))
                        found_nonunicodelanguage = true;
                    if (debug) printf("found a valid language entry: langid: %02lu, offset: 0x%08lX, size: %08lu%s", language[x].langid, language[x].offset, language[x].size, newline);
                    x++;
                }
            }
        }
        resource.num_languages = x;
    }
    
    if (extraverbose || resource.num_languages == 0) {
        // display supported languages
        if (resource.num_languages == 0) {
            if (verbose) printf("%s", sp5);
            color(yellow);
            printf("No valid language entries found%s", newline);
            color(normal);
        }
        else {
            spx = sp0;
            printf("%sDashboard Languages: %s ", sp5, sp1);
            for (m=0;m<resource.num_languages;m++) {
                if      (language[m].langid == RESOURCE_LANG_ENGLISH)             { printf("%sEnglish", spx); spx = sp28; }
                else if (language[m].langid == RESOURCE_LANG_JAPANESE)            { printf("%sJapanese", spx); spx = sp28; }
                else if (language[m].langid == RESOURCE_LANG_GERMAN)              { printf("%sGerman", spx); spx = sp28; }
                else if (language[m].langid == RESOURCE_LANG_FRENCH)              { printf("%sFrench", spx); spx = sp28; }
                else if (language[m].langid == RESOURCE_LANG_SPANISH)             { printf("%sSpanish", spx); spx = sp28; }
                else if (language[m].langid == RESOURCE_LANG_ITALIAN)             { printf("%sItalian", spx); spx = sp28; }
                else if (language[m].langid == RESOURCE_LANG_KOREAN)              { printf("%sKorean", spx); spx = sp28; }
                else if (language[m].langid == RESOURCE_LANG_CHINESE_TRADITIONAL) { printf("%sTraditional Chinese", spx); spx = sp28; }
                else if (language[m].langid == RESOURCE_LANG_PORTUGUESE)          { printf("%sPortuguese", spx); spx = sp28; }
                else if (language[m].langid == RESOURCE_LANG_CHINESE_SIMPLIFIED)  { printf("%sSimplified Chinese", spx); spx = sp28; }
                else if (language[m].langid == RESOURCE_LANG_POLISH)              { printf("%sPolish", spx); spx = sp28; }
                else if (language[m].langid == RESOURCE_LANG_RUSSIAN)             { printf("%sRussian", spx); spx = sp28; }
                else { printf("%sUnrecognized Language (%lu)", spx, language[m].langid); spx = sp28; }
                if (language[m].langid == resource.defaultlanguage) printf(" (default)%s", newline);
                else if (userlangarg && language[m].langid == (unsigned long) userlang) printf(" (preferred)%s", newline);
                else printf("%s", newline);
            }
        }
    }
    
    struct achievementstruct achievement[resource.num_achievements ? resource.num_achievements : 1];  // don't declare a 0 size array
    if (resource.num_achievements) {
        // get achievement entries
        for (m=0;m<resource.num_achievements;m++) {
            n = m * 36 + 14 + resource.XACH_offset;
            achievement[m].achievementid = getwordmsb(resourcebuffer+n);
            achievement[m].nameid =        getwordmsb(resourcebuffer+n+2);
            achievement[m].achievedid =    getwordmsb(resourcebuffer+n+4);
            achievement[m].unachievedid =  getwordmsb(resourcebuffer+n+6);
            achievement[m].imageid =       getuintmsb(resourcebuffer+n+8);
            achievement[m].gamerscore =    getwordmsb(resourcebuffer+n+12);
            achievement[m].type =          getwordmsb(resourcebuffer+n+18);
            resource.totalgamerscore += (unsigned long) achievement[m].gamerscore;
            if (debug) printf("achievement #%02lu: id: %04X, nameid: %04X, achievedid: %04X, unachievedid: %04X%s"
                                            "%s %s gamerscore: %03hu, imageid: %08lX, type: %04X%s",
                                            m + 1, achievement[m].achievementid, achievement[m].nameid, achievement[m].achievedid, achievement[m].unachievedid, newline,
                                            sp10, sp5, achievement[m].gamerscore, achievement[m].imageid, achievement[m].type, newline);
            if (getuintmsb(resourcebuffer+n+14) != 0L && (debug || testing)) {
                color(yellow);
                printf("PARSING ERROR?: found data between achievement gamerscore and type: %08lX%s", getuintmsb(resourcebuffer+n+14), newline);
                color(normal);
            }
        }
    }
    
    struct avatarawardstruct avataraward[resource.num_avatarawards ? resource.num_avatarawards : 1];  // don't declare a 0 size array
    if (resource.num_avatarawards) {
        // get avatar award entries
        for (m=0;m<resource.num_avatarawards;m++) {
            n = m * 36 + 14 + resource.XGAA_offset;
            avataraward[m].avatarawardid = getwordmsb(resourcebuffer+n+4);
            avataraward[m].nameid =        getwordmsb(resourcebuffer+n+16);
            avataraward[m].achievedid =    getwordmsb(resourcebuffer+n+18);
            avataraward[m].unachievedid =  getwordmsb(resourcebuffer+n+20);
            avataraward[m].imageid =       getuintmsb(resourcebuffer+n+24);
            if (debug) printf("avataraward #%02lu: id: %04X, nameid: %04X, achievedid: %04X, unachievedid: %04X%s"
                                            "%s %s imageid: %08lX%s",
                                            m + 1, avataraward[m].avatarawardid, avataraward[m].nameid, avataraward[m].achievedid, avataraward[m].unachievedid, newline,
                                            sp10, sp5, avataraward[m].imageid, newline);
        }
    }
    
    if (html && extractimages) {
        // need to convert backslashes to forward slashes for html source
        for (i=0;i<(int)strlen(imagedirpath);i++) {
            if (imagedirpath[i] == '\\') imagedirpath[i] = '/';
        }
        printf("%s<img src=\"", sp5);
        if (embedimages) {
            for (m=0;m<resource.num_embedable;m++) {
                if (embedable_images[m].imageid == 0x8000) {
                    base64encode(resourcebuffer+embedable_images[m].offset, embedable_images[m].size, embeddedimagebuffer);
                    printf("data:image/png;base64,%s", embeddedimagebuffer);
                    break;
                }
            }
        }
        else printf("file:///%s%02X%02X%02X%02X-00008000.png", imagedirpath, titleid[0], titleid[1], titleid[2], titleid[3]);
        printf("\" alt=\"title icon\" width=64 height=64><br>\n");
    }
    long long displaylanguageindex = -1LL;
    if (resource.num_languages) {
        // select a language to use for strings (if the user has no preferred language or their language is not available we will prefer english,
        // otherwise the default, in case of no default try to use a non-unicode language, failing that use the first language entry)
        if (userlanguageindex != -1LL) displaylanguageindex = userlanguageindex;
        else if (englishlanguageindex != -1LL) displaylanguageindex = englishlanguageindex;
        else if (defaultlanguageindex != -1LL) displaylanguageindex = defaultlanguageindex;
        else if (found_nonunicodelanguage) {
            for (m=0;m<resource.num_languages;m++) {
                if (isanonunicodelanguage(language[m].langid)) {
                    displaylanguageindex = (long long) m;
                    break;
                }
            }
        }
        else displaylanguageindex = 0LL;
        if (debug) printf("displaylanguageindex: %"LL"d%s", displaylanguageindex, newline);
    }
    if (displaylanguageindex != -1LL) {
        // found a language to use so start parsing it
        long gamenameindex = -1, publisherindex = -1, developerindex = -1, selltextindex = -1, genreindex = -1;
        unsigned short s;
        unsigned short real_num_strings = 0;
        unsigned short num_strings = getwordmsb(resourcebuffer+language[displaylanguageindex].offset+12);
        struct { unsigned short stringid, length; unsigned long offset; }
        strings[num_strings];
        m = language[displaylanguageindex].offset+14;
        n = language[displaylanguageindex].offset + language[displaylanguageindex].size;
        for (s=0;s<num_strings;s++) {
            x = m + 4;
            if (x > n) break;
            strings[real_num_strings].stringid = getwordmsb(resourcebuffer+m);
            strings[real_num_strings].length = getwordmsb(resourcebuffer+m+2);
            strings[real_num_strings].offset = m + 4;
            if ((x + strings[real_num_strings].length) > n) break;
            if (strings[real_num_strings].stringid == 0x8000) gamenameindex = (long) real_num_strings;
            else if (strings[real_num_strings].stringid == resource.publisherstringid) publisherindex = (long) real_num_strings;
            else if (strings[real_num_strings].stringid == resource.developerstringid) developerindex = (long) real_num_strings;
            else if (strings[real_num_strings].stringid == resource.selltextstringid)  selltextindex =  (long) real_num_strings;
            else if (strings[real_num_strings].stringid == resource.genretextstringid) genreindex =     (long) real_num_strings;
            m += strings[real_num_strings].length + 4;
            // trim trailing newlines/spaces
            while (strings[real_num_strings].length) {
                if (resourcebuffer[strings[real_num_strings].offset+strings[real_num_strings].length-1] == 0x0D ||
                    resourcebuffer[strings[real_num_strings].offset+strings[real_num_strings].length-1] == 0x0A ||
                    resourcebuffer[strings[real_num_strings].offset+strings[real_num_strings].length-1] == 0x20) strings[real_num_strings].length--;
                else break;
            }
            real_num_strings++;
        }
        if (debug) {
            printf("strings:%s", newline);
            for (s=0;s<real_num_strings;s++) {
                printf("id=%04X, len=%04u: %.*s%s", strings[s].stringid, strings[s].length, strings[s].length, resourcebuffer+strings[s].offset, newline);
            }
        }
        if (gamenameindex > -1) {
            if (verbose) printf("%s", sp5);
            printf("Game Name: ");
            if (verbose) printf("%s ", sp11);
            startunicode();
            color(white);
            if (html) printf("<b>");
            if (verbose) printlongunicodestring(resourcebuffer+strings[gamenameindex].offset, (int) strings[gamenameindex].length, 51, true, sp28);
            else printlongunicodestring(resourcebuffer+strings[gamenameindex].offset, (int) strings[gamenameindex].length, 68, true, sp11);
            if (html) printf("</b>");
            endunicode();
            color(normal);
            printf("%s", newline);
        }
        if (verbose) {
            if (developerindex > -1) {
                printf("%sDeveloper: %s ", sp5, sp11);
                startunicode();
                printlongunicodestring(resourcebuffer+strings[developerindex].offset, (int) strings[developerindex].length, 51, true, sp28);
                endunicode();
                color(normal);
                printf("%s", newline);
            }
            if (publisherindex > -1) {
                printf("%sPublisher: %s ", sp5, sp11);
                startunicode();
                printlongunicodestring(resourcebuffer+strings[publisherindex].offset, (int) strings[publisherindex].length, 51, true, sp28);
                endunicode();
                color(normal);
                printf("%s", newline);
            }
            if (genreindex > -1) {
                printf("%sGenre: %s %s ", sp5, sp7, sp7);
                startunicode();
                printlongunicodestring(resourcebuffer+strings[genreindex].offset, (int) strings[genreindex].length, 51, true, sp28);
                endunicode();
                color(normal);
                printf("%s", newline);
            }
            if (extraverbose && selltextindex > -1) {
                printf("%sDescription: ", sp5);
                startunicode();
                printlongunicodestring(resourcebuffer+strings[selltextindex].offset, (int) strings[selltextindex].length, 61, true, sp18);
                endunicode();
                color(normal);
                printf("%s", newline);
            }
        }
        if (extraverbose && resource.foundfeatures) {
            spx = sp0;
            printf("%sFeatures: %s ", sp5, sp12);
            if (resource.offlineplayersmax) {
                if (resource.offlineplayersmax == 1) printf("%sOffline Players: 1%s", spx, newline);
                else printf("%sOffline Players: 1-%u%s", spx, resource.offlineplayersmax, newline);
                spx = sp28;
            }
            if (resource.coop) {
                printf("%sOffline Co-op%s", spx, newline);
                spx = sp28;
            }
            if (resource.systemlink) {
                printf("%sSystem Link", spx);
                if (resource.systemlinkplayersmax >= 2) {
                    if (resource.systemlinkplayersmax == 2) printf(" Players: 2%s", newline);
                    else printf(" Players: 2-%u%s", resource.systemlinkplayersmax, newline);
                }
                else printf("%s", newline);
                spx = sp28;
            }
            if (resource.camera) {
                printf("%sCamera%s", spx, newline);
                spx = sp28;
            }
            if (resource.customsoundtrack) {
                printf("%sCustom Soundtracks%s", spx, newline);
                spx = sp28;
            }
            if (resource.dolby51) {
                printf("%sDolby 5.1%s", spx, newline);
                spx = sp28;
            }
            if (resource.harddriveenhanced) {
                printf("%sHard Drive Enhanced%s", spx, newline);
                spx = sp28;
            }
            if (resource.harddriverequired) {
                printf("%sHard Drive Required%s", spx, newline);
                spx = sp28;
            }
            if (resource.multiplayerversus) {
                printf("%sMultiplayer Versus%s", spx, newline);
                spx = sp28;
            }
            if (resource.peripherals) {
                printf("%sSpecial Peripherals%s", spx, newline);
                spx = sp28;
            }
            if (resource.premiumoffers) {
                printf("%sPremium Offers%s", spx, newline);
                spx = sp28;
            }
            if (resource.usercreatablecontent) {
                printf("%sUser Creatable Content%s", spx, newline);
                spx = sp28;
            }
            if (resource.xboxsupport) {
                printf("%sOriginal Xbox Support%s", spx, newline);
                spx = sp28;
            }
            if (resource.livemultiplayer) {
                printf("%sXbox Live Multiplayer", spx);
                if (resource.liveplayersmax >= 2) {
                    if (resource.liveplayersmax == 2) printf(": 2%s", newline);
                    else printf(": 2-%u%s", resource.liveplayersmax, newline);
                }
                else printf("%s", newline);
                spx = sp28;
            }
            if (resource.livecoop) {
                printf("%sXbox Live Co-op%s", spx, newline);
                spx = sp28;
            }
            if (resource.liveclans) {
                printf("%sXbox Live Clans%s", spx, newline);
                spx = sp28;
            }
            if (resource.livedownloads) {
                printf("%sXbox Live Downloads%s", spx, newline);
                spx = sp28;
            }
            if (resource.livescoreboard) {
                printf("%sXbox Live Scoreboard%s", spx, newline);
                spx = sp28;
            }
            if (resource.livestats) {
                printf("%sXbox Live Stats%s", spx, newline);
                spx = sp28;
            }
            if (resource.livetournaments) {
                printf("%sXbox Live Tournaments%s", spx, newline);
                spx = sp28;
            }
            if (resource.liveserviceproxy) {
                printf("%sXbox Live Service Proxy%s", spx, newline);
                spx = sp28;
            }
            if (resource.livevoice) {
                printf("%sXbox Live Voice%s", spx, newline);
                spx = sp28;
            }
            if (spx == sp0) printf("No Features%s", newline);  // this shouldn't happen because resource.foundfeatures should only be set to true if one of the above features is enabled, but this will make sure a newline gets printed in case we fucked up anyway
        }
        if (verbose || showavatarawards) {
            if (resource.num_avatarawards) {
                if (verbose) printf("%s", sp5);
                printf("Avatar Awards: ");
                if (verbose) printf("%s ", sp7);
                printf("%lu Avatar Awards%s", resource.num_avatarawards, newline);
            }
            else {
                if (verbose) printf("%s", sp5);
                printf("No Avatar Awards%s", newline);
            }
        }
        if (showavatarawards && resource.num_avatarawards) {
            if (html) printf("</span>\n<table border=\"0\" cellpadding=\"6\">\n");
            // display avatar awards
            for (m=0;m<resource.num_avatarawards;m++) {
                if (html) {
                    printf("<tr id=a_r%02lu style=\"cursor:pointer\" onClick=\"sh('a_u%02lu','a_a%02lu','a_r%02lu','a_c%02lu')\">", m+1, m+1, m+1, m+1, m+1);
                    if (extractimages) {
                        if (avataraward[m].imageid == 0xFFFFFFFFL) printf("<td class=sp><!-- no img -->&nbsp;&nbsp;</td>");  // in case we need to figure out why no image is displayed
                        else {
                            printf("<td><img src=\"");
                            if (embedimages) printf("data:image/png;base64,"
                                "iVBORw0KGgoAAAANSUhEUgAAACAAAABACAYAAAB7jnWuAAAC8UlEQVR42uxYz2sTQRR+m22iYs2h"
                                "RQxEPBUUwatQMKcQ8SCKRfHHSdBTQejFP6AntVIQFIonQTBQlIIiCErxoIIIUbQoBkVRbA3Gn22a"
                                "6Kbt+M3b52bTCNrsJrnMwLfzdmZ3v2/ee/MjsZRS1MkSoQ4XI8AIMAKMACPACDACjACLAh6IVMAP"
                                "RFo7OmtTRwSAOAncol90rO0CQJxB9QzYRQ+p0FYBIN+H6ibQQ+9wLbRRAMiPoLoKxLjhDX/9a1sE"
                                "yMgvAzY3fAFmWcqPf73bFQL5tjpyXWaA1Wx1t1pADzDRQPQdWMPW+laH4BLQV9fyAPgmQ+um7a1c"
                                "CY+KgFoZB15IMDYAccqrw2pLKzyQBEbrWnT+PwbmgU/AFGUpRwNBckBPJ4fmcF3X0Dcq8XfLbVl6"
                                "lhgl+PW4Oq/Gm90LdEKdAyY5mZyG/n7goHf3REbucEALcH/qf8n/5oH9Qp7kuwpl8MSdZc+MeNYH"
                                "4L5kPdF7CEirEfW6md1Q71rXJZJJX/8wLdY9vwfYwZZ2911gmu+KTH5mZeR/BJwEnsvHa2WWXdqP"
                                "WZKRFp3bp7x+7Zc8W3q1S6vTKyd3BcxQAllrM+GcYAH46flnWJ7Va/1WtvQW49I5EDkA8qnmV9IJ"
                                "viZoFV1kL2jSBNArGRJle6fkhisgK8lHNKjOqrFgJ6Ior9t6THuBE2ipUFk8oEQI0RWPXCfdK7bG"
                                "BCEcyWyZ9XG6gEmYgl2EDP/24q7pL4EcOz4H0UO0NoxTcVRGaoucGFOkqIpppbfVErAoo74HfOak"
                                "O4BZ4HCuBCxdiL8b55jUbmbkAS1iEiebPj5WPAU+8vQbhNC3ciQOyQMxgeWDDQ/YlIZd5PslJryG"
                                "N7L8jhXWD5NeOTzUk7uweHXbDcIKZkkJIodQ12aHHUYINsoRat6XC7ZPTJkeIeqHEPHN8MI0ty34"
                                "hAYWEPd9rLzME1VZEQt0g+uqtEe8n0XBz5Pmz2ojwAgwAowAI8AIMAKMgE4L+C3AAO1xu7ChkgSm"
                                "AAAAAElFTkSuQmCC");
                            else printf("file:///%schecked.png", imagedirpath);
                            printf("\" alt=\"checked\" width=32 height=64 id=a_c%02lu style=\"visibility:hidden\"><img src=\"", m+1);
                            if (embedimages) {
                                for (x=0;x<resource.num_embedable;x++) {
                                    if (embedable_images[x].imageid == avataraward[m].imageid) {
                                        base64encode(resourcebuffer+embedable_images[x].offset, embedable_images[x].size, embeddedimagebuffer);
                                        printf("data:image/png;base64,%s", embeddedimagebuffer);
                                      break;
                                    }
                                }
                            }
                            else printf("file:///%s%02X%02X%02X%02X-%08lX.png", imagedirpath, titleid[0], titleid[1], titleid[2], titleid[3], avataraward[m].imageid);
                            printf("\" alt=\"AA icon %02lu\" width=64 height=64></td>", m+1);
                        }
                    }
                    else printf("<td class=sp>&nbsp;&nbsp;</td>");
                    printf("<td><span class=achtitle>");
                }
                else {
                    color(white);
                    printf("     ");
                }
                // print avataraward number
                printf("%02lu. ", m+1);
                // print avataraward name
                i = 0;
                for (n=0;n<real_num_strings;n++) {
                    if (strings[n].stringid == avataraward[m].nameid) {
                        i = printlongunicodestring(resourcebuffer+strings[n].offset, (int) strings[n].length, 43, true, html ? sp0 : sp9);
                      break;
                    }
                }
                if (html) {
                    // print unachieved description
                    printf("</span><br><span class=normal_u id=a_u%02lu>", m+1);
                    // use achieved text if the string id is 0xFFFF (might be a secret avatar award with no unachieved text)
                    if (avataraward[m].unachievedid == 0xFFFF) h = avataraward[m].achievedid;
                    else h = avataraward[m].unachievedid;
                    for (n=0;n<real_num_strings;n++) {
                        if (strings[n].stringid == h) {
                            printunicodestring(resourcebuffer+strings[n].offset, (int) strings[n].length, true, sp0);
                          break;
                        }
                    }
                    // print achieved description
                    printf("</span><span class=normal_u id=a_a%02lu style=\"display:none\">", m+1);
                    for (n=0;n<real_num_strings;n++) {
                        if (strings[n].stringid == avataraward[m].achievedid) {
                            printunicodestring(resourcebuffer+strings[n].offset, (int) strings[n].length, true, sp0);
                          break;
                        }
                    }
                    printf("</span></td></tr>\n");
                }
                else {
                    // print description
                    printf("%s%s", newline, sp9);
                    color(normal);
                    // use achieved text if the string id is 0xFFFF (might be a secret avatar award with no unachieved text)
                    if (avataraward[m].unachievedid == 0xFFFF) h = avataraward[m].achievedid;
                    else h = avataraward[m].unachievedid;
                    for (n=0;n<real_num_strings;n++) {
                        if (strings[n].stringid == h) {
                            printlongunicodestring(resourcebuffer+strings[n].offset, (int) strings[n].length, 70, true, sp9);
                          break;
                        }
                    }
                    printf("%s", newline);
                }
            }
            if (html) printf("</table>\n"
                             "<span class=normal>");
        }
        if (verbose || showachievements) {
            if (resource.num_achievements) {
                if (verbose) printf("%s", sp5);
                printf("Achievements: ");
                if (verbose) printf("%s ", sp8);
                printf("%lu Achievements totaling %lu Gamerscore%s", resource.num_achievements, resource.totalgamerscore, newline);
            }
            else {
                if (verbose) printf("%s", sp5);
                printf("No Achievements%s", newline);
            }
        }
        if (showachievements && resource.num_achievements) {
            if (html) printf("</span>\n<table border=\"0\" cellpadding=\"6\">\n");
            // display achievements
            for (m=0;m<resource.num_achievements;m++) {
                if (html) {
                    printf("<tr id=r%02lu style=\"cursor:pointer\" onClick=\"sh('u%02lu','a%02lu','r%02lu','c%02lu')\">", m+1, m+1, m+1, m+1, m+1);
                    if (extractimages) {
                        if (hidesecretachievements && (achievement[m].type >= 1 && achievement[m].type <=7)) {
                            printf("<td><img src=\"");
                            if (embedimages) printf("data:image/png;base64,"
                                "iVBORw0KGgoAAAANSUhEUgAAACAAAABACAYAAAB7jnWuAAAC8UlEQVR42uxYz2sTQRR+m22iYs2h"
                                "RQxEPBUUwatQMKcQ8SCKRfHHSdBTQejFP6AntVIQFIonQTBQlIIiCErxoIIIUbQoBkVRbA3Gn22a"
                                "6Kbt+M3b52bTCNrsJrnMwLfzdmZ3v2/ee/MjsZRS1MkSoQ4XI8AIMAKMACPACDACjACLAh6IVMAP"
                                "RFo7OmtTRwSAOAncol90rO0CQJxB9QzYRQ+p0FYBIN+H6ibQQ+9wLbRRAMiPoLoKxLjhDX/9a1sE"
                                "yMgvAzY3fAFmWcqPf73bFQL5tjpyXWaA1Wx1t1pADzDRQPQdWMPW+laH4BLQV9fyAPgmQ+um7a1c"
                                "CY+KgFoZB15IMDYAccqrw2pLKzyQBEbrWnT+PwbmgU/AFGUpRwNBckBPJ4fmcF3X0Dcq8XfLbVl6"
                                "lhgl+PW4Oq/Gm90LdEKdAyY5mZyG/n7goHf3REbucEALcH/qf8n/5oH9Qp7kuwpl8MSdZc+MeNYH"
                                "4L5kPdF7CEirEfW6md1Q71rXJZJJX/8wLdY9vwfYwZZ2911gmu+KTH5mZeR/BJwEnsvHa2WWXdqP"
                                "WZKRFp3bp7x+7Zc8W3q1S6vTKyd3BcxQAllrM+GcYAH46flnWJ7Va/1WtvQW49I5EDkA8qnmV9IJ"
                                "viZoFV1kL2jSBNArGRJle6fkhisgK8lHNKjOqrFgJ6Ior9t6THuBE2ipUFk8oEQI0RWPXCfdK7bG"
                                "BCEcyWyZ9XG6gEmYgl2EDP/24q7pL4EcOz4H0UO0NoxTcVRGaoucGFOkqIpppbfVErAoo74HfOak"
                                "O4BZ4HCuBCxdiL8b55jUbmbkAS1iEiebPj5WPAU+8vQbhNC3ciQOyQMxgeWDDQ/YlIZd5PslJryG"
                                "N7L8jhXWD5NeOTzUk7uweHXbDcIKZkkJIodQ12aHHUYINsoRat6XC7ZPTJkeIeqHEPHN8MI0ty34"
                                "hAYWEPd9rLzME1VZEQt0g+uqtEe8n0XBz5Pmz2ojwAgwAowAI8AIMAKMgE4L+C3AAO1xu7ChkgSm"
                                "AAAAAElFTkSuQmCC");
                            else printf("file:///%schecked.png", imagedirpath);
                            printf("\" alt=\"checked\" width=32 height=64 id=c%02lu style=\"visibility:hidden\"><img src=\"", m+1);
                            if (embedimages) printf("data:image/png;base64,"
                                "iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAMAAACdt4HsAAAAGFBMVEX+/v6Pj486OjoyMjIlJSVb"
                                "W1vHx8fe3t6Fwq2EAAABCElEQVR42uyX6xKEIAiFQUDf/43Xyku1aSzuzDY7nn9mfCkeqIDcmIBp"
                                "SI8BsE0VYMtfBbgJ+G+ARNkBGGBVQDEACGEnLx8DAhyFHwLO8VeEDoA8vEvOpdwGkJS9+7oUrwdw"
                                "Tr+LRV9g4LSAeHXT1miktYcmgNMJ4nZbXk8a3wOI/X7JnIdaQJxIjyQDIIWwE8R0Pzt1DupM7rnr"
                                "oHji5hhbjb/UBPYA7RdHaNpgD2jGS8fJGkA3XgEoFoIgZABUE+P1vBogbARgN14PaKVYC/B2gPhF"
                                "aAccasLiA+rGKwAI3SyqT2ECfgpYWjOW9mx2Ig84sa8JmICvAGj0c/9Rf65DgDHRS4ABAEawKhSQ"
                                "sN92AAAAAElFTkSuQmCC");
                            else printf("file:///%ssecret.png", imagedirpath);
                            printf("\" alt=\"Secret Ach\" width=64 height=64></td>");
                        }
                        else {
                            if (achievement[m].imageid == 0xFFFFFFFFL) printf("<td class=sp><!-- no img -->&nbsp;&nbsp;</td>");  // in case we need to figure out why no image is displayed
                            else {
                                printf("<td><img src=\"");
                                if (embedimages) printf("data:image/png;base64,"
                                    "iVBORw0KGgoAAAANSUhEUgAAACAAAABACAYAAAB7jnWuAAAC8UlEQVR42uxYz2sTQRR+m22iYs2h"
                                    "RQxEPBUUwatQMKcQ8SCKRfHHSdBTQejFP6AntVIQFIonQTBQlIIiCErxoIIIUbQoBkVRbA3Gn22a"
                                    "6Kbt+M3b52bTCNrsJrnMwLfzdmZ3v2/ee/MjsZRS1MkSoQ4XI8AIMAKMACPACDACjACLAh6IVMAP"
                                    "RFo7OmtTRwSAOAncol90rO0CQJxB9QzYRQ+p0FYBIN+H6ibQQ+9wLbRRAMiPoLoKxLjhDX/9a1sE"
                                    "yMgvAzY3fAFmWcqPf73bFQL5tjpyXWaA1Wx1t1pADzDRQPQdWMPW+laH4BLQV9fyAPgmQ+um7a1c"
                                    "CY+KgFoZB15IMDYAccqrw2pLKzyQBEbrWnT+PwbmgU/AFGUpRwNBckBPJ4fmcF3X0Dcq8XfLbVl6"
                                    "lhgl+PW4Oq/Gm90LdEKdAyY5mZyG/n7goHf3REbucEALcH/qf8n/5oH9Qp7kuwpl8MSdZc+MeNYH"
                                    "4L5kPdF7CEirEfW6md1Q71rXJZJJX/8wLdY9vwfYwZZ2911gmu+KTH5mZeR/BJwEnsvHa2WWXdqP"
                                    "WZKRFp3bp7x+7Zc8W3q1S6vTKyd3BcxQAllrM+GcYAH46flnWJ7Va/1WtvQW49I5EDkA8qnmV9IJ"
                                    "viZoFV1kL2jSBNArGRJle6fkhisgK8lHNKjOqrFgJ6Ior9t6THuBE2ipUFk8oEQI0RWPXCfdK7bG"
                                    "BCEcyWyZ9XG6gEmYgl2EDP/24q7pL4EcOz4H0UO0NoxTcVRGaoucGFOkqIpppbfVErAoo74HfOak"
                                    "O4BZ4HCuBCxdiL8b55jUbmbkAS1iEiebPj5WPAU+8vQbhNC3ciQOyQMxgeWDDQ/YlIZd5PslJryG"
                                    "N7L8jhXWD5NeOTzUk7uweHXbDcIKZkkJIodQ12aHHUYINsoRat6XC7ZPTJkeIeqHEPHN8MI0ty34"
                                    "hAYWEPd9rLzME1VZEQt0g+uqtEe8n0XBz5Pmz2ojwAgwAowAI8AIMAKMgE4L+C3AAO1xu7ChkgSm"
                                    "AAAAAElFTkSuQmCC");
                                else printf("file:///%schecked.png", imagedirpath);
                                printf("\" alt=\"checked\" width=32 height=64 id=c%02lu style=\"visibility:hidden\"><img src=\"", m+1);
                                if (embedimages) {
                                    for (x=0;x<resource.num_embedable;x++) {
                                        if (embedable_images[x].imageid == achievement[m].imageid) {
                                            base64encode(resourcebuffer+embedable_images[x].offset, embedable_images[x].size, embeddedimagebuffer);
                                            printf("data:image/png;base64,%s", embeddedimagebuffer);
                                          break;
                                        }
                                    }
                                }
                                else printf("file:///%s%02X%02X%02X%02X-%08lX.png", imagedirpath, titleid[0], titleid[1], titleid[2], titleid[3], achievement[m].imageid);
                                printf("\" alt=\"Ach icon %02lu\" width=64 height=64></td>", m+1);
                            }
                        }
                    }
                    else printf("<td class=sp>&nbsp;&nbsp;</td>");
                    printf("<td><span class=achtitle>");
                }
                else {
                    color(white);
                    printf("     ");
                }
                // print achievement number
                printf("%02lu. ", m+1);
                // print achievement name
                i = 0;
                if (hidesecretachievements && (achievement[m].type >= 1 && achievement[m].type <=7)) {
                    i = printf("Secret Achievement");
                }
                else {
                    for (n=0;n<real_num_strings;n++) {
                        if (strings[n].stringid == achievement[m].nameid) {
                            i = printlongunicodestring(resourcebuffer+strings[n].offset, (int) strings[n].length, 43, true, html ? sp0 : sp9);
                          break;
                        }
                    }
                }
                if (html) {
                    // print unachieved description
                    printf("</span><br><span class=normal_u id=u%02lu>", m+1);
                    if (hidesecretachievements && (achievement[m].type >= 1 && achievement[m].type <=7)) {
                        printf("Continue playing to unlock this secret achievement.");
                    }
                    else {
                        // use achieved text if this is a secret achievement (unachieved text is optional and if not used the string id should be set to 0xFFFF,
                        // but sometimes it will be set to a valid string id and the text will be something we don't want like "???" or "<insert string text here>")
                        if (achievement[m].unachievedid == 0xFFFF || (achievement[m].type >= 1 && achievement[m].type <= 7)) h = achievement[m].achievedid;
                        else h = achievement[m].unachievedid;
                        for (n=0;n<real_num_strings;n++) {
                            if (strings[n].stringid == h) {
                                printunicodestring(resourcebuffer+strings[n].offset, (int) strings[n].length, true, sp0);
                              break;
                            }
                        }
                    }
                    // print achieved description
                    printf("</span><span class=normal_u id=a%02lu style=\"display:none\">", m+1);
                    if (hidesecretachievements && (achievement[m].type >= 1 && achievement[m].type <=7)) {
                        printf("Continue playing to unlock this secret achievement.");
                    }
                    else {
                        for (n=0;n<real_num_strings;n++) {
                            if (strings[n].stringid == achievement[m].achievedid) {
                                printunicodestring(resourcebuffer+strings[n].offset, (int) strings[n].length, true, sp0);
                              break;
                            }
                        }
                    }
                    printf("</span></td><td align=center><span class=white_u><b>");
                }
                else for (j=0;j<41-i;j++) printf(" ");
                // print gamerscore
                if (!html) {
                    if (achievement[m].gamerscore < 10) printf("%s", sp3);
                    else if (achievement[m].gamerscore < 100) printf("%s", sp2);
                    else if (achievement[m].gamerscore < 1000) printf("%s", sp1);
                }
                printf("%hu G", achievement[m].gamerscore);
                // print type
                if (html) printf("</b></span><br><span class=darkgray_u>");
                else {
                    printf(" %s", sp1);
                    color(darkgray);
                }
                if      (achievement[m].type == 1)  printf("Completion  [SECRET]");
                else if (achievement[m].type == 2)  printf("Leveling    [SECRET]");
                else if (achievement[m].type == 3)  printf("Unlock      [SECRET]");
                else if (achievement[m].type == 4)  printf("Event       [SECRET]");
                else if (achievement[m].type == 5)  printf("Tournament  [SECRET]");
                else if (achievement[m].type == 6)  printf("Checkpoint  [SECRET]");
                else if (achievement[m].type == 7)  printf("Other       [SECRET]");
                else if (achievement[m].type == 9)  printf("Completion");
                else if (achievement[m].type == 10) printf("Leveling");
                else if (achievement[m].type == 11) printf("Unlock");
                else if (achievement[m].type == 12) printf("Event");
                else if (achievement[m].type == 13) printf("Tournament");
                else if (achievement[m].type == 14) printf("Checkpoint");
                else if (achievement[m].type == 15) printf("Other");
                else                                printf("Unrecognized type");
                if (html) printf("</span></td></tr>\n");
                else {
                    // print description
                    printf("%s%s", newline, sp9);
                    color(normal);
                    if (hidesecretachievements && (achievement[m].type >= 1 && achievement[m].type <=7)) {
                        printf("Continue playing to unlock this secret achievement.");
                    }
                    else {
                        // use achieved text if this is a secret achievement (unachieved text is optional and if not used the string id should be set to 0xFFFF,
                        // but sometimes it will be set to a valid string id and the text will be something we don't want like "???" or "<insert string text here>")
                        if (achievement[m].unachievedid == 0xFFFF || (achievement[m].type >= 1 && achievement[m].type <= 7)) h = achievement[m].achievedid;
                        else h = achievement[m].unachievedid;
                        for (n=0;n<real_num_strings;n++) {
                            if (strings[n].stringid == h) {
                                printlongunicodestring(resourcebuffer+strings[n].offset, (int) strings[n].length, 70, true, sp9);
                              break;
                            }
                        }
                    }
                    printf("%s", newline);
                }
            }
            if (html) printf("</table>\n"
                             "<span class=normal>");
        }
    }
    else {
        // displaylanguageindex == -1LL (no language found so none of this has already been displayed)
        if (extraverbose && resource.foundfeatures) {
            spx = sp0;
            printf("%sFeatures: %s ", sp5, sp12);
            if (resource.offlineplayersmax) {
                if (resource.offlineplayersmax == 1) printf("%sOffline Players: 1%s", spx, newline);
                else printf("%sOffline Players: 1-%u%s", spx, resource.offlineplayersmax, newline);
                spx = sp28;
            }
            if (resource.coop) {
                printf("%sOffline Co-op%s", spx, newline);
                spx = sp28;
            }
            if (resource.systemlink) {
                printf("%sSystem Link", spx);
                if (resource.systemlinkplayersmax >= 2) {
                    if (resource.systemlinkplayersmax == 2) printf(" Players: 2%s", newline);
                    else printf(" Players: 2-%u%s", resource.systemlinkplayersmax, newline);
                }
                else printf("%s", newline);
                spx = sp28;
            }
            if (resource.camera) {
                printf("%sCamera%s", spx, newline);
                spx = sp28;
            }
            if (resource.customsoundtrack) {
                printf("%sCustom Soundtracks%s", spx, newline);
                spx = sp28;
            }
            if (resource.dolby51) {
                printf("%sDolby 5.1%s", spx, newline);
                spx = sp28;
            }
            if (resource.harddriveenhanced) {
                printf("%sHard Drive Enhanced%s", spx, newline);
                spx = sp28;
            }
            if (resource.harddriverequired) {
                printf("%sHard Drive Required%s", spx, newline);
                spx = sp28;
            }
            if (resource.multiplayerversus) {
                printf("%sMultiplayer Versus%s", spx, newline);
                spx = sp28;
            }
            if (resource.peripherals) {
                printf("%sSpecial Peripherals%s", spx, newline);
                spx = sp28;
            }
            if (resource.premiumoffers) {
                printf("%sPremium Offers%s", spx, newline);
                spx = sp28;
            }
            if (resource.usercreatablecontent) {
                printf("%sUser Creatable Content%s", spx, newline);
                spx = sp28;
            }
            if (resource.xboxsupport) {
                printf("%sOriginal Xbox Support%s", spx, newline);
                spx = sp28;
            }
            if (resource.livemultiplayer) {
                printf("%sXbox Live Multiplayer", spx);
                if (resource.liveplayersmax >= 2) {
                    if (resource.liveplayersmax == 2) printf(": 2%s", newline);
                    else printf(": 2-%u%s", resource.liveplayersmax, newline);
                }
                else printf("%s", newline);
                spx = sp28;
            }
            if (resource.livecoop) {
                printf("%sXbox Live Co-op%s", spx, newline);
                spx = sp28;
            }
            if (resource.liveclans) {
                printf("%sXbox Live Clans%s", spx, newline);
                spx = sp28;
            }
            if (resource.livedownloads) {
                printf("%sXbox Live Downloads%s", spx, newline);
                spx = sp28;
            }
            if (resource.livescoreboard) {
                printf("%sXbox Live Scoreboard%s", spx, newline);
                spx = sp28;
            }
            if (resource.livestats) {
                printf("%sXbox Live Stats%s", spx, newline);
                spx = sp28;
            }
            if (resource.livetournaments) {
                printf("%sXbox Live Tournaments%s", spx, newline);
                spx = sp28;
            }
            if (resource.liveserviceproxy) {
                printf("%sXbox Live Service Proxy%s", spx, newline);
                spx = sp28;
            }
            if (resource.livevoice) {
                printf("%sXbox Live Voice%s", spx, newline);
                spx = sp28;
            }
            if (spx == sp0) printf("No Features%s", newline);  // this shouldn't happen because resource.foundfeatures should only be set to true if one of the above features is enabled, but this will make sure a newline gets printed in case we fucked up anyway
        }
        if (verbose || showachievements) {
            if (resource.num_achievements) {
                if (verbose) printf("%s", sp5);
                printf("Achievements: ");
                if (verbose) printf("%s ", sp8);
                printf("%lu Achievements totaling %lu Gamerscore%s", resource.num_achievements, resource.totalgamerscore, newline);
            }
            else {
                if (verbose) printf("%s", sp5);
                printf("No Achievements%s", newline);
            }
        }
    }
    
  return;
}

int checkdefaultxex(unsigned char *defaultxexbuffer, unsigned long defaultxexsize) {
    char *spx;
    int i;
    unsigned long m, n;
    xex_crc32 = 0;
    //memset(xex_sha1, 0, 20);
    // get the starting address of code from 0x08 in the xex
    unsigned long codeoffset = getuintmsb(defaultxexbuffer+0x08);
    if (debug) printf("%scodeoffset: 0x%lX%s", sp5, codeoffset, newline);
    // check that codeoffset isn't way too large
    if (codeoffset > defaultxexsize) {
        color(red);
        printf("ERROR: starting address of Xex code is beyond the size of the default.xex!%s", newline);
        color(normal);
      return 1;
    }
    // get the starting address of the xex certificate
    unsigned long certoffset = getuintmsb(defaultxexbuffer+0x10);
    if (debug) printf("%scertoffset: 0x%lX%s", sp5, certoffset, newline);
    // check that certoffset isn't way too large
    if (certoffset > codeoffset) {
        color(red);
        printf("ERROR: Xex certificate offset is beyond the starting address of Xex code!%s", newline);
        color(normal);
      return 1;
    }
    // get the number of entries in the general info table
    unsigned long infotable_num_entries = getuintmsb(defaultxexbuffer+0x14);
    if (debug) printf("%sinfotable_num_entries: %lu%s", sp5, infotable_num_entries, newline);
    // check that there aren't way too many entries
    if (infotable_num_entries * 8 + 24 > codeoffset) {
        color(red);
        printf("ERROR: Xex general info table has entries that spill over into the Xex code!%s", newline);
        color(normal);
      return 1;
    }
    // parse info table
    unsigned long resourceinfo_address = 0L;
    unsigned char resourceinfo_tableflags[4] = {0x00,0x00,0x02,0xFF};
    unsigned long compressioninfo_address = 0L;
    unsigned char compressioninfo_tableflags[4] = {0x00,0x00,0x03,0xFF};
    unsigned long executioninfo_address = 0L;
    unsigned char executioninfo_tableflags[4] = {0x00,0x04,0x00,0x06};
    unsigned long basefiletimestamp_address = 0L;
    unsigned char basefiletimestamp_tableflags[4] = {0x00,0x01,0x80,0x02};
    unsigned long originalname_address = 0L;
    unsigned char originalname_tableflags[4] = {0x00,0x01,0x83,0xFF};
    unsigned long ratings_address = 0L;
    unsigned char ratings_tableflags[4] = {0x00,0x04,0x03,0x10};
    bool foundsystemflags = false;
    unsigned long systemflags = 0L;
    unsigned char systemflags_tableflags[4] = {0x00,0x03,0x00,0x00};
    for (m=0;m<infotable_num_entries;m++) {
        n = m*8+0x18;
        if (memcmp(defaultxexbuffer+n, resourceinfo_tableflags, 4) == 0) {
            resourceinfo_address = getuintmsb(defaultxexbuffer+n+4);
        }
        else if (memcmp(defaultxexbuffer+n, compressioninfo_tableflags, 4) == 0) {
            compressioninfo_address = getuintmsb(defaultxexbuffer+n+4);
        }
        else if (memcmp(defaultxexbuffer+n, executioninfo_tableflags, 4) == 0) {
            executioninfo_address = getuintmsb(defaultxexbuffer+n+4);
        }
        else if (memcmp(defaultxexbuffer+n, basefiletimestamp_tableflags, 4) == 0) {
            basefiletimestamp_address = getuintmsb(defaultxexbuffer+n+4);
        }
        else if (memcmp(defaultxexbuffer+n, originalname_tableflags, 4) == 0) {
            originalname_address = getuintmsb(defaultxexbuffer+n+4);
        }
        else if (memcmp(defaultxexbuffer+n, ratings_tableflags, 4) == 0) {
            ratings_address = getuintmsb(defaultxexbuffer+n+4);
        }
        else if (memcmp(defaultxexbuffer+n, systemflags_tableflags, 4) == 0) {
            foundsystemflags = true;
            systemflags = getuintmsb(defaultxexbuffer+n+4);
            if (systemflags & 0x00020000) game_has_ap25 = true;
        }
    }
    if (debug) {
        printf("%s%sresourceinfo_address: 0x%lX%s", newline, sp5, resourceinfo_address, newline);
        printf("%scompressioninfo_address: 0x%lX%s", sp5, compressioninfo_address, newline);
        printf("%sexecutioninfo_address: 0x%lX%s", sp5, executioninfo_address, newline);
        printf("%sbasefiletimestamp_address: 0x%lX%s", sp5, basefiletimestamp_address, newline);
        printf("%soriginalname_address: 0x%lX%s", sp5, originalname_address, newline);
        printf("%sratings_address: 0x%lX%s", sp5, ratings_address, newline);
        if (foundsystemflags) printf("%sfound systemflags: 0x%08lX%s", sp5, systemflags, newline);
        else printf("%sdid not find systemflags%s", sp5, newline);
        printf("%s", newline);
    }
    if (extraverbose || (foundsystemflags && (systemflags & 0x00020000))) {
        unsigned long moduleflags = getuintmsb(defaultxexbuffer+4);
        if (moduleflags == 0) printf("%sNo Module Flags%s", sp5, newline);
        else {
            // print module flags
            spx = sp0;
            printf("%sModule Flags: %s ", sp5, sp8);
            if (moduleflags & 0x00000001) { printf("%sTitle Module%s", spx, newline); spx = sp28; }
            if (moduleflags & 0x00000002) { printf("%sExports To Title%s", spx, newline); spx = sp28; }
            if (moduleflags & 0x00000004) { printf("%sSystem Debugger%s", spx, newline); spx = sp28; }
            if (moduleflags & 0x00000008) { printf("%sDLL Module%s", spx, newline); spx = sp28; }
            if (moduleflags & 0x00000010) { printf("%sModule Patch%s", spx, newline); spx = sp28; }
            if (moduleflags & 0x00000020) { printf("%sFull Patch%s", spx, newline); spx = sp28; }
            if (moduleflags & 0x00000040) { printf("%sDelta Patch%s", spx, newline); spx = sp28; }
            if (moduleflags & 0x00000080) { printf("%sUser Mode%s", spx, newline); spx = sp28; }
            if (moduleflags & 0xFFFFFF00) printf("%sUnknown Module Flags: %08lX%s", spx, moduleflags & 0xFFFFFF00, newline);
        }
        unsigned long imageflags = getuintmsb(defaultxexbuffer+certoffset+0x10C);
        // print image flags
        spx = sp0;
        printf("%sImage Flags: %s ", sp5, sp9);
        if (imageflags & 0x00000002) { printf("%sManufacturing Utility%s", spx, newline); spx = sp28; }
        if (imageflags & 0x00000004) { printf("%sManufacturing Support Tool%s", spx, newline); spx = sp28; }
        if (imageflags & 0x00000008) { printf("%sXGD2 Media Only%s", spx, newline); spx = sp28; }
        if (imageflags & 0x00000100) { printf("%sCardea Key (WMDRM-ND)%s", spx, newline); spx = sp28; }
        if (imageflags & 0x00000200) { printf("%sXeika Key (AP25)%s", spx, newline); spx = sp28; }
        if (imageflags & 0x00000400) { printf("%sTitle Usermode%s", spx, newline); spx = sp28; }
        if (imageflags & 0x00000800) { printf("%sSystem Usermode%s", spx, newline); spx = sp28; }
        if (imageflags & 0x00001000) { printf("%sOrange0%s", spx, newline); spx = sp28; }
        if (imageflags & 0x00002000) { printf("%sOrange1%s", spx, newline); spx = sp28; }
        if (imageflags & 0x00004000) { printf("%sOrange2%s", spx, newline); spx = sp28; }
        //if (imageflags & 0x00008000) { printf("%sRestricted to Signed Keyvault%s", spx, newline); spx = sp28; }  // xextool bug?
        if (imageflags & 0x00010000) { printf("%sIPTV Signup Application%s", spx, newline); spx = sp28; }
        if (imageflags & 0x00020000) { printf("%sIPTV Title Application%s", spx, newline); spx = sp28; }
        if (imageflags & 0x04000000) { printf("%sKeyvault Privileges Required%s", spx, newline); spx = sp28; }
        if (imageflags & 0x08000000) { printf("%sActivation Required%s", spx, newline); spx = sp28; }
        if (imageflags & 0x10000000) { printf("%s4 KB Pages%s", spx, newline); spx = sp28; }
        else { printf("%s64 KB Pages%s", spx, newline); spx = sp28; }
        if (imageflags & 0x20000000) { printf("%sNo Game Region%s", spx, newline); spx = sp28; }
        if (imageflags & 0x40000000) { printf("%sRevocation Check Optional%s", spx, newline); spx = sp28; }
        if (imageflags & 0x80000000) { printf("%sRevocation Check Required%s", spx, newline); spx = sp28; }
        if (imageflags & 0x07FC00F1) printf("%sUnknown Image Flags: %08lX%s", spx, imageflags & 0x07FC00F1, newline);
        if (foundsystemflags) {
            if (systemflags == 0) printf("%sNo System Flags%s", sp5, newline);
            else {
                // print system flags
                spx = sp0;
                printf("%sSystem Flags: %s ", sp5, sp8);
                if (systemflags & 0x00000001) { printf("%sNo Forced Reboot%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00000002) { printf("%sForeground Tasks%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00000004) { printf("%sNo ODD Mapping%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00000008) { printf("%sHandles MCE Input%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00000010) { printf("%sRestricted HUD Features%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00000020) { printf("%sHandles Gamepad Disconnect%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00000040) { printf("%sHas Secure Sockets%s", spx, newline); spx = sp28; } // insecure sockets?
                if (systemflags & 0x00000080) { printf("%sXbox1 Interoperability%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00000100) { printf("%sDash Context%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00000200) { printf("%sUses Game Voice Channel%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00000400) { printf("%sPal50 Incompatible%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00000800) { printf("%sInsecure Utility Drive Support%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00001000) { printf("%sXam Hooks%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00002000) { printf("%sAccesses Personally Identifiable Information (PII)%s", spx, newline); spx = sp28; } // the user has to have opted in to 3rd party communication
                if (systemflags & 0x00004000) { printf("%sCross Platform System Link%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00008000) { printf("%sMultidisc Swap%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00010000) { printf("%sSupports Insecure Multidisc Media%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00020000) {
                    // AP25! oh noes!
                    color(yellow);
                    printf("%sAntiPiracy25 Media%s", spx, newline); spx = sp28;
                    color(normal);
                }
                if (systemflags & 0x00040000) { printf("%sNo Confirm Exit%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00080000) { printf("%sAllow Background Downloading%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00100000) { printf("%sCreate Persistable Ram Drive%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00200000) { printf("%sInherit Persistent Ram Drive%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00400000) { printf("%sAllow HUD Vibration%s", spx, newline); spx = sp28; }
                if (systemflags & 0x00800000) { printf("%sAllow Access to Both Utility Partitions%s", spx, newline); spx = sp28; }
                if (systemflags & 0x01000000) { printf("%sHandles Input for IPTV%s", spx, newline); spx = sp28; }
                if (systemflags & 0x02000000) { printf("%sPrefers Big Button Input%s", spx, newline); spx = sp28; }
                if (systemflags & 0x04000000) { printf("%sAllow Extended System Reservation%s", spx, newline); spx = sp28; }
                if (systemflags & 0x08000000) { printf("%sMultidisc Cross Title%s", spx, newline); spx = sp28; }
                if (systemflags & 0x10000000) { printf("%sTitle Install Incompatible%s", spx, newline); spx = sp28; }
                if (systemflags & 0x20000000) { printf("%sAllow Avatar Get Metadata by XUID%s", spx, newline); spx = sp28; }
                if (systemflags & 0x40000000) { printf("%sAllow Controller Swapping%s", spx, newline); spx = sp28; }
                if (systemflags & 0x80000000) printf("%sDash Extensibility Module%s", spx, newline);
            }
        }
        else printf("%sNo System Flags%s", sp5, newline);
    }
    // get the basefile load address which will be used to find the relative location of the title id resource
    unsigned long basefile_loadaddress = getuintmsb(defaultxexbuffer+certoffset+0x110);
    if (debug) printf("basefile_loadaddress = %lu (0x%lX)%s", basefile_loadaddress, basefile_loadaddress, newline);
    // get title id resource address/size and calculate relative location
    bool foundtitleidresource = false;
    unsigned long titleidresource_address = 0L;
    unsigned long titleidresource_relativeaddress = 0L;
    unsigned long titleidresource_size = 0L;
    unsigned int disc_num = 0;
    unsigned int num_discs = 0;
    unsigned char titleid[4] = {0,0,0,0};
    if (executioninfo_address) {
        disc_num = (unsigned int) defaultxexbuffer[executioninfo_address+18];
        num_discs = (unsigned int) defaultxexbuffer[executioninfo_address+19];
        unsigned char titleidhex[9], cmp;
        memcpy(titleid, defaultxexbuffer+executioninfo_address+12, 4);
        if (extraverbose || extractimages) {
            if (verbose) printf("%s", sp5);
            printf("Title ID: ");
            if (verbose) printf("%s ", sp12);
            printf("%02X%02X%02X%02X", titleid[0], titleid[1], titleid[2], titleid[3]);
            if ((titleid[0] < 0x20 || (titleid[0] > 0x7E && titleid[0] < 0xA1)) || (titleid[1] < 0x20 || (titleid[1] > 0x7E && titleid[1] < 0xA1))) printf("%s", newline);
            else printf(" (%c%c-%u)%s", titleid[0], titleid[1], getwordmsb(titleid+2), newline);
        }
        // convert 4 byte title id into 8 byte ascii hex representation for comparison to resource names
        memset(titleidhex, 0, 9);
        for (i=0; i<8; i++) {
            if (i%2 == 0) cmp = titleid[i/2] & 0xF0;
            else cmp = (titleid[i/2] & 0x0F) << 4;       
            if (cmp == 0xF0) titleidhex[i] = 'F';
            else if (cmp == 0xE0) titleidhex[i] = 'E';
            else if (cmp == 0xD0) titleidhex[i] = 'D';
            else if (cmp == 0xC0) titleidhex[i] = 'C';
            else if (cmp == 0xB0) titleidhex[i] = 'B';
            else if (cmp == 0xA0) titleidhex[i] = 'A';
            else if (cmp == 0x90) titleidhex[i] = '9';
            else if (cmp == 0x80) titleidhex[i] = '8';
            else if (cmp == 0x70) titleidhex[i] = '7';
            else if (cmp == 0x60) titleidhex[i] = '6';
            else if (cmp == 0x50) titleidhex[i] = '5';
            else if (cmp == 0x40) titleidhex[i] = '4';
            else if (cmp == 0x30) titleidhex[i] = '3';
            else if (cmp == 0x20) titleidhex[i] = '2';
            else if (cmp == 0x10) titleidhex[i] = '1';
            else titleidhex[i] = '0';
        }
        if (debug) printf("title id: %s (converted to ascii)%s", titleidhex, newline);
        if (resourceinfo_address) {
            unsigned long resourceinfo_size = getuintmsb(defaultxexbuffer+resourceinfo_address);
            if (debug) printf("resourceinfo_size = %lu%s", resourceinfo_size, newline);
            if (resourceinfo_size < 20) {
                // not enough space for a resource name and address/size
                color(yellow);
                printf("ERROR: Size of resource info is too small to contain a valid resource entry!%s", newline);
                color(normal);
                if (debug) hexdump(defaultxexbuffer+resourceinfo_address, 0, 2048);
            }
            else {
                if (debug && ((resourceinfo_size - 4) % 16)) {
                    // shouldn't happen so we'll print a message in debug mode but otherwise we'll just ignore any leftover partial entries
                    color(yellow);
                    printf("resourceinfo_size - 4 is not an even multiple of 16%s", newline);
                    color(normal);
                }
                int resourceinfoentries = (int) ((resourceinfo_size - 4) / 16);
                for (i=0;i<resourceinfoentries;i++) {
                    if (memcmp(defaultxexbuffer+resourceinfo_address+4+(i*16), titleidhex, 8) == 0) {
                        titleidresource_address = getuintmsb(defaultxexbuffer+resourceinfo_address+4+(i*16)+8);
                        titleidresource_size = getuintmsb(defaultxexbuffer+resourceinfo_address+4+(i*16)+12);
                        titleidresource_relativeaddress = titleidresource_address - basefile_loadaddress;
                      break;
                    }
                }
                if (debug) printf("titleidresource_address = %lu (0x%lX)%s"
                                  "titleidresource_relativeaddress = %lu (0x%lX)%s"
                                  "titleidresource_size = %lu (0x%lX)%s",
                                  titleidresource_address, titleidresource_address, newline,
                                  titleidresource_relativeaddress, titleidresource_relativeaddress, newline,
                                  titleidresource_size, titleidresource_size, newline);
            }
        }
        if (extraverbose) {
            // print xex version and basefile version
            printf("%sVer / Base Ver: %s v%u.%u.%u.%u / v%u.%u.%u.%u%s", sp5, sp6, defaultxexbuffer[executioninfo_address+4] >> 4, defaultxexbuffer[executioninfo_address+4] & 0x0F,
                    getwordmsb(defaultxexbuffer+executioninfo_address+5), defaultxexbuffer[executioninfo_address+7], defaultxexbuffer[executioninfo_address+8] >> 4,
                    defaultxexbuffer[executioninfo_address+8] & 0x0F, getwordmsb(defaultxexbuffer+executioninfo_address+9), defaultxexbuffer[executioninfo_address+11], newline);
        }
        if (extraverbose || (verbose && disc_num && num_discs > 1)) {
            // print disc number and number of discs
            printf("%sDisc Number: %s %u of %u%s", sp5, sp9, disc_num, num_discs, newline);
        }
    }
    // get the size of the basefile
    unsigned long basefile_size = getuintmsb(defaultxexbuffer+certoffset+4);
    if (debug) printf("%sbasefile_size = %lu (0x%lX)%s", sp5, basefile_size, basefile_size, newline);
    if (verbose) {
        // print original PE filename/timestamp
        if (originalname_address) {
            unsigned long originalname_length = getuintmsb(defaultxexbuffer+originalname_address);
            if (debug) printf("%soriginalname_length = %lu%s", sp5, originalname_length, newline);
            if (originalname_length > 0 && originalname_length < 256) {
                printf("%sOriginal PE Filename: %s", sp5, sp1);
                for (m=0;m<originalname_length;m++) {
                    n = originalname_address+4+m;
                    if (defaultxexbuffer[n] == 0) break;
                    printf("%c", defaultxexbuffer[n]);
                }
                printf("%s", newline);
            }
        }
        if (basefiletimestamp_address) {
            printf("%sOriginal PE Timestamp: ", sp5);
            printunixfiletime(getuintmsb(defaultxexbuffer+basefiletimestamp_address+4));
            printf("%s", newline);
        }
    }
    // image flag 0x08 at certoffset+0x10F indicates XGD2 original disc only
    bool xgd2only = false;
    if ((defaultxexbuffer[certoffset+0x10F] & 0x08) == 0x08) xgd2only = true;
    if (extraverbose || (verbose && !xgd2only)) {
        printf("%sAllowed Media Types: %s ", sp5, sp1);
        if (xgd2only) printf("DVD-XGD2 (Xbox 360 original disc)%s", newline);
        else {
            // allowed media type flags are at certoffset + 0x17C - 0x17F
            color(cyan);
            if ( (defaultxexbuffer[certoffset+0x17C] == 0xFF) && (defaultxexbuffer[certoffset+0x17D] == 0xFF) &&
                 (defaultxexbuffer[certoffset+0x17E] == 0xFF) && (defaultxexbuffer[certoffset+0x17F] == 0xFF) ) {
                printf("All Media Types%s", newline);
            }
            else {
                spx = sp0;
                if (defaultxexbuffer[certoffset+0x17F] & 0x01) { printf("%sHard Disk%s", spx, newline); spx = sp28; }
                if (defaultxexbuffer[certoffset+0x17F] & 0x02) { printf("%sDVD-X2 (Xbox1 Original Disc)%s", spx, newline); spx = sp28; }
                if (defaultxexbuffer[certoffset+0x17F] & 0x04) { printf("%sDVD/CD%s", spx, newline); spx = sp28; }
                if (defaultxexbuffer[certoffset+0x17F] & 0x08) { printf("%sDVD-5%s", spx, newline); spx = sp28; }
                if (defaultxexbuffer[certoffset+0x17F] & 0x10) { printf("%sDVD-9%s", spx, newline); spx = sp28; }
                if (defaultxexbuffer[certoffset+0x17F] & 0x20) { printf("%sSystem Flash%s", spx, newline); spx = sp28; }
                if (defaultxexbuffer[certoffset+0x17F] & 0x80) { printf("%sMemory Unit%s", spx, newline); spx = sp28; }
                if (defaultxexbuffer[certoffset+0x17E] & 0x01) { printf("%sUSB Mass Storage Device%s", spx, newline); spx = sp28; }
                if (defaultxexbuffer[certoffset+0x17E] & 0x02) { printf("%sNetworked SMB Share%s", spx, newline); spx = sp28; }
                if (defaultxexbuffer[certoffset+0x17E] & 0x04) { printf("%sDirect from RAM%s", spx, newline); spx = sp28; }
                if (defaultxexbuffer[certoffset+0x17E] & 0x08) { printf("%sRAM Drive%s", spx, newline); spx = sp28; }
                if (defaultxexbuffer[certoffset+0x17E] & 0x10) { printf("%sSecure Virtual Optical Device%s", spx, newline); spx = sp28; }
                if (defaultxexbuffer[certoffset+0x17C] & 0x01) { printf("%sInsecure Package%s", spx, newline); spx = sp28; }
                if (defaultxexbuffer[certoffset+0x17C] & 0x02) { printf("%sSavegame Package%s", spx, newline); spx = sp28; }
                if (defaultxexbuffer[certoffset+0x17C] & 0x04) { printf("%sLocally Signed Package%s", spx, newline); spx = sp28; }
                if (defaultxexbuffer[certoffset+0x17C] & 0x08) { printf("%sLive Signed Package%s", spx, newline); spx = sp28; }
                if (defaultxexbuffer[certoffset+0x17C] & 0x10) { printf("%sXbox Package%s", spx, newline); spx = sp28; }
                if ((defaultxexbuffer[certoffset+0x17C] & 0xE0) || (defaultxexbuffer[certoffset+0x17D] & 0xFF) ||
                    (defaultxexbuffer[certoffset+0x17E] & 0xF0) || (defaultxexbuffer[certoffset+0x17F] & 0x40)) {
                    // unknown media type(s) (E0FFE040)
                    printf("%sUnknown Media: 0x%02X%02X%02X%02X%s", spx,
                            defaultxexbuffer[certoffset+0x17C] & 0xE0, defaultxexbuffer[certoffset+0x17D] & 0xFF,
                            defaultxexbuffer[certoffset+0x17E] & 0xE0, defaultxexbuffer[certoffset+0x17F] & 0x40, newline);
                }
            }
            color(normal);
        }
    }
    if (extraverbose && ratings_address) {
        if (defaultxexbuffer[ratings_address] == 255 && defaultxexbuffer[ratings_address+1] == 255 && defaultxexbuffer[ratings_address+2] == 255 &&
            defaultxexbuffer[ratings_address+3] == 255 && defaultxexbuffer[ratings_address+4] == 255 && defaultxexbuffer[ratings_address+5] == 255 &&
            defaultxexbuffer[ratings_address+6] == 255 && defaultxexbuffer[ratings_address+7] == 255 && defaultxexbuffer[ratings_address+8] == 255 &&
            defaultxexbuffer[ratings_address+9] == 255 && defaultxexbuffer[ratings_address+10] == 255 && defaultxexbuffer[ratings_address+11] == 255) {
            printf("%sNo Game Ratings%s", sp5, newline);
        }
        else {
            // print ratings
            unsigned char gamerating;
            spx = sp0;
            printf("%sGame Ratings:%s", sp5, sp10);
            // ESRB (Entertainment Software Rating Board)
            gamerating = defaultxexbuffer[ratings_address];
            if (gamerating != 255) {
                printf("%sESRB: %s ", spx, sp2);
                if (gamerating == 0)       printf("eC (Early Childhood)%s", newline);
                else if (gamerating == 2)  printf("E (Everyone 6+)%s", newline);
                else if (gamerating == 4)  printf("E10+ (Everyone 10+)%s", newline);
                else if (gamerating == 6)  printf("T (Teen 13+)%s", newline);
                else if (gamerating == 8)  printf("M (Mature 17+)%s", newline);
                else if (gamerating == 14) printf("AO (Adults Only 18+)%s", newline);
                else                       printf("Undefined rating (0x%02X)%s", gamerating, newline);
                spx = sp28;
            }
            // PEGI (Pan European Game Information)
            gamerating = defaultxexbuffer[ratings_address+1];
            if (gamerating != 255) {
                printf("%sPEGI: %s ", spx, sp2);
                if (gamerating == 0)       printf("3+%s", newline);
                else if (gamerating == 4)  printf("7+%s", newline);
                else if (gamerating == 9)  printf("12+%s", newline);
                else if (gamerating == 13) printf("16+%s", newline);
                else if (gamerating == 14) printf("18+%s", newline);
                else                       printf("Undefined rating (0x%02X)%s", gamerating, newline);
                spx = sp28;
            }
            // PEGI (Pan European Game Information) - Finland
            gamerating = defaultxexbuffer[ratings_address+2];
            if (gamerating != 255) {
                printf("%sPEGI-FI: ", spx);
                if (gamerating == 0)       printf("3+%s", newline);
                else if (gamerating == 4)  printf("7+%s", newline);
                else if (gamerating == 8)  printf("11+%s", newline);
                else if (gamerating == 12) printf("15+%s", newline);
                else if (gamerating == 14) printf("18+%s", newline);
                else                       printf("Undefined rating (0x%02X)%s", gamerating, newline);
                spx = sp28;
            }
            // PEGI (Pan European Game Information) - Portugal
            gamerating = defaultxexbuffer[ratings_address+3];
            if (gamerating != 255) {
                printf("%sPEGI-PT: ", spx);
                if (gamerating == 1)       printf("4+%s", newline);
                else if (gamerating == 3)  printf("6+%s", newline);
                else if (gamerating == 9)  printf("12+%s", newline);
                else if (gamerating == 13) printf("16+%s", newline);
                else if (gamerating == 14) printf("18+%s", newline);
                else                       printf("Undefined rating (0x%02X)%s", gamerating, newline);
                spx = sp28;
            }
            // BBFC (British Board of Film Classification) - UK/Ireland
            gamerating = defaultxexbuffer[ratings_address+4];
            if (gamerating != 255) {
                printf("%sBBFC: %s ", spx, sp2);
                if (gamerating == 0)       printf("3+%s", newline);
                else if (gamerating == 1)  printf("U (Universal)%s", newline);
                else if (gamerating == 4)  printf("7+%s", newline);
                else if (gamerating == 5)  printf("PG%s", newline);
                else if (gamerating == 9)  printf("12+%s", newline);
                else if (gamerating == 12) printf("15+%s", newline);
                else if (gamerating == 13) printf("16+%s", newline);
                else if (gamerating == 14) printf("18+%s", newline);
                else                       printf("Undefined rating (0x%02X)%s", gamerating, newline);
                spx = sp28;
            }
            // CERO (Computer Entertainment Rating Organization)
            gamerating = defaultxexbuffer[ratings_address+5];
            if (gamerating != 255) {
                printf("%sCERO: %s ", spx, sp2);
                if (gamerating == 0)      printf("A (All Ages)%s", newline);
                else if (gamerating == 2) printf("B (12+)%s", newline);
                else if (gamerating == 4) printf("C (15+)%s", newline);
                else if (gamerating == 6) printf("D (17+)%s", newline);
                else if (gamerating == 8) printf("Z (18+)%s", newline);
                else                      printf("Undefined rating (0x%02X)%s", gamerating, newline);
                spx = sp28;
            }
            // USK (Unterhaltungssoftware SelbstKontrolle)
            gamerating = defaultxexbuffer[ratings_address+6];
            if (gamerating != 255) {
                printf("%sUSK: %s ", spx, sp3);
                if (gamerating == 0)      printf("All Ages%s", newline);
                else if (gamerating == 2) printf("6+%s", newline);
                else if (gamerating == 4) printf("12+%s", newline);
                else if (gamerating == 6) printf("16+%s", newline);
                else if (gamerating == 8) printf("18+%s", newline);
                else                      printf("Undefined rating (0x%02X)%s", gamerating, newline);
                spx = sp28;
            }
            // OFLC (Office of Film and Literature Classification) - Australia
            gamerating = defaultxexbuffer[ratings_address+7];
            if (gamerating != 255) {
                printf("%sOFLC-AU: ", spx);
                if (gamerating == 0)      printf("G (General)%s", newline);
                else if (gamerating == 2) printf("PG%s", newline);
                else if (gamerating == 4) printf("M (Mature)%s", newline);
                else if (gamerating == 6) printf("MA15+%s", newline);
                else                      printf("Undefined rating (0x%02X)%s", gamerating, newline);
                spx = sp28;
            }
            // OFLC (Office of Film and Literature Classification) - New Zealand
            gamerating = defaultxexbuffer[ratings_address+8];
            if (gamerating != 255) {
                printf("%sOFLC-NZ: ", spx);
                if (gamerating == 0)      printf("G (General)%s", newline);
                else if (gamerating == 2) printf("PG%s", newline);
                else if (gamerating == 4) printf("M (Mature)%s", newline);
                else if (gamerating == 6) printf("MA15+%s", newline);
                else                      printf("Undefined rating (0x%02X)%s", gamerating, newline);
                spx = sp28;
            }
            // KMRB (Korea Media Rating Board)
            gamerating = defaultxexbuffer[ratings_address+9];
            if (gamerating != 255) {
                printf("%sKMRB: %s ", spx, sp2);
                if (gamerating == 0)      printf("All Ages%s", newline);
                else if (gamerating == 2) printf("12+%s", newline);
                else if (gamerating == 4) printf("15+%s", newline);
                else if (gamerating == 6) printf("18+%s", newline);
                else                      printf("Undefined rating (0x%02X)%s", gamerating, newline);
                spx = sp28;
            }
            // Brazil
            gamerating = defaultxexbuffer[ratings_address+10];
            if (gamerating != 255) {
                printf("%sBrazil: %s", spx, sp1);
                if (gamerating == 0)      printf("All Ages%s", newline);
                else if (gamerating == 2) printf("12+%s", newline);
                else if (gamerating == 4) printf("14+%s", newline);
                else if (gamerating == 5) printf("16+%s", newline);
                else if (gamerating == 8) printf("18+%s", newline);
                else                      printf("Undefined rating (0x%02X)%s", gamerating, newline);
                spx = sp28;
            }
            // FPB (Film and Publication Board)
            gamerating = defaultxexbuffer[ratings_address+11];
            if (gamerating != 255) {
                printf("%sFPB: %s ", spx, sp3);
                if (gamerating == 0)       printf("All Ages%s", newline);
                else if (gamerating == 6)  printf("PG%s", newline);
                else if (gamerating == 7)  printf("10+%s", newline);
                else if (gamerating == 10) printf("13+%s", newline);
                else if (gamerating == 13) printf("16+%s", newline);
                else if (gamerating == 14) printf("18+%s", newline);
                else                       printf("Undefined rating (0x%02X)%s", gamerating, newline);
                spx = sp28;
            }
        }
    }
    
    // get compression/encryption info
    bool decompressionfailed = false;
    bool xex_is_encrypted = false;
    bool xex_is_compressed = false;
    bool xex_is_compressed_basic = false;
    bool xex_is_compressed_delta = false;
    bool xex_is_compressed_unknown = false;
    uchar compressedblock_hash[20] = {0};
    uchar compressedblock_hash_calculated[20] = {0};
    unsigned long compressioninfo_size = 0L;
    unsigned long compressedblock_size = 0L;
    unsigned long compressionwindow = 0L;
    int compressionwindow_bits = 0;
    if (compressioninfo_address) {
        if (debug) {
            printf("%sencrypted: %02X%s", sp5, defaultxexbuffer[compressioninfo_address+5], newline);
            printf("%scompressed: %02X%s", sp5, defaultxexbuffer[compressioninfo_address+7], newline);
        }
        compressioninfo_size = getuintmsb(defaultxexbuffer+compressioninfo_address);
        if (defaultxexbuffer[compressioninfo_address+5] & 0x01) xex_is_encrypted = true;
        
        if      (defaultxexbuffer[compressioninfo_address+7] == 0x01) xex_is_compressed_basic = true;
        else if (defaultxexbuffer[compressioninfo_address+7] == 0x02) xex_is_compressed = true;
        else if (defaultxexbuffer[compressioninfo_address+7] == 0x03) xex_is_compressed_delta = true;
        else                                                          xex_is_compressed_unknown = true;
        
        if (xex_is_compressed) {
            if (compressioninfo_size != 0x24) {
                if (debug || testing) printf("xex_is_compressed but length of info (0x%lX) != 0x24%s", compressioninfo_size, newline);
                xex_is_compressed = false;
                xex_is_compressed_unknown = true;
            }
            else {
                compressionwindow = getuintmsb(defaultxexbuffer+compressioninfo_address+8);
                if (compressionwindow == 32768) compressionwindow_bits = 15;
                else if (compressionwindow == 65536) compressionwindow_bits = 16;
                else if (compressionwindow == 131072) compressionwindow_bits = 17;
                else if (compressionwindow == 262144) compressionwindow_bits = 18;
                else if (compressionwindow == 524288) compressionwindow_bits = 19;
                else if (compressionwindow == 1048576) compressionwindow_bits = 20;
                else if (compressionwindow == 2097152) compressionwindow_bits = 21;
                compressedblock_size = getuintmsb(defaultxexbuffer+compressioninfo_address+12);
                memcpy(compressedblock_hash, defaultxexbuffer+compressioninfo_address+16, 20);
                if (debug) {
                    if (compressionwindow_bits == 0) color(red);
                    printf("%scompressionwindow = %lu (0x%lX)%s", sp5, compressionwindow, compressionwindow, newline);
                    printf("%scompressionwindow_bits = %d%s", sp5, compressionwindow_bits, newline);
                    if (compressionwindow_bits == 0) color(normal);
                    printf("%scompressedblock_size = %lu (0x%lX)%s", sp5, compressedblock_size, compressedblock_size, newline);
                    printf("%scompressedblock_hash: ", sp5);
                    for (i=0;i<20;i++) printf("%02X", compressedblock_hash[i]);
                    printf("%s", newline);
                }
            }
        }
        else if (xex_is_compressed_basic) {
            if (compressioninfo_size % 8) {
                if (debug || testing) printf("xex_is_compressed_basic but compressioninfo_size (%lu) is not an even multiple of 8%s", compressioninfo_size, newline);
                xex_is_compressed_basic = false;
                xex_is_compressed_unknown = true;
            }
            else if (compressioninfo_size > 8) {
                int basiccompressionentries = (int) (compressioninfo_size - 8) / 8;
                struct { unsigned long address, paddingsize; } basiccompressioninfo[basiccompressionentries];
                for (i=0;i<basiccompressionentries;i++)
                    basiccompressioninfo[i].address = getuintmsb(defaultxexbuffer+compressioninfo_address+8+(i*8));
                for (i=0;i<basiccompressionentries;i++)
                    basiccompressioninfo[i].paddingsize = getuintmsb(defaultxexbuffer+compressioninfo_address+12+(i*8));
                if (debug) for (i=0;i<basiccompressionentries;i++)
                    printf("basiccompressioninfo[%d].address = %lu (0x%lX)%s"
                           "basiccompressioninfo[%d].paddingsize = %lu (0x%lX)%s%s",
                           i, basiccompressioninfo[i].address, basiccompressioninfo[i].address, newline,
                           i, basiccompressioninfo[i].paddingsize, basiccompressioninfo[i].paddingsize, newline, newline);
                if (titleidresource_relativeaddress) {
                    // calculate the title id resource's real address without having to pad the basefile with zeros
                    m = titleidresource_relativeaddress;
                    n = 0;
                    for (i=0;i<basiccompressionentries;i++) {
                        n += basiccompressioninfo[i].address + basiccompressioninfo[i].paddingsize;
                        if (titleidresource_relativeaddress >= n) {
                            if (basiccompressioninfo[i].paddingsize > m) {
                                // wtf?
                                color(yellow);
                                printf("ERROR: Basic compression information is invalid!%s", newline);
                                color(normal);
                                titleidresource_relativeaddress = 0L;
                              break;
                            }
                            else {
                                m -= basiccompressioninfo[i].paddingsize;
                            }
                        }
                    }
                    if (titleidresource_relativeaddress) {
                        if (codeoffset + m + titleidresource_size > defaultxexsize) {
                            color(yellow);
                            printf("ERROR: The Title ID Resource has an invalid address and/or size!%s", newline);
                            color(normal);
                            titleidresource_relativeaddress = 0L;
                        }
                        else {
                            titleidresource_relativeaddress = m;
                            if (debug) printf("real title id resource address = %lu (0x%lX)%s",
                                               titleidresource_relativeaddress, titleidresource_relativeaddress, newline);
                        }
                    }
                }
            }
            else {
                if (debug || testing) printf("compressioninfo_size (%lu) <= 8%s", compressioninfo_size, newline);
            }
        }
        
        if (extraverbose) {
            printf("%sCompression Info:%s ", sp5, sp5);
            if (xex_is_compressed) {
                printf("Compressed and ");
                if (xex_is_encrypted) printf("Encrypted%s", newline);
                else printf("Unencrypted%s", newline);
            }
            else if (xex_is_compressed_basic) {
                printf("Uncompressed and ");
                if (xex_is_encrypted) printf("Encrypted%s", newline);
                else printf("Unencrypted%s", newline);
            }
            else if (xex_is_compressed_delta) {
                printf("Delta Compressed and ");
                if (xex_is_encrypted) printf("Encrypted%s", newline);
                else printf("Unencrypted%s", newline);
            }
            else {
                printf("Unrecognized Compression Method ");
                if (xex_is_encrypted) printf("(Encrypted)%s", newline);
                else printf("(Unencrypted)%s", newline);
            }
        }
    }

    // get file key (at certoffset + 0x150)
    u8 xex_filekey[16];
    memcpy(xex_filekey, defaultxexbuffer+(certoffset+0x150), 16);
    if (debug) {
        printf("%sXex File Key: %s", sp5, sp3);
        for (i=0; i<16; i++) printf("%02X", (unsigned char) xex_filekey[i]);
        printf("%s", newline);
    }
    // decrypt file key with xex retail or devkit key to get session key
    u8 xex_sessionkey[16];
    int Nr;
    u32 rk[4*(MAXNR + 1)];
    const u8 xex_retailkey[16] = {0x20,0xB1,0x85,0xA5,0x9D,0x28,0xFD,0xC3,0x40,0x58,0x3F,0xBB,0x08,0x96,0xBF,0x91};
    // const u8 xex_manufacturemodekey[16] = {0xA2,0x6C,0x10,0xF7,0x1F,0xD9,0x35,0xE9,0x8B,0x99,0x92,0x2C,0xE9,0x32,0x15,0x72};
    const u8 xex_devkitkey[16] = {0x00};
    if (devkey) Nr = rijndaelKeySetupDec(rk, xex_devkitkey, 128);  // this is specified with --devkey
    else        Nr = rijndaelKeySetupDec(rk, xex_retailkey, 128);
    rijndaelDecrypt(rk, Nr, xex_filekey, xex_sessionkey);
    if (debug) {
        printf("%sXex Session Key: ", sp5);
        for (i=0; i<16; i++) printf("%02X", (unsigned char) xex_sessionkey[i]);
        printf("%s", newline);
    }
    // calculate xex crc
    xex_crc32 = crc32(0, defaultxexbuffer, defaultxexsize);
    // get default.xex media id (at certoffset + 0x140)
    memcpy(xex_mediaid, defaultxexbuffer+(certoffset+0x140), 16);
    xex_foundmediaid = true;
/*
    // calculate xex sha-1
    sha1_starts(&ctx);
    sha1_update(&ctx, defaultxexbuffer, defaultxexsize);
    sha1_finish(&ctx, xex_sha1);
    if (extraverbose) {
        printf("%sXex SHA = ", sp5);
        for (i=0; i<20; i++) printf("%02X", xex_sha1[i]);
        printf("%s", newline);
    }
*/
    if (xex_is_encrypted) {
        // decrypt it
        if (debug) {
            printf("1st 2048 bytes of code to decrypt:%s", newline);
            hexdump(defaultxexbuffer+codeoffset, 0, 2048);
        }
        u8 ivec[16], pt[16], ct[16];
        memset(ivec, 0, 16);
        if (debug && ((defaultxexsize - codeoffset) % 16)) {
            // code to decrypt is not an even multiple of 16 (the aes block size)
            // this probably shouldn't happen so we'll print a message (in debug mode only)
            // but i don't know what else we can do so we won't worry about decrypting any remainder
            color(yellow);
            printf("(defaultxexsize - codeoffset) %% 16 = %lu%s", (defaultxexsize - codeoffset) % 16, newline);
            color(normal);
        }
        Nr = rijndaelKeySetupDec(rk, xex_sessionkey, 128);
        n = (defaultxexsize - codeoffset) / 16;
        for (m=0;m<n;m++) {
            memcpy(ct, defaultxexbuffer+codeoffset+m*16, 16);
            rijndaelDecrypt(rk, Nr, ct, pt);
            for (i=0;i<16;i++) pt[i] ^= ivec[i];
            memcpy(ivec, ct, 16);
            memcpy(defaultxexbuffer+codeoffset+m*16, pt, 16);
        }
        if (debug) {
            printf("1st 2048 bytes of decrypted code:%s", newline);
            hexdump(defaultxexbuffer+codeoffset, 0, 2048);
        }
        if (debug) {
            FILE *defaultdec = NULL;
            char defaultdecpath[2048];
            memset(defaultdecpath, 0, 2048);
            if (!homeless) {
                strcat(defaultdecpath, homedir); strcat(defaultdecpath, abgxdir);
            }
            strcat(defaultdecpath, "default.dec");
            defaultdec = fopen(defaultdecpath, "wb");
            if (defaultdec == NULL) {
                printf("ERROR: Failed to open %s%s%s for writing! (%s)%s", quotation, defaultdecpath, quotation, strerror(errno), newline);
            }
            else {
                // should use proper error checking but this for debug so doesn't matter that much
                dontcare = fwrite(defaultxexbuffer+codeoffset, 1, defaultxexsize-codeoffset, defaultdec);
                fclose(defaultdec);
            }
        }
    }
    if (xex_is_compressed_basic) {
        if (titleidresource_relativeaddress) {
            if (memcmp(defaultxexbuffer+codeoffset+titleidresource_relativeaddress, "XDBF", 4) != 0) {
                if (debug || testing) {
                    color(red);
                    printf("\"XDBF\" was not found at the start of the title id resource:%s", newline);
                    color(normal);
                    hexdump(defaultxexbuffer+codeoffset+titleidresource_relativeaddress, 0, 2048);
                }
            }
            else {
                foundtitleidresource = true;
                if (debug) {
                    printf("1st 2048 bytes of the title id resource:%s", newline);
                    hexdump(defaultxexbuffer+codeoffset+titleidresource_relativeaddress, 0, titleidresource_size > 2048 ? 2048 : titleidresource_size);
                }
                parsetitleidresource(defaultxexbuffer+codeoffset+titleidresource_relativeaddress, titleidresource_size, titleid);
            }
        }
    }
    else if (xex_is_compressed) {
        // decompress it
        m = 0;
        n = codeoffset;
        unsigned long compressedblock_realsize = 0;
        unsigned short s = 0;
        unsigned long p = 0;
        unsigned long long defaultcab_totalsize = 0;
        FILE *defaultcab = NULL, *defaultpe = NULL;
        char defaultcabpath[2048], defaultpepath[2048];
        memset(defaultcabpath, 0, 2048);
        memset(defaultpepath, 0, 2048);
        if (!homeless) {
            strcat(defaultcabpath, homedir); strcat(defaultcabpath, abgxdir);
            strcat(defaultpepath, homedir); strcat(defaultpepath, abgxdir);
        }
        strcat(defaultcabpath, "default.cab");
        strcat(defaultpepath, "default.pe");
        initcheckwrite();
        defaultcab = fopen(defaultcabpath, "wb");
        if (defaultcab == NULL) {
            decompressionfailed = true;
            color(red);
            printf("ERROR: Failed to open %s%s%s for writing! (%s) Failed to decompress the Xex!%s",
                    quotation, defaultcabpath, quotation, strerror(errno), newline);
            color(normal);
        }
        else if (compressionwindow_bits == 0) {
            decompressionfailed = true;
            color(red);
            printf("ERROR: Compression window size (%.3f KB) is invalid! Failed to decompress the Xex!%s",
                    (float) compressionwindow / 1024, newline);
            color(normal);
        }
        else while(compressedblock_size) {
            m++;
            if (n + compressedblock_size > defaultxexsize) {
                // the block would be extending past the end of the default.xex!
                decompressionfailed = true;
                color(red);
                printf("ERROR: Compressed block #%lu is reporting an incorrect size! Failed to decompress the Xex!%s", m, newline);
                if (debug) {
                    printf("start address: 0x%lX%s", n, newline);
                    printf("block size: %lu (0x%lX)%s", compressedblock_size, compressedblock_size, newline);
                    printf("defaultxexsize: %lu (0x%lX)%s", defaultxexsize, defaultxexsize, newline);
                    printf("hash expected: ");
                    for (i=0;i<20;i++) printf("%02X", compressedblock_hash[i]);
                    printf("%s", newline);
                }
                color(normal);
              break;
            }
            sha1_starts(&ctx);
            sha1_update(&ctx, defaultxexbuffer+n, compressedblock_size);
            sha1_finish(&ctx, compressedblock_hash_calculated);
            if (memcmp(compressedblock_hash, compressedblock_hash_calculated, 20) != 0) {
                // expected hash doesn't match the calculated one
                decompressionfailed = true;
                color(red);
                printf("ERROR: Compressed block #%lu is corrupt! Failed to decompress the Xex!%s", m, newline);
                if (debug) {
                    printf("start address: 0x%lX%s", n, newline);
                    printf("block size: %lu (0x%lX)%s", compressedblock_size, compressedblock_size, newline);
                    printf("hash expected: %s", sp2);
                    for (i=0;i<20;i++) printf("%02X", compressedblock_hash[i]);
                    printf("%shash calculated: ", newline);
                    for (i=0;i<20;i++) printf("%02X", compressedblock_hash_calculated[i]);
                    printf("%s", newline);
                }
                color(normal);
              break;
            }
            else if (debug) {
                printf("compressed block #%03lu is valid, address = 0x%07lX, size = %06lu (0x%06lX)%s",
                        m, n, compressedblock_size, compressedblock_size, newline);
            }
            // write compressed data to default.cab
            compressedblock_realsize = 0;
            p = n+24;
            i = 0;
            while(1) {
                i++;
                s = getwordmsb(defaultxexbuffer+p);
                if (s == 0) break;
                else if (debug) printf("block segment #%02d size = %05u%s", i, s, newline);
                compressedblock_realsize += s;
                if (compressedblock_realsize > compressedblock_size) {
                    // should never happen
                    decompressionfailed = true;
                    color(red);
                    printf("ERROR: Compressed block #%lu has invalid parsing data! Failed to decompress the Xex!%s",
                            m, newline);
                    color(normal);
                  break;
                }
                if (checkwriteandprinterrors(defaultxexbuffer+p+2, 1, (size_t) s, defaultcab, 0, defaultcab_totalsize, "default.cab", "decompressing the Xex") != 0) {
                    decompressionfailed = true;
                  break;
                }
                p += s+2;
                defaultcab_totalsize += (unsigned long long) s;
            }
            if (debug) printf("compressed block #%03lu real size = %06lu (0x%06lX)%s%s",
                               m, compressedblock_realsize, compressedblock_realsize, newline, newline);
            if (decompressionfailed) break;
            // get info about the next block
            memcpy(compressedblock_hash, defaultxexbuffer+n+4, 20);
            n += compressedblock_size;
            compressedblock_size = getuintmsb(defaultxexbuffer+n-compressedblock_size);
        }
        if (defaultcab != NULL) {
            if (debug) printf("flushing defaultcab%s", newline);
            fflush(defaultcab);
        }
        if (!decompressionfailed) {
            donecheckwrite("default.cab");
            // decompress default.cab
            struct mspack_system *sys = mspack_default_system;
            struct mspack_file *lzxinput = NULL;
            struct mspack_file *lzxoutput = NULL;
            struct lzxd_stream *lzxd = NULL;
            if (debug) printf("decompressing %s%s", defaultcabpath, newline);
            lzxinput = sys->open(sys, defaultcabpath, MSPACK_SYS_OPEN_READ);
            if (lzxinput == NULL) {
                decompressionfailed = true;
                color(red);
                printf("ERROR: libmspack failed to open %s%s%s for reading! (%s) Failed to decompress the Xex!%s",
                       quotation, defaultcabpath, quotation, strerror(errno), newline);
                color(normal);
            }
            else {
                lzxoutput = sys->open(sys, defaultpepath, MSPACK_SYS_OPEN_WRITE);
                if (lzxoutput == NULL) {
                    decompressionfailed = true;
                    color(red);
                    printf("ERROR: libmspack failed to open %s%s%s for writing! (%s) Failed to decompress the Xex!%s",
                           quotation, defaultpepath, quotation, strerror(errno), newline);
                    color(normal);
                }
                else {
                    lzxd = lzxd_init(sys, lzxinput, lzxoutput, compressionwindow_bits, 0, 32768, (off_t) basefile_size);
                    if (lzxd == NULL) {
                        decompressionfailed = true;
                        color(red);
                        printf("ERROR: Initializing LZX decompression state failed! (%s) Failed to decompress the Xex!%s", strerror(errno), newline);
                        color(normal);
                    }
                    else {
                        i = lzxd_decompress(lzxd, (off_t) basefile_size);
                        if (i != MSPACK_ERR_OK) {
                            decompressionfailed = true;
                            color(red);
                            if (debug) printf("lzxd_decompress returned: %d%s", i, newline);
                            printf("ERROR: LZX decompression failed! (%s) Failed to decompress the Xex!%s", lzxstrerror(i), newline);
                            color(normal);
                        }
                        else {
                            if (debug) printf("decompression was successful%s", newline);
                            // open default.pe and allocate memory for the title id resource
                            defaultpe = fopen(defaultpepath, "rb");
                            if (defaultpe == NULL) {
                                color(red);
                                printf("ERROR: Failed to open %s%s%s for reading! (%s) Failed to read the title id resource!%s",
                                        quotation, defaultpepath, quotation, strerror(errno), newline);
                                color(normal);
                            }
                            else {
                                long long defaultpesize = getfilesize(defaultpe);
                                if (debug) printf("defaultpesize = %"LL"d%s", defaultpesize, newline);
                                if (defaultpesize != (long long) basefile_size) {
                                    if (defaultpesize != -1) {
                                        color(red);
                                        printf("ERROR: Decompressed PE filesize does not match the expected basefile size!%s", newline);
                                        color(normal);
                                    }
                                    // if defaultpesize == -1 it's a seek error and error message has already been printed
                                }
                                else if (titleidresource_relativeaddress) {
                                    if ((long long) (titleidresource_relativeaddress + titleidresource_size) > defaultpesize) {
                                        color(red);
                                        printf("ERROR: The Title ID Resource has an invalid address and/or size!%s", newline);
                                        color(normal);
                                    }
                                    else {
                                        if (fseeko(defaultpe, titleidresource_relativeaddress, SEEK_SET) != 0) {
                                            printseekerror(defaultpepath, "Reading Title ID resource");
                                        }
                                        else if (titleidresource_size >= 4) {
                                            unsigned char *resourcebuffer = malloc(titleidresource_size * sizeof(char));
                                            if (resourcebuffer == NULL) {
                                                color(red);
                                                printf("ERROR: memory allocation for resourcebuffer failed! Game over man... Game over!%s", newline);
                                                color(normal);
                                              exit(1);
                                            }
                                            initcheckread();
                                            if (checkreadandprinterrors(resourcebuffer, 1, titleidresource_size, defaultpe,
                                                                        0, titleidresource_relativeaddress, defaultpepath,
                                                                        "Reading Title ID resource") == 0) {
                                                donecheckread(defaultpepath);
                                                if (memcmp(resourcebuffer, "XDBF", 4) != 0) {
                                                    if (debug || testing) {
                                                        color(red);
                                                        printf("\"XDBF\" was not found at the start of the title id resource:%s", newline);
                                                        color(normal);
                                                        hexdump(resourcebuffer, 0, 2048);
                                                    }
                                                }
                                                else {
                                                    foundtitleidresource = true;
                                                    if (debug) {
                                                        printf("1st 2048 bytes of the title id resource:%s", newline);
                                                        hexdump(resourcebuffer, 0, titleidresource_size > 2048 ? 2048 : titleidresource_size);
                                                    }
                                                    parsetitleidresource(resourcebuffer, titleidresource_size, titleid);
                                                }
                                            }
                                            free(resourcebuffer);
                                        }
                                        else if (debug || testing) {
                                            color(yellow);
                                            printf("ERROR: Title ID resource is only %lu bytes%s", titleidresource_size, newline);
                                            color(normal);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if (lzxd != NULL) {
                        if (debug) printf("freeing lzxd%s", newline);
                        lzxd_free(lzxd);
                    }
                }
                if (lzxoutput != NULL) {
                    if (debug) printf("closing lzxoutput%s", newline);
                    sys->close(lzxoutput);
                }
            }
            if (lzxinput != NULL) {
                if (debug) printf("closing lzxinput%s", newline);
                sys->close(lzxinput);
            }
        }
        if (defaultcab != NULL) {
            if (debug) printf("closing and removing default.cab%s", newline);
            fclose(defaultcab);
            remove(defaultcabpath);
        }
        if (defaultpe != NULL) {
            if (debug) printf("closing and removing default.pe%s", newline);
            fclose(defaultpe);
            remove(defaultpepath);
        }
    }
    if (verbose) {
        printf("%sXEX CRC = %08lX%s", sp5, xex_crc32, newline);
        printf("%sXEX Media ID: ", sp5);
        printmediaid(xex_mediaid);
        printf("%s", newline);
    }
    if (!foundtitleidresource && (debug || testing)) {
        color(yellow);
        printf("Failed to find the Title ID resource%s", newline);
        color(normal);
    }

    if (checkgamecrcalways && !checkgamecrcnever) {
        if (docheckgamecrc() == 0) {
            if (corruptionoffsetcount == 0) {
                color(green);
                if (verbose) printf("%s", sp5);
                printf("AnyDVD style corruption was not detected%s", newline);
                color(normal);
            }
            if (verbose) printf("%sGame CRC = %08lX%s", sp5, game_crc32, newline);
        }
        else {
            checkgamecrcnever = true;
            gamecrcfailed = true;
        }
        if (!verbose) printf("%s", newline);
    }
    
    // display game name if a match is found in GameNameLookup.csv
    checkcsv(xex_mediaid);
    
    // the region code is at certoffset + 0x178
    if (debug) printf("%sdefault.xex region code address: 0x%lX%s", sp5, certoffset+0x178, newline);
    for (i=0;i<4;i++) regioncode[i] = defaultxexbuffer[certoffset+0x178+i];
    unsigned long regioncodelong = getuintmsb(regioncode) & 0xFFFFFFFF;
    foundregioncode = true;
    if (debug) printf("regioncode: 0x%02X%02X%02X%02X (unsigned chars), 0x%08lX (unsigned long)%s"
                      "userregion: 0x%08lX%s",
                       regioncode[0], regioncode[1], regioncode[2], regioncode[3], regioncodelong, newline, userregion, newline);
    if (verbose) printf("%s", newline);
    
    // highlight region code in green or red if user supplied their own region code
    bool userregion_other = false, userregion_ausnz = false, userregion_eur = false;
    bool userregion_jpn = false, userregion_china = false, userregion_asia = false, userregion_ntscu = false;
    bool region_other = false, region_ausnz = false, region_eur = false;
    bool region_jpn = false, region_china = false, region_asia = false, region_ntscu = false;
    if (userregion) {
        if ((userregion & 0xFF000000UL) == 0xFF000000UL) userregion_other = true;
        if ((userregion & 0x00010000UL) == 0x00010000UL) userregion_ausnz = true;
        if ((userregion & 0x00FE0000UL) == 0x00FE0000UL) userregion_eur = true;
        if ((userregion & 0x00000100UL) == 0x00000100UL) userregion_jpn = true;
        if ((userregion & 0x00000200UL) == 0x00000200UL) userregion_china = true;
        if ((userregion & 0x0000FC00UL) == 0x0000FC00UL) userregion_asia = true;
        if ((userregion & 0x000000FFUL) == 0x000000FFUL) userregion_ntscu = true;
        if ((regioncodelong & 0xFF000000UL) == 0xFF000000UL) region_other = true;
        if ((regioncodelong & 0x00010000UL) == 0x00010000UL) region_ausnz = true;
        if ((regioncodelong & 0x00FE0000UL) == 0x00FE0000UL) region_eur = true;
        if ((regioncodelong & 0x00000100UL) == 0x00000100UL) region_jpn = true;
        if ((regioncodelong & 0x00000200UL) == 0x00000200UL) region_china = true;
        if ((regioncodelong & 0x0000FC00UL) == 0x0000FC00UL) region_asia = true;
        if ((regioncodelong & 0x000000FFUL) == 0x000000FFUL) region_ntscu = true;
        if ( (userregion_other && region_other) ||
             (userregion_ausnz && region_ausnz) ||
             (userregion_eur && region_eur) ||
             (userregion_jpn && region_jpn) ||
             (userregion_china && region_china) ||
             (userregion_asia && region_asia) ||
             (userregion_ntscu && region_ntscu) ) color(green);
        else color(red);
    }
    else {
        if ((regioncodelong & 0xFFFFFFFF) == 0xFFFFFFFF) color(green);
        else color(white);
    }
    
    printf("Region Code: 0x%08lX%s", regioncodelong, newline);
    if ((regioncodelong & 0xFFFFFFFF) == 0xFFFFFFFF) {
        printf("%sRegion Free!%s", sp5, newline);
        color(normal);
      return 0;
    }
    
    if (regioncode[1] == 0xFF) {
        if (userregion) {
            if (userregion_ausnz || userregion_eur) color(green);
            else color(white);
        }
        printf("%sPAL%s", sp5, newline);
    }
    else if (regioncode[1] == 0xFE) {
        if (userregion) {
            if (userregion_eur) color(green);
            else color(white);
        }
        printf("%sPAL (Excludes AUS/NZ)%s", sp5, newline);
    }
    else if (regioncode[1] == 0x01) {
        if (userregion) {
            if (userregion_ausnz) color(green);
            else color(white);
        }
        printf("%sPAL (AUS/NZ Only)%s", sp5, newline);
    }
    else if (regioncode[1] != 0x00) {
        if (userregion) color(white);
        printf("%sPAL (Unknown code: 0x%02X)%s", sp5, regioncode[1], newline);
    }
    
    if (regioncode[2] == 0xFF) {
        if (userregion) {
            if (userregion_jpn || userregion_china || userregion_asia) color(green);
            else color(white);
        }
        printf("%sNTSC/J%s", sp5, newline);
    }
    else if (regioncode[2] == 0xFD) {
        if (userregion) {
            if (userregion_jpn || userregion_asia) color(green);
            else color(white);
        }
        printf("%sNTSC/J (Excludes China)%s", sp5, newline);
    }
    else if (regioncode[2] == 0xFE) {
        if (userregion) {
            if (userregion_china || userregion_asia) color(green);
            else color(white);
        }
        printf("%sNTSC/J (Excludes Japan)%s", sp5, newline);
    }
    else if (regioncode[2] == 0xFC) {
        if (userregion) {
            if (userregion_asia) color(green);
            else color(white);
        }
        printf("%sNTSC/J (Excludes Japan and China)%s", sp5, newline);
    }
    else if (regioncode[2] == 0x01) {
        if (userregion) {
            if (userregion_jpn) color(green);
            else color(white);
        }
        printf("%sNTSC/J (Japan Only)%s", sp5, newline);
    }
    else if (regioncode[2] == 0x02) {
        if (userregion) {
            if (userregion_china) color(green);
            else color(white);
        }
        printf("%sNTSC/J (China Only)%s", sp5, newline);
    }
    else if (regioncode[2] == 0x03) {
        if (userregion) {
            if (userregion_jpn || userregion_china) color(green);
            else color(white);
        }
        printf("%sNTSC/J (Japan and China Only)%s", sp5, newline);
    }
    else if (regioncode[2] != 0x00) {
        if (userregion) color(white);
        printf("%sNTSC/J (Unknown code: 0x%02X)%s", sp5, regioncode[2], newline);
    }
    
    if (regioncode[3] == 0xFF) {
        if (userregion) {
            if (userregion_ntscu) color(green);
            else color(white);
        }
        printf("%sNTSC/U%s", sp5, newline);
    }
    else if (regioncode[3] != 0x00) {
        if (userregion) color(white);
        printf("%sNTSC/U (Unknown code: 0x%02X)%s", sp5, regioncode[3], newline);
    }
    
    if (regioncode[0] == 0xFF) {
        if (userregion) {
            if (userregion_other) color(green);
            else color(white);
        }
        printf("%sOther%s", sp5, newline);
    }
    else if (regioncode[0] != 0x00) {
        if (userregion) color(white);
        printf("%sOther (Unknown code: 0x%02X)%s", sp5, regioncode[0], newline);
    }
    color(normal);
  return 0;
}

int docheckgamecrc() {
    int i, a;
    char letter;
    unsigned long m, n;
    if (isotoosmall) {
        color(red);
        printf("ERROR: ISO filesize is too small, Game CRC Check aborted!%s", newline);
        color(normal);
      return 1;
    }
    // check Game crc32
    const unsigned long long gamesize = 7307001856LL;
    const unsigned long gamesizeoverbuffer = (unsigned long) (gamesize / BIGBUF_SIZE);
    game_crc32 = 0;
    if (fseeko(fp, video, SEEK_SET) != 0) {
        color(red);
        printf("ERROR: Failed to seek to new file position! (%s) Game CRC Check failed!%s", strerror(errno), newline);
        color(normal);
      return 1;
    }
    printstderr = true; color(white);
    if (verbose) fprintf(stderr, "\n");
    fprintf(stderr, "Checking Game CRC...");
    color(normal);
    fprintf(stderr, " (press Q to cancel)\n");
    if (verbose) {
        fprintf(stderr, " Percent  Elapsed  Estimated   Time     Average     Current     Errors    Total\n");
        fprintf(stderr, "    Done     Time       Time   Left       Speed       Speed  Recovered  Retries\n");
    }
    color(white);
    #ifdef WIN32
        if (dvdarg && BIGBUF_SIZE % 2048) {  // will only become true if BIGBUF_SIZE is changed to some weird value
            color(normal); printstderr = false;
            color(red); printf("ERROR: BIGBUF_SIZE is not an even multiple of 2048%s", newline); color(normal);
            if (debug) printf("BIGBUF_SIZE = %d%s", BIGBUF_SIZE, newline);
          return 1;
        }
        unsigned long LBA;
        unsigned short transferlength = (unsigned short) (BIGBUF_SIZE / 2048);
        UCHAR cdb[10] = {0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0};
        cdb[0] = 0x28;  // READ (10)
        cdb[7] = (UCHAR) (transferlength >> 8);
        cdb[8] = (UCHAR) (transferlength & 0x00FF);
        if (dvdarg && transferlength > 16) {  // will only become true if BIGBUF_SIZE is changed to be greater than 32KB
            color(normal);
            printstderr = false;
            color(red);
            printf("ERROR: transferlength for dvd drive input is greater than 16%s", newline);
            color(normal);
            if (debug) printf("transferlength = %u%s", transferlength, newline);
          return 1;
        }
        unsigned long startmsecs = GetTickCount();
    #else
        // set terminal to raw to detect keypress
        init_keyboard();
        struct timeval starttime;
        struct timeval currenttime;
        gettimeofday(&starttime, NULL);
        unsigned long startmsecs = starttime.tv_sec * 1000 + starttime.tv_usec / 1000;
    #endif
    corruptionoffsetcount = 0;
    for (i=0;i<100;i++) corruptionoffset[i] = 0;
    if (verbose) charsprinted = fprintf(stderr, "                                                           ");
    else charsprinted = 0;
    readerrorcharsprinted = 0;
    unsigned long long totalbytesread = 0LL;
    unsigned long long lasttotalbytesread = 0LL;
    unsigned long long gamecrcoffset;
    float MBpsavg = 0, MBpscur = 0, MBpsrunningavg = 0;
    unsigned long etasecs, elapsedsecs = 0, leftsecs;
    unsigned long gamereaderrorstotal = 0, gamereaderrorsrecovered = 0;
    unsigned long currentmsecs = startmsecs;
    unsigned long lastmsecs = startmsecs;
    float MBpsarray[10];
    int MBpsarray_current = -1;
    unsigned long updates = 0;
    for (m=0; m<gamesizeoverbuffer; m++) {
        if (kbhit()) {
            // cancel checking game crc if user pressed Q (or q)
            letter = getch();
            if (letter == 'q' || letter == 'Q') {
                fprintf(stderr, "\n");
                color(normal);
                printstderr = false;
                game_crc32 = 0;  // reset to 0 so we don't try to autofix or verify a bad crc
                #ifndef WIN32
                    close_keyboard();
                #endif
                usercancelledgamecrc = true;
              return 1;
            }
        }
        #ifdef WIN32
            currentmsecs = GetTickCount();
        #else
            gettimeofday(&currenttime, NULL);
            currentmsecs = currenttime.tv_sec * 1000 + (currenttime.tv_usec / 1000);
        #endif
        // update progress once/sec (unless we're on the first loop) and also update on last loop (100%)
        if ((currentmsecs - lastmsecs > 999 && m) || m == gamesizeoverbuffer - 1) {
            totalbytesread = (unsigned long long) m * BIGBUF_SIZE;
            MBpsavg = (float) totalbytesread / 1048576 / ( (float) (currentmsecs - startmsecs) / 1000 );
            MBpscur = (float) (totalbytesread - lasttotalbytesread) / 1048576 / ( (float) (currentmsecs - lastmsecs) / 1000 );
            if (MBpsarray_current < 9) MBpsarray_current++;
            else MBpsarray_current = 0;
            updates++;
            MBpsarray[MBpsarray_current] = MBpscur;
            if (updates > 10) {
                // running average of the last 10 seconds (or last 10 updates) for better ETA estimates
                MBpsrunningavg = ( MBpsarray[0] + MBpsarray[1] + MBpsarray[2] + MBpsarray[3] + MBpsarray[4] +
                                   MBpsarray[5] + MBpsarray[6] + MBpsarray[7] + MBpsarray[8] + MBpsarray[9] ) / 10;
            }
            else MBpsrunningavg = MBpsavg;
            etasecs = (currentmsecs - startmsecs) / 1000 + (unsigned long) ( (gamesize - totalbytesread) / 1048576 / MBpsrunningavg );
            elapsedsecs = (currentmsecs - startmsecs) / 1000;
            leftsecs = etasecs - elapsedsecs;
            for (i=0;i<charsprinted+readerrorcharsprinted;i++) fprintf(stderr, "\b");
            if (readerrorcharsprinted) color(white);
            if (verbose) {
                if (leftsecs / 60 > 999) // not enough space for time left
                    charsprinted = fprintf(stderr, "    %3lu%% %5lu:%02lu %7lu:%02lu ???:?? %6.1f MB/s %6.1f MB/s",
                                                   m / (gamesizeoverbuffer / 100),
                                                   elapsedsecs / 60, elapsedsecs % 60,
                                                   etasecs / 60, etasecs % 60,
                                                   MBpsavg, MBpscur);
                else
                    charsprinted = fprintf(stderr, "    %3lu%% %5lu:%02lu %7lu:%02lu %3lu:%02lu %6.1f MB/s %6.1f MB/s",
                                                   m / (gamesizeoverbuffer / 100),
                                                   elapsedsecs / 60, elapsedsecs % 60,
                                                   etasecs / 60, etasecs % 60,
                                                   leftsecs / 60, leftsecs % 60,
                                                   MBpsavg, MBpscur);
            }
            else {
                for (i=0;i<charsprinted+readerrorcharsprinted;i++) fprintf(stderr, " ");
                for (i=0;i<charsprinted+readerrorcharsprinted;i++) fprintf(stderr, "\b");
                charsprinted = fprintf(stderr, "%3lu%% Done, Current Speed: %.1f MB/s, Time Left: %lu:%02lu",
                                                m / (gamesizeoverbuffer / 100), MBpscur, leftsecs / 60, leftsecs % 60);
            }
            readerrorcharsprinted = 0;
            lastmsecs = currentmsecs;
            lasttotalbytesread = totalbytesread;
        }
        if (dvdarg) {
            #ifdef WIN32
                gamecrcoffset = (unsigned long long) m * BIGBUF_SIZE + video;
                if (gamecrcoffset % 2048) {  // don't think this would ever happen (would have been caught earlier) but check anyway
                    fprintf(stderr, "\n");
                    color(normal); printstderr = false;
                    game_crc32 = 0;  // reset to 0 so we don't try to verify a bad crc
                    color(red); printf("ERROR: gamecrcoffset does not fall on the start of a sector%s", newline); color(normal);
                  return 1;
                }
                LBA = (unsigned long) (gamecrcoffset / 2048);
                // build read cdb command
                // 0 = 0x28 READ (set earlier)
                // 2-5 = LBA MSB-LSB
                cdb[2] = (UCHAR) (LBA >> 24);
                cdb[3] = (UCHAR) ((LBA & 0x00FF0000L) >> 16);
                cdb[4] = (UCHAR) ((LBA & 0x0000FF00L) >> 8);
                cdb[5] = (UCHAR) (LBA & 0x000000FFL);
                // 7-8 = transfer length (blocks) MSB-LSB (set earlier)
                if (sendcdb(DATA_IN, bigbuffer, BIGBUF_SIZE, cdb, 10, true)) {
                    color(yellow);
                    for (i=0;i<readretries;i++) {
                        gamereaderrorstotal++;
                        if (verbose) {
                            for(a=0;a<readerrorcharsprinted;a++) fprintf(stderr, "\b");
                            readerrorcharsprinted = fprintf(stderr, "   %8lu %8lu", gamereaderrorsrecovered, gamereaderrorstotal);
                        }
                        if (sendcdb(DATA_IN, bigbuffer, BIGBUF_SIZE, cdb, 10, true) == 0) {
                            gamereaderrorsrecovered++;
                            if (verbose) {
                                for(a=0;a<readerrorcharsprinted;a++) fprintf(stderr, "\b");
                                readerrorcharsprinted = fprintf(stderr, "   %8lu %8lu", gamereaderrorsrecovered, gamereaderrorstotal);
                            }
                            goto gamecrc1;
                        }
                    }
                    // unrecovered error
                    color(normal); printstderr = false;
                    color(red); printf("%sERROR: Unrecoverable read error while checking the Game CRC!%s", newline, newline); color(normal);
                    game_crc32 = 0;  // reset to 0 so we don't try to verify a bad crc
                  return 1;
                }
            #else
                fprintf(stderr, "\n");
                color(normal); printstderr = false;
                color(red); printf("ERROR: WTF? WIN32 not defined but somehow dvdarg was set?%s", newline); color(normal);
                if (debug) printf("dvdarg = %d%s", dvdarg, newline);
                game_crc32 = 0;  // reset to 0 so we don't try to autofix or verify a bad crc
                close_keyboard();
              return 1;
            #endif
        }
        else if (fread(bigbuffer, 1, BIGBUF_SIZE, fp) < BIGBUF_SIZE) {
            color(yellow);
            if (feof(fp)) {
                color(normal); printstderr = false;
                color(red); printf("%sERROR: End of File reached while checking the Game CRC, operation aborted!%s", newline, newline); color(normal);
                game_crc32 = 0;  // reset to 0 so we don't try to autofix or verify a bad crc
                #ifndef WIN32
                    close_keyboard();
                #endif
              return 1;
            }
            gamecrcoffset = (unsigned long long) m * BIGBUF_SIZE + video;
            for (i=0;i<readretries;i++) {
                gamereaderrorstotal++;
                if (verbose) {
                    for(a=0;a<readerrorcharsprinted;a++) fprintf(stderr, "\b");
                    readerrorcharsprinted = fprintf(stderr, "   %8lu %8lu", gamereaderrorsrecovered, gamereaderrorstotal);
                }
                if (fseeko(fp, gamecrcoffset, SEEK_SET) != 0) {
                    color(red); printf("ERROR: Failed to seek to new file position! (%s) Game CRC Check failed!%s", strerror(errno), newline); color(normal);
                    game_crc32 = 0;  // reset to 0 so we don't try to autofix or verify a bad crc
                    #ifndef WIN32
                        close_keyboard();
                    #endif
                  return 1;
                }
                if (fread(bigbuffer, 1, BIGBUF_SIZE, fp) == BIGBUF_SIZE) {
                    gamereaderrorsrecovered++;
                    if (verbose) {
                        for(a=0;a<readerrorcharsprinted;a++) fprintf(stderr, "\b");
                        readerrorcharsprinted = fprintf(stderr, "   %8lu %8lu", gamereaderrorsrecovered, gamereaderrorstotal);
                    }
                    goto gamecrc1;
                }
            }
            // unrecovered error
            color(normal); printstderr = false;
            color(red); printf("%sERROR: Unrecoverable read error while checking the Game CRC!%s", newline, newline); color(normal);
            game_crc32 = 0;  // reset to 0 so we don't try to autofix or verify a bad crc
            #ifndef WIN32
                close_keyboard();
            #endif
          return 1;
        }
        gamecrc1:
        game_crc32 = crc32(game_crc32, bigbuffer, BIGBUF_SIZE);
        // AnyDVD and other apps insert dvd video files into unreadable sectors to defeat sony arccos protection
        // so we'll search for "DVDVIDEO-" (DVDVIDEO-VTS and DVDVIDEO-VMG have been observed) at the start of every sector in the game data
        for (n=0;n<BIGBUF_SIZE - 9;n+=2048) {
            if (memcmp(bigbuffer+n, "DVDVIDEO-", 9) == 0) {
                if (corruptionoffsetcount < 100) {
                    corruptionoffset[corruptionoffsetcount] = (unsigned long long) m * BIGBUF_SIZE + (unsigned long long) video + (unsigned long long) n;
                    corruptionoffsetcount++;
                }
            }
        }
    }
    // this is not needed with a bigbuffer size of 32 KB, but if that was changed and the if statement below became true,
    // you should uncomment and add error checking to the code below so that it's pretty much identical to the code above
/*  if (gamesize % BIGBUF_SIZE > 0) {
        unsigned long gameremainder = gamesize % BIGBUF_SIZE;
        fread(bigbuffer, 1, gameremainder, fp);
        game_crc32 = crc32(game_crc32, bigbuffer, gameremainder);
    }  */
    fprintf(stderr, "\n");
    color(normal); printstderr = false; color(normal);
    if (corruptionoffsetcount) {
        if (verbose) printf("%s", newline);
        color(red);
        printf("Found AnyDVD style game data corruption starting at %d", corruptionoffsetcount);
        if (corruptionoffsetcount > 1) printf(" different offsets!");
        else printf(" offset!");
        if (corruptionoffsetcount > 99) printf(" (stopped counting at 100)");
        color(normal);
        printf("%s", newline);
        if (verbose) {
            printf("AnyDVD style corruption was found at: ");
            for (i=0;i<corruptionoffsetcount;i++) {
                printf("0x%09"LL"X", corruptionoffset[i]);
                if (i+2 < corruptionoffsetcount) printf(", ");
                else if (i+1 < corruptionoffsetcount) printf(" and ");
            }
            printf("%s%s", newline, newline);
        }
    }
    else if (verbose) fprintf(stderr, "\n");
    #ifndef WIN32
        close_keyboard();
    #endif
  return 0;
}

bool lookslike360ss(unsigned char *ss) {
    // startpsnL0 should be 0x04FB20
    if ((ss[0x5] != 0x04) || (ss[0x6] != 0xFB) || (ss[0x7] != 0x20)) return false;
    // these 3 bytes should be zero
    if ((ss[0x4] != 0) || (ss[0x8] != 0) || (ss[0xC] != 0)) return false;
    // 0x4ba tells us the software version (0x02 = XBOX 360)
    if (ss[0x4BA] != 0x02) return false;
  return true;
}

bool lookslikexbox1ss(unsigned char *ss) {
    // startpsnL0 should be 0x060600
    if ((ss[0x5] != 0x06) || (ss[0x6] != 0x06) || (ss[0x7] != 0)) return false;
    // these 3 bytes should be zero
    if ((ss[0x4] != 0) || (ss[0x8] != 0) || (ss[0xC] != 0)) return false;
    // 0x4ba tells us the software version (0x01 = XBOX)
    if (ss[0x4BA] != 0x01) return false;
  return true;
}

int getangledeviation(int angle, int target) {
    int dev = abs(angle - target);
    int opposite = target > 180 ? target - 180 : target + 180;
    if (dev > 180) dev = 360 - dev;
    if (target > 180 ? (angle < target && angle > opposite) : (angle > opposite || angle < target)) dev *= -1;
  return dev;
}

void printbadshit(struct badshit shit, char *badshitcolor) {
    color(badshitcolor);
    printf("%s", shit.explanation);
    if (shit.count > 1) printf("s ");
    else printf(" ");
    int i, j=0;
    for (i=0;i<shit.count;i++) {
        printf("%02X", shit.data[i]);
        j++;
        if (shit.count == j+1) printf(" and ");
        else if (shit.count > j) printf(", ");
        else printf("!%s", newline);
    }
    color(normal);
  return;
}

void printfixedshit(struct badshit shit) {
    color(green);
    printf("%s", shit.explanation);
    if (shit.count > 1) printf("s ");
    else printf(" ");
    int i, j=0;
    for (i=0;i<shit.count;i++) {
        printf("%02X", shit.data[i]);
        j++;
        if (shit.count == j+1) printf(" and ");
        else if (shit.count > j) printf(", ");
        else printf(" - Fixed!%s", newline);
    }
    color(normal);
  return;
}

int checkss() {
    int i;
    int foundangle359_count = 0;
    ss_stealthfailed = false; ss_stealthuncertain = false; ss_foundtimestamp = false; ss_foundmediaid = false;
    fixedss = false; drtfucked = false;
    ss_crc32 = 0; ss_rawcrc32 = 0;
    if (verbose && !minimal) printf("Checking SS%s", newline);
    // make sure ss isn't blank
    if (getzeros(ss, 0, 2047) == 2048) {
        ss_stealthfailed = true;
        color(red);
        printf("SS is blank!%s", newline);
        color(normal);
      return 1;
    }
    if (!lookslike360ss(ss)) {
        if (lookslikexbox1ss(ss)) {
            stealthuncertain = true;
            ss_stealthuncertain = true;
            color(yellow);
            printf("This looks like an original Xbox SS!%s", newline);
            color(normal);
          return 2;
        }
        else {
            ss_stealthfailed = true;
            color(red);
            printf("SS is not recognized as an Xbox 360 or even an original Xbox SS!%s", newline);
            color(normal);
            if (verbose) {
                printf("Displaying suspicious data in hex and ascii:%s", newline);
                hexdump(ss, 0, 2048);
            }
          return 1;
        }
    }

    // get Starting PSN of Data Area
    unsigned char ss_startpsnL0hex[4] = {ss[7], ss[6], ss[5], 0x00};
    unsigned long ss_startpsnL0 = getuint(ss_startpsnL0hex);

    // get End PSN of L0 Data Area
    unsigned char ss_endpsnL0hex[4] = {ss[15], ss[14], ss[13], 0x00};
    unsigned long ss_endpsnL0 = getuint(ss_endpsnL0hex);

    // invert bits in End PSN of L0 Data Area to find Starting PSN of L1 Data Area
    unsigned char ss_startpsnL1hex[4] = {~ss[15], ~ss[14], ~ss[13], 0x00};
    unsigned long ss_startpsnL1 = getuint(ss_startpsnL1hex);

    // get End PSN of L1 Data Area
    unsigned char ss_endpsnL1hex[4] = {ss[11], ss[10], ss[9], 0x00};
    unsigned long ss_endpsnL1 = getuint(ss_endpsnL1hex);

    // here be dragons
    int32_t layerbreakpsn = ~((layerbreak - 1 + 0x030000) ^ 0xFF000000);
    unsigned long ss_sectorsL0 = ss_endpsnL0 - ss_startpsnL0 + 1;
    unsigned long ss_sectorsL1 = ss_endpsnL1 - ss_startpsnL1 + 1;
    unsigned long long ss_offsetL0 = ((unsigned long long) ss_startpsnL0 - 0x030000) * 2048;
    unsigned long long ss_offsetL1 = ((unsigned long long) ss_startpsnL1 - (unsigned long long) layerbreakpsn) * 2048 +
                                     ((unsigned long long) layerbreak * 2048);
    unsigned long long ss_offsetL0end = (unsigned long long) (ss_endpsnL0 - ss_startpsnL0 + 1) * 2048 + ss_offsetL0 - 1;
    unsigned long long ss_offsetend = (ss_endpsnL1 - ss_startpsnL1 + 1) * 2048 + ss_offsetL1 - 1;
    unsigned long ss_sectorstotal = ss_sectorsL0 + ss_sectorsL1;

    // print that shit
    if (extraverbose && !minimal) printpfitable(ss_startpsnL0, ss_endpsnL0, ss_startpsnL1, ss_endpsnL1, ss_sectorsL0, ss_sectorsL1,
                                                ss_offsetL0, ss_offsetL0end, ss_offsetL1, ss_offsetend, ss_sectorstotal);
    
    if (ss_startpsnL0 != 0x04FB20 || ss_endpsnL0 != 0x20339F || ss_endpsnL1 != 0xFB04DF) {
        stealthuncertain = true;
        color(yellow);
        printf("SS alternate PFI appears to have changed! The LayerBreak and/or game partition%s"
               "size is probably different than what abgx360 is currently assuming! Check for%s"
               "an update to this program before doing anything further... you definitely don't%s"
               "want to burn a game with the wrong LayerBreak!%s%s", newline, newline, newline, newline, newline);
        color(normal);
    }
    
    // check for BCA (Burst Cutting Area) flag which indicates a unique barcode is
    // burned onto the original disc (but would any xbox 360 drives be able to read it?)
    if ((ss[16] & 0x80) == 0x80) {
        stealthuncertain = true;
        color(yellow);
        printf("SS BCA flag is on!%s", newline);
        color(normal);
    }

    // get timestamps of authoring and mastering
    ss_authored = getuint64(ss+0x49F);
    ss_mastered = getuint64(ss+0x5DF);
    ss_foundtimestamp = true;
    if (verbose) {
        printf("%sTimestamp of Authoring: ", sp5);
        printwin32filetime(ss_authored);
        printf("%s%sTimestamp of Mastering: ", newline, sp5);
        printwin32filetime(ss_mastered);
        printf("%s", newline);
    }
    // decrypt the crypted challenge response table (ccrt) and store it as dcrt
    int num_entries = (int) ss[0x301];
    if (debug) printf("num_entries = %d%s", num_entries, newline);
    if (num_entries != 21) {
        color(yellow);
        printf("WTF? CCRT reports %d entries, expected 21! CCRT decryption might be incorrect!%s", num_entries, newline);
        color(normal);
    }
    int j, Nr;
    unsigned char dcrt[252];
    memset(dcrt, 0, 252);
    u32 rk[4*(MAXNR + 1)];
    const u8 cipherKey[16] = {0xD1,0xE3,0xB3,0x3A,0x6C,0x1E,0xF7,0x70,0x5F,0x6D,0xE9,0x3B,0xB6,0xC0,0xDC,0x71};
    u8 ct[16], pt[16], ivec[16];
    memset(ivec, 0, 16);
    Nr = rijndaelKeySetupDec(rk, cipherKey, 128);
    for (i=0;i<15;i++) {
        memcpy(ct, ss+(0x304+(i*16)), 16);
        rijndaelDecrypt(rk, Nr, ct, pt);
        for (j=0;j<16;j++) pt[j] ^= ivec[j];
        memcpy(ivec, ct, 16);
        memcpy(dcrt+(i*16), pt, 16);
    }
    memcpy(dcrt+240, ss+0x3F4, 12);
    if (debug) {
        printf("ccrt:");
        for (i=0x304;i<0x400;i++) {
            if ((i-0x304) % 12 == 0) printf("%s", newline);
            printf("%02X", ss[i]);
        }
        printf("%s", newline);
        printf("dcrt:");
        for (i=0;i<252;i++) {
            if (i % 12 == 0) printf("%s", newline);
            printf("%02X", dcrt[i]);
        }
        printf("%s", newline);
    }
    struct {unsigned char CT, CID, Mod, WTF, CD[4], Response[4]; unsigned int angle, matches; int drtmatch;} dcrtentry[21];
    int CT01_count = 0;
    unsigned char CT01_firstCD[4];
    bool CT01_conflict = false;
    for (i=0;i<21;i++) {
        dcrtentry[i].CT = dcrt[i*12];
        dcrtentry[i].CID = dcrt[i*12+1];
        dcrtentry[i].Mod = dcrt[i*12+2];
        dcrtentry[i].WTF = dcrt[i*12+3];
        memcpy(dcrtentry[i].CD, dcrt+i*12+4, 4);
        memcpy(dcrtentry[i].Response, dcrt+i*12+8, 4);
        dcrtentry[i].angle = (unsigned int) dcrt[i*12+10] << 8;
        dcrtentry[i].angle |= (unsigned int) dcrt[i*12+11];
        dcrtentry[i].matches = 0;
        dcrtentry[i].drtmatch = -1;
        if (dcrtentry[i].CT == 0x01) {
            CT01_count++;
            if (CT01_count == 1) memcpy(CT01_firstCD, dcrtentry[i].CD, 4);
            else if (memcmp(CT01_firstCD, dcrtentry[i].CD, 4) != 0) CT01_conflict = true;
        }
    }
    if (debug) for (i=0;i<21;i++) {
        printf("dcrtentry[%d].CT = %02X%s", i, dcrtentry[i].CT, newline);
        printf("dcrtentry[%d].CID = %02X%s", i, dcrtentry[i].CID, newline);
        printf("dcrtentry[%d].Mod = %02X%s", i, dcrtentry[i].Mod, newline);
        printf("dcrtentry[%d].WTF = %02X%s", i, dcrtentry[i].WTF, newline);
        printf("dcrtentry[%d].CD = %02X%02X%02X%02X%s", i, dcrtentry[i].CD[0], dcrtentry[i].CD[1], dcrtentry[i].CD[2], dcrtentry[i].CD[3], newline);
        printf("dcrtentry[%d].Response = %02X%02X%02X%02X%s", i, dcrtentry[i].Response[0], dcrtentry[i].Response[1], dcrtentry[i].Response[2], dcrtentry[i].Response[3], newline);
        printf("dcrtentry[%d].angle = %u%s", i, dcrtentry[i].angle, newline);
        printf("dcrtentry[%d].matches = %u%s", i, dcrtentry[i].matches, newline);
        printf("dcrtentry[%d].drtmatch = %d%s%s", i, dcrtentry[i].drtmatch, newline, newline);
    }
    struct {unsigned char RT, CID, Mod, Data[6], CD[4], Response[5]; unsigned int angle, angle2; int deviation, deviation2, deviation3;} driveentry[21];
    for (i=0;i<21;i++) {
        driveentry[i].RT = ss[0x730+i*9];
        driveentry[i].CID = ss[0x730+i*9+1];
        driveentry[i].Mod = ss[0x730+i*9+2];
        memcpy(driveentry[i].Data, ss+0x730+i*9+3, 6);
        memcpy(driveentry[i].CD, ss+0x200+i*9, 4);
        memcpy(driveentry[i].Response, ss+0x200+i*9+4, 5);
        driveentry[i].angle   = (unsigned int) ss[0x200+i*9+5] << 8;
        driveentry[i].angle  |= (unsigned int) ss[0x200+i*9+4];
        driveentry[i].angle2  = (unsigned int) ss[0x200+i*9+8] << 8;
        driveentry[i].angle2 |= (unsigned int) ss[0x200+i*9+7];
        driveentry[i].deviation = 0;
        driveentry[i].deviation2 = 0;
        driveentry[i].deviation3 = 0;
    }
    if (debug) for (i=0;i<21;i++) {
        printf("driveentry[%d].RT = %02X%s", i, driveentry[i].RT, newline);
        printf("driveentry[%d].CID = %02X%s", i, driveentry[i].CID, newline);
        printf("driveentry[%d].Mod = %02X%s", i, driveentry[i].Mod, newline);
        printf("driveentry[%d].Data = %02X%02X%02X %02X%02X%02X%s", i,
                driveentry[i].Data[0], driveentry[i].Data[1], driveentry[i].Data[2],
                driveentry[i].Data[3], driveentry[i].Data[4], driveentry[i].Data[5], newline);
        printf("driveentry[%d].CD = %02X%02X%02X%02X%s", i,
                driveentry[i].CD[0], driveentry[i].CD[1], driveentry[i].CD[2], driveentry[i].CD[3], newline);
        printf("driveentry[%d].Response = %02X%02X%02X%02X%02X%s", i,
                driveentry[i].Response[0], driveentry[i].Response[1], driveentry[i].Response[2], driveentry[i].Response[3], driveentry[i].Response[4], newline);
        //printf("driveentry[%d].WTF = %02X%s", i, driveentry[i].WTF, newline);
        printf("driveentry[%d].angle = %u%s", i, driveentry[i].angle, newline);
        printf("driveentry[%d].angle2 = %u%s", i, driveentry[i].angle2, newline);
        printf("driveentry[%d].deviation = %d%s", i, driveentry[i].deviation, newline);
        printf("driveentry[%d].deviation2 = %d%s", i, driveentry[i].deviation2, newline);
        printf("driveentry[%d].deviation3 = %d%s%s", i, driveentry[i].deviation3, newline, newline);
    }
    // match drive entries to dcrt entries with same CID
    for (i=0;i<21;i++) {
        for (j=0;j<21;j++) {
            if (dcrtentry[i].CID == driveentry[j].CID) {
                dcrtentry[i].matches++;
                dcrtentry[i].drtmatch = j;
                if (debug) printf("dcrtentry[%d].CID (%02X) matches driveentry[%d].CID (%02X)%s"
                                  "dcrtentry[%d].matches = %d%sdcrtentry[%d].drtmatch = %d%s%s",
                                   i, dcrtentry[i].CID, j, driveentry[j].CID, newline,
                                   i, dcrtentry[i].matches, newline, i, dcrtentry[i].drtmatch, newline, newline);
            }
        }
    }
    // see if this is ss v2
    bool ssv2 = false;
    for (i=0;i<21;i++) {
        // assuming ss v2 if at least one of the challenge types 24/25 have a non-zero second angle (ignoring unmatched or multiple matched dcrt entries)
        if (dcrtentry[i].matches == 1 && (dcrtentry[i].CT == 0x24 || dcrtentry[i].CT == 0x25) && driveentry[dcrtentry[i].drtmatch].angle2 != 0) {
            ssv2 = true;
            break;
        }
    }
    // detect bad shit - struct badshit {unsigned char c[21], d[21], data[21]; int count; char* explanation;};
    struct badshit unmatcheddcrtentry, multiplematcheddcrtentry, CTRTmismatch, badCD, badResponse, weirdCDResponse,
                   unrecognizedCT, badcpr_mai, type0dataresponsemismatch,
                   mediumangledev, highangledev, userangledev,
                   mediumangledev2, highangledev2, userangledev2,
                   mediumangledev3, highangledev3, userangledev3,
                   baddcrtangle, baddriveangle, baddriveangle2, CT01conflictingCD;
    CT01conflictingCD.explanation = "Found multiple CT 01 entries with conflicting CD for Challenge ID";
    CT01conflictingCD.count = 0;
    unmatcheddcrtentry.explanation = "Failed to find a drive entry for Challenge ID";
    unmatcheddcrtentry.count = 0;
    multiplematcheddcrtentry.explanation = "Found multiple drive entries for Challenge ID";
    multiplematcheddcrtentry.count = 0;
    CTRTmismatch.explanation = "Challenge and Response Type do not correspond for Challenge ID";
    CTRTmismatch.count = 0;
    badCD.explanation = "CD does not match for Challenge ID";
    badCD.count = 0;
    badResponse.explanation = "Response does not match for Challenge ID";
    badResponse.count = 0;
    weirdCDResponse.explanation = "CCRT CD and Response do not appear to match for Challenge ID";
    weirdCDResponse.count = 0;
    unrecognizedCT.explanation = "Challenge Type is unrecognized for Challenge ID";
    unrecognizedCT.count = 0;
    badcpr_mai.explanation = "CPR_MAI bytes do not match the Challenge Data for Challenge ID";
    badcpr_mai.count = 0;
    type0dataresponsemismatch.explanation = "Type 00 Data does not match the Response for Challenge ID";
    type0dataresponsemismatch.count = 0;
    // SS v1 angle deviation from CCRT
    char *s;
    if ( (s = (char *) malloc(86)) == NULL ) {
        color(red);
        printf("ERROR: memory allocation for mediumangledev.explanation failed! Game over man... Game over!%s", newline);
        color(normal);
      exit(1);
    }
    sprintf(s, "SS v1 angle deviation from CCRT is %s %d degrees for Challenge ID", greaterthan, mediumangledev_value);
    mediumangledev.explanation = s;
    mediumangledev.count = 0;
    if ( (s = (char *) malloc(86)) == NULL ) {
        color(red);
        printf("ERROR: memory allocation for highangledev.explanation failed! Game over man... Game over!%s", newline);
        color(normal);
      exit(1);
    }
    sprintf(s, "SS v1 angle deviation from CCRT is %s %d degrees for Challenge ID", greaterthan, highangledev_value);
    highangledev.explanation = s;
    highangledev.count = 0;
    if ( (s = (char *) malloc(86)) == NULL ) {
        color(red);
        printf("ERROR: memory allocation for userangledev.explanation failed! Game over man... Game over!%s", newline);
        color(normal);
      exit(1);
    }
    sprintf(s, "SS v1 angle deviation from CCRT is %s %d degrees for Challenge ID", greaterthan, fixangledev_value);
    userangledev.explanation = s;
    userangledev.count = 0;
    // SS v2 angle deviation from CCRT
    if ( (s = (char *) malloc(86)) == NULL ) {
        color(red);
        printf("ERROR: memory allocation for mediumangledev2.explanation failed! Game over man... Game over!%s", newline);
        color(normal);
      exit(1);
    }
    sprintf(s, "SS v2 angle deviation from CCRT is %s %d degrees for Challenge ID", greaterthan, mediumangledev_value);
    mediumangledev2.explanation = s;
    mediumangledev2.count = 0;
    if ( (s = (char *) malloc(86)) == NULL ) {
        color(red);
        printf("ERROR: memory allocation for highangledev2.explanation failed! Game over man... Game over!%s", newline);
        color(normal);
      exit(1);
    }
    sprintf(s, "SS v2 angle deviation from CCRT is %s %d degrees for Challenge ID", greaterthan, highangledev_value);
    highangledev2.explanation = s;
    highangledev2.count = 0;
    if ( (s = (char *) malloc(86)) == NULL ) {
        color(red);
        printf("ERROR: memory allocation for userangledev2.explanation failed! Game over man... Game over!%s", newline);
        color(normal);
      exit(1);
    }
    sprintf(s, "SS v2 angle deviation from CCRT is %s %d degrees for Challenge ID", greaterthan, fixangledev_value);
    userangledev2.explanation = s;
    userangledev2.count = 0;
    // SS v1 angle deviation from v2
    if ( (s = (char *) malloc(94)) == NULL ) {
        color(red);
        printf("ERROR: memory allocation for mediumangledev3.explanation failed! Game over man... Game over!%s", newline);
        color(normal);
      exit(1);
    }
    sprintf(s, "SS v1 angle deviation from the v2 angle is %s %d degrees for Challenge ID", greaterthan, mediumangledev_value);
    mediumangledev3.explanation = s;
    mediumangledev3.count = 0;
    if ( (s = (char *) malloc(94)) == NULL ) {
        color(red);
        printf("ERROR: memory allocation for highangledev3.explanation failed! Game over man... Game over!%s", newline);
        color(normal);
      exit(1);
    }
    sprintf(s, "SS v1 angle deviation from the v2 angle is %s %d degrees for Challenge ID", greaterthan, highangledev_value);
    highangledev3.explanation = s;
    highangledev3.count = 0;
    if ( (s = (char *) malloc(94)) == NULL ) {
        color(red);
        printf("ERROR: memory allocation for userangledev3.explanation failed! Game over man... Game over!%s", newline);
        color(normal);
      exit(1);
    }
    sprintf(s, "SS v1 angle deviation from the v2 angle is %s %d degrees for Challenge ID", greaterthan, fixangledev_value);
    userangledev3.explanation = s;
    userangledev3.count = 0;
    
    baddcrtangle.explanation = "CCRT angle value is invalid for Challenge ID";
    baddcrtangle.count = 0;
    baddriveangle.explanation = "SS v1 drive angle value is invalid for Challenge ID";
    baddriveangle.count = 0;
    baddriveangle2.explanation = "SS v2 drive angle value is invalid for Challenge ID";
    baddriveangle2.count = 0;
    for (i=0;i<21;i++) {
        // ignore padding entries 0xF0 - 0xFF
        if ((dcrtentry[i].CT & 0xF0) == 0xF0) continue;
        // should be one (and only one) matching drive entry
        if (dcrtentry[i].matches != 1) {
            if (dcrtentry[i].matches == 0) {
                unmatcheddcrtentry.data[unmatcheddcrtentry.count] = dcrtentry[i].CID;
                unmatcheddcrtentry.count++;
            }
            else {
                multiplematcheddcrtentry.data[multiplematcheddcrtentry.count] = dcrtentry[i].CID;
                multiplematcheddcrtentry.count++;
            }
        }
        else {  // only one match
            if (dcrtentry[i].CT == 0x14 || dcrtentry[i].CT == 0x15 || dcrtentry[i].CT == 0x24 || dcrtentry[i].CT == 0x25) {
                // CT and RT should correspond... probably
                if ((dcrtentry[i].CT == 0x14 && driveentry[dcrtentry[i].drtmatch].RT != 0x03) ||
                    (dcrtentry[i].CT == 0x15 && driveentry[dcrtentry[i].drtmatch].RT != 0x01) ||
                    (dcrtentry[i].CT == 0x24 && driveentry[dcrtentry[i].drtmatch].RT != 0x07) ||
                    (dcrtentry[i].CT == 0x25 && driveentry[dcrtentry[i].drtmatch].RT != 0x05)) {
                        CTRTmismatch.c[CTRTmismatch.count] = i;
                        CTRTmismatch.d[CTRTmismatch.count] = dcrtentry[i].drtmatch;
                        CTRTmismatch.data[CTRTmismatch.count] = dcrtentry[i].CID;
                        CTRTmismatch.count++;
                }
                // challenge data should match
                if (memcmp(dcrtentry[i].CD, driveentry[dcrtentry[i].drtmatch].CD, 4) != 0) {
                    badCD.c[badCD.count] = i;
                    badCD.d[badCD.count] = dcrtentry[i].drtmatch;
                    badCD.data[badCD.count] = dcrtentry[i].CID;
                    badCD.count++;
                }
                if (dcrtentry[i].CT == 0x14 || dcrtentry[i].CT == 0x15) {
                    // response data should match
                    if (memcmp(dcrtentry[i].Response, driveentry[dcrtentry[i].drtmatch].Response, 4) != 0) {
                        badResponse.c[badResponse.count] = i;
                        badResponse.d[badResponse.count] = dcrtentry[i].drtmatch;
                        badResponse.data[badResponse.count] = dcrtentry[i].CID;
                        badResponse.count++;
                    }
                }
                if (dcrtentry[i].CT == 0x24 || dcrtentry[i].CT == 0x25) {
                    // first 2 bytes of dcrt response and challenge data should match or something is weird
                    if (memcmp(dcrtentry[i].CD, dcrtentry[i].Response, 2) != 0) {
                        weirdCDResponse.c[weirdCDResponse.count] = i;
                        weirdCDResponse.d[weirdCDResponse.count] = dcrtentry[i].drtmatch;
                        weirdCDResponse.data[weirdCDResponse.count] = dcrtentry[i].CID;
                        weirdCDResponse.count++;
                    }
                    if (ssv2) {
                        // angles should be 0-359
                        if (dcrtentry[i].angle > 359 || driveentry[dcrtentry[i].drtmatch].angle > 359 || driveentry[dcrtentry[i].drtmatch].angle2 > 359) {
                            if (dcrtentry[i].angle > 359) {
                                baddcrtangle.c[baddcrtangle.count] = i;
                                baddcrtangle.d[baddcrtangle.count] = dcrtentry[i].drtmatch;
                                baddcrtangle.data[baddcrtangle.count] = dcrtentry[i].CID;
                                baddcrtangle.count++;
                            }
                            if (driveentry[dcrtentry[i].drtmatch].angle > 359) {
                                baddriveangle.c[baddriveangle.count] = i;
                                baddriveangle.d[baddriveangle.count] = dcrtentry[i].drtmatch;
                                baddriveangle.data[baddriveangle.count] = dcrtentry[i].CID;
                                baddriveangle.count++;
                            }
                            if (driveentry[dcrtentry[i].drtmatch].angle2 > 359) {
                                baddriveangle2.c[baddriveangle2.count] = i;
                                baddriveangle2.d[baddriveangle2.count] = dcrtentry[i].drtmatch;
                                baddriveangle2.data[baddriveangle2.count] = dcrtentry[i].CID;
                                baddriveangle2.count++;
                            }
                        }
                        else {
                            if (driveentry[dcrtentry[i].drtmatch].angle == 359) {  // only the ss v1 angle will cause the detectable response as 1.6+ will use the ssv2 angle
                                foundangle359_count++;
                            }
                            // calculate angle deviation
                            driveentry[dcrtentry[i].drtmatch].deviation =
                                getangledeviation((int) driveentry[dcrtentry[i].drtmatch].angle, (int) dcrtentry[i].angle);
                            driveentry[dcrtentry[i].drtmatch].deviation2 =
                                getangledeviation((int) driveentry[dcrtentry[i].drtmatch].angle2, (int) dcrtentry[i].angle);
                            driveentry[dcrtentry[i].drtmatch].deviation3 =
                                getangledeviation((int) driveentry[dcrtentry[i].drtmatch].angle, (int) driveentry[dcrtentry[i].drtmatch].angle2);
                            if (!trustssv2angles) {
                                // check all angles individually against the ccrt target if we don't trust v2 angles
                                if (abs(driveentry[dcrtentry[i].drtmatch].deviation) > highangledev_value) {
                                    highangledev.c[highangledev.count] = i;
                                    highangledev.d[highangledev.count] = dcrtentry[i].drtmatch;
                                    highangledev.data[highangledev.count] = dcrtentry[i].CID;
                                    highangledev.count++;
                                }
                                else if (abs(driveentry[dcrtentry[i].drtmatch].deviation) > mediumangledev_value) {
                                    mediumangledev.c[mediumangledev.count] = i;
                                    mediumangledev.d[mediumangledev.count] = dcrtentry[i].drtmatch;
                                    mediumangledev.data[mediumangledev.count] = dcrtentry[i].CID;
                                    mediumangledev.count++;
                                }
                                else if (abs(driveentry[dcrtentry[i].drtmatch].deviation) > fixangledev_value) {
                                    userangledev.c[userangledev.count] = i;
                                    userangledev.d[userangledev.count] = dcrtentry[i].drtmatch;
                                    userangledev.data[userangledev.count] = dcrtentry[i].CID;
                                    userangledev.count++;
                                }
                                
                                if (abs(driveentry[dcrtentry[i].drtmatch].deviation2) > highangledev_value) {
                                    highangledev2.c[highangledev2.count] = i;
                                    highangledev2.d[highangledev2.count] = dcrtentry[i].drtmatch;
                                    highangledev2.data[highangledev2.count] = dcrtentry[i].CID;
                                    highangledev2.count++;
                                }
                                else if (abs(driveentry[dcrtentry[i].drtmatch].deviation2) > mediumangledev_value) {
                                    mediumangledev2.c[mediumangledev2.count] = i;
                                    mediumangledev2.d[mediumangledev2.count] = dcrtentry[i].drtmatch;
                                    mediumangledev2.data[mediumangledev2.count] = dcrtentry[i].CID;
                                    mediumangledev2.count++;
                                }
                                else if (abs(driveentry[dcrtentry[i].drtmatch].deviation2) > fixangledev_value) {
                                    userangledev2.c[userangledev2.count] = i;
                                    userangledev2.d[userangledev2.count] = dcrtentry[i].drtmatch;
                                    userangledev2.data[userangledev2.count] = dcrtentry[i].CID;
                                    userangledev2.count++;
                                }
                            }
                            else {
                                // we trust v2 angles but we should check the v1 angle deviation from the v2 angle
                                if (abs(driveentry[dcrtentry[i].drtmatch].deviation3) > highangledev_value) {
                                    highangledev3.c[highangledev3.count] = i;
                                    highangledev3.d[highangledev3.count] = dcrtentry[i].drtmatch;
                                    highangledev3.data[highangledev3.count] = dcrtentry[i].CID;
                                    highangledev3.count++;
                                }
                                else if (abs(driveentry[dcrtentry[i].drtmatch].deviation3) > mediumangledev_value) {
                                    mediumangledev3.c[mediumangledev3.count] = i;
                                    mediumangledev3.d[mediumangledev3.count] = dcrtentry[i].drtmatch;
                                    mediumangledev3.data[mediumangledev3.count] = dcrtentry[i].CID;
                                    mediumangledev3.count++;
                                }
                                else if (abs(driveentry[dcrtentry[i].drtmatch].deviation3) > fixangledev_value) {
                                    userangledev3.c[userangledev3.count] = i;
                                    userangledev3.d[userangledev3.count] = dcrtentry[i].drtmatch;
                                    userangledev3.data[userangledev3.count] = dcrtentry[i].CID;
                                    userangledev3.count++;
                                }
                            }
                        }
                    }
                    else {  // ss v1
                        // angles should be 0-359
                        if (dcrtentry[i].angle > 359 || driveentry[dcrtentry[i].drtmatch].angle > 359) {
                            if (dcrtentry[i].angle > 359) {
                                baddcrtangle.c[baddcrtangle.count] = i;
                                baddcrtangle.d[baddcrtangle.count] = dcrtentry[i].drtmatch;
                                baddcrtangle.data[baddcrtangle.count] = dcrtentry[i].CID;
                                baddcrtangle.count++;
                            }
                            if (driveentry[dcrtentry[i].drtmatch].angle > 359) {
                                baddriveangle.c[baddriveangle.count] = i;
                                baddriveangle.d[baddriveangle.count] = dcrtentry[i].drtmatch;
                                baddriveangle.data[baddriveangle.count] = dcrtentry[i].CID;
                                baddriveangle.count++;
                            }
                        }
                        else {
                            if (driveentry[dcrtentry[i].drtmatch].angle == 359) {
                                foundangle359_count++;
                            }
                            // calculate angle deviation
                            driveentry[dcrtentry[i].drtmatch].deviation =
                                getangledeviation((int) driveentry[dcrtentry[i].drtmatch].angle, (int) dcrtentry[i].angle);
                            if (abs(driveentry[dcrtentry[i].drtmatch].deviation) > highangledev_value) {
                                highangledev.c[highangledev.count] = i;
                                highangledev.d[highangledev.count] = dcrtentry[i].drtmatch;
                                highangledev.data[highangledev.count] = dcrtentry[i].CID;
                                highangledev.count++;
                            }
                            else if (abs(driveentry[dcrtentry[i].drtmatch].deviation) > mediumangledev_value) {
                                mediumangledev.c[mediumangledev.count] = i;
                                mediumangledev.d[mediumangledev.count] = dcrtentry[i].drtmatch;
                                mediumangledev.data[mediumangledev.count] = dcrtentry[i].CID;
                                mediumangledev.count++;
                            }
                            else if (abs(driveentry[dcrtentry[i].drtmatch].deviation) > fixangledev_value) {
                                userangledev.c[userangledev.count] = i;
                                userangledev.d[userangledev.count] = dcrtentry[i].drtmatch;
                                userangledev.data[userangledev.count] = dcrtentry[i].CID;
                                userangledev.count++;
                            }
                        }
                    }
                }
            }
            else if (dcrtentry[i].CT == 0x01 || dcrtentry[i].CT == 0xE0) {
                if ((dcrtentry[i].CT == 0x01 && driveentry[dcrtentry[i].drtmatch].RT != 0x00) ||
                    (dcrtentry[i].CT == 0xE0 && driveentry[dcrtentry[i].drtmatch].RT != 0xE0)) {
                        CTRTmismatch.c[CTRTmismatch.count] = i;
                        CTRTmismatch.d[CTRTmismatch.count] = dcrtentry[i].drtmatch;
                        CTRTmismatch.data[CTRTmismatch.count] = dcrtentry[i].CID;
                        CTRTmismatch.count++;
                }
                if (dcrtentry[i].CT == 0x01) {
                    // there should probably only be one of these types but if more than one they should definitely all
                    // share the same CD - this condition was already tested for earlier while filling in the dcrt entries
                    if (CT01_conflict) {
                        CT01conflictingCD.c[CT01conflictingCD.count] = i;
                        CT01conflictingCD.d[CT01conflictingCD.count] = dcrtentry[i].drtmatch;
                        CT01conflictingCD.data[CT01conflictingCD.count] = dcrtentry[i].CID;
                        CT01conflictingCD.count++;
                    }
                    // cpr_mai bytes should match dcrt CD unless there are multiple entries with conflicting CD
                    // in that case we wouldn't know what cpr_mai should be
                    else if (memcmp(dcrtentry[i].CD, ss+0x2D0, 4) != 0) {
                        badcpr_mai.c[badcpr_mai.count] = i;
                        badcpr_mai.d[badcpr_mai.count] = dcrtentry[i].drtmatch;
                        badcpr_mai.data[badcpr_mai.count] = dcrtentry[i].CID;
                        badcpr_mai.count++;
                    }
                    // bytes 2,3,5,6 of driveentry data should match dcrt response
                    if (memcmp(dcrtentry[i].Response, driveentry[dcrtentry[i].drtmatch].Data+1, 2) != 0 ||
                        memcmp(dcrtentry[i].Response+2, driveentry[dcrtentry[i].drtmatch].Data+4, 2) != 0) {
                            type0dataresponsemismatch.c[type0dataresponsemismatch.count] = i;
                            type0dataresponsemismatch.d[type0dataresponsemismatch.count] = dcrtentry[i].drtmatch;
                            type0dataresponsemismatch.data[type0dataresponsemismatch.count] = dcrtentry[i].CID;
                            type0dataresponsemismatch.count++;
                    }
                }
            }
            else {
                unrecognizedCT.c[unrecognizedCT.count] = i;
                unrecognizedCT.d[unrecognizedCT.count] = dcrtentry[i].drtmatch;
                unrecognizedCT.data[unrecognizedCT.count] = dcrtentry[i].CID;
                unrecognizedCT.count++;
            }
        }
    }
    // print bad shit
    if (verbose && (unmatcheddcrtentry.count || multiplematcheddcrtentry.count || CTRTmismatch.count ||
        badCD.count || badResponse.count || weirdCDResponse.count || unrecognizedCT.count || badcpr_mai.count ||
        type0dataresponsemismatch.count || baddcrtangle.count || baddriveangle.count || baddriveangle2.count ||
        highangledev.count || mediumangledev.count || userangledev.count || CT01conflictingCD.count ||
        highangledev2.count || mediumangledev2.count || userangledev2.count ||
        highangledev3.count || mediumangledev3.count || userangledev3.count))
            printf("%s", newline);
    if (CT01conflictingCD.count) {
        ss_stealthfailed = true;
        printbadshit(CT01conflictingCD, red);
    }
    if (unmatcheddcrtentry.count) {
        ss_stealthfailed = true;
        printbadshit(unmatcheddcrtentry, red);
    }
    if (multiplematcheddcrtentry.count) {
        ss_stealthfailed = true;
        printbadshit(multiplematcheddcrtentry, red);
    }
    if (CTRTmismatch.count) {
        ss_stealthfailed = true;
        printbadshit(CTRTmismatch, red);
    }
    if (badCD.count) {
        if (ssv2 || !fixDRT || !writefile) { ss_stealthfailed = true; drtfucked = true; }
        printbadshit(badCD, red);
    }
    if (badResponse.count) {
        if (ssv2 || !fixDRT || !writefile) { ss_stealthfailed = true; drtfucked = true; }
        printbadshit(badResponse, red);
    }
    if (weirdCDResponse.count) {
        ss_stealthfailed = true;
        printbadshit(weirdCDResponse, red);
    }
    if (unrecognizedCT.count) {
        ss_stealthuncertain = true;
        printbadshit(unrecognizedCT, yellow);
    }
    if (badcpr_mai.count) {
        if (ssv2 || !fixDRT || !writefile) { ss_stealthfailed = true; drtfucked = true; }
        printbadshit(badcpr_mai, red);
    }
    if (type0dataresponsemismatch.count) {
        ss_stealthfailed = true;
        printbadshit(type0dataresponsemismatch, red);
    }
    if (baddcrtangle.count) {
        ss_stealthfailed = true;
        printbadshit(baddcrtangle, red);
    }
    if (baddriveangle.count) {
        if (ssv2 || !fixDRT || !writefile) { ss_stealthfailed = true; drtfucked = true; }
        printbadshit(baddriveangle, red);
    }
    if (baddriveangle2.count) {
        ss_stealthfailed = true;
        drtfucked = true;
        printbadshit(baddriveangle2, red);
    }
    // SS v1 angle deviation from CCRT
    if (highangledev.count) {
        if (ssv2 || !fixdeviation || !writefile) { ss_stealthfailed = true; drtfucked = true; }
        printbadshit(highangledev, red);
    }
    if (mediumangledev.count) {
        if (ssv2 || !fixdeviation || !writefile) { ss_stealthuncertain = true; drtfucked = true; }
        printbadshit(mediumangledev, yellow);
    }
    if (userangledev.count) {
        printbadshit(userangledev, normal);
    }
    // SS v2 angle deviation from CCRT (for when we don't trust ss v2 angles)
    if (highangledev2.count) {
        ss_stealthfailed = true;
        drtfucked = true;
        printbadshit(highangledev2, red);
    }
    if (mediumangledev2.count) {
        ss_stealthuncertain = true;
        drtfucked = true;
        printbadshit(mediumangledev2, yellow);
    }
    if (userangledev2.count) {
        printbadshit(userangledev2, normal);
    }
    // SS v1 angle deviation from v2 (for when we trust ss v2 angles)
    if (highangledev3.count) {
        ss_stealthfailed = true;
        drtfucked = true;
        printbadshit(highangledev3, red);
    }
    if (mediumangledev3.count) {
        ss_stealthuncertain = true;
        drtfucked = true;
        printbadshit(mediumangledev3, yellow);
    }
    if (userangledev3.count) {
        printbadshit(userangledev3, normal);
    }
    
    if (verbose && (unmatcheddcrtentry.count || multiplematcheddcrtentry.count || CTRTmismatch.count ||
        badCD.count || badResponse.count || weirdCDResponse.count || unrecognizedCT.count || badcpr_mai.count ||
        type0dataresponsemismatch.count || baddcrtangle.count || baddriveangle.count || baddriveangle2.count ||
        highangledev.count || mediumangledev.count || userangledev.count || CT01conflictingCD.count ||
        highangledev2.count || mediumangledev2.count || userangledev2.count ||
        highangledev3.count || mediumangledev3.count || userangledev3.count))
            printf("%s", newline);
    
    if (extraverbose || (verbose && (unmatcheddcrtentry.count || multiplematcheddcrtentry.count || CTRTmismatch.count || badCD.count ||
        badResponse.count || weirdCDResponse.count || unrecognizedCT.count || badcpr_mai.count || type0dataresponsemismatch.count ||
        baddcrtangle.count || baddriveangle.count || baddriveangle2.count || CT01conflictingCD.count ||
        highangledev.count || mediumangledev.count || userangledev.count ||
        highangledev2.count || mediumangledev2.count || userangledev2.count ||
        highangledev3.count || mediumangledev3.count || userangledev3.count))) {
        if (ssv2) {
            printf("%sSS Version: 2", sp5);
            if (trustssv2angles) printf(" (trusted)%s", newline);
            else printf(" (not trusted)%s", newline);
        }
        else printf("%sSS Version: 1%s", sp5, newline);
        color(blue);
        if (terminal) printf("%sÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ%s", sp5, newline);
        else          printf("%s------------------------------------------------------------------------%s", sp5, newline);
        color(normal);
        printf("%sCT RT CID Mod? Pad? Data %s CD %s Response %s Angle Deviation%s", sp5, sp8, sp5, sp1, newline);
        color(blue);
        if (terminal) printf("%sÄÄ ÄÄ ÄÄ%s ÄÄ %s ÄÄ %s ÄÄÄÄÄÄÄÄÄÄÄÄÄ ÄÄÄÄÄÄÄÄ ÄÄÄÄÄÄÄÄÄÄ ÄÄÄÄÄ ÄÄÄÄÄÄÄÄÄÄÄÄ%s", sp5, sp1, sp1, sp1, newline);
        else          printf("%s-- -- --%s -- %s -- %s ------------- -------- ---------- ----- ------------%s", sp5, sp1, sp1, sp1, newline);
        color(normal);
        bool stayred;
        for (i=0;i<21;i++) {
            // print decrypted challenge response table entries
            if (!showfulltable && (dcrtentry[i].CT & 0xF0) == 0xF0) continue;
            printf("%s", sp5);
            if (unrecognizedCT.count) for (j=0;j<unrecognizedCT.count;j++) if (unrecognizedCT.c[j] == i) color(yellow);
            printf("%02X %s %02X%s %02X %s %02X %s%s ",
                    dcrtentry[i].CT, sp2, dcrtentry[i].CID, sp1, dcrtentry[i].Mod, sp1, dcrtentry[i].WTF, sp10, sp5);
            if (weirdCDResponse.count) for (j=0;j<weirdCDResponse.count;j++) if (weirdCDResponse.c[j] == i) color(red);
            if (CT01conflictingCD.count && dcrtentry[i].CT == 0x01) color(red);
            printf("%02X%02X", dcrtentry[i].CD[0], dcrtentry[i].CD[1]);
            if (weirdCDResponse.count) for (j=0;j<weirdCDResponse.count;j++) if (weirdCDResponse.c[j] == i) color(normal);
            printf("%02X%02X ", dcrtentry[i].CD[2], dcrtentry[i].CD[3]);
            if (CT01conflictingCD.count && dcrtentry[i].CT == 0x01) color(normal);
            if (weirdCDResponse.count) for (j=0;j<weirdCDResponse.count;j++) if (weirdCDResponse.c[j] == i) color(red);
            printf("%02X%02X", dcrtentry[i].Response[0], dcrtentry[i].Response[1]);
            if (weirdCDResponse.count) for (j=0;j<weirdCDResponse.count;j++) if (weirdCDResponse.c[j] == i) color(normal);
            printf("%02X%02X", dcrtentry[i].Response[2], dcrtentry[i].Response[3]);
            if (unrecognizedCT.count) for (j=0;j<unrecognizedCT.count;j++) if (unrecognizedCT.c[j] == i) color(normal);
            if (dcrtentry[i].CT == 0x24 || dcrtentry[i].CT == 0x25) {  // corresponds to RT 7/5 in the DRT
                printf(" %s %u", sp1, dcrtentry[i].angle);
                if (dcrtentry[i].angle < 10) printf("%s", sp4);
                else if (dcrtentry[i].angle < 100) printf("%s", sp3);
                else if (dcrtentry[i].angle < 1000) printf("%s", sp2);
                else if (dcrtentry[i].angle < 10000) printf("%s", sp1);
                if (dcrtentry[i].angle > 359) {
                    color(red); printf(" INVALID ANGLE"); color(normal);
                }
            }
            printf("%s", newline);
            if (dcrtentry[i].matches == 1) {
                // print matching drive entry
                stayred = false;
                if (CTRTmismatch.count) for (j=0;j<CTRTmismatch.count;j++) if (CTRTmismatch.c[j] == i) { stayred = true; color(red); }
                printf("%s%02X %02X%s %02X %s %02X", sp8, driveentry[dcrtentry[i].drtmatch].RT, driveentry[dcrtentry[i].drtmatch].CID, sp1,
                       driveentry[dcrtentry[i].drtmatch].Mod, sp6, driveentry[dcrtentry[i].drtmatch].Data[0]);
                if (type0dataresponsemismatch.count && !stayred) for (j=0;j<type0dataresponsemismatch.count;j++) if (type0dataresponsemismatch.c[j] == i) color(red);
                printf("%02X%02X", driveentry[dcrtentry[i].drtmatch].Data[1], driveentry[dcrtentry[i].drtmatch].Data[2]);
                if (type0dataresponsemismatch.count && !stayred) for (j=0;j<type0dataresponsemismatch.count;j++) if (type0dataresponsemismatch.c[j] == i) color(normal);
                printf(" %02X", driveentry[dcrtentry[i].drtmatch].Data[3]);
                if (type0dataresponsemismatch.count && !stayred) for (j=0;j<type0dataresponsemismatch.count;j++) if (type0dataresponsemismatch.c[j] == i) color(red);
                printf("%02X%02X ", driveentry[dcrtentry[i].drtmatch].Data[4], driveentry[dcrtentry[i].drtmatch].Data[5]);
                if (type0dataresponsemismatch.count && !stayred) for (j=0;j<type0dataresponsemismatch.count;j++) if (type0dataresponsemismatch.c[j] == i) color(normal);
                if (badCD.count && !stayred) for (j=0;j<badCD.count;j++) if (badCD.c[j] == i) color(red);
                printf("%02X%02X%02X%02X ", driveentry[dcrtentry[i].drtmatch].CD[0], driveentry[dcrtentry[i].drtmatch].CD[1],
                        driveentry[dcrtentry[i].drtmatch].CD[2], driveentry[dcrtentry[i].drtmatch].CD[3]);
                if (badCD.count && !stayred) for (j=0;j<badCD.count;j++) if (badCD.c[j] == i) color(normal);
                if (badResponse.count && !stayred) for (j=0;j<badResponse.count;j++) if (badResponse.c[j] == i) color(red);
                printf("%02X%02X%02X%02X%02X", driveentry[dcrtentry[i].drtmatch].Response[0], driveentry[dcrtentry[i].drtmatch].Response[1],
                        driveentry[dcrtentry[i].drtmatch].Response[2], driveentry[dcrtentry[i].drtmatch].Response[3], driveentry[dcrtentry[i].drtmatch].Response[4]);
                if (badResponse.count && !stayred) for (j=0;j<badResponse.count;j++) if (badResponse.c[j] == i) color(normal);
                if (ssv2) {
                    if (dcrtentry[i].CT == 0x24 || dcrtentry[i].CT == 0x25) {  // corresponds to RT 7/5 in the DRT
                        printf("%s%s%s v1 deviation from CCRT %s %u", newline, sp21, sp3, sp10, driveentry[dcrtentry[i].drtmatch].angle);
                        if (driveentry[dcrtentry[i].drtmatch].angle < 10) printf("%s", sp4);
                        else if (driveentry[dcrtentry[i].drtmatch].angle < 100) printf("%s", sp3);
                        else if (driveentry[dcrtentry[i].drtmatch].angle < 1000) printf("%s", sp2);
                        else if (driveentry[dcrtentry[i].drtmatch].angle < 10000) printf("%s", sp1);
                        if (driveentry[dcrtentry[i].drtmatch].angle > 359) {
                            color(red); printf(" INVALID ANGLE"); color(normal);
                        }
                        else if (dcrtentry[i].angle < 360 && driveentry[dcrtentry[i].drtmatch].angle2 < 360) {  // dcrt angle and drive angle 1 & 2 must be 0-359 or deviation isn't calculated
                            // print angle 1 deviation from ccrt
                            printf("%s", sp1);
                            if (!trustssv2angles) {
                                if (abs(driveentry[dcrtentry[i].drtmatch].deviation) > highangledev_value) color(red);
                                else if (abs(driveentry[dcrtentry[i].drtmatch].deviation) > mediumangledev_value) color(yellow);
                            }
                            driveentry[dcrtentry[i].drtmatch].deviation == 0 ? printf(" 0") : printf("%+d", driveentry[dcrtentry[i].drtmatch].deviation);
                            if (abs(driveentry[dcrtentry[i].drtmatch].deviation) < 10) printf("%s", sp2);
                            else if (abs(driveentry[dcrtentry[i].drtmatch].deviation) < 100) printf("%s", sp1);
                            printf(" (%04.1f%%)", (float) abs(driveentry[dcrtentry[i].drtmatch].deviation) / 1.8);
                            if (!trustssv2angles && abs(driveentry[dcrtentry[i].drtmatch].deviation) > (mediumangledev_value < highangledev_value ? mediumangledev_value : highangledev_value)) color(normal);
                        }
                        printf("%s%s%s v2 deviation from CCRT %s %u", newline, sp21, sp3, sp10, driveentry[dcrtentry[i].drtmatch].angle2);
                        if (driveentry[dcrtentry[i].drtmatch].angle2 < 10) printf("%s", sp4);
                        else if (driveentry[dcrtentry[i].drtmatch].angle2 < 100) printf("%s", sp3);
                        else if (driveentry[dcrtentry[i].drtmatch].angle2 < 1000) printf("%s", sp2);
                        else if (driveentry[dcrtentry[i].drtmatch].angle2 < 10000) printf("%s", sp1);
                        if (driveentry[dcrtentry[i].drtmatch].angle2 > 359) {
                            color(red); printf(" INVALID ANGLE"); color(normal);
                        }
                        else if (dcrtentry[i].angle < 360 && driveentry[dcrtentry[i].drtmatch].angle < 360) {  // dcrt angle and drive angle 1 & 2 must be 0-359 or deviation isn't calculated
                            // print angle 2 deviation from ccrt
                            printf("%s", sp1);
                            if (!trustssv2angles) {
                                if (abs(driveentry[dcrtentry[i].drtmatch].deviation2) > highangledev_value) color(red);
                                else if (abs(driveentry[dcrtentry[i].drtmatch].deviation2) > mediumangledev_value) color(yellow);
                            }
                            driveentry[dcrtentry[i].drtmatch].deviation2 == 0 ? printf(" 0") : printf("%+d", driveentry[dcrtentry[i].drtmatch].deviation2);
                            if (abs(driveentry[dcrtentry[i].drtmatch].deviation2) < 10) printf("%s", sp2);
                            else if (abs(driveentry[dcrtentry[i].drtmatch].deviation2) < 100) printf("%s", sp1);
                            printf(" (%04.1f%%)", (float) abs(driveentry[dcrtentry[i].drtmatch].deviation2) / 1.8);
                            if (!trustssv2angles && abs(driveentry[dcrtentry[i].drtmatch].deviation2) > (mediumangledev_value < highangledev_value ? mediumangledev_value : highangledev_value)) color(normal);
                        }
                        printf("%s%s%s v1 deviation from v2 %s", newline, sp21, sp3, sp18);
                        if (driveentry[dcrtentry[i].drtmatch].angle > 359) {
                            color(red); printf(" INVALID ANGLE"); color(normal);
                        }
                        else if (dcrtentry[i].angle < 360 && driveentry[dcrtentry[i].drtmatch].angle2 < 360) {  // dcrt angle and drive angle 1 & 2 must be 0-359 or deviation isn't calculated
                            // print angle 1 deviation from angle 2
                            printf("%s", sp1);
                            if (trustssv2angles) {
                                if (abs(driveentry[dcrtentry[i].drtmatch].deviation3) > highangledev_value) color(red);
                                else if (abs(driveentry[dcrtentry[i].drtmatch].deviation3) > mediumangledev_value) color(yellow);
                            }
                            driveentry[dcrtentry[i].drtmatch].deviation3 == 0 ? printf(" 0") : printf("%+d", driveentry[dcrtentry[i].drtmatch].deviation3);
                            if (abs(driveentry[dcrtentry[i].drtmatch].deviation3) < 10) printf("%s", sp2);
                            else if (abs(driveentry[dcrtentry[i].drtmatch].deviation3) < 100) printf("%s", sp1);
                            printf(" (%04.1f%%)", (float) abs(driveentry[dcrtentry[i].drtmatch].deviation3) / 1.8);
                            if (trustssv2angles && abs(driveentry[dcrtentry[i].drtmatch].deviation3) > (mediumangledev_value < highangledev_value ? mediumangledev_value : highangledev_value)) color(normal);
                        }
                    }
                }
                else {
                    // ss v1
                    if (dcrtentry[i].CT == 0x24 || dcrtentry[i].CT == 0x25) {  // corresponds to RT 7/5 in the DRT
                        printf(" %u", driveentry[dcrtentry[i].drtmatch].angle);
                        if (driveentry[dcrtentry[i].drtmatch].angle < 10) printf("%s", sp4);
                        else if (driveentry[dcrtentry[i].drtmatch].angle < 100) printf("%s", sp3);
                        else if (driveentry[dcrtentry[i].drtmatch].angle < 1000) printf("%s", sp2);
                        else if (driveentry[dcrtentry[i].drtmatch].angle < 10000) printf("%s", sp1);
                        if (driveentry[dcrtentry[i].drtmatch].angle > 359) {
                            color(red); printf(" INVALID ANGLE"); color(normal);
                        }
                        else if (dcrtentry[i].angle < 360) {  // both dcrt angle and drive angle must be 0-359 or deviation isn't calculated
                            // print angle deviation
                            printf("%s", sp1);
                            if (abs(driveentry[dcrtentry[i].drtmatch].deviation) > highangledev_value) color(red);
                            else if (abs(driveentry[dcrtentry[i].drtmatch].deviation) > mediumangledev_value) color(yellow);
                            driveentry[dcrtentry[i].drtmatch].deviation == 0 ? printf(" 0") : printf("%+d", driveentry[dcrtentry[i].drtmatch].deviation);
                            if (abs(driveentry[dcrtentry[i].drtmatch].deviation) < 10) printf("%s", sp2);
                            else if (abs(driveentry[dcrtentry[i].drtmatch].deviation) < 100) printf("%s", sp1);
                            printf(" (%04.1f%%)", (float) abs(driveentry[dcrtentry[i].drtmatch].deviation) / 1.8);
                            if (abs(driveentry[dcrtentry[i].drtmatch].deviation) > (mediumangledev_value < highangledev_value ? mediumangledev_value : highangledev_value)) color(normal);
                        }
                    }
                }
                if (stayred) color(normal);
                printf("%s", newline);
            }
            else if (dcrtentry[i].matches == 0) {
                color(red); printf("%sFailed to find a drive entry for Challenge ID %02X%s", sp8, dcrtentry[i].CID, newline); color(normal);
            }
            else {
                color(red); printf("%sFound multiple drive entries for Challenge ID %02X:%s", sp8, dcrtentry[i].CID, newline); color(normal);
                for (j=0;j<21;j++) {
                    if (dcrtentry[i].CID == driveentry[j].CID) {
                        printf("%s%02X %02X%s %02X %s %02X%02X%02X %02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X%02X",
                                sp8, driveentry[j].RT, driveentry[j].CID, sp1, driveentry[j].Mod, sp6,
                                driveentry[j].Data[0], driveentry[j].Data[1], driveentry[j].Data[2], driveentry[j].Data[3], driveentry[j].Data[4], driveentry[j].Data[5],
                                driveentry[j].CD[0], driveentry[j].CD[1], driveentry[j].CD[2], driveentry[j].CD[3],
                                driveentry[j].Response[0], driveentry[j].Response[1], driveentry[j].Response[2], driveentry[j].Response[3], driveentry[j].Response[4]);
                        if (driveentry[j].RT == 0x05 || driveentry[j].RT == 0x07) {
                            printf(" %u", driveentry[j].angle);
                            if (driveentry[j].angle < 10) printf("%s", sp4);
                            else if (driveentry[j].angle < 100) printf("%s", sp3);
                            else if (driveentry[j].angle < 1000) printf("%s", sp2);
                            else if (driveentry[j].angle < 10000) printf("%s", sp1);
                            if (driveentry[j].angle > 359) {
                                color(red); printf(" INVALID ANGLE"); color(normal);
                            }
                        }
                        printf("%s", newline);
                    }
                }
            }
        }
        color(blue);
        if (terminal) printf("%sÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ%s", sp5, newline);
        else          printf("%s------------------------------------------------------------------------%s", sp5, newline);
        color(normal);
    }
    else if (verbose) {
        if (ssv2) {
            printf("%sSS Version: 2", sp5);
            if (trustssv2angles) printf(" (trusted)%s", newline);
            else printf(" (not trusted)%s", newline);
        }
        else printf("%sSS Version: 1%s", sp5, newline);
    }
    if (extraverbose || debug || (verbose && badcpr_mai.count)) {
        printf("%sCPR_MAI: ", sp5);
        if (badcpr_mai.count) color(red);
        for (i=0x2D0; i<0x2D4; i++) printf("%02X", ss[i]);
        if (badcpr_mai.count) color(normal);
        printf("%s", newline);
    }
    bool triedtofix = false;
    if (ssv2) {
        if (foundangle359_count && fixangle359) {
            // the only thing we are willing to 'fix' in ss v2
            triedtofix = true;
            if (verbose) printf("%s", newline);
            if (!writefile) {
                printf("Unable to fix the following item(s) because writing is disabled:%s", newline);
                color(yellow);
                printf("Found a 359 degree SS v1 angle%s", newline);
                color(normal);
            }
            else {
                for (i=0;i<21;i++) {
                    if (driveentry[i].RT == 0x07 || driveentry[i].RT == 0x05) {
                        if (ss[0x200+i*9+4] == 0x67 && ss[0x200+i*9+5] == 0x01) {
                            ss[0x200+i*9+4] = 0x0;
                            ss[0x200+i*9+5] = 0x0;
                            fixedss = true;
                        }
                    }
                }
                foundangle359_count = 0;
                color(green);
                printf("All 359 degree SS v1 angles have been adjusted for compatibility with\niXtreme %s v1.4%s", lessthan, newline);
                color(normal);
            }
            if (verbose) printf("%s", newline);
        }
        // anything else bad will not be fixed (there's a pretty good chance we'll fuck it up), so they should try to rip a new ss or use the one retrieved through autofix
    }
    else if ( ( fixDRT && (badCD.count || badResponse.count || badcpr_mai.count || baddriveangle.count) ) ||
              ( fixdeviation && (highangledev.count || mediumangledev.count || userangledev.count) ) ||
              ( foundangle359_count && fixangle359 ) ) {
        triedtofix = true;
        if (verbose) printf("%s", newline);
        printf("Fixing SS Challenge / Response Data...%s", newline);
        if (!writefile) {
            printf("Unable to fix the following item(s) because writing is disabled:%s", newline);
            if (fixDRT && (badCD.count || badResponse.count || badcpr_mai.count || baddriveangle.count)) {
                if (badCD.count) printbadshit(badCD, red);
                if (badResponse.count) printbadshit(badResponse, red);
                if (badcpr_mai.count) printbadshit(badcpr_mai, red);
                if (baddriveangle.count) printbadshit(baddriveangle, red);
            }
            if (fixdeviation && (highangledev.count || mediumangledev.count || userangledev.count)) {
                if (highangledev.count) printbadshit(highangledev, red);
                if (mediumangledev.count) printbadshit(mediumangledev, yellow);
                if (userangledev.count) printbadshit(userangledev, normal);
            }
            if (foundangle359_count && fixangle359) {
                color(yellow); printf("Found a 359 degree SS v1 angle%s", newline); color(normal);
            }
        }
        else {
            if (fixDRT) {
                if (badCD.count) {
                    for (i=0;i<badCD.count;i++) {
                        memcpy(ss+0x200+badCD.d[i]*9, dcrtentry[badCD.c[i]].CD, 4);
                    }
                    fixedss = true;
                    printfixedshit(badCD);
                }
                if (badResponse.count) {
                    for (i=0;i<badResponse.count;i++) {
                        memcpy(ss+0x200+badResponse.d[i]*9+4, dcrtentry[badResponse.c[i]].Response, 4);
                    }
                    fixedss = true;
                    printfixedshit(badResponse);
                }
                if (badcpr_mai.count) {
                    memcpy(ss+0x2D0, dcrtentry[badcpr_mai.c[0]].CD, 4);
                    fixedss = true;
                    printfixedshit(badcpr_mai);
                }
                if (baddriveangle.count) {
                    int fixedbaddriveangle_count = 0;
                    for (i=0;i<baddriveangle.count;i++) {
                        if (fixangle359 && dcrtentry[baddriveangle.c[i]].angle == 359) {
                            ss[0x200+baddriveangle.d[i]*9+4] = 0x0;
                            ss[0x200+baddriveangle.d[i]*9+5] = 0x0;
                            fixedss = true;
                            fixedbaddriveangle_count++;
                        }
                        else if (dcrtentry[baddriveangle.c[i]].angle < 360) {
                            ss[0x200+baddriveangle.d[i]*9+4] = dcrtentry[baddriveangle.c[i]].Response[3];
                            ss[0x200+baddriveangle.d[i]*9+5] = dcrtentry[baddriveangle.c[i]].Response[2];
                            fixedss = true;
                            fixedbaddriveangle_count++;
                        }
                        else {
                            // replace a bad angle with a bad angle?
                            ss_stealthfailed = true;
                            drtfucked = true;
                        }
                    }
                    if (fixedbaddriveangle_count == baddriveangle.count) printfixedshit(baddriveangle);
                    else if (baddriveangle.count - fixedbaddriveangle_count == 1) {
                        color(red);
                        printf("1 invalid SS v1 drive angle value was not fixed because its CCRT target is also invalid!%s", newline);
                        color(normal);
                    }
                    else {
                        color(red);
                        printf("%d invalid SS v1 drive angle values were not fixed because their CCRT targets are also invalid!%s",
                                baddriveangle.count - fixedbaddriveangle_count, newline);
                        color(normal);
                    }
                }
            }
            if (fixdeviation) {
                if (highangledev.count) {
                    int fixedhighangledev_count = 0;
                    for (i=0;i<highangledev.count;i++) {
                        if (abs(driveentry[highangledev.d[i]].deviation) > fixangledev_value) {
                            //if (foundangle359_count && driveentry[highangledev.d[i]].angle == 359) foundangle359_count--;
                            fixedhighangledev_count++;
                            fixedss = true;
                            if (fixangle359 && dcrtentry[highangledev.c[i]].angle == 359) {
                                ss[0x200+highangledev.d[i]*9+4] = 0x0;
                                ss[0x200+highangledev.d[i]*9+5] = 0x0;
                            }
                            else {
                                ss[0x200+highangledev.d[i]*9+4] = dcrtentry[highangledev.c[i]].Response[3];
                                ss[0x200+highangledev.d[i]*9+5] = dcrtentry[highangledev.c[i]].Response[2];
                            }
                        }
                        else {
                            ss_stealthfailed = true;
                            drtfucked = true;
                        }
                    }
                    if (fixedhighangledev_count == highangledev.count) printfixedshit(highangledev);
                    else if (highangledev.count - fixedhighangledev_count == 1) {
                        color(red);
                        printf("1 angle deviation %s 9 degrees was not fixed because you set the limit to %d%s",
                                greaterthan, fixangledev_value, newline);
                        color(normal);
                    }
                    else {
                        color(red);
                        printf("%d angle deviations %s 9 degrees were not fixed because you set the limit to %d%s",
                                highangledev.count - fixedhighangledev_count, greaterthan, fixangledev_value, newline);
                        color(normal);
                    }
                }
                if (mediumangledev.count) {
                    int fixedmediumangledev_count = 0;
                    for (i=0;i<mediumangledev.count;i++) {
                        if (abs(driveentry[mediumangledev.d[i]].deviation) > fixangledev_value) {
                            //if (foundangle359_count && driveentry[mediumangledev.d[i]].angle == 359) foundangle359_count--;
                            fixedmediumangledev_count++;
                            fixedss = true;
                            if (fixangle359 && dcrtentry[mediumangledev.c[i]].angle == 359) {
                                ss[0x200+mediumangledev.d[i]*9+4] = 0x0;
                                ss[0x200+mediumangledev.d[i]*9+5] = 0x0;
                            }
                            else {
                                ss[0x200+mediumangledev.d[i]*9+4] = dcrtentry[mediumangledev.c[i]].Response[3];
                                ss[0x200+mediumangledev.d[i]*9+5] = dcrtentry[mediumangledev.c[i]].Response[2];
                            }
                        }
                        else {
                            ss_stealthuncertain = true;
                            drtfucked = true;
                        }
                    }
                    if (fixedmediumangledev_count == mediumangledev.count) printfixedshit(mediumangledev);
                    else if (mediumangledev.count - fixedmediumangledev_count == 1) {
                        color(yellow);
                        printf("1 angle deviation %s 3 degrees was not fixed because you set the limit to %d%s",
                                greaterthan, fixangledev_value, newline);
                        color(normal);
                    }
                    else {
                        color(yellow);
                        printf("%d angle deviations %s 3 degrees were not fixed because you set the limit to %d%s",
                                mediumangledev.count - fixedmediumangledev_count, greaterthan, fixangledev_value, newline);
                        color(normal);
                    }
                }
                if (userangledev.count) {
                    for (i=0;i<userangledev.count;i++) {
                        //if (foundangle359_count && driveentry[userangledev.d[i]].angle == 359) foundangle359_count--;
                        fixedss = true;
                        if (fixangle359 && dcrtentry[userangledev.c[i]].angle == 359) {
                            ss[0x200+userangledev.d[i]*9+4] = 0x0;
                            ss[0x200+userangledev.d[i]*9+5] = 0x0;
                        }
                        else {
                            ss[0x200+userangledev.d[i]*9+4] = dcrtentry[userangledev.c[i]].Response[3];
                            ss[0x200+userangledev.d[i]*9+5] = dcrtentry[userangledev.c[i]].Response[2];
                        }
                    }
                    printfixedshit(userangledev);
                }
            }
            if (foundangle359_count && fixangle359) {
                for (i=0;i<21;i++) {
                    if (driveentry[i].RT == 0x07 || driveentry[i].RT == 0x05) {
                        if (ss[0x200+i*9+4] == 0x67 && ss[0x200+i*9+5] == 0x01) {
                            ss[0x200+i*9+4] = 0x0;
                            ss[0x200+i*9+5] = 0x0;
                            fixedss = true;
                        }
                    }
                }
                foundangle359_count = 0;
                color(green);
                printf("All 359 degree SS v1 angles have been adjusted for compatibility with\niXtreme %s v1.4%s", lessthan, newline);
                color(normal);
            }
        }
        if (!fixDRT && (badCD.count || badResponse.count || badcpr_mai.count || baddriveangle.count)) {
            printf("Unable to fix the following item(s) because fixing bad C/R data is disabled:%s", newline);
            if (badCD.count) printbadshit(badCD, red);
            if (badResponse.count) printbadshit(badResponse, red);
            if (badcpr_mai.count) printbadshit(badcpr_mai, red);
            if (baddriveangle.count) printbadshit(baddriveangle, red);
        }
        if (!fixdeviation && (highangledev.count || mediumangledev.count || userangledev.count)) {
            printf("Unable to fix the following item(s) because fixing angle deviation is disabled:%s", newline); color(normal);
            if (highangledev.count) printbadshit(highangledev, red);
            if (mediumangledev.count) printbadshit(mediumangledev, yellow);
            if (userangledev.count) printbadshit(userangledev, normal);
        }
        if (unmatcheddcrtentry.count || multiplematcheddcrtentry.count || CTRTmismatch.count || weirdCDResponse.count ||
        type0dataresponsemismatch.count || baddcrtangle.count || CT01conflictingCD.count || unrecognizedCT.count) {
            printf("Unable to fix the following item(s) because they can't (or shouldn't) be fixed:%s", newline);
            if (unmatcheddcrtentry.count) printbadshit(unmatcheddcrtentry, red);
            if (multiplematcheddcrtentry.count) printbadshit(multiplematcheddcrtentry, red);
            if (CTRTmismatch.count) printbadshit(CTRTmismatch, red);
            if (weirdCDResponse.count) printbadshit(weirdCDResponse, red);
            if (type0dataresponsemismatch.count) printbadshit(type0dataresponsemismatch, red);
            if (baddcrtangle.count) printbadshit(baddcrtangle, red);
            if (CT01conflictingCD.count) printbadshit(CT01conflictingCD, red);
            if (unrecognizedCT.count) printbadshit(unrecognizedCT, yellow);
        }
        if (verbose) printf("%s", newline);
    }
    
    if (foundangle359_count && (!fixangle359 || !writefile)) {
        color(yellow);
        if (verbose && !triedtofix) printf("%s", newline);
        printf("Caution: This SS contains a value (angle 359) that will cause older versions%s", newline);
        printf("of iXtreme to return a detectable bad response! Make sure your drive is flashed%s", newline);
        printf("with iXtreme version 1.4 or newer.");
        if (!dvdarg)                      printf(" Alternatively, you may enable the option to%s"
               "adjust angle 359 for compatibility with iXtreme %s v1.4%s%s", newline, lessthan, newline, newline);
        else printf("%s%s", newline, newline);
        color(normal);
    }
    
    // calculate SS crc32 with 0x200 to 0x2FF set as 0xFF
    unsigned char ss_overwritten[2048];
    memcpy(ss_overwritten, ss, 2048);
    memset(ss_overwritten+0x200, 0xFF, 0x100);
    ss_crc32 = crc32(0, ss_overwritten, 2048);
    if (verbose) printf("%sSS CRC = %08lX", sp5, ss_crc32);

    // calculate raw SS crc32
    ss_rawcrc32 = crc32(0, ss, 2048);
    if (verbose) printf(" (RawSS = %08lX)%s", ss_rawcrc32, newline);

    // copy the media id from 0x460 and compare to the xex
    memcpy(ss_mediaid, ss+0x460, 16);
    ss_foundmediaid = true;
    if (verbose) {
        printf("%sSS Media ID: ", sp5);
        printmediaid(ss_mediaid);
    }
    if (xex_foundmediaid) {
        if (memcmp(xex_mediaid, ss_mediaid, 16) != 0) {
            ss_stealthfailed = true;
            if (verbose) printf("%s", newline);
            color(red); printf("SS media id does not match this game!%s", newline); color(normal);
        }
        else if (verbose) printf(" (matches game)%s", newline);
    }
    else {
        if (verbose) printf("%s", newline);
        if (!checkssbin) {
            stealthuncertain = true;
            ss_stealthuncertain = true;
            color(yellow); printf("SS media id could not be compared to the Xex%s", newline); color(normal);
        }
        else checkcsv(ss_mediaid);
    }

    if (!ss_stealthfailed && !ss_stealthuncertain) {
        color(green);
        printf("SS looks valid%s", newline);
        color(normal);
    }
  return 0;
}

void showap25data(unsigned char *ap25bin) {
    int i;
    int num_challenges;
    unsigned int angle;
    if (verbose) {
        num_challenges = 0;
        for (i=0; i<128; i++) {
            if (memcmp(ap25bin+i*16, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16) != 0) {
                num_challenges++;
            }
        }
        if (num_challenges == 1) printf("%s1 challenge/response found", sp5);
        else printf("%s%d challenges/responses found", sp5, num_challenges);
        if (extraverbose) printf(":");
        printf("%s", newline);
    }
    if (extraverbose) {
        color(blue);
        if (terminal) printf("%sÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ%s", sp5, newline);
        else          printf("%s-------------------------------------%s", sp5, newline);
        color(normal);
        printf("%sPSN 1 %s PSN 2 %s Data %s Angle Pad%s", sp5, sp2, sp2, sp3, newline);
        color(blue);
        if (terminal) printf("%sÄÄÄÄÄÄÄÄ ÄÄÄÄÄÄÄÄ ÄÄÄÄÄÄÄÄ ÄÄÄÄÄ ÄÄÄÄ%s", sp5, newline);
        else          printf("%s-------- -------- -------- ----- ----%s", sp5, newline);
        color(normal);
        for (i=0; i<128; i++) {
            if (memcmp(ap25bin+i*16, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 16) != 0) {
                angle  = (unsigned int) ap25bin[i*16+12] << 8;
                angle |= (unsigned int) ap25bin[i*16+13];
                // start psn, end psn, data, angle
                printf("%s%02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X %u ",
                       sp5, ap25bin[i*16], ap25bin[i*16+1], ap25bin[i*16+2], ap25bin[i*16+3],
                       ap25bin[i*16+4], ap25bin[i*16+5], ap25bin[i*16+6], ap25bin[i*16+7],
                       ap25bin[i*16+8], ap25bin[i*16+9], ap25bin[i*16+10], ap25bin[i*16+11],
                       angle);
                if      (angle < 10)    printf("%s", sp4);
                else if (angle < 100)   printf("%s", sp3);
                else if (angle < 1000)  printf("%s", sp2);
                else if (angle < 10000) printf("%s", sp1);
                // pad
                printf("%02X%02X%s", ap25bin[i*16+14], ap25bin[i*16+15], newline);
            }
            // else break;  // todo: should we break when finding a blank line or will LT+ keep going?
        }
        color(blue);
        if (terminal) printf("%sÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ%s", sp5, newline);
        else          printf("%s-------------------------------------%s", sp5, newline);
        color(normal);
    }
  return;
}

void checkap25() {
    fixedap25 = false;
    long long filesize;
    unsigned char ap25hashfilebuffer[24], ap25binfilebuffer[2048];
    char ap25hashfilename[19], ap25binfilename[18];
    FILE *ap25hashfile = NULL, *ap25binfile = NULL;
    uchar ap25_sha1[20] = {0}, ap25_verified_sha1[20] = {0}, ap25_binfile_sha1[20] = {0};
    int i;
    if (verbose && !minimal) printf("Checking AP25 replay sector%s", newline);
    if (getzeros(ap25, 0, 2047) == 2048) {
        color(red);
        printf("AP25 replay sector is blank!%s", newline);
        color(normal);
        if (autofix) {
            printf("Attempting to fix the AP25 replay sector%s", newline);
            if (!xex_foundmediaid) {
                color(red);
                printf("Unable to fix AP25 replay sector because the Xex Media ID could not be found%s", newline);
                color(normal);
              return;
            }
            if (!writefile) {
                color(red);
                printf("Unable to fix because writing is disabled%s", newline);
                color(normal);
              return;
            }
            if ((stayoffline || localonly) && !offlinewarningprinted) {
                color(yellow);
                printf("You need to enable online functions (assuming your connection works and the db%s"
                       "is up) if you want to check for the latest files in the online database.%s", newline, newline);
                color(normal);
                offlinewarningprinted = true;
            }
            // get ap25.sha1
            memset(ap25hashfilename, 0, 19);
            sprintf(ap25hashfilename, "AP25_%02X%02X%02X%02X.sha1",
                    xex_mediaid[12], xex_mediaid[13], xex_mediaid[14], xex_mediaid[15]);
            ap25hashfile = openstealthfile(ap25hashfilename, stealthdir, webstealthdir, AP25_HASH_FILE, "the online verified database");
            if (ap25hashfile == NULL) {
                // failed to find the file online or locally
                color(yellow);
                printf("Unable to fix AP25 replay data for this Media ID%s", newline);
                color(normal);
              return;
            }
            // ap25 hash file should be exactly 24 bytes
            filesize = getfilesize(ap25hashfile);
            if (filesize == -1) {  // seek error
                fclose(ap25hashfile);
              return;
            }
            if (filesize != 24) {
                color(red);
                printf("ERROR: %s is %"LL"d bytes! (should have been exactly 24 bytes -- deleting it)%s",
                       ap25hashfilename, filesize, newline);
                color(normal);
                fclose(ap25hashfile);
                deletestealthfile(ap25hashfilename, stealthdir, false);
              return;
            }
            // read hash file into the buffer
            if (trytoreadstealthfile(ap25hashfilebuffer, 1, 24, ap25hashfile, ap25hashfilename, 0) != 0) {
                fclose(ap25hashfile);
                deletestealthfile(ap25hashfilename, stealthdir, false);  // should we be trying to delete a file we can't read?
              return;
            }
            fclose(ap25hashfile);
            // make sure the file isn't corrupt by checking the crc of the hash against the embedded crc at the end of the file
            if (crc32(0, ap25hashfilebuffer, 20) != getuintmsb(ap25hashfilebuffer+20)) {
                color(red);
                printf("ERROR: %s appears to be corrupt! (deleting it)%s", ap25hashfilename, newline);
                color(normal);
                deletestealthfile(ap25hashfilename, stealthdir, false);
              return;
            }
            memcpy(ap25_verified_sha1, ap25hashfilebuffer, 20);
            // get ap25.bin, hash it and compare to ap25_verified_sha1
            memset(ap25binfilename, 0, 18);
            sprintf(ap25binfilename, "AP25_%02X%02X%02X%02X.bin",
                    xex_mediaid[12], xex_mediaid[13], xex_mediaid[14], xex_mediaid[15]);
            ap25binfile = openstealthfile(ap25binfilename, stealthdir, webstealthdir, AP25_BIN_FILE, "the online verified database");
            if (ap25binfile == NULL) {
                // failed to find the file online or locally
                color(red);
                printf("Unable to fix AP25 replay data for this Media ID%s", newline);
                color(normal);
              return;
            }
            // ap25 bin file should be exactly 2048 bytes
            filesize = getfilesize(ap25binfile);
            if (filesize == -1) {  // seek error
                fclose(ap25binfile);
              return;
            }
            if (filesize != 2048) {
                color(red);
                printf("ERROR: %s is %"LL"d bytes! (should have been exactly 2048 bytes -- deleting it)%s",
                       ap25binfilename, filesize, newline);
                color(normal);
                fclose(ap25binfile);
                deletestealthfile(ap25binfilename, stealthdir, false);
              return;
            }
            // read bin file into the buffer
            if (trytoreadstealthfile(ap25binfilebuffer, 1, 2048, ap25binfile, ap25binfilename, 0) != 0) {
                fclose(ap25binfile);
                deletestealthfile(ap25binfilename, stealthdir, false);  // should we be trying to delete a file we can't read?
              return;
            }
            fclose(ap25binfile);
            // show some data
            showap25data(ap25binfilebuffer);
            // make sure the file isn't corrupt by checking the sha-1 of the file against ap25_verified_sha1
            sha1_starts(&ctx);
            sha1_update(&ctx, ap25binfilebuffer, 2048);
            sha1_finish(&ctx, ap25_binfile_sha1);
            if (extraverbose) {
                printf("%sAP25 replay sector SHA-1 = ", sp5);
                for (i=0; i<20; i++) printf("%02X", ap25_binfile_sha1[i]);
                printf("%s", newline);
            }
            if (memcmp(ap25_binfile_sha1, ap25_verified_sha1, 20) != 0) {
                color(red);
                printf("ERROR: %s appears to be corrupt! (deleting it)%s", ap25binfilename, newline);
                color(normal);
                deletestealthfile(ap25binfilename, stealthdir, false);
              return;
            }
            // overwrite ap25 buffer with ap25.bin and set fixedap25 = true so it will try to get patched
            memcpy(ap25, ap25binfilebuffer, 2048);
            fixedap25 = true;
          return;
        }
        else {
            color(red);
            printf("%sSet AutoFix Threshold to Level 1 or higher in order to fix the AP25 replay sector%s", newline, newline);
            color(normal);
        }
      return;
    }
    // todo: do some simple validation of ap25 replay table entries (valid psn's, angles 0-359, ...) and do it when checking ap25.bin files for patching too
    
    
    // show some data
    showap25data(ap25);
    // calculate ap25 sha-1
    sha1_starts(&ctx);
    sha1_update(&ctx, ap25, 2048);
    sha1_finish(&ctx, ap25_sha1);
    if (extraverbose) {
        printf("%sAP25 replay sector SHA-1 = ", sp5);
        for (i=0; i<20; i++) printf("%02X", ap25_sha1[i]);
        printf("%s", newline);
    }
    if (verify) {
        printf("Attempting to verify the AP25 replay sector%s", newline);
        if (!xex_foundmediaid) {
            color(yellow);
            printf("Unable to verify AP25 replay sector because the Xex Media ID could not be found%s", newline);
            color(normal);
          return;
        }
        if ((stayoffline || localonly) && !offlinewarningprinted) {
            color(yellow);
            printf("You need to enable online functions (assuming your connection works and the db%s"
                   "is up) if you want to check for the latest files in the online database.%s", newline, newline);
            color(normal);
            offlinewarningprinted = true;
        }
        // get ap25.sha1 file
        memset(ap25hashfilename, 0, 19);
        sprintf(ap25hashfilename, "AP25_%02X%02X%02X%02X.sha1",
                xex_mediaid[12], xex_mediaid[13], xex_mediaid[14], xex_mediaid[15]);
        ap25hashfile = openstealthfile(ap25hashfilename, stealthdir, webstealthdir, AP25_HASH_FILE, "the online verified database");
        if (ap25hashfile == NULL) {
            // failed to find the file online or locally
            color(yellow);
            printf("Unable to verify AP25 replay data for this Media ID%s", newline);
            color(normal);
          return;
        }
        // ap25 hash file should be exactly 24 bytes
        filesize = getfilesize(ap25hashfile);
        if (filesize == -1) {  // seek error
            fclose(ap25hashfile);
          return;
        }
        if (filesize != 24) {
            color(red);
            printf("ERROR: %s is %"LL"d bytes! (should have been exactly 24 bytes -- deleting it)%s",
                   ap25hashfilename, filesize, newline);
            color(normal);
            fclose(ap25hashfile);
            deletestealthfile(ap25hashfilename, stealthdir, false);
          return;
        }
        // read hash file into the buffer
        if (trytoreadstealthfile(ap25hashfilebuffer, 1, 24, ap25hashfile, ap25hashfilename, 0) != 0) {
            fclose(ap25hashfile);
            deletestealthfile(ap25hashfilename, stealthdir, false);  // should we be trying to delete a file we can't read?
          return;
        }
        fclose(ap25hashfile);
        // make sure the file isn't corrupt by checking the crc of the hash against the embedded crc at the end of the file
        if (crc32(0, ap25hashfilebuffer, 20) != getuintmsb(ap25hashfilebuffer+20)) {
            color(red);
            printf("ERROR: %s appears to be corrupt! (deleting it)%s", ap25hashfilename, newline);
            color(normal);
            deletestealthfile(ap25hashfilename, stealthdir, false);
          return;
        }
        memcpy(ap25_verified_sha1, ap25hashfilebuffer, 20);
        // does it match the existing data?
        if (memcmp(ap25_sha1, ap25_verified_sha1, 20) == 0) {
            color(green);
            printf("AP25 replay sector is currently verified%s", newline);
            color(normal);
          return;
        }
        color(yellow);
        printf("AP25 replay sector does not match the one in the database!\n"
               "It could be corrupt or might be obsolete and missing new data%s", newline);
        color(normal);
        if (autofix) {
            printf("Attempting to fix the AP25 replay sector%s", newline);
            if (!writefile) {
                color(yellow);
                printf("Aborting fix because writing is disabled%s", newline);
                color(normal);
              return;
            }
            // get ap25.bin, hash it and compare to ap25_verified_sha1
            memset(ap25binfilename, 0, 18);
            sprintf(ap25binfilename, "AP25_%02X%02X%02X%02X.bin",
                    xex_mediaid[12], xex_mediaid[13], xex_mediaid[14], xex_mediaid[15]);
            ap25binfile = openstealthfile(ap25binfilename, stealthdir, webstealthdir, AP25_BIN_FILE, "the online verified database");
            if (ap25binfile == NULL) {
                // failed to find the file online or locally
                color(red);
                printf("Unable to fix AP25 replay data for this Media ID%s", newline);
                color(normal);
              return;
            }
            // ap25 bin file should be exactly 2048 bytes
            filesize = getfilesize(ap25binfile);
            if (filesize == -1) {  // seek error
                fclose(ap25binfile);
              return;
            }
            if (filesize != 2048) {
                color(red);
                printf("ERROR: %s is %"LL"d bytes! (should have been exactly 2048 bytes -- deleting it)%s",
                       ap25binfilename, filesize, newline);
                color(normal);
                fclose(ap25binfile);
                deletestealthfile(ap25binfilename, stealthdir, false);
              return;
            }
            // read bin file into the buffer
            if (trytoreadstealthfile(ap25binfilebuffer, 1, 2048, ap25binfile, ap25binfilename, 0) != 0) {
                fclose(ap25binfile);
                deletestealthfile(ap25binfilename, stealthdir, false);  // should we be trying to delete a file we can't read?
              return;
            }
            fclose(ap25binfile);
            // show some data
            showap25data(ap25binfilebuffer);
            // make sure the file isn't corrupt by checking the sha-1 of the file against ap25_verified_sha1
            sha1_starts(&ctx);
            sha1_update(&ctx, ap25binfilebuffer, 2048);
            sha1_finish(&ctx, ap25_binfile_sha1);
            if (extraverbose) {
                printf("%sAP25 replay sector SHA-1 = ", sp5);
                for (i=0; i<20; i++) printf("%02X", ap25_binfile_sha1[i]);
                printf("%s", newline);
            }
            if (memcmp(ap25_binfile_sha1, ap25_verified_sha1, 20) != 0) {
                color(red);
                printf("ERROR: %s appears to be corrupt! (deleting it)%s", ap25binfilename, newline);
                color(normal);
                deletestealthfile(ap25binfilename, stealthdir, false);
              return;
            }
            // overwrite ap25 buffer with ap25.bin and set fixedap25 = true so it will try to get patched
            memcpy(ap25, ap25binfilebuffer, 2048);
            fixedap25 = true;
          return;
        }
        else {
            color(red);
            printf("%sSet AutoFix Threshold to Level 1 or higher in order to fix the AP25 replay sector%s", newline, newline);
            color(normal);
          return;
        }
    }
    else {
        // verify is disabled
        color(yellow);
        printf("AP25 replay data will not be verified because you have disabled the option to verify%s", newline);
        color(normal);
      return;
    }
  return;
}

bool isacontrolchar(unsigned char character) {
    if (character < 0x20 || (character > 0x7E && character < 0xA0)) return true;
  return false;
}

void checkdmi(unsigned char *dmi) {
    dmi_stealthfailed = false;
    dmi_stealthuncertain = false;
    dmi_crc32 = 0;
    int i;
    if (verbose && !minimal) printf("%sChecking DMI%s", newline, newline);
    if (getzeros(dmi, 0, 2047) == 2048) {
        dmi_stealthfailed = true;
        color(red);
        printf("DMI is blank!%s", newline);
        color(normal);
      return;
    }
    // get the DMI timestamp
    dmi_authored = getuint64(dmi+0x10);
    if (verbose) {
        printf("%sTimestamp of Authoring: ", sp5);
        printwin32filetime(dmi_authored);
    }
    if (ss_foundtimestamp) {
        if (ss_authored != dmi_authored) {
            dmi_stealthfailed = true;
            if (verbose) printf("%s", newline);
            color(red);
            printf("DMI timestamp of authoring does not match SS!%s", newline);
            color(normal);
        }
        else if (verbose) printf(" (matches SS)%s", newline);
    }
    else {
        if (verbose) printf("%s", newline);
        if (!justastealthfile) {
            stealthuncertain = true;
            dmi_stealthuncertain = true;
            color(yellow);
            printf("DMI timestamp of authoring could not be compared to the SS%s", newline);
            color(normal);
        }
    }
    if (extraverbose) {
        // show the catalog number, if that's what this is? (as long as it doesn't contain control characters)
        bool validdmicatalognumber = true;
        if (lookslikexbox1dmi(dmi)) {
            for (i=8;i<=15;i++) {
                if (isacontrolchar(dmi[i])) {
                    validdmicatalognumber = false;
                    break;
                }
            }
            if (validdmicatalognumber) printf("%sCatalog Number: %c%c-%c%c%c%c%c%c%s",
                                               sp5, dmi[8], dmi[9], dmi[10], dmi[11], dmi[12], dmi[13], dmi[14], dmi[15], newline);
        }
        else {
            for (i=64;i<=76;i++) {
                if (isacontrolchar(dmi[i])) {
                    validdmicatalognumber = false;
                    break;
                }
            }
            if (validdmicatalognumber) printf("%sCatalog Number: %c%c-%c%c%c%c-%c%c-%c%c-%c%c%c%s",
                                               sp5, dmi[64], dmi[65], dmi[66], dmi[67], dmi[68], dmi[69], dmi[70], dmi[71], dmi[72], dmi[73], dmi[74], dmi[75], dmi[76], newline);
        }
    }
    
    // calculate DMI crc32
    dmi_crc32 = crc32(0, dmi, 2048);
    if (verbose) printf("%sDMI CRC = %08lX%s", sp5, dmi_crc32, newline);
    
    if (lookslikexbox1dmi(dmi)) {
        color(yellow);
        printf("Looks like Xbox 1 DMI%s", newline);
        color(normal);
        stealthuncertain = true;
        dmi_stealthuncertain = true;
      return;
    }
    
    // get the DMI media id
    memcpy(dmi_mediaid, dmi+0x20, 16);
    dmi_foundmediaid = true;
    if (verbose) {
        printf("%sDMI Media ID: ", sp5);
        printmediaid(dmi_mediaid);
    }
    if (xex_foundmediaid) {
        if (memcmp(xex_mediaid, dmi_mediaid, 16) != 0) {
            dmi_stealthfailed = true;
            if (verbose) printf("%s", newline);
            color(red);
            printf("DMI media id does not match this game!%s", newline);
            color(normal);
        }
        else if (verbose) printf(" (matches game)%s", newline);
    }
    else {
        if (verbose) printf("%s", newline);
        if (justastealthfile) checkcsv(dmi_mediaid);
        else {
            stealthuncertain = true;
            dmi_stealthuncertain = true;
            color(yellow);
            printf("DMI media id could not be compared to the Xex%s", newline);
            color(normal);
        }
    }
    // check a few things that seem to be true for all xbox 360 dmi sectors
    if (!lookslike360dmi(dmi)) {
        stealthuncertain = true;
        dmi_stealthuncertain = true;
        color(yellow);
        printf("DMI does not look valid%s", newline);
        color(normal);
        if (verbose) {
            printf("Displaying suspicious DMI in hex and ascii:%s", newline);
            hexdump(dmi, DMI_HEX, 2048);
        }
    }
    else if (memcmp(dmi+0x7FC, "\xFF\xFF\xFF\xFF", 4) == 0) {  // check for corruption that seems to be related to a certain sata->usb adapter
        dmi_stealthuncertain = true;
        color(yellow);
        printf("DMI appears to be corrupt!%s", newline);
        color(normal);
        if (verbose) {
            printf("Displaying suspicious DMI in hex and ascii:%s", newline);
            hexdump(dmi, DMI_HEX, 2048);
        }
    }
    else if (!dmi_stealthfailed && !dmi_stealthuncertain) {
        color(green);
        printf("DMI looks valid%s", newline);
        color(normal);
    }
  return;
}

bool lookslike360dmi(unsigned char* dmi) {
    if ((dmi[0] != 0x02) ||
        (memcmp(dmi+0x7E8, "XBOX", 4) != 0) ||
        (getzeros(dmi, 0x50, 0x633) != 1508)) return false;
  return true;
}

bool lookslikexbox1dmi(unsigned char* dmi) {
    if ((dmi[0] != 0x01) || (getzeros(dmi, 0x8, 0xF) != 0) || (getzeros(dmi, 0x10, 0x17) == 8)) return false;
  return true;
} 

void checkpfi(unsigned char *pfi) {
    int i;
    unsigned long m;
    bool pfirecognized = false;
    pfi_crc32 = 0;
    pfi_stealthfailed = false; pfi_stealthuncertain = false; pfi_foundsectorstotal = false; pfi_alreadydumped = false;
    if (verbose && !minimal) printf("%sChecking PFI%s", newline, newline);
    if (getzeros(pfi, 0, 2047) == 2048) {
        pfi_stealthfailed = true;
        color(red);
        printf("PFI is blank!%s", newline);
        color(normal);
      return;
    }
    // get Starting PSN of Data Area
    unsigned char pfi_startpsnL0hex[4] = {pfi[7], pfi[6], pfi[5], 0x00};
    unsigned long pfi_startpsnL0 = getuint(pfi_startpsnL0hex);
    
    // get End PSN of L0 Data Area
    unsigned char pfi_endpsnL0hex[4] = {pfi[15], pfi[14], pfi[13], 0x00};
    unsigned long pfi_endpsnL0 = getuint(pfi_endpsnL0hex);
    
    // invert bits in End PSN of L0 Data Area to find Starting PSN of L1 Data Area
    unsigned char pfi_startpsnL1hex[4] = {~pfi[15], ~pfi[14], ~pfi[13], 0x00};
    unsigned long pfi_startpsnL1 = getuint(pfi_startpsnL1hex);
    
    // get End PSN of L1 Data Area
    unsigned char pfi_endpsnL1hex[4] = {pfi[11], pfi[10], pfi[9], 0x00};
    unsigned long pfi_endpsnL1 = getuint(pfi_endpsnL1hex);
    
    // here be dragons
    int32_t layerbreakpsn = ~((layerbreak - 1 + 0x030000) ^ 0xFF000000);
    pfi_sectorsL0 = pfi_endpsnL0 - pfi_startpsnL0 + 1;
    pfi_sectorsL1 = pfi_endpsnL1 - pfi_startpsnL1 + 1;
    unsigned long long pfi_offsetL0 = ((unsigned long long) pfi_startpsnL0 - 0x030000) * 2048;
    pfi_offsetL1 = ((unsigned long long) pfi_startpsnL1 - (unsigned long long) layerbreakpsn) * 2048 + ((unsigned long long) layerbreak * 2048);
    pfi_offsetL0end = (unsigned long long) (pfi_endpsnL0 - pfi_startpsnL0 + 1) * 2048 + pfi_offsetL0 - 1;
    unsigned long long pfi_offsetend = (pfi_endpsnL1 - pfi_startpsnL1 + 1) * 2048 + pfi_offsetL1 - 1;
    pfi_sectorstotal = pfi_sectorsL0 + pfi_sectorsL1;
    pfi_foundsectorstotal = true;

    // print that shit
    if (extraverbose) printpfitable(pfi_startpsnL0, pfi_endpsnL0, pfi_startpsnL1, pfi_endpsnL1, pfi_sectorsL0, pfi_sectorsL1,
                                           pfi_offsetL0, pfi_offsetL0end, pfi_offsetL1, pfi_offsetend, pfi_sectorstotal);
    
    if (pfi_sectorstotal > total_sectors_available_for_video_data) {
        // may or may not matter in LT (would depend on L0 data size) but causes a problem for the way abgx360 handles video crc and the splitvid check
        pfi_stealthuncertain = true;
        stealthuncertain = true;
        color(yellow);
        printf("PFI Data Area is more than %lu sectors! This overlaps stealth files and/or game data in backups!%s", total_sectors_available_for_video_data, newline);
        color(normal);
    }
    
    // check for BCA (Burst Cutting Area) flag which indicates a unique barcode is burned onto the original disc
    // (but would any xbox 360 drives be able to read it?)
    if ((pfi[16] & 0x80) == 0x80) {
        stealthuncertain = true;
        color(yellow);
        printf("PFI BCA flag is on!%s", newline);
        color(normal);
    }
    
    // calculate PFI crc32
    pfi_crc32 = crc32(0, pfi, 2048);
    if (verbose) printf("%sPFI CRC = %08lX%s", sp5, pfi_crc32, newline);
    if (debug) {
        // calculate PFI sha-1
        sha1_starts(&ctx);
        sha1_update(&ctx, pfi, 2048);
        sha1_finish(&ctx, pfi_sha1);
        printf("%sPFI SHA-1 = ", sp5);
        for (i=0; i<20; i++) printf("%02X", pfi_sha1[i]);
        printf("%s", newline);
    }
    
    for (m=0;m<num_pfientries;m++) {
        if (pfi_crc32 == mostrecentpfientries[m].crc) {
            printf("PFI matches known data (%s)%s", mostrecentpfientries[m].description, newline);
            pfirecognized = true;
            break;
        }
    }
    if (!pfirecognized) {
        pfi_stealthuncertain = true;
        color(yellow);
        if (pfi_crc32 == xbox1pficrc) printf("PFI matches known data (Xbox 1)%s", newline);
        else if (ss_foundtimestamp && ss_authored <= lastknownwave) printf("PFI does not match known data (almost certainly corrupt)%s", newline);
        else printf("PFI does not match known data (probably corrupt but might be a\nbrand new wave if this is a new game and abgx360.dat hasn't been updated yet)%s", newline);
        color(normal);
        if (!lookslikepfi(pfi)) {
            color(yellow);
            printf("PFI doesn't even look like valid data%s", newline);
            color(normal);
            if (verbose) {
                printf("Displaying suspicious PFI in hex and ascii:%s", newline);
                hexdump(pfi, PFI_HEX, 2048);
            }
        }
    }
  return;
}

bool lookslikepfi(unsigned char* pfi) {
    // startpsnL0 should be 0x030000
    if ((pfi[0x5] != 0x03) || (pfi[0x6] != 0) || (pfi[0x7] != 0)) return false;
    // these 3 bytes should be zero
    if ((pfi[0x4] != 0) || (pfi[0x8] != 0) || (pfi[0xC] != 0)) return false;
    // as well as all of these
    if (getzeros(pfi, 0x11, 0x7FF) != 2031) return false;
  return true;
}

void checkvideo(char *isofilename, FILE *stream, bool justavideoiso, bool checkvideopadding) {
    int i, b;
    unsigned long m;
    bool videorecognized = false;
    unsigned char spaces[128];
    memset(spaces, 0x20, 128);
    video_crc32 = 0; videoL0_crc32 = 0; videoL1_crc32 = 0;
    video_stealthfailed = false; video_stealthuncertain = false;
    // seek to sector 16 and look for a video partition
    if (fseeko(stream, 32768, SEEK_SET) != 0) {
        printseekerror(isofilename, "Checking Video");
        video_stealthuncertain = true;
      return;
    }
    memset(buffer, 0, 2048);
    initcheckread();
    if (checkreadandprinterrors(buffer, 1, 2048, stream, 0, 32768, isofilename, "Video check") != 0) {
        video_stealthuncertain = true;
      return;
    }
    donecheckread(isofilename);
    if (getzeros((unsigned char*) buffer, 0, 2047) == 2048) {
        video_stealthfailed = true;
        color(red);
        printf("Video partition sector 16 is blank! (most likely the rest is too)%s", newline);
        color(normal);
      return;
    }
    if (memcmp(buffer+1, "CD001", 5) != 0) {
        // "CD001" was not found at sector 16
        video_stealthfailed = true;
        color(red);
        printf("Video partition Standard Identifier not found!%s", newline);
        color(normal);
      return;
    }
    unsigned long videosize = getint(buffer+80);
    if (verbose) printf("Video partition found%s", newline);
    if (extraverbose) {
        // print the volume identifier (omitting control codes and stopping at trailing space(s) or a null byte)
        printf("%sVolume ID: %s", sp5, quotation);
        //memset(spaces, 0x20, 32);
        for (b=40; b<72; b++) {
            if (memcmp(buffer+b, spaces, 72-b) == 0) break;
            if (buffer[b] == 0) break;
            if (buffer[b] < 0x20) continue;
            printf("%c", buffer[b]);
        }
        printf("%s%s", quotation, newline);
        // print the volume space size
        printf("%sVolume space size: %lu sectors (%lu bytes)%s", sp5, videosize, videosize*2048, newline);
        // print the volume creation date and time
        printf("%sVolume creation date %s time: ", sp5, ampersand);
        int timezonehrs = buffer[829]*15/60;
        int timezonemins = buffer[829]*15%60;
        printf("%c%c%c%c/%c%c/%c%c %c%c:%c%c:%c%c (GMT",
                buffer[813], buffer[814], buffer[815], buffer[816], buffer[817],
                buffer[818], buffer[819], buffer[820], buffer[821], buffer[822],
                buffer[823], buffer[824], buffer[825], buffer[826]);
        if ((timezonehrs < 0) || (timezonemins < 0)) printf("-");
        else printf("+");
        if (timezonemins < 0) timezonemins = timezonemins * -1;
        if (timezonehrs < 0) timezonehrs = timezonehrs * -1;
        printf("%02d:%02d)%s", timezonehrs, timezonemins, newline);
        /*
        // publisher identifier
        printf("%sPublisher ID: %s", sp5, quotation);
        for (b=318; b<446; b++) {
            if (memcmp(buffer+b, spaces, 446-b) == 0) break;
            if (buffer[b] == 0) break;
            if (buffer[b] < 0x20) continue;
            printf("%c", buffer[b]);
        }
        printf("%s%s", quotation, newline);
        */
    }
    if (checkvideopadding && !justavideoiso) {
        if (pfi_foundsectorstotal) {
            if (pfi_sectorstotal == total_sectors_available_for_video_data) {
                if (verbose) printf("%s", sp5);
                printf("Cannot check padding because there is none%s", newline);
                goto skipvideopadding;
            }
            else if (pfi_sectorstotal > total_sectors_available_for_video_data) {
                color(yellow);
                printf("Cannot check padding because the PFI Data Area is too large%s", newline);
                color(normal);
                goto skipvideopadding;
            }
            // total_sectors_available_for_video_data - pfi_sectorstotal sectors should be blank starting at offset pfi_sectorstotal*2048
            if (fseeko(stream, (unsigned long long) pfi_sectorstotal*2048, SEEK_SET) != 0) {
                printseekerror(isofilename, "Checking Video padding");
                goto skipvideopadding;
            }
            memset(bigbuffer, 0, BIGBUF_SIZE);
            sizeoverbuffer = ((unsigned long) total_sectors_available_for_video_data*2048 - pfi_sectorstotal*2048) / BIGBUF_SIZE;
            bufferremainder = ((unsigned long) total_sectors_available_for_video_data*2048 - pfi_sectorstotal*2048) % BIGBUF_SIZE;
            initcheckread();
            bool videoL0zeropadding = true;
            long dataloop = 0;
            if (verbose) printf("%s", sp5);
            fprintf(stderr, "Checking Video padding... ");
            charsprinted = 0;
            for (m=0;m<sizeoverbuffer;m++) {
                if (m && m % (sizeoverbuffer / 100) == 0 && m / (sizeoverbuffer / 100) <= 100) {
                    for (i=0;i<charsprinted;i++) fprintf(stderr, "\b");
                    charsprinted = fprintf(stderr, "%2lu%% ", m / (sizeoverbuffer / 100));
                }
                if (checkreadandprinterrors(bigbuffer, 1, BIGBUF_SIZE, stream, m, pfi_sectorstotal*2048, "Video", "checking zero padding") != 0) goto skipvideopadding;
                if (getzeros(bigbuffer, 0, BIGBUF_SIZE - 1) != BIGBUF_SIZE) {
                    videoL0zeropadding = false;
                    dataloop = m;
                    if (debug) printf("data found at 0x%lX, current offset: 0x%"LL"X, dataloop = %ld%s",
                                       m*BIGBUF_SIZE + pfi_sectorstotal*2048, (unsigned long long) ftello(stream), dataloop, newline);
                    goto skipL0remainder;
                }
            }
            if (bufferremainder) {
                if (checkreadandprinterrors(bigbuffer, 1, bufferremainder, stream, 0, (unsigned long) total_sectors_available_for_video_data*2048 - bufferremainder,
                    "L0 Video padding", "checking zero padding") != 0) goto skipvideopadding;
                if (getzeros(bigbuffer, 0, bufferremainder - 1) != bufferremainder) {
                    videoL0zeropadding = false;
                    dataloop = -1;  // identify that data was found in the bufferremainder
                    if (debug) printf("data found at 0x%lX, current offset: 0x%"LL"X, dataloop = %ld%s",
                                       (unsigned long) total_sectors_available_for_video_data*2048 - bufferremainder, (unsigned long long) ftello(stream), dataloop, newline);
                    goto skipL0remainder;
                }
            }
            for (i=0;i<charsprinted;i++) fprintf(stderr, "\b");
            for (i=0;i<charsprinted;i++) fprintf(stderr, " ");
            for (i=0;i<charsprinted;i++) fprintf(stderr, "\b");
            skipL0remainder:
            donecheckread("L0 Video padding");
            
            if (videoL0zeropadding) {
                color(green);
                printf("Video is zero padded%s", newline);
                color(normal);
            }
            else {
                if (!writefile) {
                    color(yellow);
                    printf("Video padding contains data but it won't be zeroed because writing is disabled%s", newline);
                    color(normal);
                    goto skipvideopadding;
                }
                printf("Video padding contains data, overwriting it with zeroes...%s", newline);
                if (extraverbose) {
                    if (dataloop == -1) memset(bigbuffer+bufferremainder, 0, BIGBUF_SIZE - bufferremainder);
                    unsigned long sectoroffset = 0;
                    for (i=0;i<(BIGBUF_SIZE/2048);i++) {
                        if (debug) printf("%d: %lu zeros%s", i,
                                           getzeros(bigbuffer, (unsigned long) i*2048, (unsigned long) i*2048+2047), newline);
                        if (getzeros(bigbuffer, (unsigned long) i*2048, (unsigned long) i*2048+2047) != 2048) {
                            sectoroffset = (unsigned long) i*2048;
                          break;
                        }
                    }
                    printf("Showing first sector of padding data (0x%lX) in hex and ascii:%s",
                            dataloop == -1 ? ((unsigned long) total_sectors_available_for_video_data*2048 - bufferremainder + sectoroffset) :
                                             (dataloop*BIGBUF_SIZE + pfi_sectorstotal*2048 + sectoroffset), newline);
                    hexdump(bigbuffer+sectoroffset, 0, 2048);
                    printf("%s", newline);
                }
                stream = freopen(isofilename, "rb+", stream);
                if (stream == NULL) {
                    color(red);
                    printf("ERROR: Failed to reopen %s for writing! (%s) Zero padding failed!%s",
                            isofilename, strerror(errno), newline);
                    color(normal);
                    stream = fopen(isofilename, "rb");
                    if (stream == NULL) {
                        color(red);
                        printf("ERROR: Failed to reopen %s for reading! (%s) Game over man... Game over!%s",
                                isofilename, strerror(errno), newline);
                        color(normal);
                      exit(1);
                    }
                  goto skipvideopadding;
                }
                if (dataloop == -1) {
                    if (fseeko(stream, (unsigned long long) total_sectors_available_for_video_data*2048 - (unsigned long long) bufferremainder, SEEK_SET) != 0) {
                        printseekerror(isofilename, "Zero padding");
                      goto skipvideopadding;
                    }
                }
                else if ( fseeko(stream, (unsigned long long) pfi_sectorstotal*2048 +
                                         (unsigned long long) dataloop*BIGBUF_SIZE, SEEK_SET) != 0 ) {
                    printseekerror(isofilename, "Zero padding");
                  goto skipvideopadding;
                }
                if (debug) printf("Current offset: 0x%"LL"X%s", (unsigned long long) ftello(stream), newline);
                initcheckwrite();
                memset(bigbuffer, 0, BIGBUF_SIZE);
                if (dataloop == -1) {
                    if (checkwriteandprinterrors(bigbuffer, 1, bufferremainder, stream, 0, (unsigned long) total_sectors_available_for_video_data*2048 - bufferremainder,
                        "L0 Video padding", "zero padding") != 0) goto skipvideopadding;
                }
                else {
                    for (m=0;m<sizeoverbuffer - dataloop;m++) {
                        if (checkwriteandprinterrors(bigbuffer, 1, BIGBUF_SIZE, stream, m, (unsigned long long) pfi_sectorstotal*2048 + (unsigned long long) dataloop*BIGBUF_SIZE,
                            "L0 Video padding", "zero padding") != 0) goto skipvideopadding;
                    }
                    if (bufferremainder) {
                        if (checkwriteandprinterrors(bigbuffer, 1, bufferremainder, stream, 0, (unsigned long) total_sectors_available_for_video_data*2048 - bufferremainder,
                            "L0 Video padding", "zero padding") != 0) goto skipvideopadding;
                    }
                }
                donecheckwrite("L0 Video Padding");
                color(green);
                if (verbose) printf("%s", sp5);
                printf("Video padding zeroed successfully%s", newline);
                color(normal);
            }
        }
        else {
            color(yellow);
            printf("Cannot check padding because PFI is missing%s", newline);
            color(normal);
        }
    }
    skipvideopadding:
    
    // check video crc32
    if (fseeko(stream, 0, SEEK_SET) != 0) {
        printseekerror(isofilename, "Checking Video CRC");
        video_stealthuncertain = true;
      return;
    }
    initcheckread();
    if (pfi_foundsectorstotal) {
        if (pfi_sectorstotal > total_sectors_available_for_video_data) {
            video_crc32 = 0;
            stealthuncertain = true;
            video_stealthuncertain = true;
            color(yellow);
            printf("PFI Data Area is too large! (%lu sectors) Video CRC check was aborted%s", pfi_sectorstotal, newline);
            color(normal);
          goto endofvideocrc;
        }
        unsigned long totalsizeoverbuffer = pfi_sectorstotal * 2048 / BIGBUF_SIZE;
        sizeoverbuffer = pfi_sectorsL0 * 2048 / BIGBUF_SIZE;
        unsigned long firstsizeoverbuffer = sizeoverbuffer;
        bufferremainder = pfi_sectorsL0 * 2048 % BIGBUF_SIZE;
        for (m=0; m<sizeoverbuffer; m++) {
            if (totalsizeoverbuffer >= 100 && m && (m % (totalsizeoverbuffer / 100) == 0) && (m / (totalsizeoverbuffer / 100) <= 100)) {
                for (i=0;i<charsprinted;i++) fprintf(stderr, "\b");
                charsprinted = fprintf(stderr, "Checking Video CRC... %2lu%% ", m / (totalsizeoverbuffer / 100));
            }
            if (checkreadandprinterrors(bigbuffer, 1, BIGBUF_SIZE, stream, m, 0, "Video", "CRC check") != 0) {
                video_crc32 = 0;  // reset to 0 so we don't try to autofix or verify a bad crc
                goto endofvideocrc;
            }
            video_crc32 = crc32(video_crc32, bigbuffer, BIGBUF_SIZE);
        }
        if (bufferremainder) {
            if (checkreadandprinterrors(bigbuffer, 1, bufferremainder, stream, 0, pfi_sectorsL0 * 2048 - bufferremainder, "Video", "CRC check") != 0) {
                video_crc32 = 0;
                goto endofvideocrc;
            }
            video_crc32 = crc32(video_crc32, bigbuffer, bufferremainder);
        }
        videoL0_crc32 = video_crc32;
        sizeoverbuffer = pfi_sectorsL1 * 2048 / BIGBUF_SIZE;
        bufferremainder = pfi_sectorsL1 * 2048 % BIGBUF_SIZE;
        for (m=0; m<sizeoverbuffer; m++) {
            if (totalsizeoverbuffer >= 100 && m && ((m + firstsizeoverbuffer) % (totalsizeoverbuffer / 100) == 0) && ((m + firstsizeoverbuffer) / (totalsizeoverbuffer / 100) <= 100)) {
                for (i=0;i<charsprinted;i++) fprintf(stderr, "\b");
                charsprinted = fprintf(stderr, "Checking Video CRC... %2lu%% ", (m + firstsizeoverbuffer) / (totalsizeoverbuffer / 100));
            }
            if (checkreadandprinterrors(bigbuffer, 1, BIGBUF_SIZE, stream, m, pfi_sectorsL0 * 2048, "Video", "CRC check") != 0) {
                video_crc32 = 0;
                goto endofvideocrc;
            }
            video_crc32 = crc32(video_crc32, bigbuffer, BIGBUF_SIZE);
            videoL1_crc32 = crc32(videoL1_crc32, bigbuffer, BIGBUF_SIZE);
        }
        if (bufferremainder) {
            if (checkreadandprinterrors(bigbuffer, 1, bufferremainder, stream, 0, pfi_sectorstotal * 2048 - bufferremainder, "Video", "CRC check") != 0) {
                video_crc32 = 0;
                goto endofvideocrc;
            }
            video_crc32 = crc32(video_crc32, bigbuffer, bufferremainder);
            videoL1_crc32 = crc32(videoL1_crc32, bigbuffer, bufferremainder);
        }
    }
    else {
        if (videosize > total_sectors_available_for_video_data) {
            video_crc32 = 0;
            stealthuncertain = true;
            video_stealthuncertain = true;
            color(yellow);
            printf("Video volume space size is too large! (%lu sectors) CRC check was aborted%s", videosize, newline);
            color(normal);
          goto endofvideocrc;
        }
        sizeoverbuffer = videosize * 2048 / BIGBUF_SIZE;
        bufferremainder = videosize * 2048 % BIGBUF_SIZE;
        for (m=0; m<sizeoverbuffer; m++) {
            if (sizeoverbuffer >= 100 && m && (m % (sizeoverbuffer / 100) == 0) && (m / (sizeoverbuffer / 100) <= 100)) {
                for (i=0;i<charsprinted;i++) fprintf(stderr, "\b");
                charsprinted = fprintf(stderr, "Checking Video CRC... %2lu%% ", m / (sizeoverbuffer / 100));
            }
            if (checkreadandprinterrors(bigbuffer, 1, BIGBUF_SIZE, stream, m, 0, "Video", "CRC check") != 0) {
                video_crc32 = 0;
                goto endofvideocrc;
            }
            video_crc32 = crc32(video_crc32, bigbuffer, BIGBUF_SIZE);
        }
        if (bufferremainder) {
            if (checkreadandprinterrors(bigbuffer, 1, bufferremainder, stream, 0, videosize * 2048 - bufferremainder, "Video", "CRC check") != 0) {
                video_crc32 = 0;
                goto endofvideocrc;
            }
            video_crc32 = crc32(video_crc32, bigbuffer, bufferremainder);
        }
    }
    for (i=0;i<charsprinted;i++) fprintf(stderr, "\b");
    for (i=0;i<charsprinted;i++) fprintf(stderr, " ");
    for (i=0;i<charsprinted;i++) fprintf(stderr, "\b");
    donecheckread("Video");

    if (verbose) {
        printf("%sVideo CRC = %08lX", sp5, video_crc32);
        if (pfi_foundsectorstotal) {
            printf(" (V0 = %08lX, V1 = %08lX)%s", videoL0_crc32, videoL1_crc32, newline);
        }
        else if (justavideoiso) {
            printf("%s%sCRC might not be correct - PFI is needed to know the full Data Area size%s", newline, sp5, newline);
        }
        else {
            color(yellow);
            printf(" (V0 %s V1 skipped due to missing PFI)%s", ampersand, newline);
            color(normal);
        }
    }
    
    endofvideocrc:
    
    for (m=0;m<num_videoentries;m++) {
        if (video_crc32 == mostrecentvideoentries[m].crc) {
            printf("Video partition matches known data (%s)%s", mostrecentvideoentries[m].description, newline);
            videorecognized = true;
            break;
        }
    }
    if (!videorecognized) {
        video_stealthuncertain = true;
        if (video_crc32 == 0) return;
        color(yellow);
        if (video_crc32 == xbox1videocrc) printf("Video partition matches known data (Xbox 1)%s", newline);
        else if (pfi_foundsectorstotal) {
            if (ss_foundtimestamp && ss_authored <= lastknownwave) printf("Video partition does not match known data (almost certainly corrupt)%s", newline);
            else printf("Video partition does not match known data (probably corrupt but might be a\nbrand new wave if this is a new game and abgx360.dat hasn't been updated yet)%s", newline);
        }
        else printf("Video partition does not match known data but that's probably because there is\nno PFI to tell us what its real size is%s", newline);
        color(normal);
    }
    
  return;
}
