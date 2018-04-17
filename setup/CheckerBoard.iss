; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
AppName=CheckerBoard
AppVerName=CheckerBoard version 1.75
AppPublisher=Martin Fierz
DefaultDirName={pf32}\CheckerBoard
UsePreviousAppDir=no
AppendDefaultDirName=no
DefaultGroupName=CheckerBoard
DisableProgramGroupPage=no
SourceDir=.
OutputDir=Output
OutputBaseFilename=CheckerBoardSetup32.175
Compression=lzma/ultra
SolidCompression=yes
Uninstallable=yes
DirExistsWarning=no

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "Executables\checkerboard.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\help.htm"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\helpspanish.htm"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\nutshell.htm"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\toolbar.bmp"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\11_man_english.pdn"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\11_man_italian.pdn"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\bmp\*"; DestDir: "{app}\bmp"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\db\db2.cpr"; DestDir: "{app}\db"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\db\db2.idx"; DestDir: "{app}\db"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\db\db3.cpr"; DestDir: "{app}\db"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\db\db3.idx"; DestDir: "{app}\db"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\db\db4.cpr"; DestDir: "{app}\db"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\db\db4.idx"; DestDir: "{app}\db"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\nutshellgif\*"; DestDir: "{app}\nutshellgif"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\source\cb_interface.h"; DestDir: "{app}\source"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\source\dama.c"; DestDir: "{app}\source"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\source\dama.def"; DestDir: "{app}\source"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\source\damahelp.htm"; DestDir: "{app}\source"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\source\enginedefs.h"; DestDir: "{app}\source"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\source\simplech.c"; DestDir: "{app}\source"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\source\simplech.def"; DestDir: "{app}\source"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "..\source\simplechhelp.htm"; DestDir: "{app}\source"; Flags: ignoreversion recursesubdirs createallsubdirs

[Dirs]
Name: "{app}\engines"

[Tasks]
Name: startmenu; Description: Create a start menu entry
Name: desktopicon; Description: Create a desktop shortcut

[Icons]
Name: "{commonprograms}\CheckerBoard\CheckerBoard"; Filename: "{app}\CheckerBoard.exe"; WorkingDir: "{app}"; Tasks: startmenu
Name: "{commondesktop}\CheckerBoard"; Filename: "{app}\CheckerBoard.exe"; WorkingDir: "{app}"; Tasks: desktopicon
