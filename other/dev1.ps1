$installPath = &"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -version 16.0 -property installationpath
$installPathString = $installPath.ToString()

Enter-VsDevShell -VsInstallPath $installPathString -SkipAutomaticLocation -DevCmdArguments "-arch=amd64"

./4coder.bat