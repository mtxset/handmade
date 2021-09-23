@echo off

set search_cmd=findstr -d:"source" -s -n -i -l
set file_wildcard=*.h *.cpp *.c *.inl *.hpp

echo static found:
%search_cmd% "static" %file_wildcard%

echo --------------------------------------------------------------------------------------------------------

@echo globals found:
%search_cmd% "global_var" %file_wildcard%

echo --------------------------------------------------------------------------------------------------------

@echo locals found:
%search_cmd% "local_persist" %file_wildcard%