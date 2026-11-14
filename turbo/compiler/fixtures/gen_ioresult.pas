{$MODE DELPHI}{$B+}{$H-}
{ Recoverable file I/O (beads initech-n6r9v): the $I switch, IOResult,
  Close, Erase and Rename, after Turbo Pascal 5.5 (Reference Guide:
  IOResult, the I compiler directive, Close, Erase, Rename; run-time error
  103 "File not open"). DOS error codes: 2 file not found (DOS 3.30
  Technical Reference, extended error codes).

  Golden provenance: HAND-COMPUTED from those rules, not FPC-minted:
  a failed operation leaves its code pending, IOResult returns and clears
  it, and while a code is pending further I/O does nothing. IOResult is
  always taken into a variable before Writeln, because a pending error
  would also suppress Free Pascal's own Writeln.
  Expected exact stdout:
    RESET-MISSING 2
    CLEARED 0
    CLOSE-UNOPENED 103
    WRITE 8 0
    RENAME 0
    OLD-NAME 2
    READ 8 ABCDEFGH 0
    ERASE 0
    ERASE-AGAIN 2
    RENAME-MISSING 2
    SKIPPED 2 2
    DONE }
program IoResultShared;
var
  F, G: file;
  Buf: string[8];
  Got, Code, Again: integer;
begin
  {$I-}
  Assign(F, 'NOFILE.BIN');
  Reset(F, 1);
  Code := IOResult();
  Writeln('RESET-MISSING ', Code);
  Code := IOResult();
  Writeln('CLEARED ', Code);
  Close(F);
  Code := IOResult();
  Writeln('CLOSE-UNOPENED ', Code);
  Assign(F, 'IORES.BIN');
  Rewrite(F, 1);
  Buf := 'ABCDEFGH';
  BlockWrite(F, Buf[1], 8, Got);
  Close(F);
  Code := IOResult();
  Writeln('WRITE ', Got, ' ', Code);
  Rename(F, 'IORES2.BIN');
  Code := IOResult();
  Writeln('RENAME ', Code);
  Assign(G, 'IORES.BIN');
  Reset(G, 1);
  Code := IOResult();
  Writeln('OLD-NAME ', Code);
  Reset(F, 1);
  Buf := '????????';
  BlockRead(F, Buf[1], 8, Got);
  Close(F);
  Code := IOResult();
  Writeln('READ ', Got, ' ', Buf, ' ', Code);
  Erase(F);
  Code := IOResult();
  Writeln('ERASE ', Code);
  Erase(F);
  Code := IOResult();
  Writeln('ERASE-AGAIN ', Code);
  Rename(G, 'IORES4.BIN');
  Code := IOResult();
  Writeln('RENAME-MISSING ', Code);
  Reset(G, 1);
  Assign(F, 'IORES3.BIN');
  Rewrite(F, 1);
  Code := IOResult();
  Reset(F, 1);
  Again := IOResult();
  Writeln('SKIPPED ', Code, ' ', Again);
  {$I+}
  Writeln('DONE')
end.
