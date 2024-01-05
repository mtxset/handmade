$installPath = &"C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe" -version 16.0 -property installationPath | Select-Object -First 1
$installPathString = $installPath.ToString()
Import-Module (Join-Path $installPathString "Common7\Tools\Microsoft.VisualStudio.DevShell.dll")
Enter-VsDevShell -VsInstallPath $installPathString -SkipAutomaticLocation -DevCmdArguments "-arch=amd64"