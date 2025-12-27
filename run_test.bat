@echo off
echo Running CPU test with full output...
.\build\Release\cpu_exec_test.exe tests\test_roms\donkeykong.nes > test_output.txt 2>&1
echo.
echo Test completed. Showing output:
echo.
type test_output.txt
