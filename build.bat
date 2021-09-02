@echo off

set common_compiler_flags=-Od -MT -EHa- -Gm- -GR- -Oi -DINTERNAL=1 -DDEBUG=1 -WX -W4 -wd4201 -wd4459 -wd4100 -wd4189 -FC -Fm -Z7 -nologo
set common_linker_flags=-incremental:no  -opt:ref user32.lib gdi32.lib winmm.lib

if not exist build mkdir build
pushd build
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

:: Get start time:
for /F "tokens=1-4 delims=:.," %%a in ("%time%") do (
   set /A "start=(((%%a*60)+1%%b %% 100)*60+1%%c %% 100)*100+1%%d %% 100"
)
:: 32-bit
:: cl.exe %common_compiler_flags% "..\source\main.cpp" /link -subsystem:windows,5.1  %common_linker_flags%

:: 64-bit
cl.exe %common_compiler_flags% "..\source\game.cpp" /LD /link /EXPORT:game_get_sound_samples /EXPORT:game_update_render
cl.exe %common_compiler_flags% "..\source\main.cpp" /link %common_linker_flags%
popd

:: -Od   - disable all optimizations
:: -WX   - treat each warning as error
:: -W4   - warning level 4
:: -Oi   - let compiler use CPU intrinsic functions (like cpu's asm sine instead of c/cpp sine)
:: -wd*  - supressing specific error
:: -FC   - outputs full (instead of relative) soure code path
:: -Fm   - enables map file which has every function showing it's source (function name - module name)
:: -Z7   - debug file format (some old one)
:: -GR-  - disables checking object types during run time
:: -Gm-  - disable any incremental build
:: -EHa- - disables exceptions
:: -opt-ref - removes all imports which are not needed
:: /link - subsytem:windows,5.1 - support xp
:: -MT   - package everything instead of expecting user to have dll which does laoding of exe into windows

:: Get end time:
for /F "tokens=1-4 delims=:.," %%a in ("%time%") do (
   set /A "end=(((%%a*60)+1%%b %% 100)*60+1%%c %% 100)*100+1%%d %% 100"
)

:: Get elapsed time:
set /A elapsed=end-start

:: Show elapsed time:
set /A hh=elapsed/(60*60*100), rest=elapsed%%(60*60*100), mm=rest/(60*100), rest%%=60*100, ss=rest/100, cc=rest%%100
if %mm% lss 10 set mm=0%mm%
if %ss% lss 10 set ss=0%ss%
if %cc% lss 10 set cc=0%cc%
echo Build time: %ss%.%cc% seconds
:: cl -Dsome_preprocessor_param=1
