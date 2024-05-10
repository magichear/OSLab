@echo off
echo plz write first
"D:\Study_Work\Electronic_data\CS\AAAUniversity\OS\OSLab\Lab4\tools\tools\windows\RKDevTool.exe"
pause
pause
echo ---------------------------------------------------------------
echo hdc start help
echo ---------------------------------------------------------------

hdc list targets
pause
echo ---------------------------------------------------------------
echo Sending student ID file
echo ---------------------------------------------------------------
hdc file send D:\Study_Work\Electronic_data\CS\AAAUniversity\OS\OSLab\Lab4\ID_part\stuid /data/local/tmp/
echo ---------------------------------------------------------------
echo "cd /data/local/tmp/ ; ls"
echo "chmod +x stuid ; ./stuid"
echo "simpleshell"
echo ">> a echo 66666666 ; cat a ; ls ; pwd ; rm a  ; ls ; rm stuid ; ls"
echo ---------------------------------------------------------------
hdc shell

pause