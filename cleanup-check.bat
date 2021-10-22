@echo off

set search_cmd=findstr -d:"source" -s -n -i -l
set file_wildcard=*.h *.cpp *.c *.inl *.hpp

%search_cmd% "@cleanup" %file_wildcard%