@echo off

set output=H:\Games\
set input=D:\xbox\Games\


for /f "delims=|" %%G in ('dir /b %input% ') DO (call :extract "%%G")
GOTO :eof

:extract

	set file=%1
	set file=%file:~1,-1%
	set base=%file:~0,-4%
	set ext=%file:~-3%
	
	if NOT %ext%==iso goto :EOF
	echo Processing %base%:
	if EXIST %output%%base% goto :already
	
	:: Start extraction
	exiso.exe -xqs -d %output%%base% %input%%file%
	echo    Done

goto :EOF

:already
	echo    Skipping, already extracted


