rem installs the ecolab library into appdata, for non-cygwin/msys applications
mkdir "%APPDATA%\ecolab"
xcopy /e /y include\*.tcl  "%APPDATA%\ecolab"
xcopy /e /y include\Xecolab  "%APPDATA%\ecolab\Xecolab"
