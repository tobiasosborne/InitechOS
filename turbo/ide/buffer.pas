{ buffer.pas -- the paged document behind the editor.

  The whole document lives in a work file; only one page of whole lines is
  in memory. Lines are held LF-terminated (the last line has no
  terminator); loading turns CR LF into LF and stops at the first 0x1A,
  saving writes CR LF between lines and one 0x1A at the end, as TP 5.5
  saved (turbopascal-decomp MINT06 disk/DEMO.PAS). Leaving the page writes
  prefix + page + suffix to the other work file, so the document is never
  copied whole into memory and a file of any size the disk holds can be
  edited. The page is refilled to half its size, leaving room to grow. }
const
  PageMax = 16384;
  PageFill = 8192;
  PageLinesMax = 16385;
  BufChunkMax = 255;     { BlockRead/BlockWrite move ShortString bytes }
  LineMax = 250;
  BufOk = 0;
  BufTooLong = 1;      { a line longer than PageFill bytes }

var
  Pg: array[1..PageMax] of char;
  PgLs: array[1..PageLinesMax] of integer;  { start of each page line }
  PgLen, PgLines, PgLine, PgStart, PgOrig: integer;
  PgDirty: boolean;
  DocLen, DocLines: integer;
  BufStatus: integer;
  WorkSide: integer;
  WorkName0, WorkName1: string;
  WorkIn, WorkOut: file;
  Chunk, OutChunk: string;
  OutUsed: integer;
  BufErr: integer;     { first DOS error of the last load/save/copy; 0 if none }

{ Take any pending I/O error; keep the first one in BufErr. }
procedure IoNote;
var Code: integer;
begin
  Code := IOResult();
  if (Code <> 0) and (BufErr = 0) then BufErr := Code
end;

procedure WorkNames(Side: integer; var Name: string);
begin
  if Side = 0 then Name := WorkName0 else Name := WorkName1
end;

procedure OutStart(var Name: string);
begin
  Assign(WorkOut, Name);
  Rewrite(WorkOut, 1);
  IoNote();
  OutUsed := 0
end;

procedure OutFlush;
var Wrote: integer;
begin
  if OutUsed > 0 then
  begin
    Wrote := 0;
    BlockWrite(WorkOut, OutChunk[1], OutUsed, Wrote);
    IoNote();
    { a short write is DOS's disk-full answer: TP run-time error 101 }
    if (Wrote <> OutUsed) and (BufErr = 0) then BufErr := 101
  end;
  OutUsed := 0
end;

procedure OutByte(C: char);
begin
  if OutUsed = BufChunkMax then OutFlush();
  OutUsed := OutUsed + 1;
  OutChunk[OutUsed] := C
end;

{ Rebuild the page line index from PgLs[From] onwards. }
procedure PageIndex(From: integer);
var I, N: integer;
begin
  N := From;
  if N < 1 then N := 1;
  if N = 1 then PgLs[1] := 1;
  I := PgLs[N];
  while I <= PgLen do
  begin
    if Pg[I] = chr(10) then
    begin
      N := N + 1;
      if N <= PageLinesMax then PgLs[N] := I + 1
    end;
    I := I + 1
  end;
  { The page always ends with whole lines: either the last ends in LF and
    the next page line is a line of the following page, or it is the
    document's last line. }
  if PgStart + PgOrig >= DocLen then PgLines := N
  else PgLines := N - 1
end;

{ Copy Count bytes of the current work file to WorkOut, reading from the
  current position of WorkIn. }
procedure CopyBytes(Count: integer);
var Want, Got, I: integer;
begin
  while (Count > 0) and (BufErr = 0) do
  begin
    Want := Count;
    if Want > BufChunkMax then Want := BufChunkMax;
    BlockRead(WorkIn, Chunk[1], Want, Got);
    IoNote();
    if (Got <> Want) and (BufErr = 0) then BufErr := 100;
    I := 1;
    while I <= Got do
    begin
      OutByte(Chunk[I]);
      I := I + 1
    end;
    if Got = 0 then Count := 0 else Count := Count - Got
  end
end;

{ Write the page back: prefix + page + suffix into the other work file. }
procedure PageFlush;
var Name: string;
    I, Got0: integer;
begin
  if PgDirty then
  begin
    BufErr := 0;
    WorkNames(WorkSide, Name);
    Assign(WorkIn, Name);
    Reset(WorkIn, 1);
    IoNote();
    WorkNames(1 - WorkSide, Name);
    OutStart(Name);
    CopyBytes(PgStart);
    I := 1;
    while I <= PgLen do
    begin
      OutByte(Pg[I]);
      I := I + 1
    end;
    { skip the old page bytes, then copy the suffix }
    I := PgOrig;
    while (I > 0) and (BufErr = 0) do
    begin
      if I > BufChunkMax then
      begin
        BlockRead(WorkIn, Chunk[1], BufChunkMax, Got0);
        IoNote();
        if (Got0 <> BufChunkMax) and (BufErr = 0) then BufErr := 100;
        I := I - BufChunkMax
      end
      else
      begin
        BlockRead(WorkIn, Chunk[1], I, Got0);
        IoNote();
        if (Got0 <> I) and (BufErr = 0) then BufErr := 100;
        I := 0
      end
    end;
    CopyBytes(DocLen - PgStart - PgOrig);
    OutFlush();
    Close(WorkIn); IoNote();
    Close(WorkOut); IoNote();
    { Commit only a complete, closed replacement. On failure the old
      work file and every byte of the dirty page remain retryable. }
    if BufErr = 0 then
    begin
    DocLen := DocLen - PgOrig + PgLen;
    PgOrig := PgLen;
    WorkSide := 1 - WorkSide;
    PgDirty := false
    end
  end
end;

{ Load the page so that it starts at document line First (or earlier). }
procedure PageLoad(First: integer);
var Name: string;
    Line, Pos, Got, I, Take, LastLf: integer;
    Done: boolean;
begin
  PageFlush();
  if BufErr = 0 then
  begin
  if First < 1 then First := 1;
  if First > DocLines then First := DocLines;
  WorkNames(WorkSide, Name);
  Assign(WorkIn, Name);
  Reset(WorkIn, 1);
  Line := 1; Pos := 0; PgLen := 0; Done := false; LastLf := 0;
  BufStatus := BufOk;
  while not Done do
  begin
    BlockRead(WorkIn, Chunk[1], BufChunkMax, Got);
    if Got = 0 then Done := true;
    I := 1;
    while I <= Got do
    begin
      if Line < First then
      begin
        if Chunk[I] = chr(10) then
        begin
          Line := Line + 1;
          if Line = First then PgStart := Pos + 1
        end
      end
      else if PgLen < PageFill then
      begin
        PgLen := PgLen + 1;
        Pg[PgLen] := Chunk[I];
        if Chunk[I] = chr(10) then LastLf := PgLen
      end
      else Done := true;
      Pos := Pos + 1;
      I := I + 1
    end
  end;
  Assign(WorkIn, Name);
  if First = 1 then PgStart := 0;
  { keep whole lines only, unless the page reaches the end of the document }
  if PgStart + PgLen < DocLen then
  begin
    if LastLf = 0 then BufStatus := BufTooLong
    else PgLen := LastLf
  end;
  PgLine := First;
  PgOrig := PgLen;
  PgDirty := false;
  PageIndex(1)
  end
end;

{ Is document line L in the page? }
function PageHas(L: integer): boolean;
begin
  PageHas := (L >= PgLine) and (L < PgLine + PgLines)
end;

{ Make sure lines L .. L+Span-1 (those that exist) are in the page. }
procedure PageCover(L, Span: integer);
var Last: integer;
begin
  BufErr := 0;
  Last := L + Span - 1;
  if Last > DocLines then Last := DocLines;
  if not (PageHas(L) and PageHas(Last)) then
  begin
    if L > 22 then PageLoad(L - 22) else PageLoad(1)
  end
end;

{ Bytes of page line L (document numbering): start index and length. }
procedure LineSpan(L: integer; var Start, Len: integer);
var K, Stop: integer;
begin
  K := L - PgLine + 1;
  Start := PgLs[K];
  if K < PgLines then Stop := PgLs[K + 1] - 1
  else if PgStart + PgOrig >= DocLen then Stop := PgLen + 1
  else Stop := PgLs[K + 1] - 1;
  Len := Stop - Start
end;

procedure LineText(L: integer; var S: string);
var Start, Len, I: integer;
    C: string;
begin
  S := '';
  if (L >= 1) and (L <= DocLines) and PageHas(L) then
  begin
    LineSpan(L, Start, Len);
    if Len > 255 then Len := 255;
    I := 0;
    while I < Len do
    begin
      C := Pg[Start + I];
      S := S + C;
      I := I + 1
    end
  end
end;

{ Insert one byte at page index At; lines after it move on by one. }
procedure PageInsert(At: integer; C: char; L: integer);
var I, Off: integer;
begin
  if PgLen >= PageMax then
  begin
    Off := At - PgLs[L - PgLine + 1];
    PageLoad(L);
    At := PgLs[1] + Off
  end;
  if BufErr = 0 then
  begin
  I := PgLen;
  while I >= At do
  begin
    Pg[I + 1] := Pg[I];
    I := I - 1
  end;
  Pg[At] := C;
  PgLen := PgLen + 1;
  PgDirty := true;
  if C = chr(10) then DocLines := DocLines + 1;
  PageIndex(L - PgLine + 1)
  end
end;

procedure PageDelete(At: integer; L: integer);
var I: integer;
begin
  if BufErr = 0 then
  begin
  if Pg[At] = chr(10) then DocLines := DocLines - 1;
  I := At;
  while I < PgLen do
  begin
    Pg[I] := Pg[I + 1];
    I := I + 1
  end;
  PgLen := PgLen - 1;
  PgDirty := true;
  PageIndex(L - PgLine + 1)
  end
end;

{ Load a DOS text file into a fresh work file. }
procedure BufLoad(var FileName: string);
var Src: file;
    Got, I: integer;
    Done, PendingCr: boolean;
    Name: string;
begin
  BufErr := 0;
  Assign(WorkIn, FileName);
  Reset(WorkIn, 1);
  IoNote();
  if BufErr = 0 then
  begin
  WorkSide := 0;
  WorkNames(0, Name);
  OutStart(Name);
  DocLen := 0; DocLines := 1; Done := false; PendingCr := false;
  while not Done do
  begin
    Got := 0;
    BlockRead(WorkIn, Chunk[1], BufChunkMax, Got);
    IoNote();
    if BufErr <> 0 then Got := 0;
    if Got = 0 then Done := true;
    I := 1;
    while I <= Got do
    begin
      if Done then I := Got
      else if Chunk[I] = chr(26) then Done := true
      else if Chunk[I] = chr(13) then
      begin
        if PendingCr then begin OutByte(chr(13)); DocLen := DocLen + 1 end;
        PendingCr := true
      end
      else
      begin
        if PendingCr and (Chunk[I] <> chr(10)) then
        begin OutByte(chr(13)); DocLen := DocLen + 1 end;
        PendingCr := false;
        OutByte(Chunk[I]);
        DocLen := DocLen + 1;
        if Chunk[I] = chr(10) then DocLines := DocLines + 1
      end;
      I := I + 1
    end
  end;
  if PendingCr then begin OutByte(chr(13)); DocLen := DocLen + 1 end;
  OutFlush();
  Assign(WorkIn, Name);
  Assign(WorkOut, Name);
  PgDirty := false;
  PageLoad(1)
  end
end;

procedure BufNew;
var Name: string;
begin
  WorkSide := 0;
  WorkNames(0, Name);
  OutStart(Name);
  Assign(WorkOut, Name);
  DocLen := 0; DocLines := 1;
  PgDirty := false;
  PageLoad(1)
end;

{ Copy file From to file Into byte for byte (the .BAK copy). }
procedure FileCopy(var From, Into: string);
var Got, Wrote: integer;
    Done: boolean;
begin
  BufErr := 0;
  Assign(WorkIn, From);
  Reset(WorkIn, 1);
  IoNote();
  if BufErr = 0 then
  begin
    OutStart(Into);
    IoNote();
    Done := BufErr <> 0;
    while not Done do
    begin
      Got := 0;
      BlockRead(WorkIn, Chunk[1], BufChunkMax, Got);
      IoNote();
      if (Got = 0) or (BufErr <> 0) then Done := true
      else
      begin
        Wrote := 0;
        BlockWrite(WorkOut, Chunk[1], Got, Wrote);
        IoNote();
        if (Wrote <> Got) and (BufErr = 0) then BufErr := 101
      end
    end
  end;
  Close(WorkIn);
  Close(WorkOut);
  Got := IOResult()
end;

{ Write the document as a DOS text file. }
procedure BufSave(var FileName: string);
var Name: string;
    Got, I: integer;
    Done: boolean;
begin
  BufErr := 0;
  PageFlush();
  if BufErr = 0 then
  begin
  WorkNames(WorkSide, Name);
  Assign(WorkIn, Name);
  Reset(WorkIn, 1);
  IoNote();
  OutStart(FileName);
  IoNote();
  Done := BufErr <> 0;
  while not Done do
  begin
    Got := 0;
    BlockRead(WorkIn, Chunk[1], BufChunkMax, Got);
    IoNote();
    if (Got = 0) or (BufErr <> 0) then Done := true;
    I := 1;
    while I <= Got do
    begin
      if Chunk[I] = chr(10) then OutByte(chr(13));
      OutByte(Chunk[I]);
      I := I + 1
    end
  end;
  if BufErr = 0 then
  begin
    OutByte(chr(26));
    OutFlush()
  end;
  Close(WorkIn); IoNote();
  Close(WorkOut); IoNote()
  end
end;
