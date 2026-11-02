@echo off
echo Starting CineBook Movie Ticket Booking Server...
if not exist "cinebook_server.exe" (
    echo [Notice] cinebook_server.exe not found. Building first...
    call build.bat
)
cinebook_server.exe config/cinebook.conf
