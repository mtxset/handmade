@echo off

:: -wd4100: unreferenced formal parameter
:: -wd4189: local variable is initialized but not referenced
:: -wd4201: nonstandard extension used: nameless struct/union
:: -wd4459: declaration of 'x' hides global declaration
:: -wd4505: unreferenced local function has been removed
:: -wd4668: 'x' is not defined as a preprocessor macro, replacing with '0' for '#if/#elif' (found in win headers)
set include_iaca=-Id:/tools/iaca/
set ignore_errors=-wd4100 -wd4189 -wd4201 -wd4459 -wd4505 -wd4668
set enable_errors=-w44800
set common_compiler_flags=-Zo -Od -MTd -EHa- -Gm- -GR- -Oi -fp:fast -DAR1610 -DINTERNAL=1 -DDEBUG=1 -WX -W4 %ignore_errors% %enable_errors% -FC -Fm -Z7 -nologo
set common_linker_flags=-incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

if not exist build mkdir build
pushd build
:: call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

del *.pdb > NUL 2> NUL

:: Get start time:
for /F "tokens=1-4 delims=:.," %%a in ("%time%") do (
   set /A "start=(((%%a*60)+1%%b %% 100)*60+1%%c %% 100)*100+1%%d %% 100"
)
:: 32-bit
:: cl.exe %common_compiler_flags% "..\source\main.cpp" /link -subsystem:windows,5.1  %common_linker_flags%

:: 64-bit
:: /O2 /Oi /fp:fast - optimizations
:: lock is done so that vs has time to create debug file for dll, so breakpoints work for new code
:: game wont load new code till there is lock file
echo Waiting for pbd > lock.tmp
cl.exe %include_iaca% %common_compiler_flags% "..\source\game.cpp" /LD /link -incremental:no -opt:ref -PDB:game%random%.pdb  -EXPORT:game_get_sound_samples -EXPORT:game_update_render
del lock.tmp
cl.exe %common_compiler_flags% "..\source\main.cpp" /link %common_linker_flags%
popd

:: -Zo      - enables additional debug info, so you can debug "better" with optimizations on otherwise you won't have vars in watch, because code is different
:: -Od      - disable all optimizations
:: -WX      - treat each warning as error
:: -W4      - warning level 4
:: -Oi      - let compiler use CPU intrinsic functions (like cpu's asm sine instead of c/cpp sine)
:: -wd*     - supressing specific error
:: -FC      - outputs full (instead of relative) soure code path
:: -Fm      - enables map file which has every function showing it's source (function name - module name)
:: -Z7      - debug file format (some old one)
:: -GR-     - disables checking object types during run time
:: -Gm-     - disable any incremental build
:: -EHa-    - disables exceptions
:: -opt-ref - removes all imports which are not needed
:: /link    - subsytem:windows,5.1 - support xp
:: -MTd     - package everything instead of expecting user to have dll which does laoding of exe into windows (d - debug)
:: -DAR1610 - get 16:10 aspect ratio (window and DIB)
:: -02      - enable optimizations
:: -Oi      - enable intrinsics
:: -fp:fast - allow for faster (by lossing some precision?) floating point calculations, if enabled floorf dissappears in asm

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
