@set dir=%CD%
@set "files=bshelves.exe bookshelf.dll msvc*.dll Qt*.dll ui/* plugins/*.dll imageformats/*.dll"
@cd ..\..\output\%1%\
@del bshelves_1.0.exe
@del bshelves_1.0.7z
@del bshelves_1.0.zip
@7z a -y -tzip bshelves_1.0.zip %files%
@7z a -y bshelves_1.0.7z %files%
@7z a -y -sfx bshelves_1.0.exe %files%
@cd %dir%