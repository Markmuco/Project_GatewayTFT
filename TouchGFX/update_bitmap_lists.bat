@echo off
rem Update the bitmap cache lists of the views (run after Generate Code in TouchGFX Designer)
cd /d "%~dp0"
python tools\update_bitmap_lists.py
if errorlevel 1 (
    echo.
    echo FAILED, see message above
)
pause
