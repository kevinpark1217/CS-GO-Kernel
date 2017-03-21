start "" "T:\Personal\Visual Studio\CS-GO-Kernel\dsefix.exe"
timeout /t 2
sc create csgo binpath="T:\Personal\Visual Studio\CS-GO-Kernel\x64\Release\KernelDriver.sys" type=kernel
sc start csgo
timeout /t 5
start "" "T:\Personal\Visual Studio\CS-GO-Kernel\dsefix.exe" -e
timeout /t 2