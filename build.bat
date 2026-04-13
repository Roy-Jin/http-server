@echo off
taskkill /F /IM http-server.exe 2>nul
gcc -o http-server.exe main.c src\server.c src\http_handler.c src\http_response.c src\file_handler.c src\utils.c src\config.c -lws2_32 -liphlpapi -lz
if %errorlevel% neq 0 exit /b %errorlevel%
copy /Y http-server.exe test\http-server.exe >nul
echo Build successful!
echo Testing version parameter...
http-server.exe -v