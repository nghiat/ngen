@echo off

pushd %~dp0..
@rem "C:\Program Files\Git\usr\bin\find.exe" core sample test -type f \( -name *.cpp -or -name *.h -or -name *.inl \) -exec sed -i 's/\bOld_term\b/New_term/g' {} ;
popd
