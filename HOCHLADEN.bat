@echo off
cd /d "%~dp0"
echo Mochi: Branch claude/pc-test wird zu GitHub hochgeladen.
echo Falls ein Browser-Fenster zur Anmeldung aufgeht, dort bei GitHub bestaetigen.
echo.
git push -u origin claude/pc-test
echo.
if errorlevel 1 (echo FEHLER, bitte Text kopieren und schicken.) else (echo FERTIG, hochgeladen.)
pause
