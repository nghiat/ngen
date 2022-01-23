@echo off
"C:\Program Files\Git\usr\bin\find.exe" core sample -type f \( -name *.cpp -or -name *.h -or -name *.inl \) -exec sed -i 's/\bOld_term\b/New_term/g' {} ;
