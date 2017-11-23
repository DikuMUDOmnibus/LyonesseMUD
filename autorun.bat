@echo off
if exist .\log\syslog del .\log\syslog

:BIGLOOP
echo.
echo Starting LyonesseMud.
echo.

.\bin\lyonesse.exe

if exist .killscript	goto KILLSCRIPT
if exist pause		goto PAUSE
if exist .fastboot	goto FASTBOOT
if not exist .fastboot	goto SLOWBOOT

:KILLSCRIPT
echo Autoscript killed %DATE%  >> .\log\syslog
del .killscript > NUL
goto :EXIT

:PAUSE
pause
del .\pause > NUL
goto BIGLOOP

:FASTBOOT
del .\.fastboot > NUL
goto BIGLOOP

:SLOWBOOT
goto BIGLOOP

:EXIT
find "self-delete"		.\log\syslog >> .\log\delete.log
find "death trap"		.\log\syslog >> .\log\dts.log
find "killed"			.\log\syslog >> .\log\rip.log
find "Running"			.\log\syslog >> .\log\restarts.log
find "advanced"			.\log\syslog >> .\log\levels.log
find "usage"			.\log\syslog >> .\log\usage.log
find "new player"		.\log\syslog >> .\log\newplayers.log
find "SYSERR"			.\log\syslog >> .\log\errors.log
find "(GC)"			.\log\syslog >> .\log\godcmds.log
find "Bad PW"			.\log\syslog >> .\log\badpws.log

del .\log\syslog.1 > NUL
if exist .\log\syslog.2	copy .\log\syslog.2 .\log\syslog.1	> NUL
if exist .\log\syslog.3	copy .\log\syslog.3 .\log\syslog.2	> NUL
if exist .\log\syslog.4	copy .\log\syslog.4 .\log\syslog.3	> NUL
if exist .\log\syslog.5	copy .\log\syslog.5 .\log\syslog.4	> NUL
if exist .\log\syslog.6	copy .\log\syslog.6 .\log\syslog.5	> NUL
if exist .\log\syslog	copy .\log\syslog   .\log\syslog.6	> NUL
del .\log\syslog > NUL
cls

:END
echo.
echo.
echo.echo Autorun Batch file ended...
@echo on
