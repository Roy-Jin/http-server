@echo off
gcc -o http-server.exe main.c src\server.c src\http_handler.c src\http_response.c src\file_handler.c src\utils.c -lws2_32 -liphlpapi
if %errorlevel% neq 0 exit /b %errorlevel%
copy /Y http-server.exe test\http-server.exe >nul
echo Build successful!