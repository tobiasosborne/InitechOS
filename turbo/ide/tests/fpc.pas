{ Free Pascal adapter: Reset opens read-only, as Turbo Initech's runtime
  does (seed/rt/fileio.asm, AH=3Dh AL=0); FPC's default FileMode is 2. }
procedure HostSetup;
begin
  FileMode := 0
end;
