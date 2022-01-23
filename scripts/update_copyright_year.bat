@echo off
@rem < and > need to be escaped with ^ on Windows
"C:\Program Files\Git\usr\bin\find.exe"^
  %~dp0..^
  -type f \(^
  -name *.c -or^
  -name *.cpp -or^
  -name *.h -or^
  -name *.gn -or^
  -name *.gni -or^
  -name *.inl^
  -name *.py^
  \)^
  -exec sed -i 's/^<trantuannghia95@gmail.com^> 2021/^<trantuannghia95@gmail.com^> 2022/g' {} ;
