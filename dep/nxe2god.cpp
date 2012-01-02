#include <xtl.h>
#include "AtgConsole.h"
#include "AtgUtil.h"
#include "AtgInput.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <fstream>
#include <sys/stat.h> 
#include "xfilecache.h"
#include <string>
#include "xbox.h"
#include <vector>

using std::vector;
using std::string;
using std::ifstream;
using std::ofstream;
using std::fstream;

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
    PCHAR Buffer;
} STRING, *PSTRING;

extern "C" int __stdcall ObCreateSymbolicLink( STRING*, STRING*);
extern "C" int __stdcall ObDeleteSymbolicLink( STRING* );
extern "C" VOID XeCryptSha(LPVOID DataBuffer1, UINT DataSize1, LPVOID DataBuffer2, UINT DataSize2, LPVOID DataBuffer3, UINT DataSize3, LPVOID DigestBuffer, UINT DigestSize);

class NXE {

	void readTitle() 
	{
		string fullPath;
		fullPath += path;
		fullPath += fileName;

		ifstream file;
		file.open ((char*)fullPath.c_str(), ifstream::in);
		if (file.is_open())
		{
			char buffer[_MAX_PATH];
			file.seekg(0x00000411);
			file.read(buffer,_MAX_PATH);
			swprintf_s(title, _MAX_PATH,L"%s", buffer);
			file.close();
		}
	}

	public:
		string fileName;
		string path;
		wchar_t title[_MAX_PATH];
		NXE (string, string);
		bool status;	
};

NXE::NXE (string strFileName, string strPath) {
	fileName = strFileName;
	path = strPath;
	readTitle();
	status = true;
}


vector<NXE> allNXE;

string sDirAct;
string filePathzzz = "hdd:\\Content\\0000000000000000";
string fileList[5000];
NXE* nxeGames;
int fileCount = 0;
bool debugEnabled = false;

ATG::Console console;

HRESULT Map( CHAR* szDrive, CHAR* szDevice )
{
    CHAR * szSourceDevice;
    CHAR szDestinationDrive[16];

    szSourceDevice = szDevice;

    sprintf_s( szDestinationDrive,"\\??\\%s", szDrive );

    STRING DeviceName =
    {
        strlen(szSourceDevice),
        strlen(szSourceDevice) + 1,
        szSourceDevice
    };

    STRING LinkName =
    {
        strlen(szDestinationDrive),
        strlen(szDestinationDrive) + 1,
        szDestinationDrive
    };

    return ( HRESULT )ObCreateSymbolicLink( &LinkName, &DeviceName );
}

HRESULT unMap( CHAR* szDrive )
{
    CHAR szDestinationDrive[16];
    sprintf_s( szDestinationDrive,"\\??\\%s", szDrive );

    STRING LinkName =
    {
        strlen(szDestinationDrive),
        strlen(szDestinationDrive) + 1,
        szDestinationDrive
    };

    return ( HRESULT )ObDeleteSymbolicLink( &LinkName );
}

int DeleteDirectory(const std::string &refcstrRootDirectory,
                    bool              bDeleteSubdirectories = true)
{
  bool            bSubdirectory = false;       // Flag, indicating whether
                                               // subdirectories have been found
  HANDLE          hFile;                       // Handle to directory
  std::string     strFilePath;                 // Filepath
  std::string     strPattern;                  // Pattern
  WIN32_FIND_DATA FileInformation;             // File information


  strPattern = refcstrRootDirectory + "\\*.*";
  hFile = ::FindFirstFile(strPattern.c_str(), &FileInformation);
  if(hFile != INVALID_HANDLE_VALUE)
  {
    do
    {
      if(FileInformation.cFileName[0] != '.')
      {
        strFilePath.erase();
        strFilePath = refcstrRootDirectory + "\\" + FileInformation.cFileName;

        if(FileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
          if(bDeleteSubdirectories)
          {
            // Delete subdirectory
            int iRC = DeleteDirectory(strFilePath, bDeleteSubdirectories);
            if(iRC)
              return iRC;
          }
          else
            bSubdirectory = true;
        }
        else
        {
          // Set file attributes
          if(::SetFileAttributes(strFilePath.c_str(),
                                 FILE_ATTRIBUTE_NORMAL) == FALSE)
            return ::GetLastError();

          // Delete file
          if(::DeleteFile(strFilePath.c_str()) == FALSE)
            return ::GetLastError();
        }
      }
    } while(::FindNextFile(hFile, &FileInformation) == TRUE);

    // Close handle
    ::FindClose(hFile);

    DWORD dwError = ::GetLastError();
    if(dwError != ERROR_NO_MORE_FILES)
      return dwError;
    else
    {
      if(!bSubdirectory)
      {
        // Set directory attributes
        if(::SetFileAttributes(refcstrRootDirectory.c_str(),
                               FILE_ATTRIBUTE_NORMAL) == FALSE)
          return ::GetLastError();

        // Delete directory
        if(::RemoveDirectory(refcstrRootDirectory.c_str()) == FALSE)
          return ::GetLastError();
      }
    }
  }

  return 0;
}

void debugLog(char* output)
{
	if (debugEnabled)
	{
		ofstream writeLog;

		writeLog.open("game:\\debug.log",ofstream::app);
		if (writeLog.is_open())
		{
		  writeLog.write(output,strlen(output));
		  writeLog.write("\n",1);
		}
		writeLog.close();
	}
}

bool FileExists(char* strFilename) {
  struct stat stFileInfo;
  bool returnValue;
  int intStat;

  intStat = stat(strFilename,&stFileInfo);
  if(intStat == 0) {
    returnValue = true;
  } else {

    returnValue = false;
  }
  
  return(returnValue);
}

void nxetogod(NXE nxeGame)
{
	console.Format("Converting ");
	console.Format(nxeGame.title);
	console.Format(" to GOD...\n");

	string goddirectory;
	string backupdirectory;
	string confile;
	string condirectory;
	string datadirectory;
	string olddatadirectory;

	goddirectory += nxeGame.path;
	goddirectory.erase(goddirectory.size() - 9, 9);
	goddirectory += "00007000";

	olddatadirectory += nxeGame.path;
	olddatadirectory += nxeGame.fileName;
	olddatadirectory += ".data";

	datadirectory += goddirectory;
	datadirectory += "\\";
	datadirectory += nxeGame.fileName;
	datadirectory += ".data";

	backupdirectory += goddirectory;
	backupdirectory += "\\backup";

	confile += nxeGame.path;
	confile += nxeGame.fileName;

	condirectory += nxeGame.path;
	condirectory.erase(condirectory.size() - 1, 1);

	//Create 00007000 directory if it doens't already exist
	::CreateDirectory(goddirectory.c_str(),0);
	//Create backup directory to store original CON file if it doesn't already exist
	::CreateDirectory(backupdirectory.c_str(),0);

    //Append CON file name to existing path strings to give the full path
	goddirectory += "\\";
	backupdirectory += "\\";
	goddirectory += nxeGame.fileName;
	backupdirectory += nxeGame.fileName;

	//Move CON file from 00004000 to 00007000
	if(FileExists((char*)goddirectory.c_str()))
	{
		//Delete target file if it already exists
		::DeleteFile(goddirectory.c_str());
	}
	//Then move
	::MoveFile(confile.c_str(),goddirectory.c_str());

	//Move .data directory from 00004000 to 00007000
	if(::CreateDirectory(datadirectory.c_str(),0) == 0)
	{
		//Target directory exists in 00007000 - Delete it to empty it
		DeleteDirectory(datadirectory.c_str(),true);
		//Recreate it
		::CreateDirectory(datadirectory.c_str(),0);
	}
	//Now move data files there
	HANDLE hFind;
	WIN32_FIND_DATA wfd;

	string searchdir;
	searchdir += olddatadirectory;
	searchdir += "\\*";

	hFind = FindFirstFile( searchdir.c_str(), &wfd );
	
	if( INVALID_HANDLE_VALUE == hFind )
	{

	}
	else
	{
		do
		{		
			string oldFile;
			string newFile;

			oldFile += olddatadirectory;
			oldFile += "\\";
			oldFile += wfd.cFileName;

			newFile += datadirectory;
			newFile += "\\";
			newFile += wfd.cFileName;

			::MoveFile(oldFile.c_str(), newFile.c_str());

		} while( FindNextFile( hFind, &wfd ));
		FindClose( hFind );
	}
	
	//Delete original file from backup directory if it exists
	if(FileExists((char*)backupdirectory.c_str()))
	{
		//Delete target file if it already exists
		::DeleteFile(backupdirectory.c_str());
	}
	
	//Copy original CON file to the backup directory
	::CopyFile(goddirectory.c_str(), backupdirectory.c_str(),true);
	
	char* SHA1Buffer[0xACBC];
	char* SHA1Digest[20];
	
	fstream LIVE;

	LIVE.open(goddirectory.c_str(), fstream::in | fstream::out | fstream::binary );
	if (LIVE.is_open())
	{
		LIVE.seekp(0x340);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0xAD);
		LIVE.put(0x0E);
		LIVE.seekp(0x346);
		LIVE.put(0x70);
		LIVE.seekp(0x379);
		LIVE.put(0x24);
		LIVE.put(0x5);
		LIVE.put(0x5);
		LIVE.put(0x11);
		LIVE.seekp(0x3FD);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.put(0x0);
		LIVE.seekp(0x22c);
		LIVE.put(0xFF);
		LIVE.put(0xFF);
		LIVE.put(0xFF);
		LIVE.put(0xFF);
		LIVE.put(0xFF);
		LIVE.put(0xFF);
		LIVE.put(0xFF);
		LIVE.put(0xFF);
		LIVE.seekp(0);
		LIVE.put(0x4C);
		LIVE.put(0x49);
		LIVE.put(0x56);
		LIVE.put(0x45);
		LIVE.flush();
		LIVE.seekg(0x344);
		LIVE.read((char*)SHA1Buffer,0xACBC);
		XeCryptSha(&SHA1Buffer, 0xACBC, NULL,0, NULL,0, &SHA1Digest, 20);
		LIVE.seekp(0x32C);
		LIVE.write((char*) SHA1Digest, 20);
		LIVE.close();
		debugLog("New GOD container created, game should now launch without disk.");
	} else
	{
		debugEnabled = true;
		debugLog("Unable to open new Games On Demand Live file at:");
		debugLog((char*)goddirectory.c_str());
		nxeGame.status = false;
	}

	if(nxeGame.status)
	{
		//Delete old data directory
		::RemoveDirectory(olddatadirectory.c_str());
	}
}

bool isNXE(WCHAR* path, string path2)
{
	string pathcheck;
	pathcheck += path2;
	if (!pathcheck.compare(pathcheck.length() - 9 , 8,"00004000") == 0)
	{
		debugLog("Not in 00004000 dir, file is either an original backup made by NXE2GOD or has been moved since being ripped by NXE, not attempting to convert.");
		return false;
	}

	ifstream NXE;
	NXE.open (path, ifstream::in);
	if (NXE.is_open())
	{
		char * containerBuffer1;
		char * containerBuffer2;
		containerBuffer1 = new char [4];
		containerBuffer2 = new char [4];
		NXE.seekg(0);
		NXE.seekg(0x344);
		NXE.read(containerBuffer1,4);		

		if (int(containerBuffer1[0] == 0))
		{
			if (int(containerBuffer1[1] == 0))
			{
				if (int(containerBuffer1[2] == 64))
				{
					if (int(containerBuffer1[3] == 0))
					{
						NXE.close();
						return true;
					}
				}
			}
		}
	}
	NXE.close();
	return false;
}

HRESULT ScanDir(string strFind)
	{
		HANDLE hFind;
		WIN32_FIND_DATA wfd;
		LPSTR lpPath = new char[MAX_PATH];
		LPWSTR lpPathW = new wchar_t[MAX_PATH];
		LPSTR lpFileName = new char[MAX_PATH];
		LPWSTR lpFileNameW = new wchar_t[MAX_PATH];
		string sFileName;

		sDirAct = strFind;
		strFind+= "\\*";

		strFind._Copy_s(lpPath, strFind.length(),strFind.length());

		lpPath[strFind.length()]='\0';

		::MultiByteToWideChar( CP_ACP, NULL,lpPath, -1, lpPathW, MAX_PATH);

		hFind = FindFirstFile( lpPath, &wfd );
		int nIndex = 0;
		if( INVALID_HANDLE_VALUE == hFind )
		{
			debugLog("Invalid handle type - Directory most likely empty");
			debugLog(lpPath);
			debugLog(lpFileName);
		}
		else
		{
			nIndex = 0;
		do
		{		
			sFileName = wfd.cFileName;
			sFileName._Copy_s(lpFileName, sFileName.length(), sFileName.length());
			lpFileName[sFileName.length()]='\0';
			::MultiByteToWideChar( CP_ACP, NULL,lpFileName, -1, lpFileNameW, MAX_PATH);
		
			if(FILE_ATTRIBUTE_DIRECTORY == wfd.dwFileAttributes)
			{
				
				string nextDir;
				nextDir += lpPath;
				string nextDir1;
				nextDir1 += lpFileName;
				nextDir.erase(nextDir.size() - 1, 1);
				nextDir += nextDir1;
				ScanDir(nextDir);

			} else
			{
				string filePath;
				string filePathX;
				string fileNameX;
				filePath += lpPath;
				string fileName;
				fileName += lpFileName;
				filePath.erase(filePath.size() - 1, 1);
				filePathX += filePath;
				fileNameX += fileName;
				filePath += fileName;

				LPSTR FileA = new char[MAX_PATH];

				LPSTR FileNameA = new char[MAX_PATH];
				LPSTR FilePathA = new char[MAX_PATH];
				filePathX._Copy_s(FilePathA, filePathX.length(),filePathX.length());
				fileNameX._Copy_s(FileNameA , fileNameX.length(),fileNameX.length());
				filePath._Copy_s(FileA, filePath.length(),filePath.length());
				FileA[filePath.length()]='\0';
				LPWSTR FileB = new wchar_t[MAX_PATH];
				::MultiByteToWideChar( CP_ACP, NULL, FileA, -1, FileB, MAX_PATH);

				if (isNXE(FileB,filePathX))
				{
					NXE temp(fileNameX,filePathX);
					allNXE.push_back(temp);
				}else
				{
				}
			}

			nIndex++;
			} while( FindNextFile( hFind, &wfd ));
			FindClose( hFind );
		}
		return S_OK;
	}


//--------------------------------------------------------------------------------------
// Name: main
// Desc: Entry point to the program
//--------------------------------------------------------------------------------------
VOID __cdecl main()
{

	console.Create( "game:\\Media\\Fonts\\Arial_12.xpr", 0x00000000, 0xFFFFFFFF );
    
	if(FileExists("debug.log"))
	{
		debugEnabled = true;
	}

	debugLog("\n");
	debugLog("*Fresh Application run*");

	unMap("hdd:");
	if (Map("hdd:","\\Device\\Harddisk0\\Partition1") == S_OK)
	{
		debugLog("Drive mounted, Scanning folders");
		console.Format( "NXE2GOD V1.1 by Dstruktiv\n" );
		console.Format("For updates please visit www.digitalreality.co.nz\\xbox360\n\n");
		console.Format( "Scanning folders, please wait...\n" );
		ScanDir(filePathzzz);
		if (allNXE.size() == 0)
		{
			console.Format("\nNo NXE titles found\n");
			console.Format( "\nPush any key to exit to NXE" );
		}
		else
		{
			console.Format("\n");
			console.Format( "NXE Files found:\n" );
			console.Format("\n");

			for(unsigned int i = 0; i < allNXE.size(); i++) {
				console.Format(allNXE[i].title);
				console.Format(" at location: ");
				console.Format(allNXE[i].path.c_str());
				console.Format(allNXE[i].fileName.c_str());
				console.Format("\n");
			}

			console.Format("\nPush A to convert files from NXE to GOD or B to cancel and quit to NXE\n\n");
			bool keypush = false;
			while(!keypush)
			{
				ATG::GAMEPAD* pGamepad = ATG::Input::GetMergedInput();
				if( pGamepad->wPressedButtons  & XINPUT_GAMEPAD_A)
				{
					keypush = true;
				}
				if( pGamepad->wPressedButtons  & XINPUT_GAMEPAD_B)
				{
					XLaunchNewImage( XLAUNCH_KEYWORD_DEFAULT_APP, 0 );
				}
			}
			console.Format("Converting NXE2GOD, please wait...\n\n");
			for(unsigned int i = 0; i < allNXE.size(); i++) {
				nxetogod(allNXE[i]);
			}		
			console.Format("\n");
			console.Format("Conversion results:\n\n");
			for(unsigned int i = 0; i < allNXE.size(); i++) {
				if(allNXE[i].status)
				{
					console.Format(allNXE[i].title);
					console.Format(" converted successfully.\n");
				}
				else
				{
					console.Format(allNXE[i].title);
					console.Format(" failed to convert, please check debug.log on root of USB stick.\n");
				}
			}

			console.Format( "\nProcessing complete, push any key to exit to NXE" );
		} 
	}
	
	bool keypush = false;
	while(!keypush)
	{
		ATG::GAMEPAD* pGamepad = ATG::Input::GetMergedInput();
		if( pGamepad->wPressedButtons)
		{
			keypush = true;
		}
	}
}