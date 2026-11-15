{$MODE DELPHI}{$B+}{$H-}
{
  turbo.pas -- Turbo Initech single-file bootstrap, B9.4 x86 codegen slice.

  Ref: legacy Pascal subset plan Sec 3 and Sec 5; ADR-0007
  DEC-02, DEC-04, DEC-05, and DEC-07; beads initech-6m52 and
  initech-lh83. Pascal lives here by CLAUDE.md Law 3.

  Input is the fixed B8 filename TURBOIN.PAS. A two-ShortString rolling window
  supplies exact bytes to the monotonic lexer and is reopened for each driver
  pass; this lets Turbo Initech check its grown source without enlarging BSS. The B9.2
  SourceWords arena is retired; the build gate now proves the rolling window
  and the absence of that dead 80 KiB reservation. Overflow is loud.
  Identifiers are canonical lower-case and live in one global PoolChars array.

  Dump protocols are specified in LEXER-DUMP.md, PARSER-TRACE.md, and
  SYMBOLS-DUMP.md. The driver emits the unchanged lexer bracket, resets for
  the unchanged syntax-only recursive-descent trace, then resets once more
  for an AST-free semantic parse and deterministic symbol dump.

  PHASE DIVERGENCE (deliberate): seed/typecheck.c parses to an AST, collects
  declarations, then checks that AST (lines 10-23 and 2076-2127). Turbo Initech owns no
  AST by design. Its third driver pass mirrors the seed's semantic rules while
  checking expressions/statements as they are parsed. This is the plan's
  sanctioned single-pass architecture; it changes phase structure, not the
  type compatibility rules. A call to a later routine therefore needs an
  explicit prior forward in Turbo Initech, while the seed's collect_procs-before-bodies
  structure can already see every later header (seed/typecheck.c:2090-2108).

  SEMANTIC REFERENCE MAP (rules mirrored, phase structure excepted above):
    - insertion-ordered globals/local-first lookup and duplicate policy:
      seed/typecheck.c:248-358,443-589,1944-2071;
    - routine signatures, forwards, result variables, and call/var-parameter
      rules: seed/typecheck.c:598-778,1015-1185,2014-2127;
    - B7 char/string coercion table and ordinal operators:
      seed/typecheck.c:1189-1365;
    - array/string indexing and named-record scalar fields:
      seed/typecheck.c:1368-1557;
    - exact scalar, string/char, field, indexed, and named-record assignment:
      seed/typecheck.c:1560-1868;
    - B8 storage-only file surface: seed/typecheck.c:829-1007,1425-1429,
      1593-1597; type/bound forms enforced by seed/parser.c:1170-1333,
      scalar-only record fields by seed/parser.c:1554-1665.

  Lexer dump:
    TURBO-LEX-BEGIN
    one token per line
    TURBO-LEX-END
  Keywords use KW plus their lower-case spelling. Identifiers use IDENT plus
  the pooled spelling. Integers use INTEGER plus decimal value. Strings use
  STRING, decoded length, then one decimal byte per payload character. Fixed
  punctuation kind names and EOF carry no payload. Errors use one located
  TURBO-LEX-ERROR line, stop the driver, then close the output bracket.

  Parser trace:
    TURBO-PARSE-BEGIN
    ENTER production Lline / EXIT production Lline
    TURBO-PARSE-OK or one located TURBO-PARSE-ERROR line
    TURBO-PARSE-END

  Source discipline: boolean evaluation is complete. Every bounds-sensitive
  access is guarded by nested if statements or by a sentinel accessor, and
  flag-controlled loops test the flag alone.
}
program TurboInitech;

const
  SourceMax = 262144;
  PoolMax = 16384;
  NameMax = 1536;
  ChunkMax = 255;
  EscapeQuoteCode = 39; { TURBO_LEX_MUT_STRESC }

  { Fixed arenas, sized for one TURBO.COM holding the compiler and the IDE
    (initech-n6r9v): ten integer columns (nine symbol columns and
    NativeRoutines) x 2,048 rows = 81,920 bytes, 1,536 names x 8 bytes and
    a 16,384-character pool. The compiler with the IDE measured 1,292
    symbols, 795 names and 7,216 pool characters before the IDE's compile
    and run commands. Memory: 57,856 bytes more BSS than the 1,024/704/6,144
    tables, inside the extended module's reserve (turbo/README.md). }
  SymbolMax = 2048;
  HeaderParamMax = 32;
  PendingNameMax = 32;

  TyUnknown = 0;
  TyInteger = 1;
  TyBoolean = 2;
  TyChar = 3;
  TyString = 4;
  TyRecord = 5;
  TyFile = 6;
  TyNone = 7;

  SkConst = 1;
  SkType = 2;
  SkField = 3;
  SkVar = 4;
  SkArray = 5;
  SkProcedure = 6;
  SkFunction = 7;
  SkValueParam = 8;
  SkVarParam = 9;
  SkResult = 10;
  SkLocal = 11;
  SkLocalArray = 12;

  TeDuplicate = 1;
  TeUnknownType = 2;
  TeAssignMismatch = 3;
  TeVarParam = 4;
  TeBoundForm = 5;
  TeFileAssign = 6;
  TeRule = 7;

  TkNone = -1;
  TkEof = 0;
  TkIdent = 1;
  TkInteger = 2;
  TkString = 3;

  TkProgram = 10;
  TkVar = 11;
  TkBegin = 12;
  TkEnd = 13;
  TkIntegerType = 14;
  TkDiv = 15;
  TkMod = 16;
  TkWrite = 17;
  TkWriteln = 18;
  TkBoolean = 19;
  TkAnd = 20;
  TkOr = 21;
  TkNot = 22;
  TkTrue = 23;
  TkFalse = 24;
  TkIf = 25;
  TkThen = 26;
  TkElse = 27;
  TkWhile = 28;
  TkDo = 29;
  TkFor = 30;
  TkTo = 31;
  TkDownto = 32;
  TkRepeat = 33;
  TkUntil = 34;
  TkCase = 35;
  TkConst = 36;
  TkChar = 37;
  TkOrd = 38;
  TkChr = 39;
  TkProcedure = 40;
  TkFunction = 41;
  TkForward = 42;
  TkArray = 43;
  TkOf = 44;
  TkType = 45;
  TkRecord = 46;
  TkStringType = 47;
  TkLength = 48;
  TkFile = 49;

  TkSemi = 60;
  TkDot = 61;
  TkComma = 62;
  TkColon = 63;
  TkAssign = 64;
  TkLParen = 65;
  TkRParen = 66;
  TkPlus = 67;
  TkMinus = 68;
  TkStar = 69;
  TkEq = 70;
  TkNe = 71;
  TkLt = 72;
  TkLe = 73;
  TkGt = 74;
  TkGe = 75;
  TkLBracket = 76;
  TkRBracket = 77;
  TkDotDot = 78;

  ErrBadChar = 1;
  ErrString = 2;
  ErrBraceComment = 3;
  ErrParenComment = 4;
  ErrTokenLong = 5;
  ErrSourceFull = 6;
  ErrPoolFull = 7;
  ErrNameFull = 8;
  ErrPoolInvariant = 9;
  ErrIntegerRange = 10;

  ExpFactor = 100;
  ExpTypeName = 101;
  ExpResultType = 102;
  ExpConstLiteral = 103;
  ExpParamName = 104;
  ExpFieldName = 105;
  ExpVarName = 106;
  ExpRoutineName = 107;
  ExpProgramName = 108;
  ExpArrayBound = 109;
  ExpToOrDownto = 110;
  ExpNoChainedRelation = 111;

  PrProgram = 1;
  PrConstSection = 2;
  PrConstDecl = 3;
  PrConstLiteral = 4;
  PrTypeSection = 5;
  PrTypeDecl = 6;
  PrRecordType = 7;
  PrFieldGroup = 8;
  PrVarSection = 9;
  PrVarDecl = 10;
  PrTypeName = 11;
  PrArrayType = 12;
  PrArrayBound = 13;
  PrResultType = 14;
  PrRoutineProcedure = 15;
  PrRoutineFunction = 16;
  PrParamList = 17;
  PrParamGroup = 18;
  PrBlock = 19;
  PrStatement = 20;
  PrIdentStatement = 21;
  PrAssignment = 22;
  PrCall = 23;
  PrWrite = 24;
  PrWriteArg = 25;
  PrIf = 26;
  PrElseClause = 27;
  PrWhile = 28;
  PrFor = 29;
  PrRepeat = 30;
  PrExpression = 31;
  PrSimpleExpression = 32;
  PrTerm = 33;
  PrFactor = 34;
  PrIndexSuffix = 35;
  PrFieldSuffix = 36;

var
  InputFile: file;
  ReadChunk, NextChunk: string;
  SourceLen, ScanPos, ScanLine, ScanCol: integer;
  ChunkBase, ChunkLen, NextChunkLen: integer;
  StreamEof: boolean;

  PoolChars: array[1..PoolMax] of char;
  PoolOffset, PoolLength: array[1..NameMax] of integer;
  PoolUsed, PoolCount: integer;

  TokKind, TokValue, TokOffset, TokLength: integer;
  TokPos, TokLine, TokCol: integer;
  TokText, FoldedText, DumpText: string;
  HadError, ParseFailed: boolean;
  InputName, OutputAsmName, OutputComName, OutputExeName: string;
  DumpMode, QuietCompile: boolean;
  FirstKind, FirstLine, FirstCol, FirstExpected, FirstDetail: integer;
  CompilerIoError: integer;
  ParseIfDepth: integer;

  EmitParseTrace, TypeMode, TypeFailed, SyntaxClean: boolean;
  SymNameOffset: array[1..SymbolMax] of integer;
  SymKind, SymType, SymScope, SymRef: array[1..SymbolMax] of integer;
  SymLo, SymHi, SymOffset, SymSize: array[1..SymbolMax] of integer;
  SymCount, GlobalBytes, ActiveRoutine, LocalSlots: integer;

  PendingNameOffset: array[1..PendingNameMax] of integer;
  PendingNameLine, PendingNameCol: array[1..PendingNameMax] of integer;
  PendingNameCount: integer;

  HeaderNameOffset: array[1..HeaderParamMax] of integer;
  HeaderType, HeaderKind, HeaderRef: array[1..HeaderParamMax] of integer;
  HeaderLine, HeaderCol: array[1..HeaderParamMax] of integer;
  HeaderCount: integer;

  ArgType, ArgLvalue, ArgForm, ArgSym: array[1..HeaderParamMax] of integer;
  ArgLine, ArgCol: array[1..HeaderParamMax] of integer;
  ArgCount, ParsingFileBuiltin: integer;
  IoChecks: boolean; { the $I switch; TP 5.5 Reference Guide, IOResult }
  CallWantsValue: boolean;

  ParsedType, ParsedRef, ParsedStrCap: integer;
  ParsedLo, ParsedHi, ParsedBound: integer;
  ParsedIsArray: boolean;
  ParsedConstValue, ParsedConstLen: integer;
  CurrentRecord: integer;

  LastExprType, LastExprSym, LastExprRecord, LastExprStrCap: integer;
  LastExprForm: integer;
  LastExprLvalue: boolean;
  PendingIdentOffset, PendingIdentLength, PendingIdentLine, PendingIdentCol: integer;
  ParsedFieldOffset, ParsedFieldLength, ParsedFieldLine, ParsedFieldCol: integer;

  { B9.4 code generator. Assembly is buffered into one ShortString and written
    through the B8 byte-file surface to the fixed name TURBOOUT.S. Generation is
    a fourth check-as-you-parse pass: the semantic tables are rebuilt while
    the same parser emits stack-machine x86. }
  OutputFile: file;
  OutputChunk, GenText: string;
  OutputActual: integer;
  { GenData: 1 reserved, 2 bytes, 3 labels, 4 section, 5 expr-temp.
    GenFlags: 1 mode, 2 failed, 3 strings, 4 file-I/O, 5 want-address.
    Packing state into two arrays preserves the seed's TC_MAX_SYMBOLS budget. }
  GenData: array[1..5] of integer;
  GenFlags: array[1..5] of boolean;
  ArgTemp: array[1..HeaderParamMax] of integer;

  { Native state: pass (0=assembly,1=count,2=emit), PC, text end,
    BSS base/end, entry, label key, section, link base, extended flag,
    relocation count, header bytes. No program-sized byte buffer.
    Native encodings: Intel 80386 PRM 17.2 and instruction tables;
    layout: seed/rt/seed_dos.ld. See NATIVE.md for the independent oracle. }
  Native: array[1..12] of integer;
  NativeLabels: array[1..8192] of integer;
  NativeRoutines: array[1..SymbolMax] of integer;
  NativeRelocs: array[1..16384] of integer;
  NativeLineText: string;

procedure NativeLine(); forward;

procedure FailLex(Code, P, L, C, Detail: integer); forward;
procedure GenFail(L, C, Code: integer); forward;
function PoolLengthAt(NameOffset: integer): integer; forward;
procedure GenEmitGlobal(Index: integer); forward;

function SourceGet(P: integer): char;
var
  Index: integer;
  Valid: boolean;
begin
  SourceGet := Chr(0);
  Valid := P >= ChunkBase;
  if Valid then
    if P <= ChunkBase + ChunkLen - 1 then
    begin
      Index := P - ChunkBase + 1;
      SourceGet := ReadChunk[Index]
    end
    else
    begin
      Valid := P >= ChunkBase + ChunkLen;
      if Valid then
        if P <= ChunkBase + ChunkLen + NextChunkLen - 1 then
        begin
          Index := P - (ChunkBase + ChunkLen) + 1;
          SourceGet := NextChunk[Index]
        end
    end
end;

procedure CompilerIO(Code: integer);
begin
  if Code <> 0 then
  begin
    HadError := true; GenFlags[2] := true;
    if FirstKind = 0 then
    begin
      FirstKind := 5; FirstLine := 1; FirstCol := 1; FirstDetail := Code;
      if not QuietCompile then Writeln('TURBO-IO-ERROR code=', Code)
    end
  end
end;

procedure ReadStreamBlock(var Text: string; var Got: integer);
begin
  Text := '';
  Got := 0;
  {$I-}
  BlockRead(InputFile, Text[1], ChunkMax, Got);
  CompilerIoError := IOResult();
  {$I+}
  CompilerIO(CompilerIoError);
  Text[0] := Chr(Got);
  if SourceLen + Got > SourceMax then
  begin
    FailLex(ErrSourceFull, SourceMax + 1, 0, 0, SourceMax);
    Got := 0;
    Text := ''
  end
  else
    SourceLen := SourceLen + Got
end;

procedure AdvanceSourceWindow();
var
  Got: integer;
begin
  if ScanPos > ChunkBase + ChunkLen - 1 then
    if ChunkLen > 0 then
    begin
      ChunkBase := ChunkBase + ChunkLen;
      ReadChunk := NextChunk;
      ChunkLen := NextChunkLen;
      NextChunk := '';
      NextChunkLen := 0;
      if not StreamEof then
      begin
        ReadStreamBlock(NextChunk, Got);
        NextChunkLen := Got;
        if Got = 0 then StreamEof := true
      end
    end
end;

function AtEnd(): boolean;
begin
  AdvanceSourceWindow();
  AtEnd := ChunkLen = 0;
  if ChunkLen > 0 then AtEnd := SourceGet(ScanPos) = chr(26)
end;

function PeekChar(): char;
begin
  if ScanPos > SourceLen then
    PeekChar := Chr(0)
  else
    PeekChar := SourceGet(ScanPos)
end;

function PeekNextChar(): char;
begin
  if ScanPos >= SourceLen then
    PeekNextChar := Chr(0)
  else
    PeekNextChar := SourceGet(ScanPos + 1)
end;

function TakeChar(): char;
var
  C: char;
begin
  C := PeekChar();
  if not AtEnd() then
  begin
    ScanPos := ScanPos + 1;
    if C = Chr(10) then
    begin
      ScanLine := ScanLine + 1;
      ScanCol := 1
    end
    else
      ScanCol := ScanCol + 1
  end;
  TakeChar := C
end;

function LowerAscii(C: char): char;
begin
  LowerAscii := C;
  if C >= 'A' then
    if C <= 'Z' then
      LowerAscii := Chr(Ord(C) - Ord('A') + Ord('a'))
end;

function IsDigit(C: char): boolean;
begin
  IsDigit := false;
  if C >= '0' then
    if C <= '9' then
      IsDigit := true
end;

function IsIdentStart(C: char): boolean;
begin
  IsIdentStart := false;
  if C >= 'a' then
    if C <= 'z' then
      IsIdentStart := true;
  if C >= 'A' then
    if C <= 'Z' then
      IsIdentStart := true;
  if C = '_' then
    IsIdentStart := true
end;

function IsIdentChar(C: char): boolean;
begin
  IsIdentChar := IsIdentStart(C);
  if not IsIdentStart(C) then
    if IsDigit(C) then
      IsIdentChar := true
end;

function IsSpace(C: char): boolean;
var
  N: integer;
begin
  N := Ord(C);
  IsSpace := C = ' ';
  if N = 9 then
    IsSpace := true;
  if N = 10 then
    IsSpace := true;
  if N = 11 then
    IsSpace := true;
  if N = 12 then
    IsSpace := true;
  if N = 13 then
    IsSpace := true
end;

procedure FailLex(Code, P, L, C, Detail: integer);
begin
  if GenFlags[1] then
    GenFail(L, C, 300 + Code)
  else if not HadError then
  begin
    if FirstKind = 0 then
    begin FirstKind := 1; FirstLine := L; FirstCol := C; FirstDetail := Code end;
    HadError := true;
    if not QuietCompile then
    begin
    Write('TURBO-LEX-ERROR pos=', P, ' line=', L, ' col=', C);
    if Code = ErrBadChar then
      Writeln(' bad character code=', Detail)
    else if Code = ErrString then
      Writeln(' unterminated string')
    else if Code = ErrBraceComment then
      Writeln(' unterminated brace comment')
    else if Code = ErrParenComment then
      Writeln(' unterminated paren-star comment')
    else if Code = ErrTokenLong then
      Writeln(' token text exceeds 255 bytes')
    else if Code = ErrSourceFull then
      Writeln(' input exceeds SOURCEMAX=', Detail)
    else if Code = ErrPoolFull then
      Writeln(' char pool exceeds POOLMAX=', Detail)
    else if Code = ErrNameFull then
      Writeln(' name table exceeds NAMEMAX=', Detail)
    else if Code = ErrPoolInvariant then
      Writeln(' char pool invariant failed')
    else if Code = ErrIntegerRange then
      Writeln(' decimal integer exceeds 2147483647')
    else
      Writeln(' unknown lexer failure code=', Code)
    end
  end
end;

procedure ResetToken();
begin
  TokKind := TkNone;
  TokValue := 0;
  TokOffset := 0;
  TokLength := 0;
  TokText := '';
  FoldedText := ''
end;

procedure AppendTokenChar(C: char);
begin
  if Length(TokText) >= 255 then
    FailLex(ErrTokenLong, TokPos, TokLine, TokCol, 255)
  else
    TokText := TokText + C
end;

function PoolEntryEquals(Index: integer; var Text: string): boolean;
var
  I: integer;
  Same: boolean;
begin
  Same := PoolLength[Index] = Length(Text);
  I := 1;
  while I <= Length(Text) do
  begin
    if Same then
      if PoolChars[PoolOffset[Index] + I - 1] <> Text[I] then
        Same := false;
    I := I + 1
  end;
  PoolEntryEquals := Same
end;

function PoolAdd(var Text: string): integer;
var
  I, J, Found, Start, Need: integer;
  Done: boolean;
begin
  Found := 0;
  I := 1;
  Done := false;
  while not Done do
  begin
    if I > PoolCount then
      Done := true
    else
    begin
      if PoolEntryEquals(I, Text) then
      begin
        Found := PoolOffset[I];
        Done := true
      end
      else
        I := I + 1
    end
  end;

  if Found = 0 then
  begin
    Need := Length(Text);
    if PoolCount >= NameMax then
      FailLex(ErrNameFull, TokPos, TokLine, TokCol, NameMax)
    else
    begin
      if PoolUsed + Need > PoolMax then
        FailLex(ErrPoolFull, TokPos, TokLine, TokCol, PoolMax)
      else
      begin
        PoolCount := PoolCount + 1;
        Start := PoolUsed + 1;
        PoolOffset[PoolCount] := Start;
        PoolLength[PoolCount] := Need;
        J := 1;
        while J <= Need do
        begin
          PoolChars[Start + J - 1] := Text[J];
          J := J + 1
        end;
        PoolUsed := PoolUsed + Need;
        Found := Start
      end
    end
  end;
  PoolAdd := Found
end;

procedure PoolGet(Offset, TextLen: integer; var Text: string);
var
  I: integer;
  Valid: boolean;
begin
  Valid := Offset >= 1;
  if Valid then
    if TextLen < 0 then
      Valid := false;
  if Valid then
    if Offset + TextLen - 1 > PoolUsed then
      Valid := false;
  if not Valid then
    FailLex(ErrPoolInvariant, TokPos, TokLine, TokCol, 0)
  else
  begin
    Text := '';
    I := 0;
    while I < TextLen do
    begin
      Text := Text + PoolChars[Offset + I];
      I := I + 1
    end
  end
end;

procedure GenFail(L, C, Code: integer);
begin
  if not GenFlags[2] then
  begin
    if FirstKind = 0 then
    begin FirstKind := 4; FirstLine := L; FirstCol := C; FirstDetail := Code end;
    GenFlags[2] := true;
    ParseFailed := true;
    TypeFailed := true;
    if not QuietCompile then
    begin
    Write('TURBO-GEN-ERROR line=', L, ' col=', C, ' ');
    if Code = 1 then Writeln('output short write')
    else if Code = 2 then Writeln('invalid symbol storage')
    else if Code = 3 then Writeln('invalid expression storage')
    else if Code = 4 then Writeln('call stack invariant')
    else Writeln('internal invariant code=', Code)
    end
  end
end;

procedure GenFlush();
var
  Need: integer;
begin
  Need := Length(OutputChunk);
  if Need > 0 then
  begin
    OutputActual := 0;
    {$I-}
    BlockWrite(OutputFile, OutputChunk[1], Need, OutputActual);
    CompilerIoError := IOResult();
    {$I+}
    CompilerIO(CompilerIoError);
    if OutputActual <> Need then
      GenFail(TokLine, TokCol, 1)
    else
      GenData[2] := GenData[2] + OutputActual;
    OutputChunk := ''
  end
end;

procedure NativeByte(Value: integer);
var B: integer;
begin
  B := Value mod 256;
  if B < 0 then B := B + 256;
  if Native[1] = 2 then
  begin
    if Length(OutputChunk) = 255 then GenFlush();
    OutputChunk := OutputChunk + Chr(B)
  end;
  Native[2] := Native[2] + 1
end;

procedure NativeWord(Value: integer);
var I, B: integer;
begin
  I := 0;
  while I < 4 do
  begin
    B := Value mod 256;
    if B < 0 then B := B + 256;
    NativeByte(B);
    { Arithmetic division alone rounds negatives toward zero. }
    Value := (Value - B) div 256;
    I := I + 1
  end
end;

{ Record all absolute dword sites while counting. The second pass streams
  the header/table before emitting the same module. No linker in the guest.
  Format authority: kernel/include/initechmz_extended.h; placement and heap
  authority: documented loader_high_plan rules in kernel/loader_validation.c. }
procedure NativeAbsolute(Value: integer);
begin
  if Native[1] = 1 then
  begin
    if Native[11] >= 16384 then GenFail(TokLine, TokCol, 413)
    else
    begin
      Native[11] := Native[11] + 1;
      NativeRelocs[Native[11]] := Native[2] { NATIVE_RELOCATION }
    end
  end;
  NativeWord(Native[9] + Value)
end;


function NativeNumber(var Text: string; Start: integer): integer;
var I, N, Sign, Base, D: integer;
begin
  I := Start; N := 0; Sign := 1; Base := 10;
  if I <= Length(Text) then
    if Text[I] = '-' then begin Sign := -1; I := I + 1 end;
  if I + 1 <= Length(Text) then
    if Text[I] = '0' then
      if Text[I+1] = 'x' then begin Base := 16; I := I + 2 end;
  while I <= Length(Text) do
  begin
    D := Ord(Text[I]) - 48;
    if Text[I] >= 'A' then D := Ord(Text[I]) - 55;
    if Text[I] >= 'a' then D := Ord(Text[I]) - 87;
    if (D < 0) or (D >= Base) then GenFail(TokLine, TokCol, 401)
    else N := N * Base + D;
    I := I + 1
  end;
  NativeNumber := N * Sign
end;

procedure NativeHex(var Text: string);
var I, N, D: integer;
begin
  I := 1;
  while I < Length(Text) do
  begin
    D := Ord(Text[I]) - 48;
    if Text[I] >= 'A' then D := Ord(Text[I]) - 55;
    N := D * 16;
    D := Ord(Text[I+1]) - 48;
    if Text[I+1] >= 'A' then D := Ord(Text[I+1]) - 55;
    NativeByte(N + D);
    I := I + 2
  end
end;

function NativeRegister(var Text: string): integer;
begin
  NativeRegister := -1;
  if (Text = 'eax') or (Text = 'al') then NativeRegister := 0
  else if (Text = 'ecx') or (Text = 'cl') then NativeRegister := 1
  else if (Text = 'edx') or (Text = 'dl') then NativeRegister := 2
  else if Text = 'ebx' then NativeRegister := 3
  else if Text = 'esp' then NativeRegister := 4
  else if Text = 'ebp' then NativeRegister := 5
  else if Text = 'esi' then NativeRegister := 6
  else if Text = 'edi' then NativeRegister := 7
end;

{ Only the fixed backend vocabulary is accepted. No external assembly input. }
procedure NativeMemory(var Text: string; Reg: integer);
var Base, Disp, I, Mode: integer;
    Part: string;
begin
  Part := ''; I := 2;
  while I <= 4 do begin Part := Part + Text[I]; I := I + 1 end;
  Base := NativeRegister(Part);
  if Base < 0 then GenFail(TokLine, TokCol, 402);
  Part := ''; I := 5;
  while I < Length(Text) do begin Part := Part + Text[I]; I := I + 1 end;
  Disp := 0;
  if Length(Part) > 0 then
  begin
    I := 1;
    if Part[1] = '+' then I := 2;
    Disp := NativeNumber(Part, I)
  end;
  Mode := 0;
  if (Disp <> 0) or (Base = 5) then
  begin
    Mode := 1;
    if (Disp < -128) or (Disp > 127) then Mode := 2
  end;
  NativeByte(Mode * 64 + Reg * 8 + Base); { NATIVE_MODRM }
  if Base = 4 then NativeByte(36);
  if Mode = 1 then NativeByte(Disp)
  else if Mode = 2 then NativeWord(Disp)
end;

{ Generated label numbers are unique across families. Each needs at most two
  addresses (e.g. else/end). The first 32 slots are the string helper labels. }
function NativeKey(var Text: string): integer;
var I, N: integer;
    Part: string;
begin
  N := 0;
  if Length(Text) > 2 then
    if Text[2] = 'L' then
    begin
      I := Length(Text);
      while IsDigit(Text[I]) do I := I - 1;
      N := NativeNumber(Text, I+1) * 2 + 33;
      Part := ''; I := 1;
      while I <= Length(Text) do
      begin
        if not IsDigit(Text[I]) then Part := Part + Text[I];
        I := I + 1
      end;
      if (Part = '.Lstr_after_') or (Part = '.Lendif_') or
         (Part = '.Lwhile_end_') or (Part = '.Lfor_end_') or
         (Part = '.Lbdone_') or (Part = '.Ldiv_end_') then N := N + 1
    end;
  if Text = '__str_assign' then N := 1
  else if Text = '__str_concat' then N := 2
  else if Text = '__str_cmp' then N := 3
  else if Text = '__str_write' then N := 4
  else if Text = '.len_ok' then N := 5
  else if Text = '.n_ok' then N := 6
  else if Text = '.have_min' then N := 7
  else if Text = '.cmp_loop' then N := 8
  else if Text = '.prefix_equal' then N := 9
  else if Text = '.a_less' then N := 10
  else if Text = '.a_greater' then N := 11
  else if Text = '.len_less' then N := 12
  else if Text = '.len_greater' then N := 13
  else if Text = '.w_loop' then N := 14
  else if Text = '.w_done' then N := 15;
  if (N < 1) or (N > 8192) then GenFail(TokLine, TokCol, 403);
  NativeKey := N
end;

function NativeSymbol(var Text: string; First: integer): integer;
var I, J, Found: integer;
    Part, Name: string;
begin
  Part := ''; I := First;
  while I <= Length(Text) do begin Part := Part + Text[I]; I := I + 1 end;
  Found := 0; J := 1;
  while J <= SymCount do
  begin
    if SymScope[J] = 0 then
    begin
      PoolGet(SymNameOffset[J], PoolLengthAt(SymNameOffset[J]), Name);
      if Name = Part then Found := J
    end;
    J := J + 1
  end;
  if Found = 0 then GenFail(TokLine, TokCol, 404);
  NativeSymbol := Found
end;

function NativeAddress(var Text: string): integer;
var N: integer;
begin
  N := 0;
  if Native[1] = 2 then
  begin
    if Text = 'serial_putc' then N := 49
    else if Text = 'serial_puts' then N := 73
    else if Text = 'serial_put_int' then N := 92
    else if Text = 'rtl_file_assign' then N := 205
    else if Text = 'rtl_file_reset' then N := 255
    else if Text = 'rtl_file_rewrite' then N := 319
    else if Text = 'rtl_file_blockread' then N := 385
    else if Text = 'rtl_file_blockwrite' then N := 444
    else if Text = 'rtl_file_close' then N := 503
    else if Text = 'rtl_file_erase' then N := 551
    else if Text = 'rtl_file_rename' then N := 581
    else if Text = 'rtl_ioresult' then N := 685
    else if Text = 'rtl_io_check' then N := 701
    else if Text = 'rtl_readkey' then N := 750
    else if Text = 'rtl_file_attributes' then N := 767
    else if Text = 'rtl_paramcount' then N := 823
    else if Text = 'rtl_paramstr' then N := 873
    else if Text = 'rtl_screen_page' then N := 1028
    else if Text = 'rtl_execute' then N := 1128
    else if Text = 'rtl_halt' then N := 1235
    else if Text = 'rtl_dos_memory' then N := 1255
    else if Text = 'str_bool_true' then N := Native[3] + 20
    else if Text = 'str_bool_false' then N := Native[3] + 25
    else if Text[1] = 'v' then
      N := Native[4] + SymOffset[NativeSymbol(Text, 3)]
    else if Text[1] = 'p' then
      N := NativeRoutines[NativeSymbol(Text, 4)]
    else
    begin
      N := NativeLabels[NativeKey(Text)];
      if N = 0 then GenFail(TokLine, TokCol, 405)
    end
  end;
  NativeAddress := N
end;

procedure NativeDefine(var Text: string);
var N: integer;
begin
  if Text = 'pas_main' then Native[6] := Native[2]
  else if Text[1] = 'p' then
  begin
    N := NativeSymbol(Text, 4);
    if Native[1] = 1 then NativeRoutines[N] := Native[2]
    else if NativeRoutines[N] <> Native[2] then GenFail(TokLine, TokCol, 406)
  end
  else if Text[1] <> 'v' then
  begin
    N := NativeKey(Text);
    if Native[1] = 1 then NativeLabels[N] := Native[2] { NATIVE_FIXUP }
    else if NativeLabels[N] <> Native[2] then GenFail(TokLine, TokCol, 407)
  end
end;

procedure NativeLine();
var Op, A, B, Part: string;
    I, R, S, N, Code, Small, Width: integer;
begin
  I := 1; Op := ''; A := ''; B := '';
  { Leading spaces, mnemonic or label, then operands with spaces removed. }
  R := 0;
  while I <= Length(NativeLineText) do
  begin
    if R = 0 then
    begin
      if NativeLineText[I] <> ' ' then
      begin Op := Op + NativeLineText[I]; R := 1 end
    end
    else if R = 1 then
    begin
      if NativeLineText[I] = ' ' then R := 2
      else Op := Op + NativeLineText[I]
    end
    else if R = 2 then
    begin
      if NativeLineText[I] = ',' then R := 3
      else if NativeLineText[I] <> ' ' then A := A + NativeLineText[I]
    end
    else if NativeLineText[I] <> ' ' then B := B + NativeLineText[I];
    I := I + 1
  end;
  if Length(Op) > 0 then
  begin
    if Op[Length(Op)] = ':' then
    begin
      Op[0] := Chr(Length(Op)-1);
      if Native[8] = 3 then NativeDefine(Op);
      Op := ''
    end;
    if Length(Op) > 0 then
    if Op[1] = 'j' then
    begin
      Part := ''; I := 5;
      while I <= Length(A) do begin Part := Part + A[I]; I := I + 1 end;
      A := Part
    end;
    if Op = 'section' then
    begin
      if A = '.text' then Native[8] := 3
      else if A = '.bss' then Native[8] := 2
      else if A = '.rodata' then Native[8] := 1
      else GenFail(TokLine, TokCol, 408)
    end
    else if (Op = '') or (Op = 'align') or (Op = 'bits') or
            (Op = 'global') or (Op = 'extern') then begin end
    else if Op[1] = ';' then begin end
    else if Native[8] = 3 then
    begin
      R := NativeRegister(A); S := NativeRegister(B);
      Width := 4;
      if Length(A) = 2 then Width := 1;
      if Op = 'db' then
      begin
        Part := A;
        NativeByte(NativeNumber(Part, 1));
        Part := ''; I := 1;
        while I <= Length(B) + 1 do
        begin
          if I > Length(B) then
          begin NativeByte(NativeNumber(Part, 1)); Part := '' end
          else if B[I] = ',' then
          begin NativeByte(NativeNumber(Part, 1)); Part := '' end
          else Part := Part + B[I];
          I := I + 1
        end
      end
      else if (Op = 'jmp') or (Op = 'call') or (Op[1] = 'j') then
      begin
        N := NativeAddress(A);
        if Op = 'jmp' then NativeByte(233)
        else if Op = 'call' then NativeByte(232)
        else
        begin
          Code := -1;
          if Op = 'jz' then Code := 132 else if Op = 'jg' then Code := 143
          else if Op = 'jl' then Code := 140 else if Op = 'ja' then Code := 135
          else if Op = 'jb' then Code := 130 else if Op = 'jbe' then Code := 134
          else if Op = 'jae' then Code := 131;
          if Code < 0 then GenFail(TokLine, TokCol, 409);
          NativeByte(15); NativeByte(Code)
        end;
        NativeWord(N - (Native[2] + 4)) { NATIVE_REL32 }
      end
      else if (Op = 'push') or (Op = 'pop') or (Op = 'inc') or (Op = 'dec') then
      begin
        if Op = 'push' then Code := 80 else if Op = 'pop' then Code := 88
        else if Op = 'inc' then Code := 64 else Code := 72;
        if R >= 0 then NativeByte(Code + R)
        else if (Op = 'push') and (A = 'dword1') then begin NativeByte(106); NativeByte(1) end
        else GenFail(TokLine, TokCol, 410)
      end
      else if Op = 'leave' then NativeByte(201)
      else if Op = 'ret' then NativeByte(195)
      else if Op = 'cdq' then NativeByte(153)
      else if (Op = 'rep') and (A = 'movsb') then begin NativeByte(243); NativeByte(164) end
      else if Op = 'neg' then begin NativeByte(247); NativeByte(216) end
      else if Op = 'idiv' then begin NativeByte(247); NativeByte(249) end
      else if Op = 'lea' then
      begin
        NativeByte(141);
        if B = '[edi+eax+1]' then begin NativeByte(84); NativeByte(7); NativeByte(1) end
        else NativeMemory(B, R)
      end
      else if Op = 'movzx' then
      begin
        NativeByte(15); NativeByte(182);
        if B = 'al' then NativeByte(192 + R*8)
        else
        begin
          Part := ''; I := 5;
          while I <= Length(B) do begin Part := Part + B[I]; I := I + 1 end;
          NativeMemory(Part, R)
        end
      end
      else if (Op = 'sete') or (Op = 'setne') or (Op = 'setl') or
              (Op = 'setle') or (Op = 'setg') or (Op = 'setge') then
      begin
        Code := 148;
        if Op = 'setne' then Code := 149 else if Op = 'setl' then Code := 156
        else if Op = 'setle' then Code := 158 else if Op = 'setg' then Code := 159
        else if Op = 'setge' then Code := 157;
        NativeByte(15); NativeByte(Code); NativeByte(192)
      end
      else if Op = 'mov' then
      begin
        if (R >= 0) and (S >= 0) then
        begin NativeByte(137); NativeByte(192 + S*8 + R) end
        else if R >= 0 then
        begin
          if B[1] = '[' then
          begin
            Code := 139; if Width = 1 then Code := 138;
            NativeByte(Code); NativeMemory(B, R)
          end
          else
          begin
            Code := 184; if Width = 1 then Code := 176;
            NativeByte(Code + R);
            if IsDigit(B[1]) or (B[1] = '-') then N := NativeNumber(B, 1)
            else N := NativeAddress(B);
            if Width = 1 then NativeByte(N)
            else if IsDigit(B[1]) or (B[1] = '-') then NativeWord(N)
            else NativeAbsolute(N)
          end
        end
        else if S >= 0 then
        begin
          Code := 137; if Length(B) = 2 then Code := 136;
          Part := A;
          if A[1] <> '[' then
          begin
            Part := ''; I := 5;
            while I <= Length(A) do begin Part := Part + A[I]; I := I + 1 end
          end;
          NativeByte(Code); NativeMemory(Part, S)
        end
        else
        begin
          I := 5; Code := 198;
          if A[1] = 'd' then begin I := 6; Code := 199 end;
          Part := '';
          while I <= Length(A) do begin Part := Part + A[I]; I := I + 1 end;
          NativeByte(Code); NativeMemory(Part, 0);
          N := NativeNumber(B, 1);
          if Code = 198 then NativeByte(N) else NativeWord(N)
        end
      end
      else if (Op = 'add') or (Op = 'sub') or (Op = 'cmp') or (Op = 'and') or
              (Op = 'or') or (Op = 'xor') or (Op = 'test') or (Op = 'imul') then
      begin
        Code := 0;
        if Op = 'or' then Code := 1 else if Op = 'and' then Code := 4
        else if Op = 'sub' then Code := 5 else if Op = 'xor' then Code := 6
        else if Op = 'cmp' then Code := 7;
        if S >= 0 then
        begin
          if Op = 'test' then NativeByte(133)
          else if Op = 'imul' then begin NativeByte(15); NativeByte(175) end
          else if Width = 1 then NativeByte(Code*8) else NativeByte(Code*8 + 1);
          if Op = 'imul' then NativeByte(192 + R*8 + S)
          else NativeByte(192 + S*8 + R)
        end
        else if B[1] = '[' then
        begin NativeByte(Code*8+3); NativeMemory(B, R) end
        else
        begin
          N := NativeNumber(B, 1);
          Small := 0;
          if (N >= -128) and (N <= 127) then Small := 1;
          if Op = 'imul' then
          begin
            NativeByte(105 + Small*2); NativeByte(192 + R*9)
          end
          else
          begin
            if (R = 0) and (Small = 0) then NativeByte(Code*8+5)
            else
            begin
              NativeByte(129 + Small*2);
              if R >= 0 then NativeByte(192 + Code*8 + R)
              else
              begin
                Part := ''; I := 6;
                while I <= Length(A) do begin Part := Part + A[I]; I := I + 1 end;
                NativeMemory(Part, Code)
              end
            end
          end;
          if Small = 1 then NativeByte(N) else NativeWord(N)
        end
      end
      else GenFail(TokLine, TokCol, 411)
    end
  end;
  NativeLineText := ''
end;


{ BEGIN NASM RUNTIME TABLE -- tests/runtime-table.c; do not edit. }
procedure NativeRuntime();
begin
  GenText := 'FCBF'; NativeHex(GenText)
  ;
  NativeAbsolute(Native[4]);
  GenText := 'B9'; NativeHex(GenText)
  ;
  NativeAbsolute(Native[5]);
  GenText := '29F931C089CAC1E902F3AB89D183E103F3AABCFCFF0700E8'; NativeHex(GenText)
  ;
  NativeWord(Native[6] - 39);
  GenText := 'B8004C0000CD21F4EBFD608D54241CB901000000BB01000000B800400000CD2161C35689C68A0684C07408E8DAFFFFFF'; NativeHex(GenText)
  ;
  GenText := '46EBF25EC35631F685C07907BE01000000F7D8B900000000BB0A00000031D2F7F380C230524185C075F385F67407B02D'; NativeHex(GenText)
  ;
  GenText := 'E8A5FFFFFF58E89FFFFFFF4975F75EC3909090909090909090E800000000580556020000C3E8EFFFFFFF833800C38B1E'; NativeHex(GenText)
  ;
  GenText := '83FB05720FB8003E0000CD217207C70600000000F8C35589E55356578B7508E8DAFFFFFF0F82BD0100008B5D088B750C'; NativeHex(GenText)
  ;
  GenText := '0FB60E468D7B04FCF3A4C60700C70300000000E9A90100005589E5535657E8A2FFFFFF0F8598010000837D0C010F857D'; NativeHex(GenText)
  ;
  GenText := '0100008B7508E893FFFFFF0F82760100008D5604B8003D0000CD210F82660100008906E9690100005589E5535657E862'; NativeHex(GenText)
  ;
  GenText := 'FFFFFF0F8558010000837D0C010F853D0100008B7508E853FFFFFF0F82360100008D560431C9B8003C0000CD210F8224'; NativeHex(GenText)
  ;
  GenText := '0100008906E9270100005589E5535657E820FFFFFF0F85160100008B4D1085C90F88FA0000008B75088B1E8B550CB800'; NativeHex(GenText)
  ;
  GenText := '3F0000CD210F82EC0000008B7D148907E9EC0000005589E5535657E8E5FEFFFF0F85DB0000008B4D1085C90F88BF0000'; NativeHex(GenText)
  ;
  GenText := '008B7D088B1F8B550CB800400000CD210F82B10000008B7D148907E9B10000005589E5535657E8AAFEFFFF0F85A00000'; NativeHex(GenText)
  ;
  GenText := '008B7508833E057210E8A0FEFFFF0F8283000000E988000000B867000000EB775589E5535657E87AFEFFFF75748B7508'; NativeHex(GenText)
  ;
  GenText := '8D5604B800410000CD21725BEB635589E5535657E85CFEFFFF755681EC040100008B750C0FB60E4689E7FCF3A4C60700'; NativeHex(GenText)
  ;
  GenText := '8B75088D560489E7B800560000CD21721789E68B7D0883C704B900010000F3A481C404010000EB1981C404010000EB07'; NativeHex(GenText)
  ;
  GenText := 'B801000000EB000FB7D0E8FAFDFFFF89105F5E5B5DC3E8EEFDFFFF89C28B02C70200000000C350E8DDFDFFFF8B0085C0'; NativeHex(GenText)
  ;
  GenText := '750258C350B8'; NativeHex(GenText)
  ;
  NativeAbsolute(Native[3]);
  GenText := 'E873FDFFFF58E880FDFFFFB00AE84EFDFFFFB8014C0000CD21FAF4EBFD53B800070000CD210FB6C05BC300000000538B'; NativeHex(GenText)
  ;
  GenText := '54240883C204B800430000CD2172050FB7C15BC30FB7C0F7D85BC3B800620000CD210FB7F3C1E6040FB68E8000000081'; NativeHex(GenText)
  ;
  GenText := 'C681000000C3535657E8DDFFFFFF31C031FF31D285C9741AAC3C2074103C09740C85D2750A47BA01000000EB0231D249'; NativeHex(GenText)
  ;
  GenText := 'EBE289F85F5E5BC35589E55356578B7D0CC60700E8A2FFFFFF8B5D0883FB007C7B743831D285C97473803E207405803E'; NativeHex(GenText)
  ;
  GenText := '0975044649EBEE4285C974608A063C2074E33C0974DF39DA750DFE070FB6078A1E881C078B5D084649EBDD81EE810000'; NativeHex(GenText)
  ;
  GenText := '000FB7762CC1E604B9FD0F000066833E00740546E2F7EB2466837E0201751D83C6048A0684C074140FB61781FAFF0000'; NativeHex(GenText)
  ;
  GenText := '00740942880417881746EBE689F85F5E5B5DC35589E553565781ECA40F0000837D080074128B750C89E7B9A20F00008A'; NativeHex(GenText)
  ;
  GenText := '06AA83C604E2F88B450825FF0000000D00FB000089E2CD217222837D0800751189E68B7D0CB9A20F00000FB606AB46E2'; NativeHex(GenText)
  ;
  GenText := 'F981C4A40F0000E94BFEFFFF81C4A40F0000E936FEFFFF5589E553565781EC180100008B75080FB60E4689E7F3A4C607'; NativeHex(GenText)
  ;
  GenText := '0089E28D9C2404010000C703000000008D84241401000089430466C700000DC7430800000000C7430C00000000B8004B'; NativeHex(GenText)
  ;
  GenText := '0000CD21720CB8004D0000CD210FB7C0EB050FB7C0F7D881C4180100005F5E5B5DC38B44240425FF0000000D004C0000'; NativeHex(GenText)
  ;
  GenText := 'CD21FAF4EBFD53BBFFFF0000B800480000CD2173080FB7C3C1E0045BC30FB7D8B800490000CD21B8F0FF0F005BC3'; NativeHex(GenText)
  ;
  while Native[2] < 1296 do NativeByte(144)
end;
{ END NASM RUNTIME TABLE }
{ BEGIN NASM RUNTIME TABLE -- tests/runtime-table.c; do not edit. }
procedure NativeExtendedRuntime();
begin
  GenText := 'FCBF'; NativeHex(GenText)
  ;
  NativeAbsolute(Native[4]);
  GenText := 'B9'; NativeHex(GenText)
  ;
  NativeAbsolute(Native[5]);
  GenText := '29F931C089CAC1E902F3AB89D183E103F3AA9090909090E8'; NativeHex(GenText)
  ;
  NativeWord(Native[6] - 39);
  GenText := 'B8004C0000CD21F4EBFD608D54241CB901000000BB01000000B800400000CD2161C35689C68A0684C07408E8DAFFFFFF'; NativeHex(GenText)
  ;
  GenText := '46EBF25EC35631F685C07907BE01000000F7D8B900000000BB0A00000031D2F7F380C230524185C075F385F67407B02D'; NativeHex(GenText)
  ;
  GenText := 'E8A5FFFFFF58E89FFFFFFF4975F75EC3909090909090909090E800000000580556020000C3E8EFFFFFFF833800C38B1E'; NativeHex(GenText)
  ;
  GenText := '83FB05720FB8003E0000CD217207C70600000000F8C35589E55356578B7508E8DAFFFFFF0F82BD0100008B5D088B750C'; NativeHex(GenText)
  ;
  GenText := '0FB60E468D7B04FCF3A4C60700C70300000000E9A90100005589E5535657E8A2FFFFFF0F8598010000837D0C010F857D'; NativeHex(GenText)
  ;
  GenText := '0100008B7508E893FFFFFF0F82760100008D5604B8003D0000CD210F82660100008906E9690100005589E5535657E862'; NativeHex(GenText)
  ;
  GenText := 'FFFFFF0F8558010000837D0C010F853D0100008B7508E853FFFFFF0F82360100008D560431C9B8003C0000CD210F8224'; NativeHex(GenText)
  ;
  GenText := '0100008906E9270100005589E5535657E820FFFFFF0F85160100008B4D1085C90F88FA0000008B75088B1E8B550CB800'; NativeHex(GenText)
  ;
  GenText := '3F0000CD210F82EC0000008B7D148907E9EC0000005589E5535657E8E5FEFFFF0F85DB0000008B4D1085C90F88BF0000'; NativeHex(GenText)
  ;
  GenText := '008B7D088B1F8B550CB800400000CD210F82B10000008B7D148907E9B10000005589E5535657E8AAFEFFFF0F85A00000'; NativeHex(GenText)
  ;
  GenText := '008B7508833E057210E8A0FEFFFF0F8283000000E988000000B867000000EB775589E5535657E87AFEFFFF75748B7508'; NativeHex(GenText)
  ;
  GenText := '8D5604B800410000CD21725BEB635589E5535657E85CFEFFFF755681EC040100008B750C0FB60E4689E7FCF3A4C60700'; NativeHex(GenText)
  ;
  GenText := '8B75088D560489E7B800560000CD21721789E68B7D0883C704B900010000F3A481C404010000EB1981C404010000EB07'; NativeHex(GenText)
  ;
  GenText := 'B801000000EB000FB7D0E8FAFDFFFF89105F5E5B5DC3E8EEFDFFFF89C28B02C70200000000C350E8DDFDFFFF8B0085C0'; NativeHex(GenText)
  ;
  GenText := '750258C350B8'; NativeHex(GenText)
  ;
  NativeAbsolute(Native[3]);
  GenText := 'E873FDFFFF58E880FDFFFFB00AE84EFDFFFFB8014C0000CD21FAF4EBFD53B800070000CD210FB6C05BC300000000538B'; NativeHex(GenText)
  ;
  GenText := '54240883C204B800430000CD2172050FB7C15BC30FB7C0F7D85BC3B800620000CD210FB7F3C1E6040FB68E8000000081'; NativeHex(GenText)
  ;
  GenText := 'C681000000C3535657E8DDFFFFFF31C031FF31D285C9741AAC3C2074103C09740C85D2750A47BA01000000EB0231D249'; NativeHex(GenText)
  ;
  GenText := 'EBE289F85F5E5BC35589E55356578B7D0CC60700E8A2FFFFFF8B5D0883FB007C7B743831D285C97473803E207405803E'; NativeHex(GenText)
  ;
  GenText := '0975044649EBEE4285C974608A063C2074E33C0974DF39DA750DFE070FB6078A1E881C078B5D084649EBDD81EE810000'; NativeHex(GenText)
  ;
  GenText := '000FB7762CC1E604B9FD0F000066833E00740546E2F7EB2466837E0201751D83C6048A0684C074140FB61781FAFF0000'; NativeHex(GenText)
  ;
  GenText := '00740942880417881746EBE689F85F5E5B5DC35589E553565781ECA40F0000837D080074128B750C89E7B9A20F00008A'; NativeHex(GenText)
  ;
  GenText := '06AA83C604E2F88B450825FF0000000D00FB000089E2CD217222837D0800751189E68B7D0CB9A20F00000FB606AB46E2'; NativeHex(GenText)
  ;
  GenText := 'F981C4A40F0000E94BFEFFFF81C4A40F0000E936FEFFFF5589E553565781EC180100008B75080FB60E4689E7F3A4C607'; NativeHex(GenText)
  ;
  GenText := '0089E28D9C2404010000C703000000008D84241401000089430466C700000DC7430800000000C7430C00000000B8004B'; NativeHex(GenText)
  ;
  GenText := '0000CD21720CB8004D0000CD210FB7C0EB050FB7C0F7D881C4180100005F5E5B5DC38B44240425FF0000000D004C0000'; NativeHex(GenText)
  ;
  GenText := 'CD21FAF4EBFD53BBFFFF0000B800480000CD2173080FB7C3C1E0045BC30FB7D8B800490000CD21B8F0FF0F005BC3'; NativeHex(GenText)
  ;
  while Native[2] < 1296 do NativeByte(144)
end;
{ END NASM RUNTIME TABLE }



procedure GenChar(C: char);
var
  N: integer;
begin
  if Native[1] > 0 then
  begin
    if C = Chr(10) then NativeLine()
    else if Length(NativeLineText) >= 255 then GenFail(TokLine, TokCol, 412)
    else NativeLineText := NativeLineText + C
  end
  else
  if not GenFlags[2] then
  begin
    if Length(OutputChunk) = 255 then GenFlush();
    if not GenFlags[2] then
    begin
      N := Length(OutputChunk);
      OutputChunk[N + 1] := C;
      OutputChunk[0] := Chr(N + 1)
    end
  end
end;

procedure GenPut(var Text: string);
var
  I: integer;
begin
  I := 1;
  while I <= Length(Text) do
  begin
    GenChar(Text[I]);
    I := I + 1
  end
end;

procedure GenInt(Value: integer);
var
  Digits: array[1..12] of char;
  Count, N, D, I: integer;
begin
  if Value = 0 then
    GenChar('0')
  else
  begin
    N := Value;
    if N < 0 then
    begin
      GenChar('-');
      N := -N
    end;
    Count := 0;
    while N > 0 do
    begin
      D := N mod 10;
      Count := Count + 1;
      Digits[Count] := Chr(Ord('0') + D);
      N := N div 10
    end;
    I := Count;
    while I > 0 do
    begin
      GenChar(Digits[I]);
      I := I - 1
    end
  end
end;

procedure GenLine(var Text: string);
begin
  GenPut(Text);
  GenChar(Chr(10))
end;

function GenNewLabel(): integer;
begin
  GenNewLabel := GenData[3];
  GenData[3] := GenData[3] + 1
end;

procedure GenSetSection(SectionId: integer);
begin
  if GenData[4] <> SectionId then
  begin
    if SectionId = 1 then begin GenText := 'section .rodata'; GenLine(GenText) end
    else if SectionId = 2 then
    begin
      begin GenText := 'section .bss'; GenLine(GenText) end;
      begin GenText := 'align 4'; GenLine(GenText) end
    end
    else if SectionId = 3 then begin GenText := 'section .text'; GenLine(GenText) end;
    GenData[4] := SectionId
  end
end;

function KeywordKind(var Text: string): integer;
begin
  KeywordKind := TkIdent;
  if Text = 'program' then KeywordKind := TkProgram
  else if Text = 'var' then KeywordKind := TkVar
  else if Text = 'begin' then KeywordKind := TkBegin
  else if Text = 'end' then KeywordKind := TkEnd
  else if Text = 'integer' then KeywordKind := TkIntegerType
  else if Text = 'div' then KeywordKind := TkDiv
  else if Text = 'mod' then KeywordKind := TkMod
  else if Text = 'write' then KeywordKind := TkWrite
  else if Text = 'writeln' then KeywordKind := TkWriteln
  else if Text = 'boolean' then KeywordKind := TkBoolean
  else if Text = 'and' then KeywordKind := TkAnd
  else if Text = 'or' then KeywordKind := TkOr
  else if Text = 'not' then KeywordKind := TkNot
  else if Text = 'true' then KeywordKind := TkTrue
  else if Text = 'false' then KeywordKind := TkFalse
  else if Text = 'if' then KeywordKind := TkIf
  else if Text = 'then' then KeywordKind := TkThen
  else if Text = 'else' then KeywordKind := TkElse
  else if Text = 'while' then KeywordKind := TkWhile
  else if Text = 'do' then KeywordKind := TkDo
  else if Text = 'for' then KeywordKind := TkFor
  else if Text = 'to' then KeywordKind := TkTo
  else if Text = 'downto' then KeywordKind := TkDownto
  else if Text = 'repeat' then KeywordKind := TkRepeat
  else if Text = 'until' then KeywordKind := TkUntil
  else if Text = 'case' then KeywordKind := TkCase
  else if Text = 'const' then KeywordKind := TkConst
  else if Text = 'char' then KeywordKind := TkChar
  else if Text = 'ord' then KeywordKind := TkOrd
  else if Text = 'chr' then KeywordKind := TkChr
  else if Text = 'procedure' then KeywordKind := TkProcedure
  else if Text = 'function' then KeywordKind := TkFunction
  else if Text = 'forward' then KeywordKind := TkForward
  else if Text = 'array' then KeywordKind := TkArray
  else if Text = 'of' then KeywordKind := TkOf
  else if Text = 'type' then KeywordKind := TkType
  else if Text = 'record' then KeywordKind := TkRecord
  else if Text = 'string' then KeywordKind := TkStringType
  else if Text = 'length' then KeywordKind := TkLength
  else if Text = 'file' then KeywordKind := TkFile
end;

procedure FoldTokenForKeyword();
var
  I: integer;
begin
  FoldedText := '';
  I := 1;
  while I <= Length(TokText) do
  begin
    FoldedText := FoldedText + LowerAscii(TokText[I]);
    I := I + 1
  end
end;

procedure CanonicalizeIdentifier();
var
  I: integer;
begin
  I := 1;
  while I <= Length(TokText) do
  begin
    TokText[I] := LowerAscii(TokText[I]); { TURBO_LEX_MUT_CASEFOLD }
    I := I + 1
  end
end;

procedure SkipTrivia();
var
  C: char;
  Again, Done: boolean;
  StartPos, StartLine, StartCol, Inside: integer;
  Directive: string;
begin
  Again := true;
  while Again do
  begin
    Again := false;
    C := PeekChar();
    if IsSpace(C) then
    begin
      C := TakeChar();
      Again := true
    end
    else
    begin
      if C = '{' then
      begin
        StartPos := ScanPos;
        StartLine := ScanLine;
        StartCol := ScanCol;
        C := TakeChar();
        Done := false;
        Inside := 0;
        Directive := '';
        while not Done do
        begin
          if AtEnd() then
          begin
            FailLex(ErrBraceComment, StartPos, StartLine, StartCol, 0);
            Done := true
          end
          else
          begin
            if PeekChar() = '}' then
            begin
              C := TakeChar();
              Done := true
            end
            else
            begin
              C := TakeChar();
              Inside := Inside + 1;
              if Inside <= 3 then Directive := Directive + C
            end
          end
        end;
        { A comment holding exactly $I+ or $I- switches I/O checking;
          other comments, including other directives, are only comments. }
        if Inside = 3 then
          if (Directive = '$I+') or (Directive = '$i+') then IoChecks := true
          else if (Directive = '$I-') or (Directive = '$i-') then IoChecks := false;
        if not HadError then
          Again := true
      end
      else
      begin
        if C = '(' then
        begin
          if PeekNextChar() = '*' then
          begin
            StartPos := ScanPos;
            StartLine := ScanLine;
            StartCol := ScanCol;
            C := TakeChar();
            C := TakeChar();
            Done := false;
            while not Done do
            begin
              if AtEnd() then
              begin
                FailLex(ErrParenComment, StartPos, StartLine, StartCol, 0);
                Done := true
              end
              else
              begin
                if PeekChar() = '*' then
                begin
                  if PeekNextChar() = ')' then
                  begin
                    C := TakeChar();
                    C := TakeChar();
                    Done := true
                  end
                  else
                    C := TakeChar()
                end
                else
                  C := TakeChar()
              end
            end;
            if not HadError then
              Again := true
          end
        end
      end
    end
  end
end;

procedure ScanIdentifier();
var
  C: char;
  Done: boolean;
begin
  Done := false;
  while not Done do
  begin
    C := PeekChar();
    if IsIdentChar(C) then
    begin
      C := TakeChar();
      AppendTokenChar(C);
      if HadError then
        Done := true
    end
    else
      Done := true
  end;
  if not HadError then
  begin
    FoldTokenForKeyword();
    TokKind := KeywordKind(FoldedText);
    if TokKind = TkIdent then
    begin
      CanonicalizeIdentifier();
      TokLength := Length(TokText);
      TokOffset := PoolAdd(TokText)
    end
  end
end;

procedure ScanNumber();
var
  C: char;
  Digit: integer;
  Done: boolean;
begin
  TokKind := TkInteger;
  TokValue := 0;
  Done := false;
  while not Done do
  begin
    C := PeekChar();
    if IsDigit(C) then
    begin
      Digit := Ord(C) - Ord('0');
      if TokValue > 214748364 then
      begin
        FailLex(ErrIntegerRange, TokPos, TokLine, TokCol, 0);
        Done := true
      end
      else
      begin
        if TokValue = 214748364 then
        begin
          if Digit > 7 then
          begin
            FailLex(ErrIntegerRange, TokPos, TokLine, TokCol, 0);
            Done := true
          end
        end;
        if not Done then
        begin
          TokValue := TokValue * 10 + Digit;
          C := TakeChar()
        end
      end
    end
    else
      Done := true
  end
end;

procedure ScanString();
var
  C: char;
  Done: boolean;
begin
  TokKind := TkString;
  C := TakeChar();
  Done := false;
  while not Done do
  begin
    if AtEnd() then
    begin
      FailLex(ErrString, TokPos, TokLine, TokCol, 0);
      Done := true
    end
    else
    begin
      C := PeekChar();
      if C = Chr(10) then
      begin
        FailLex(ErrString, TokPos, TokLine, TokCol, 0);
        Done := true
      end
      else if C = Chr(13) then
      begin
        FailLex(ErrString, TokPos, TokLine, TokCol, 0);
        Done := true
      end
      else if C = Chr(39) then
      begin
        C := TakeChar();
        if PeekChar() = Chr(39) then
        begin
          C := TakeChar();
          AppendTokenChar(Chr(EscapeQuoteCode));
          if HadError then
            Done := true
        end
        else
          Done := true
      end
      else
      begin
        C := TakeChar();
        AppendTokenChar(C);
        if HadError then
          Done := true
      end
    end
  end;
  TokLength := Length(TokText)
end;

procedure NextToken();
var
  C: char;
begin
  ResetToken();
  SkipTrivia();
  if not HadError then
  begin
    TokPos := ScanPos;
    TokLine := ScanLine;
    TokCol := ScanCol;
    if AtEnd() then
      TokKind := TkEof
    else
    begin
      C := PeekChar();
      if IsIdentStart(C) then
        ScanIdentifier()
      else if IsDigit(C) then
        ScanNumber()
      else if C = Chr(39) then
        ScanString()
      else if C = ';' then
      begin C := TakeChar(); TokKind := TkSemi end
      else if C = '.' then
      begin
        C := TakeChar();
        if PeekChar() = '.' then
        begin C := TakeChar(); TokKind := TkDotDot end
        else TokKind := TkDot
      end
      else if C = ',' then
      begin C := TakeChar(); TokKind := TkComma end
      else if C = ':' then
      begin
        C := TakeChar();
        if PeekChar() = '=' then
        begin C := TakeChar(); TokKind := TkAssign end
        else TokKind := TkColon
      end
      else if C = '(' then
      begin C := TakeChar(); TokKind := TkLParen end
      else if C = ')' then
      begin C := TakeChar(); TokKind := TkRParen end
      else if C = '[' then
      begin C := TakeChar(); TokKind := TkLBracket end
      else if C = ']' then
      begin C := TakeChar(); TokKind := TkRBracket end
      else if C = '+' then
      begin C := TakeChar(); TokKind := TkPlus end
      else if C = '-' then
      begin C := TakeChar(); TokKind := TkMinus end
      else if C = '*' then
      begin C := TakeChar(); TokKind := TkStar end
      else if C = '=' then
      begin C := TakeChar(); TokKind := TkEq end
      else if C = '<' then
      begin
        C := TakeChar();
        if PeekChar() = '>' then
        begin C := TakeChar(); TokKind := TkNe end
        else if PeekChar() = '=' then
        begin C := TakeChar(); TokKind := TkLe end
        else TokKind := TkLt
      end
      else if C = '>' then
      begin
        C := TakeChar();
        if PeekChar() = '=' then
        begin C := TakeChar(); TokKind := TkGe end
        else TokKind := TkGt
      end
      else
      begin
        C := TakeChar();
        FailLex(ErrBadChar, TokPos, TokLine, TokCol, Ord(C))
      end
    end
  end
end;

procedure DumpToken();
var
  I: integer;
begin
  if TokKind = TkEof then Writeln('EOF')
  else if TokKind = TkIdent then
  begin
    PoolGet(TokOffset, TokLength, DumpText);
    if not HadError then Writeln('IDENT ', DumpText)
  end
  else if TokKind = TkInteger then Writeln('INTEGER ', TokValue)
  else if TokKind = TkString then
  begin
    Write('STRING ', TokLength);
    I := 1;
    while I <= TokLength do
    begin
      Write(' ', Ord(TokText[I]));
      I := I + 1
    end;
    Writeln()
  end
  else if TokKind = TkProgram then Writeln('KW program')
  else if TokKind = TkVar then Writeln('KW var')
  else if TokKind = TkBegin then Writeln('KW begin')
  else if TokKind = TkEnd then Writeln('KW end')
  else if TokKind = TkIntegerType then Writeln('KW integer')
  else if TokKind = TkDiv then Writeln('KW div')
  else if TokKind = TkMod then Writeln('KW mod')
  else if TokKind = TkWrite then Writeln('KW write')
  else if TokKind = TkWriteln then Writeln('KW writeln')
  else if TokKind = TkBoolean then Writeln('KW boolean')
  else if TokKind = TkAnd then Writeln('KW and')
  else if TokKind = TkOr then Writeln('KW or')
  else if TokKind = TkNot then Writeln('KW not')
  else if TokKind = TkTrue then Writeln('KW true')
  else if TokKind = TkFalse then Writeln('KW false')
  else if TokKind = TkIf then Writeln('KW if')
  else if TokKind = TkThen then Writeln('KW then')
  else if TokKind = TkElse then Writeln('KW else')
  else if TokKind = TkWhile then Writeln('KW while')
  else if TokKind = TkDo then Writeln('KW do')
  else if TokKind = TkFor then Writeln('KW for')
  else if TokKind = TkTo then Writeln('KW to')
  else if TokKind = TkDownto then Writeln('KW downto')
  else if TokKind = TkRepeat then Writeln('KW repeat')
  else if TokKind = TkUntil then Writeln('KW until')
  else if TokKind = TkCase then Writeln('KW case')
  else if TokKind = TkConst then Writeln('KW const')
  else if TokKind = TkChar then Writeln('KW char')
  else if TokKind = TkOrd then Writeln('KW ord')
  else if TokKind = TkChr then Writeln('KW chr')
  else if TokKind = TkProcedure then Writeln('KW procedure')
  else if TokKind = TkFunction then Writeln('KW function')
  else if TokKind = TkForward then Writeln('KW forward')
  else if TokKind = TkArray then Writeln('KW array')
  else if TokKind = TkOf then Writeln('KW of')
  else if TokKind = TkType then Writeln('KW type')
  else if TokKind = TkRecord then Writeln('KW record')
  else if TokKind = TkStringType then Writeln('KW string')
  else if TokKind = TkLength then Writeln('KW length')
  else if TokKind = TkFile then Writeln('KW file')
  else if TokKind = TkSemi then Writeln('SEMI')
  else if TokKind = TkDot then Writeln('DOT')
  else if TokKind = TkDotDot then Writeln('DOTDOT')
  else if TokKind = TkComma then Writeln('COMMA')
  else if TokKind = TkColon then Writeln('COLON')
  else if TokKind = TkAssign then Writeln('ASSIGN')
  else if TokKind = TkLParen then Writeln('LPAREN')
  else if TokKind = TkRParen then Writeln('RPAREN')
  else if TokKind = TkLBracket then Writeln('LBRACKET')
  else if TokKind = TkRBracket then Writeln('RBRACKET')
  else if TokKind = TkPlus then Writeln('PLUS')
  else if TokKind = TkMinus then Writeln('MINUS')
  else if TokKind = TkStar then Writeln('STAR')
  else if TokKind = TkEq then Writeln('EQ')
  else if TokKind = TkNe then Writeln('NE')
  else if TokKind = TkLt then Writeln('LT')
  else if TokKind = TkLe then Writeln('LE')
  else if TokKind = TkGt then Writeln('GT')
  else if TokKind = TkGe then Writeln('GE')
  else FailLex(ErrPoolInvariant, TokPos, TokLine, TokCol, TokKind)
end;

procedure ReadInput();
var
  Got: integer;
begin
  SourceLen := 0;
  {$I-}
  Assign(InputFile, InputName);
  Reset(InputFile, 1);
  CompilerIoError := IOResult();
  {$I+}
  CompilerIO(CompilerIoError);
  ChunkBase := 1;
  ChunkLen := 0;
  NextChunkLen := 0;
  StreamEof := false;
  ReadStreamBlock(ReadChunk, Got);
  ChunkLen := Got;
  if Got = 0 then StreamEof := true;
  if not StreamEof then
  begin
    ReadStreamBlock(NextChunk, Got);
    NextChunkLen := Got;
    if Got = 0 then StreamEof := true
  end
end;

procedure RunLexer();
var
  Done: boolean;
begin
  ScanPos := 1;
  ScanLine := 1;
  IoChecks := true;
  ScanCol := 1;
  PoolUsed := 0;
  PoolCount := 0;
  Done := HadError;
  while not Done do
  begin
    NextToken();
    if HadError then
      Done := true
    else
    begin
      DumpToken();
      if HadError then
        Done := true
      else if TokKind = TkEof then
        Done := true
    end
  end
end;

procedure WriteTokenName(Kind: integer);
begin
  if Kind = TkEof then Write('EOF')
  else if Kind = TkIdent then Write('IDENT')
  else if Kind = TkInteger then Write('INTEGER')
  else if Kind = TkString then Write('STRING')
  else if Kind = TkProgram then Write('KW_PROGRAM')
  else if Kind = TkVar then Write('KW_VAR')
  else if Kind = TkBegin then Write('KW_BEGIN')
  else if Kind = TkEnd then Write('KW_END')
  else if Kind = TkIntegerType then Write('KW_INTEGER')
  else if Kind = TkDiv then Write('KW_DIV')
  else if Kind = TkMod then Write('KW_MOD')
  else if Kind = TkWrite then Write('KW_WRITE')
  else if Kind = TkWriteln then Write('KW_WRITELN')
  else if Kind = TkBoolean then Write('KW_BOOLEAN')
  else if Kind = TkAnd then Write('KW_AND')
  else if Kind = TkOr then Write('KW_OR')
  else if Kind = TkNot then Write('KW_NOT')
  else if Kind = TkTrue then Write('KW_TRUE')
  else if Kind = TkFalse then Write('KW_FALSE')
  else if Kind = TkIf then Write('KW_IF')
  else if Kind = TkThen then Write('KW_THEN')
  else if Kind = TkElse then Write('KW_ELSE')
  else if Kind = TkWhile then Write('KW_WHILE')
  else if Kind = TkDo then Write('KW_DO')
  else if Kind = TkFor then Write('KW_FOR')
  else if Kind = TkTo then Write('KW_TO')
  else if Kind = TkDownto then Write('KW_DOWNTO')
  else if Kind = TkRepeat then Write('KW_REPEAT')
  else if Kind = TkUntil then Write('KW_UNTIL')
  else if Kind = TkCase then Write('KW_CASE')
  else if Kind = TkConst then Write('KW_CONST')
  else if Kind = TkChar then Write('KW_CHAR')
  else if Kind = TkOrd then Write('KW_ORD')
  else if Kind = TkChr then Write('KW_CHR')
  else if Kind = TkProcedure then Write('KW_PROCEDURE')
  else if Kind = TkFunction then Write('KW_FUNCTION')
  else if Kind = TkForward then Write('KW_FORWARD')
  else if Kind = TkArray then Write('KW_ARRAY')
  else if Kind = TkOf then Write('KW_OF')
  else if Kind = TkType then Write('KW_TYPE')
  else if Kind = TkRecord then Write('KW_RECORD')
  else if Kind = TkStringType then Write('KW_STRING')
  else if Kind = TkLength then Write('KW_LENGTH')
  else if Kind = TkFile then Write('KW_FILE')
  else if Kind = TkSemi then Write('SEMI')
  else if Kind = TkDot then Write('DOT')
  else if Kind = TkComma then Write('COMMA')
  else if Kind = TkColon then Write('COLON')
  else if Kind = TkAssign then Write('ASSIGN')
  else if Kind = TkLParen then Write('LPAREN')
  else if Kind = TkRParen then Write('RPAREN')
  else if Kind = TkPlus then Write('PLUS')
  else if Kind = TkMinus then Write('MINUS')
  else if Kind = TkStar then Write('STAR')
  else if Kind = TkEq then Write('EQ')
  else if Kind = TkNe then Write('NE')
  else if Kind = TkLt then Write('LT')
  else if Kind = TkLe then Write('LE')
  else if Kind = TkGt then Write('GT')
  else if Kind = TkGe then Write('GE')
  else if Kind = TkLBracket then Write('LBRACKET')
  else if Kind = TkRBracket then Write('RBRACKET')
  else if Kind = TkDotDot then Write('DOTDOT')
  else Write('UNKNOWN')
end;

procedure WriteExpected(Code: integer);
begin
  if Code = ExpFactor then Write('FACTOR')
  else if Code = ExpTypeName then Write('TYPE_NAME')
  else if Code = ExpResultType then Write('RESULT_TYPE')
  else if Code = ExpConstLiteral then Write('CONST_LITERAL')
  else if Code = ExpParamName then Write('PARAM_NAME')
  else if Code = ExpFieldName then Write('FIELD_NAME')
  else if Code = ExpVarName then Write('VAR_NAME')
  else if Code = ExpRoutineName then Write('ROUTINE_NAME')
  else if Code = ExpProgramName then Write('PROGRAM_NAME')
  else if Code = ExpArrayBound then Write('ARRAY_BOUND')
  else if Code = ExpToOrDownto then Write('TO_OR_DOWNTO')
  else if Code = ExpNoChainedRelation then Write('NO_CHAINED_RELATION')
  else WriteTokenName(Code)
end;

procedure WriteProductionName(Code: integer);
begin
  if Code = PrProgram then Write('program')
  else if Code = PrConstSection then Write('section-const')
  else if Code = PrConstDecl then Write('const-decl')
  else if Code = PrConstLiteral then Write('const-literal')
  else if Code = PrTypeSection then Write('section-type')
  else if Code = PrTypeDecl then Write('type-decl')
  else if Code = PrRecordType then Write('record-type')
  else if Code = PrFieldGroup then Write('field-group')
  else if Code = PrVarSection then Write('section-var')
  else if Code = PrVarDecl then Write('var-decl')
  else if Code = PrTypeName then Write('type-name')
  else if Code = PrArrayType then Write('array-type')
  else if Code = PrArrayBound then Write('array-bound')
  else if Code = PrResultType then Write('result-type')
  else if Code = PrRoutineProcedure then Write('routine-procedure')
  else if Code = PrRoutineFunction then Write('routine-function')
  else if Code = PrParamList then Write('param-list')
  else if Code = PrParamGroup then Write('param-group')
  else if Code = PrBlock then Write('block')
  else if Code = PrStatement then Write('statement')
  else if Code = PrIdentStatement then Write('ident-statement')
  else if Code = PrAssignment then Write('assignment')
  else if Code = PrCall then Write('call')
  else if Code = PrWrite then Write('stmt-write')
  else if Code = PrWriteArg then Write('write-arg')
  else if Code = PrIf then Write('stmt-if')
  else if Code = PrElseClause then Write('else-clause')
  else if Code = PrWhile then Write('stmt-while')
  else if Code = PrFor then Write('stmt-for')
  else if Code = PrRepeat then Write('stmt-repeat')
  else if Code = PrExpression then Write('expression')
  else if Code = PrSimpleExpression then Write('simple-expression')
  else if Code = PrTerm then Write('term')
  else if Code = PrFactor then Write('factor')
  else if Code = PrIndexSuffix then Write('index-suffix')
  else if Code = PrFieldSuffix then Write('field-suffix')
  else Write('unknown-production')
end;

procedure TraceEnter(Code: integer);
begin
  if EmitParseTrace then
  begin
    Write('ENTER ');
    WriteProductionName(Code);
    Writeln(' L', TokLine)
  end
end;

procedure TraceExit(Code: integer);
begin
  if EmitParseTrace then
  begin
    Write('EXIT ');
    WriteProductionName(Code);
    Writeln(' L', TokLine)
  end
end;

procedure FailParse(Expected: integer);
begin
  if GenFlags[1] then
    GenFail(TokLine, TokCol, 200 + Expected)
  else if not ParseFailed then
  begin
    if FirstKind = 0 then
    begin FirstKind := 2; FirstLine := TokLine; FirstCol := TokCol; FirstExpected := Expected end;
    ParseFailed := true;
    if not QuietCompile then
    begin
    Write('TURBO-PARSE-ERROR pos=', TokPos, ' line=', TokLine,
          ' col=', TokCol, ' expected=');
    WriteExpected(Expected);
    Write(' found=');
    WriteTokenName(TokKind);
    Writeln()
    end
  end
end;

procedure ExpectToken(Kind, Expected: integer);
begin
  if not ParseFailed then
  begin
    if TokKind = Kind then
      NextToken()
    else
      FailParse(Expected)
  end
end;

procedure WriteNameAt(NameOffset, NameLength: integer);
begin
  PoolGet(NameOffset, NameLength, DumpText);
  if not HadError then Write(DumpText)
end;

function PoolLengthAt(NameOffset: integer): integer;
var
  I, Found: integer;
begin
  I := 1;
  Found := 0;
  while I <= PoolCount do
  begin
    if Found = 0 then
      if PoolOffset[I] = NameOffset then Found := PoolLength[I];
    I := I + 1
  end;
  PoolLengthAt := Found
end;

procedure WriteSymbolName(Index: integer);
begin
  WriteNameAt(SymNameOffset[Index], PoolLengthAt(SymNameOffset[Index]))
end;

procedure WriteTypeName(TypeId, RecordIndex, StrCap: integer);
begin
  if TypeId = TyInteger then Write('integer')
  else if TypeId = TyBoolean then Write('boolean')
  else if TypeId = TyChar then Write('char')
  else if TypeId = TyString then
  begin
    Write('string');
    if StrCap <> 255 then Write('[', StrCap, ']')
  end
  else if TypeId = TyRecord then
  begin
    if RecordIndex > 0 then WriteSymbolName(RecordIndex)
    else Write('record')
  end
  else if TypeId = TyFile then Write('file')
  else if TypeId = TyNone then Write('none')
  else Write('unknown')
end;

procedure FailType(Code, L, C, NameOffset, NameLength, A, B: integer);
begin
  if GenFlags[1] then
    GenFail(L, C, 100 + Code)
  else if not TypeFailed then
  begin
    if FirstKind = 0 then
    begin FirstKind := 3; FirstLine := L; FirstCol := C; FirstDetail := Code end;
    TypeFailed := true;
    ParseFailed := true;
    if not QuietCompile then
    begin
    Write('TURBO-TYPE-ERROR line=', L, ' col=', C, ' ');
    if Code = TeDuplicate then
    begin
      Write('duplicate identifier: ');
      WriteNameAt(NameOffset, NameLength)
    end
    else if Code = TeUnknownType then
    begin
      Write('unknown type: ');
      WriteNameAt(NameOffset, NameLength)
    end
    else if Code = TeAssignMismatch then
    begin
      Write('type mismatch in assignment: expected ');
      WriteTypeName(A, 0, 255);
      Write(' got ');
      WriteTypeName(B, 0, 255)
    end
    else if Code = TeVarParam then
    begin
      Write('var parameter requires a variable: argument ', A, ' of ');
      WriteNameAt(NameOffset, NameLength)
    end
    else if Code = TeBoundForm then
      Write('array bound requires an integer literal or integer constant')
    else if Code = TeFileAssign then
      Write('file variables cannot be assigned')
    else if Code = TeRule then
      Write('semantic rule violation')
    else
      Write('unknown semantic failure code=', Code);
    Writeln()
    end
  end
end;

function NamesEqual(AOffset, ALength, BOffset, BLength: integer): boolean;
var
  I: integer;
  Same: boolean;
begin
  Same := ALength = BLength;
  I := 0;
  while I < ALength do
  begin
    if Same then
      if PoolChars[AOffset + I] <> PoolChars[BOffset + I] then Same := false;
    I := I + 1
  end;
  NamesEqual := Same
end;

function SymbolNameEqual(Index, NameOffset, NameLength: integer): boolean;
begin
  SymbolNameEqual := NamesEqual(SymNameOffset[Index],
                                PoolLengthAt(SymNameOffset[Index]),
                                NameOffset, NameLength)
end;

function IsDataKind(Kind: integer): boolean;
begin
  IsDataKind := false;
  if Kind = SkConst then IsDataKind := true
  else if Kind = SkVar then IsDataKind := true
  else if Kind = SkArray then IsDataKind := true
end;

function FindGlobalData(NameOffset, NameLength: integer): integer;
var
  I, Found: integer;
begin
  Found := 0;
  I := 1;
  while I <= SymCount do
  begin
    if Found = 0 then
      if SymScope[I] = 0 then
        if IsDataKind(SymKind[I]) then
          if SymbolNameEqual(I, NameOffset, NameLength) then Found := I;
    I := I + 1
  end;
  FindGlobalData := Found
end;

function FindRoutine(NameOffset, NameLength: integer): integer;
var
  I, Found: integer;
begin
  Found := 0;
  I := 1;
  while I <= SymCount do
  begin
    if Found = 0 then
      if SymScope[I] = 0 then
        if (SymKind[I] = SkProcedure) or (SymKind[I] = SkFunction) then
          if SymbolNameEqual(I, NameOffset, NameLength) then Found := I;
    I := I + 1
  end;
  FindRoutine := Found
end;

function FindRecordType(NameOffset, NameLength: integer): integer;
var
  I, Found: integer;
begin
  Found := 0;
  I := 1;
  while I <= SymCount do
  begin
    if Found = 0 then
      if SymKind[I] = SkType then
        if SymbolNameEqual(I, NameOffset, NameLength) then Found := I;
    I := I + 1
  end;
  FindRecordType := Found
end;

function FindLocal(NameOffset, NameLength: integer): integer;
var
  I, Found: integer;
begin
  Found := 0;
  I := 1;
  while I <= SymCount do
  begin
    if Found = 0 then
      if ActiveRoutine <> 0 then
        if SymScope[I] = ActiveRoutine then
          if SymbolNameEqual(I, NameOffset, NameLength) then Found := I;
    I := I + 1
  end;
  FindLocal := Found
end;

function ResolveName(NameOffset, NameLength: integer): integer;
var
  Found: integer;
begin
  Found := FindLocal(NameOffset, NameLength);
  if Found = 0 then Found := FindGlobalData(NameOffset, NameLength);
  ResolveName := Found
end;

function NameIsFileBuiltin(NameOffset, NameLength: integer): boolean;
begin
  PoolGet(NameOffset, NameLength, DumpText);
  NameIsFileBuiltin := false;
  if DumpText = 'assign' then NameIsFileBuiltin := true
  else if DumpText = 'reset' then NameIsFileBuiltin := true
  else if DumpText = 'rewrite' then NameIsFileBuiltin := true
  else if DumpText = 'blockread' then NameIsFileBuiltin := true
  else if DumpText = 'blockwrite' then NameIsFileBuiltin := true
  else if DumpText = 'close' then NameIsFileBuiltin := true
  else if DumpText = 'erase' then NameIsFileBuiltin := true
  else if DumpText = 'rename' then NameIsFileBuiltin := true
  else if DumpText = 'ioresult' then NameIsFileBuiltin := true
  else if DumpText = 'readkey' then NameIsFileBuiltin := true
  else if DumpText = 'fileattributes' then NameIsFileBuiltin := true
  else if DumpText = 'paramcount' then NameIsFileBuiltin := true
  else if DumpText = 'paramstr' then NameIsFileBuiltin := true
  else if DumpText = 'screenpage' then NameIsFileBuiltin := true
  else if DumpText = 'execute' then NameIsFileBuiltin := true
  else if DumpText = 'halt' then NameIsFileBuiltin := true
  else if DumpText = 'dosmemory' then NameIsFileBuiltin := true
end;

function FileBuiltinId(NameOffset, NameLength: integer): integer;
begin
  PoolGet(NameOffset, NameLength, DumpText);
  FileBuiltinId := 0;
  if DumpText = 'assign' then FileBuiltinId := 1
  else if DumpText = 'reset' then FileBuiltinId := 2
  else if DumpText = 'rewrite' then FileBuiltinId := 3
  else if DumpText = 'blockread' then FileBuiltinId := 4
  else if DumpText = 'blockwrite' then FileBuiltinId := 5
  else if DumpText = 'close' then FileBuiltinId := 6
  else if DumpText = 'erase' then FileBuiltinId := 7
  else if DumpText = 'rename' then FileBuiltinId := 8
  else if DumpText = 'ioresult' then FileBuiltinId := 9
  else if DumpText = 'readkey' then FileBuiltinId := 10
  else if DumpText = 'fileattributes' then FileBuiltinId := 11
  else if DumpText = 'paramcount' then FileBuiltinId := 12
  else if DumpText = 'paramstr' then FileBuiltinId := 13
  else if DumpText = 'screenpage' then FileBuiltinId := 14
  else if DumpText = 'execute' then FileBuiltinId := 15
  else if DumpText = 'halt' then FileBuiltinId := 16
  else if DumpText = 'dosmemory' then FileBuiltinId := 17
end;

function AddRawSymbol(NameOffset, NameLength, Kind, TypeId, Scope,
                      RefValue, LoValue, HiValue, OffsetValue,
                      SizeValue, L, C: integer): integer;
begin
  AddRawSymbol := 0;
  if SymCount >= SymbolMax then
    FailType(TeRule, L, C, NameOffset, NameLength, 0, 0)
  else
  begin
    SymCount := SymCount + 1;
    SymNameOffset[SymCount] := NameOffset;
    SymKind[SymCount] := Kind;
    SymType[SymCount] := TypeId;
    SymScope[SymCount] := Scope;
    SymRef[SymCount] := RefValue;
    SymLo[SymCount] := LoValue;
    SymHi[SymCount] := HiValue;
    SymOffset[SymCount] := OffsetValue;
    SymSize[SymCount] := SizeValue;
    AddRawSymbol := SymCount
  end
end;

function TypeStorageSize(TypeId, RecordIndex, StrCap: integer): integer;
var
  N: integer;
begin
  N := 4;
  if TypeId = TyRecord then N := SymSize[RecordIndex]
  else if TypeId = TyString then N := ((StrCap + 1 + 3) div 4) * 4
  else if TypeId = TyFile then N := 260;
  TypeStorageSize := N
end;

procedure AddDataSymbol(NameOffset, NameLength, L, C, TypeId, RecordIndex,
                        StrCap, LoValue, HiValue: integer; IsArray: boolean);
var
  Existing, Kind, Count, SizeValue, OffsetValue, NewIndex: integer;
begin
  Existing := 0;
  if ActiveRoutine = 0 then
  begin
    Existing := FindGlobalData(NameOffset, NameLength);
    if Existing = 0 then Existing := FindRoutine(NameOffset, NameLength)
  end
  else
    Existing := FindLocal(NameOffset, NameLength);
  if Existing <> 0 then begin { TURBO_TYPE_MUT_DUPOK }
    FailType(TeDuplicate, L, C, NameOffset, NameLength, 0, 0)
  end
  else if NameIsFileBuiltin(NameOffset, NameLength) then
    FailType(TeRule, L, C, NameOffset, NameLength, 0, 0)
  else
  begin
    if not IsArray then
    begin
      LoValue := 0;
      HiValue := 0;
      if TypeId = TyString then HiValue := StrCap
    end;
    Kind := SkVar;
    if ActiveRoutine <> 0 then Kind := SkLocal;
    if IsArray then
    begin
      Kind := SkArray;
      if ActiveRoutine <> 0 then Kind := SkLocalArray
    end;
    Count := 1;
    if IsArray then Count := HiValue - LoValue + 1;
    SizeValue := TypeStorageSize(TypeId, RecordIndex, StrCap) * Count;
    if ActiveRoutine = 0 then
    begin
      OffsetValue := GlobalBytes;
      GlobalBytes := GlobalBytes + SizeValue
    end
    else
    begin
      Count := SizeValue div 4;
      OffsetValue := -(4 * (LocalSlots + 1));
      if TypeId = TyString then OffsetValue := -(4 * (LocalSlots + Count))
      else if (TypeId = TyFile) or (TypeId = TyRecord) then
        OffsetValue := -(4 * (LocalSlots + Count));
      LocalSlots := LocalSlots + Count
    end;
    NewIndex := AddRawSymbol(NameOffset, NameLength, Kind, TypeId,
                             ActiveRoutine, RecordIndex, LoValue, HiValue,
                             OffsetValue, SizeValue, L, C);
    if TypeFailed then NewIndex := 0
    else if ActiveRoutine = 0 then GenEmitGlobal(NewIndex)
  end
end;

procedure AddConstSymbol(NameOffset, NameLength, L, C, TypeId,
                         Value, TextLength: integer);
var
  Existing, NewIndex: integer;
begin
  Existing := FindGlobalData(NameOffset, NameLength);
  if Existing = 0 then Existing := FindRoutine(NameOffset, NameLength);
  if Existing <> 0 then
    FailType(TeDuplicate, L, C, NameOffset, NameLength, 0, 0)
  else if NameIsFileBuiltin(NameOffset, NameLength) then
    FailType(TeRule, L, C, NameOffset, NameLength, 0, 0)
  else
  begin
    NewIndex := AddRawSymbol(NameOffset, NameLength, SkConst, TypeId, 0,
                             Value, 0, 0, 0, TextLength, L, C);
    if TypeFailed then NewIndex := 0
  end
end;

function AddRecordType(NameOffset, NameLength, L, C: integer): integer;
var
  Existing: integer;
begin
  AddRecordType := 0;
  Existing := FindRecordType(NameOffset, NameLength);
  if Existing <> 0 then
    FailType(TeDuplicate, L, C, NameOffset, NameLength, 0, 0)
  else
    AddRecordType := AddRawSymbol(NameOffset, NameLength, SkType, TyRecord,
                                  0, 0, 0, 0, 0, 0, L, C)
end;

procedure AddRecordField(NameOffset, NameLength, L, C, TypeId: integer);
var
  I, Existing, FieldIndex, NewIndex: integer;
begin
  Existing := 0;
  I := 1;
  while I <= SymCount do
  begin
    if Existing = 0 then
      if SymKind[I] = SkField then
        if SymScope[I] = -CurrentRecord then
          if SymbolNameEqual(I, NameOffset, NameLength) then Existing := I;
    I := I + 1
  end;
  if Existing <> 0 then
    FailType(TeRule, L, C, NameOffset, NameLength, 0, 0)
  else
  begin
    FieldIndex := SymLo[CurrentRecord];
    NewIndex := AddRawSymbol(NameOffset, NameLength, SkField, TypeId,
                             -CurrentRecord, FieldIndex, 0, 0,
                             FieldIndex * 4, 4, L, C);
    if not TypeFailed then
    begin
      SymLo[CurrentRecord] := FieldIndex + 1;
      SymSize[CurrentRecord] := (FieldIndex + 1) * 4
    end
    else
      NewIndex := 0
  end
end;

function FindRecordField(RecordIndex, NameOffset, NameLength: integer): integer;
var
  I, Found: integer;
begin
  Found := 0;
  I := 1;
  while I <= SymCount do
  begin
    if Found = 0 then
      if SymKind[I] = SkField then
        if SymScope[I] = -RecordIndex then
          if SymbolNameEqual(I, NameOffset, NameLength) then Found := I;
    I := I + 1
  end;
  FindRecordField := Found
end;

procedure ResetLastExpression();
begin
  LastExprType := TyUnknown;
  LastExprSym := 0;
  LastExprRecord := 0;
  LastExprStrCap := 0;
  LastExprForm := 0;
  LastExprLvalue := false;
  GenData[5] := 0
end;

procedure MarkComputed(TypeId: integer);
begin
  LastExprType := TypeId;
  LastExprSym := 0;
  LastExprRecord := 0;
  LastExprStrCap := 0;
  LastExprForm := 0;
  LastExprLvalue := false;
  GenData[5] := 0
end;

function RoutineParamAt(RoutineIndex, Ordinal: integer): integer;
var
  I, Seen, Found: integer;
begin
  I := 1;
  Seen := 0;
  Found := 0;
  while I <= SymCount do
  begin
    if Found = 0 then
      if SymScope[I] = RoutineIndex then
        if (SymKind[I] = SkValueParam) or (SymKind[I] = SkVarParam) then
        begin
          Seen := Seen + 1;
          if Seen = Ordinal then Found := I
        end;
    I := I + 1
  end;
  RoutineParamAt := Found
end;

procedure ValidateParsedCall();
var
  Builtin, RoutineIndex, ParamIndex, I: integer;
begin
  Builtin := FileBuiltinId(PendingIdentOffset, PendingIdentLength);
  if Builtin >= 12 then
  begin
    if Builtin = 14 then
    begin
      if CallWantsValue or (ArgCount <> 2) then
        FailType(TeRule, PendingIdentLine, PendingIdentCol, 0, 0, 0, 0)
      else if (ArgType[1] <> TyInteger) or (ArgType[2] <> TyChar) or
              (ArgLvalue[2] = 0) or (ArgForm[2] <> 2) or (ArgSym[2] = 0) then
        FailType(TeRule, PendingIdentLine, PendingIdentCol, 0, 0, 0, 0)
      else if SymType[ArgSym[2]] = TyString then
        FailType(TeRule, ArgLine[2], ArgCol[2], 0, 0, 0, 0)
    end
    else
    begin
      I := 1;
      if (Builtin = 12) or (Builtin = 17) then I := 0;
      if (ArgCount <> I) or (CallWantsValue = (Builtin = 16)) then
        FailType(TeRule, PendingIdentLine, PendingIdentCol, 0, 0, 0, 0)
      else if I = 1 then
      begin
        ParamIndex := TyInteger;
        if Builtin = 15 then ParamIndex := TyString;
        if ArgType[1] <> ParamIndex then
          FailType(TeRule, ArgLine[1], ArgCol[1], 0, 0, 0, 0)
      end
    end;
    if Builtin = 13 then MarkComputed(TyString)
    else if (Builtin = 14) or (Builtin = 16) then MarkComputed(TyUnknown)
    else MarkComputed(TyInteger)
  end
  else
  if Builtin = 11 then
  begin
    if (not CallWantsValue) or (ArgCount <> 1) then
      FailType(TeRule, PendingIdentLine, PendingIdentCol, 0, 0, 0, 0)
    else if (ArgType[1] <> TyFile) or (ArgLvalue[1] = 0) or (ArgForm[1] <> 1) then
      FailType(TeRule, ArgLine[1], ArgCol[1], 0, 0, 0, 0);
    MarkComputed(TyInteger)
  end
  else if Builtin >= 9 then
  begin
    { IOResult() and ReadKey() are functions without arguments. }
    if not CallWantsValue then
      FailType(TeRule, PendingIdentLine, PendingIdentCol,
               PendingIdentOffset, PendingIdentLength, 0, 0)
    else if ArgCount <> 0 then
      FailType(TeRule, PendingIdentLine, PendingIdentCol,
               PendingIdentOffset, PendingIdentLength, 0, 0);
    if not ParseFailed then
    begin
      if Builtin = 9 then MarkComputed(TyInteger) else MarkComputed(TyChar)
    end
  end
  else if Builtin <> 0 then
  begin
    if CallWantsValue then
      FailType(TeRule, PendingIdentLine, PendingIdentCol,
               PendingIdentOffset, PendingIdentLength, 0, 0)
    else
    begin
      if (Builtin = 1) or (Builtin = 8) then
      begin
        if ArgCount <> 2 then
          FailType(TeRule, PendingIdentLine, PendingIdentCol,
                   PendingIdentOffset, PendingIdentLength, 0, 0)
      end
      else if (Builtin = 2) or (Builtin = 3) then
      begin
        if (ArgCount <> 1) and (ArgCount <> 2) then
          FailType(TeRule, PendingIdentLine, PendingIdentCol,
                   PendingIdentOffset, PendingIdentLength, 0, 0)
      end
      else if (Builtin = 6) or (Builtin = 7) then
      begin
        if ArgCount <> 1 then
          FailType(TeRule, PendingIdentLine, PendingIdentCol,
                   PendingIdentOffset, PendingIdentLength, 0, 0)
      end
      else if ArgCount <> 4 then
        FailType(TeRule, PendingIdentLine, PendingIdentCol,
                 PendingIdentOffset, PendingIdentLength, 0, 0);
      if not ParseFailed then
      begin
        if ArgCount < 1 then
          FailType(TeRule, PendingIdentLine, PendingIdentCol,
                   PendingIdentOffset, PendingIdentLength, 0, 0)
        else if ArgType[1] <> TyFile then
          FailType(TeRule, ArgLine[1], ArgCol[1], 0, 0, 0, 0)
        else if ArgLvalue[1] = 0 then
          FailType(TeRule, ArgLine[1], ArgCol[1], 0, 0, 0, 0)
        else if ArgForm[1] <> 1 then
          FailType(TeRule, ArgLine[1], ArgCol[1], 0, 0, 0, 0)
      end;
      if not ParseFailed then
      begin
        if (Builtin = 1) or (Builtin = 8) then
        begin
          if (ArgType[2] <> TyString) and (ArgType[2] <> TyChar) then
            FailType(TeRule, ArgLine[2], ArgCol[2], 0, 0, 0, 0)
        end
        else if (Builtin = 2) or (Builtin = 3) then
        begin
          if ArgCount = 2 then
            if ArgType[2] <> TyInteger then
              FailType(TeRule, ArgLine[2], ArgCol[2], 0, 0, 0, 0)
        end
        else if (Builtin = 4) or (Builtin = 5) then
        begin
          if ArgForm[2] <> 2 then
            FailType(TeRule, ArgLine[2], ArgCol[2], 0, 0, 0, 0)
          else if ArgType[2] <> TyChar then
            FailType(TeRule, ArgLine[2], ArgCol[2], 0, 0, 0, 0)
          else if ArgSym[2] = 0 then
            FailType(TeRule, ArgLine[2], ArgCol[2], 0, 0, 0, 0)
          else if SymType[ArgSym[2]] <> TyString then
            FailType(TeRule, ArgLine[2], ArgCol[2], 0, 0, 0, 0)
          else if ArgLvalue[2] = 0 then
            FailType(TeRule, ArgLine[2], ArgCol[2], 0, 0, 0, 0);
          if not ParseFailed then
            if ArgType[3] <> TyInteger then
              FailType(TeRule, ArgLine[3], ArgCol[3], 0, 0, 0, 0);
          if not ParseFailed then
            if (ArgType[4] <> TyInteger) or (ArgLvalue[4] = 0) or
               (ArgForm[4] <> 1) then
              FailType(TeRule, ArgLine[4], ArgCol[4], 0, 0, 0, 0)
        end
      end
    end;
    MarkComputed(TyUnknown)
  end
  else
  begin
    RoutineIndex := FindRoutine(PendingIdentOffset, PendingIdentLength);
    if RoutineIndex = 0 then
      FailType(TeRule, PendingIdentLine, PendingIdentCol,
               PendingIdentOffset, PendingIdentLength, 0, 0)
    else
    begin
      if CallWantsValue then
      begin
        if SymKind[RoutineIndex] <> SkFunction then
          FailType(TeRule, PendingIdentLine, PendingIdentCol,
                   PendingIdentOffset, PendingIdentLength, 0, 0)
      end
      else if SymKind[RoutineIndex] <> SkProcedure then
        FailType(TeRule, PendingIdentLine, PendingIdentCol,
                 PendingIdentOffset, PendingIdentLength, 0, 0);
      if not ParseFailed then
        if ArgCount <> SymLo[RoutineIndex] then
          FailType(TeRule, PendingIdentLine, PendingIdentCol,
                   PendingIdentOffset, PendingIdentLength, 0, 0);
      I := 1;
      while I <= ArgCount do
      begin
        if not ParseFailed then
        begin
          ParamIndex := RoutineParamAt(RoutineIndex, I);
          if SymKind[ParamIndex] = SkVarParam then
            if ArgLvalue[I] = 0 then
              FailType(TeVarParam, ArgLine[I], ArgCol[I], PendingIdentOffset,
                       PendingIdentLength, I, 0);
          if not ParseFailed then
            if ArgType[I] <> SymType[ParamIndex] then
              FailType(TeRule, ArgLine[I], ArgCol[I], PendingIdentOffset,
                       PendingIdentLength, I, 0);
          if not ParseFailed then
            if SymType[ParamIndex] = TyRecord then
              if ArgSym[I] > 0 then
              begin
                if SymRef[ArgSym[I]] <> SymRef[ParamIndex] then
                  FailType(TeRule, ArgLine[I], ArgCol[I],
                           PendingIdentOffset, PendingIdentLength, I, 0)
              end
              else
                FailType(TeRule, ArgLine[I], ArgCol[I],
                         PendingIdentOffset, PendingIdentLength, I, 0);
          if not ParseFailed then
            if SymKind[ParamIndex] = SkVarParam then
              if SymType[ParamIndex] = TyString then
                if ArgSym[I] > 0 then
                begin
                  if SymHi[ArgSym[I]] <> 255 then
                    FailType(TeRule, ArgLine[I], ArgCol[I],
                             PendingIdentOffset, PendingIdentLength, I, 0)
                end
        end;
        I := I + 1
      end;
      if not ParseFailed then
      begin
        if SymKind[RoutineIndex] = SkFunction then
          MarkComputed(SymType[RoutineIndex])
        else
          MarkComputed(TyUnknown)
      end
    end
  end
end;

function RegisterRoutine(NameOffset, NameLength, L, C: integer;
                         IsFunction, IsForward: boolean): integer;
var
  Existing, DataHit, Kind, I, P, NewIndex, ResultIndex, ParamCap: integer;
  Matches: boolean;
begin
  RegisterRoutine := 0;
  DataHit := FindGlobalData(NameOffset, NameLength);
  if DataHit <> 0 then
    FailType(TeDuplicate, L, C, NameOffset, NameLength, 0, 0)
  else if NameIsFileBuiltin(NameOffset, NameLength) then
    FailType(TeRule, L, C, NameOffset, NameLength, 0, 0)
  else
  begin
    Existing := FindRoutine(NameOffset, NameLength);
    if Existing = 0 then
    begin
      Kind := SkProcedure;
      if IsFunction then Kind := SkFunction;
      NewIndex := AddRawSymbol(NameOffset, NameLength, Kind, ParsedType, 0,
                               0, HeaderCount, 0, L * 1000 + C, 0, L, C);
      if not TypeFailed then
      begin
        SymHi[NewIndex] := 0;
        if not IsForward then SymHi[NewIndex] := 1;
        I := 1;
        while I <= HeaderCount do
        begin
          ParamCap := 0;
          if HeaderType[I] = TyString then ParamCap := 255;
          P := AddRawSymbol(HeaderNameOffset[I],
                            PoolLengthAt(HeaderNameOffset[I]),
                            HeaderKind[I], HeaderType[I], NewIndex,
                            HeaderRef[I], 0, ParamCap, 8 + 4 * (I - 1),
                            4, HeaderLine[I], HeaderCol[I]);
          if I = 1 then SymRef[NewIndex] := P;
          I := I + 1
        end;
        if not TypeFailed then
          if IsFunction then
          begin
            Matches := true;
            I := 1;
            while I <= HeaderCount do
            begin
              if NamesEqual(HeaderNameOffset[I],
                            PoolLengthAt(HeaderNameOffset[I]),
                            NameOffset, NameLength) then Matches := false;
              I := I + 1
            end;
            if not Matches then
              FailType(TeDuplicate, L, C, NameOffset, NameLength, 0, 0)
            else
            begin
              ActiveRoutine := NewIndex;
              ResultIndex := AddRawSymbol(NameOffset, NameLength, SkResult,
                                          ParsedType, NewIndex, 0, 0, 0,
                                          -4, 4, L, C);
              ActiveRoutine := 0;
              if TypeFailed then ResultIndex := 0
            end
          end;
        RegisterRoutine := NewIndex
      end
    end
    else
    begin
      Matches := true;
      if IsForward then Matches := false;
      if SymHi[Existing] <> 0 then Matches := false;
      if IsFunction then
      begin
        if SymKind[Existing] <> SkFunction then Matches := false
      end
      else if SymKind[Existing] <> SkProcedure then Matches := false;
      if SymType[Existing] <> ParsedType then Matches := false;
      if SymLo[Existing] <> HeaderCount then Matches := false;
      I := 1;
      while I <= HeaderCount do
      begin
        P := RoutineParamAt(Existing, I);
        if P = 0 then Matches := false
        else
        begin
          if SymKind[P] <> HeaderKind[I] then Matches := false;
          if SymType[P] <> HeaderType[I] then Matches := false;
          if SymRef[P] <> HeaderRef[I] then Matches := false
        end;
        I := I + 1
      end;
      if not Matches then
        FailType(TeRule, L, C, NameOffset, NameLength, 0, 0)
      else
      begin
        SymHi[Existing] := 1;
        I := 1;
        while I <= HeaderCount do
        begin
          P := RoutineParamAt(Existing, I);
          SymNameOffset[P] := HeaderNameOffset[I];
          I := I + 1
        end;
        RegisterRoutine := Existing
      end
    end
  end
end;

procedure DumpOneDataSymbol(Index: integer);
begin
  Write('SYM kind=');
  if SymKind[Index] = SkConst then Write('const')
  else if SymKind[Index] = SkVar then Write('var')
  else if SymKind[Index] = SkArray then Write('var')
  else if SymKind[Index] = SkProcedure then Write('procedure')
  else if SymKind[Index] = SkFunction then Write('function')
  else if SymKind[Index] = SkValueParam then Write('value-param')
  else if SymKind[Index] = SkVarParam then Write('var-param')
  else if SymKind[Index] = SkResult then Write('result')
  else if SymKind[Index] = SkLocal then Write('local')
  else if SymKind[Index] = SkLocalArray then Write('local')
  else Write('unknown');
  Write(' name=');
  WriteSymbolName(Index);
  Write(' type=');
  if SymKind[Index] = SkConst then
    WriteTypeName(SymType[Index], 0, 255)
  else
    WriteTypeName(SymType[Index], SymRef[Index], SymHi[Index]);
  if SymKind[Index] = SkConst then
  begin
    if SymType[Index] = TyString then Write(' value-length=', SymSize[Index])
    else Write(' value=', SymRef[Index])
  end
  else if (SymKind[Index] = SkProcedure) or (SymKind[Index] = SkFunction) then
  begin
    Write(' params=', SymLo[Index], ' state=');
    if SymHi[Index] = 1 then Write('defined') else Write('forward');
    Write(' frame=', SymSize[Index])
  end
  else
  begin
    if (SymKind[Index] = SkArray) or (SymKind[Index] = SkLocalArray) then
      Write(' array=', SymLo[Index], '..', SymHi[Index]);
    Write(' offset=', SymOffset[Index], ' size=', SymSize[Index])
  end;
  Writeln()
end;

procedure DumpSymbols();
var
  I, J: integer;
begin
  Writeln('SCOPE global bytes=', GlobalBytes);
  I := 1;
  while I <= SymCount do
  begin
    if SymScope[I] = 0 then
    begin
      if SymKind[I] = SkType then
      begin
        Write('SYM kind=type name=');
        WriteSymbolName(I);
        Writeln(' type=record fields=', SymLo[I], ' size=', SymSize[I]);
        J := 1;
        while J <= SymCount do
        begin
          if SymKind[J] = SkField then
            if SymScope[J] = -I then
            begin
              Write('FIELD owner=');
              WriteSymbolName(I);
              Write(' index=', SymRef[J], ' name=');
              WriteSymbolName(J);
              Write(' type=');
              WriteTypeName(SymType[J], 0, 0);
              Writeln(' offset=', SymOffset[J], ' size=', SymSize[J])
            end;
          J := J + 1
        end
      end
      else
        DumpOneDataSymbol(I)
    end;
    I := I + 1
  end;
  I := 1;
  while I <= SymCount do
  begin
    if SymScope[I] = 0 then
      if (SymKind[I] = SkProcedure) or (SymKind[I] = SkFunction) then
      begin
        Write('SCOPE routine name=');
        WriteSymbolName(I);
        Writeln(' frame=', SymSize[I]);
        J := 1;
        while J <= SymCount do
        begin
          if SymScope[J] = I then DumpOneDataSymbol(J);
          J := J + 1
        end
      end;
    I := I + 1
  end
end;

procedure CheckUnresolvedForwards();
var
  I, PackedLoc, L, C: integer;
begin
  I := 1;
  while I <= SymCount do
  begin
    if not ParseFailed then
      if SymScope[I] = 0 then
        if (SymKind[I] = SkProcedure) or (SymKind[I] = SkFunction) then
          if SymHi[I] = 0 then
          begin
            PackedLoc := SymOffset[I];
            L := PackedLoc div 1000;
            C := PackedLoc mod 1000;
            FailType(TeRule, L, C, SymNameOffset[I],
                     PoolLengthAt(SymNameOffset[I]), 0, 0)
          end;
    I := I + 1
  end
end;

function GenFrameOffset(Index: integer): integer;
begin
  GenFrameOffset := SymOffset[Index];
  if (SymKind[Index] = SkValueParam) or (SymKind[Index] = SkVarParam) then
    GenFrameOffset := SymOffset[Index] { TURBO_GEN_MUT_OFFBYONE }
end;

procedure GenEbp(Index: integer);
var
  OffsetValue: integer;
begin
  OffsetValue := GenFrameOffset(Index);
  begin GenText := '[ebp'; GenPut(GenText) end;
  if OffsetValue >= 0 then GenChar('+');
  GenInt(OffsetValue);
  GenChar(']')
end;

procedure GenDesignatorAddress(Index: integer);
begin
  if Index <= 0 then
    GenFail(TokLine, TokCol, 2)
  else if SymScope[Index] = 0 then
  begin
    begin GenText := '    mov eax, v_'; GenPut(GenText) end;
    PoolGet(SymNameOffset[Index], PoolLengthAt(SymNameOffset[Index]), DumpText);
    GenPut(DumpText);
    GenChar(Chr(10))
  end
  else if SymKind[Index] = SkVarParam then
  begin
    begin GenText := '    mov eax, '; GenPut(GenText) end;
    GenEbp(Index);
    GenChar(Chr(10))
  end
  else
  begin
    begin GenText := '    lea eax, '; GenPut(GenText) end;
    GenEbp(Index);
    GenChar(Chr(10))
  end
end;

{ Assignment destinations have a named seam for the B9.5 semantic mutation
  proof. The clean path is deliberately identical to every other designator
  address. TURBO_GEN_MUT_VARPARAM changes only a var-parameter assignment
  destination to the callee's parameter slot, so reads still observe the
  caller but writes update a dead local copy instead of the caller address. }
procedure GenAssignmentAddress(Index: integer);
begin
  if SymKind[Index] = SkVarParam then
    GenDesignatorAddress(Index) { TURBO_GEN_MUT_VARPARAM }
  else
    GenDesignatorAddress(Index)
end;

procedure GenApplyIndex(Index, BaseType: integer);
var
  Stride: integer;
begin
  begin GenText := '    mov ecx, eax'; GenLine(GenText) end;
  begin GenText := '    pop eax'; GenLine(GenText) end;
  if BaseType = TyString then
    begin GenText := '    add eax, ecx'; GenLine(GenText) end
  else
  begin
    if SymLo[Index] <> 0 then
    begin
      begin GenText := '    sub ecx, '; GenPut(GenText) end;
      GenInt(SymLo[Index]);
      GenChar(Chr(10))
    end;
    Stride := 4;
    if SymType[Index] = TyRecord then Stride := SymSize[SymRef[Index]];
    begin GenText := '    imul ecx, '; GenPut(GenText) end;
    GenInt(Stride);
    GenChar(Chr(10));
    if (SymKind[Index] = SkLocalArray) and (SymType[Index] <> TyRecord) then
      begin GenText := '    sub eax, ecx'; GenLine(GenText) end
    else
      begin GenText := '    add eax, ecx'; GenLine(GenText) end
  end
end;

procedure GenApplyField(BaseIndex, FieldIndex: integer; HadIndex: boolean);
var
  OffsetValue: integer;
begin
  OffsetValue := SymOffset[FieldIndex];
  if OffsetValue <> 0 then
  begin
    begin GenText := '    add eax, '; GenPut(GenText) end;
    GenInt(OffsetValue);
    GenChar(Chr(10))
  end
end;

procedure GenEmitGlobal(Index: integer);
begin
  if GenFlags[1] then
  begin
    GenSetSection(2);
    begin GenText := 'v_'; GenPut(GenText) end;
    PoolGet(SymNameOffset[Index], PoolLengthAt(SymNameOffset[Index]), DumpText);
    GenPut(DumpText);
    if SymType[Index] = TyString then
    begin
      begin GenText := ': resb '; GenPut(GenText) end;
      GenInt(SymSize[Index])
    end
    else if SymType[Index] = TyFile then
    begin
      begin GenText := ': resd '; GenPut(GenText) end;
      GenInt(SymSize[Index] div 4)
    end
    else
    begin
      begin GenText := ': resd '; GenPut(GenText) end;
      GenInt(SymSize[Index] div 4)
    end;
    GenChar(Chr(10))
  end
end;

procedure GenInlineString(var Text: string; ShortForm: boolean);
var
  LabelNumber, I: integer;
begin
  LabelNumber := GenNewLabel();
  if Native[1] > 0 then
  begin
    NativeByte(233);
    NativeWord(NativeLabels[LabelNumber*2+34] - Native[2] - 4);
    NativeLabels[LabelNumber*2+33] := Native[2];
    if ShortForm then NativeByte(Length(Text));
    I := 1;
    while I <= Length(Text) do
    begin NativeByte(Ord(Text[I])); I := I + 1 end;
    if not ShortForm then NativeByte(0);
    NativeLabels[LabelNumber*2+34] := Native[2];
    NativeByte(184);
    NativeAbsolute(NativeLabels[LabelNumber*2+33])
  end
  else
  begin
  begin GenText := '    jmp near '; GenPut(GenText) end;
  begin GenText := '.Lstr_after_'; GenPut(GenText); GenInt(LabelNumber) end;
  GenChar(Chr(10));
  begin GenText := '.Lstr_'; GenPut(GenText); GenInt(LabelNumber) end;
  begin GenText := ': db '; GenPut(GenText) end;
  if ShortForm then
    GenInt(Length(Text));
  I := 1;
  while I <= Length(Text) do
  begin
    if ShortForm then GenChar(',');
    GenInt(Ord(Text[I]));
    if not ShortForm then GenChar(',');
    I := I + 1
  end;
  if not ShortForm then GenInt(0);
  GenChar(Chr(10));
  begin GenText := '.Lstr_after_'; GenPut(GenText); GenInt(LabelNumber) end;
  begin GenText := ':'; GenPut(GenText) end;
  GenChar(Chr(10));
  begin GenText := '    mov eax, '; GenPut(GenText) end;
  begin GenText := '.Lstr_'; GenPut(GenText); GenInt(LabelNumber) end;
  GenChar(Chr(10))
  end
end;

procedure GenCoerceChar(var TempBytes: integer);
begin
  begin GenText := '    sub esp, 4'; GenLine(GenText) end;
  begin GenText := '    mov byte [esp], 1'; GenLine(GenText) end;
  begin GenText := '    mov byte [esp+1], al'; GenLine(GenText) end;
  begin GenText := '    mov eax, esp'; GenLine(GenText) end;
  TempBytes := TempBytes + 4;
  GenFlags[3] := true
end;

procedure GenConcat(LeftTemp, RightTemp: integer);
var
  Total: integer;
begin
  begin GenText := '    push eax'; GenLine(GenText) end;
  begin GenText := '    sub esp, 256'; GenLine(GenText) end;
  begin GenText := '    mov esi, [esp+'; GenPut(GenText) end;
  GenInt(260 + RightTemp);
  begin GenText := ']'; GenPut(GenText) end;
  GenChar(Chr(10));
  begin GenText := '    mov edi, esp'; GenLine(GenText) end;
  begin GenText := '    mov ecx, 255'; GenLine(GenText) end;
  begin GenText := '    call __str_assign'; GenLine(GenText) end;
  begin GenText := '    mov esi, [esp+256]'; GenLine(GenText) end;
  begin GenText := '    mov edi, esp'; GenLine(GenText) end;
  begin GenText := '    call __str_concat'; GenLine(GenText) end;
  begin GenText := '    mov eax, esp'; GenLine(GenText) end;
  Total := LeftTemp + 4 + RightTemp + 4 + 256;
  GenData[5] := Total;
  GenFlags[3] := true
end;

procedure GenStringCompare(OperatorKind, LeftTemp, RightTemp: integer);
var
  Total: integer;
begin
  begin GenText := '    push eax'; GenLine(GenText) end;
  begin GenText := '    mov esi, [esp+'; GenPut(GenText) end;
  GenInt(4 + RightTemp);
  begin GenText := ']'; GenPut(GenText) end;
  GenChar(Chr(10));
  begin GenText := '    mov edi, [esp]'; GenLine(GenText) end;
  begin GenText := '    call __str_cmp'; GenLine(GenText) end;
  Total := LeftTemp + 4 + RightTemp + 4;
  begin GenText := '    add esp, '; GenPut(GenText) end;
  GenInt(Total);
  GenChar(Chr(10));
  begin GenText := '    cmp eax, 0'; GenLine(GenText) end;
  if OperatorKind = TkEq then begin GenText := '    sete al'; GenLine(GenText) end
  else if OperatorKind = TkNe then begin GenText := '    setne al'; GenLine(GenText) end
  else if OperatorKind = TkLt then begin GenText := '    setl al'; GenLine(GenText) end
  else if OperatorKind = TkLe then begin GenText := '    setle al'; GenLine(GenText) end
  else if OperatorKind = TkGt then begin GenText := '    setg al'; GenLine(GenText) end
  else if OperatorKind = TkGe then begin GenText := '    setge al'; GenLine(GenText) end
  else GenFail(TokLine, TokCol, 3);
  begin GenText := '    movzx eax, al'; GenLine(GenText) end;
  GenData[5] := 0;
  GenFlags[3] := true
end;

{ FPC -Mtp, corpus Delphi/B+/H-: minimum Integer div -1 wraps when
  stored to Integer; mod -1 is zero. IDIV would raise #DE even for mod.
  Both operands are already evaluated. Tests: div_min.pas and mod_min.pas
  in tests/execution/. }
procedure GenDivMod();
var LabelNumber: integer;
begin
  LabelNumber := GenNewLabel();
  begin GenText := '    cmp ecx, -1'; GenLine(GenText) end;
  begin GenText := '    jz near '; GenPut(GenText) end;
  begin GenText := '.Ldiv_neg_'; GenPut(GenText); GenInt(LabelNumber) end;
  GenChar(Chr(10));
  begin GenText := '    cdq'; GenLine(GenText) end;
  begin GenText := '    idiv ecx'; GenLine(GenText) end;
  begin GenText := '    jmp near '; GenPut(GenText) end;
  begin GenText := '.Ldiv_end_'; GenPut(GenText); GenInt(LabelNumber) end;
  GenChar(Chr(10));
  begin GenText := '.Ldiv_neg_'; GenPut(GenText); GenInt(LabelNumber) end;
  begin GenText := ':'; GenLine(GenText) end;
  begin GenText := '    neg eax'; GenLine(GenText) end;
  begin GenText := '    xor edx, edx'; GenLine(GenText) end;
  begin GenText := '.Ldiv_end_'; GenPut(GenText); GenInt(LabelNumber) end;
  begin GenText := ':'; GenLine(GenText) end
end;

procedure GenScalarBinary(OperatorKind: integer);
begin
  begin GenText := '    mov ecx, eax'; GenLine(GenText) end;
  begin GenText := '    pop eax'; GenLine(GenText) end;
  if OperatorKind = TkPlus then begin GenText := '    add eax, ecx'; GenLine(GenText) end
  else if OperatorKind = TkMinus then begin GenText := '    sub eax, ecx'; GenLine(GenText) end
  else if OperatorKind = TkStar then begin GenText := '    imul eax, ecx'; GenLine(GenText) end
  else if OperatorKind = TkDiv then
    GenDivMod()
  else if OperatorKind = TkMod then
  begin
    GenDivMod();
    begin GenText := '    mov eax, edx'; GenLine(GenText) end
  end
  else if OperatorKind = TkAnd then begin GenText := '    and eax, ecx'; GenLine(GenText) end
  else if OperatorKind = TkOr then begin GenText := '    or eax, ecx'; GenLine(GenText) end
  else
  begin
    begin GenText := '    cmp eax, ecx'; GenLine(GenText) end;
    if OperatorKind = TkEq then begin GenText := '    sete al'; GenLine(GenText) end
    else if OperatorKind = TkNe then begin GenText := '    setne al'; GenLine(GenText) end
    else if OperatorKind = TkLt then begin GenText := '    setl al'; GenLine(GenText) end
    else if OperatorKind = TkLe then begin GenText := '    setle al'; GenLine(GenText) end
    else if OperatorKind = TkGt then begin GenText := '    setg al'; GenLine(GenText) end
    else if OperatorKind = TkGe then begin GenText := '    setge al'; GenLine(GenText) end
    else GenFail(TokLine, TokCol, 3);
    begin GenText := '    movzx eax, al'; GenLine(GenText) end
  end;
  GenData[5] := 0
end;

procedure GenStringIntrinsics();
begin
  GenSetSection(3);
  begin GenText := '__str_assign:'; GenLine(GenText) end;
  begin GenText := '    movzx eax, byte [esi]'; GenLine(GenText) end;
  begin GenText := '    cmp eax, ecx'; GenLine(GenText) end;
  begin GenText := '    jbe near .len_ok'; GenLine(GenText) end;
  begin GenText := '    mov eax, ecx'; GenLine(GenText) end;
  begin GenText := '.len_ok:'; GenLine(GenText) end;
  begin GenText := '    mov [edi], al'; GenLine(GenText) end;
  begin GenText := '    mov ecx, eax'; GenLine(GenText) end;
  begin GenText := '    inc esi'; GenLine(GenText) end;
  begin GenText := '    inc edi'; GenLine(GenText) end;
  begin GenText := '    rep movsb'; GenLine(GenText) end;
  begin GenText := '    ret'; GenLine(GenText) end;
  begin GenText := '__str_concat:'; GenLine(GenText) end;
  begin GenText := '    movzx eax, byte [edi]'; GenLine(GenText) end;
  begin GenText := '    movzx ecx, byte [esi]'; GenLine(GenText) end;
  begin GenText := '    mov edx, 255'; GenLine(GenText) end;
  begin GenText := '    sub edx, eax'; GenLine(GenText) end;
  begin GenText := '    cmp ecx, edx'; GenLine(GenText) end;
  begin GenText := '    jbe near .n_ok'; GenLine(GenText) end;
  begin GenText := '    mov ecx, edx'; GenLine(GenText) end;
  begin GenText := '.n_ok:'; GenLine(GenText) end;
  begin GenText := '    lea edx, [edi+eax+1]'; GenLine(GenText) end;
  begin GenText := '    add eax, ecx'; GenLine(GenText) end;
  begin GenText := '    mov [edi], al'; GenLine(GenText) end;
  begin GenText := '    inc esi'; GenLine(GenText) end;
  begin GenText := '    mov edi, edx'; GenLine(GenText) end;
  begin GenText := '    rep movsb'; GenLine(GenText) end;
  begin GenText := '    ret'; GenLine(GenText) end;
  begin GenText := '__str_cmp:'; GenLine(GenText) end;
  begin GenText := '    movzx eax, byte [esi]'; GenLine(GenText) end;
  begin GenText := '    movzx edx, byte [edi]'; GenLine(GenText) end;
  begin GenText := '    mov ecx, eax'; GenLine(GenText) end;
  begin GenText := '    cmp edx, ecx'; GenLine(GenText) end;
  begin GenText := '    jae near .have_min'; GenLine(GenText) end;
  begin GenText := '    mov ecx, edx'; GenLine(GenText) end;
  begin GenText := '.have_min:'; GenLine(GenText) end;
  begin GenText := '    push eax'; GenLine(GenText) end;
  begin GenText := '    push edx'; GenLine(GenText) end;
  begin GenText := '    inc esi'; GenLine(GenText) end;
  begin GenText := '    inc edi'; GenLine(GenText) end;
  begin GenText := '.cmp_loop:'; GenLine(GenText) end;
  begin GenText := '    test ecx, ecx'; GenLine(GenText) end;
  begin GenText := '    jz near .prefix_equal'; GenLine(GenText) end;
  begin GenText := '    mov al, [esi]'; GenLine(GenText) end;
  begin GenText := '    mov dl, [edi]'; GenLine(GenText) end;
  begin GenText := '    cmp al, dl'; GenLine(GenText) end;
  begin GenText := '    jb near .a_less'; GenLine(GenText) end;
  begin GenText := '    ja near .a_greater'; GenLine(GenText) end;
  begin GenText := '    inc esi'; GenLine(GenText) end;
  begin GenText := '    inc edi'; GenLine(GenText) end;
  begin GenText := '    dec ecx'; GenLine(GenText) end;
  begin GenText := '    jmp near .cmp_loop'; GenLine(GenText) end;
  begin GenText := '.a_less:'; GenLine(GenText) end;
  begin GenText := '    add esp, 8'; GenLine(GenText) end;
  begin GenText := '    mov eax, -1'; GenLine(GenText) end;
  begin GenText := '    ret'; GenLine(GenText) end;
  begin GenText := '.a_greater:'; GenLine(GenText) end;
  begin GenText := '    add esp, 8'; GenLine(GenText) end;
  begin GenText := '    mov eax, 1'; GenLine(GenText) end;
  begin GenText := '    ret'; GenLine(GenText) end;
  begin GenText := '.prefix_equal:'; GenLine(GenText) end;
  begin GenText := '    pop edx'; GenLine(GenText) end;
  begin GenText := '    pop eax'; GenLine(GenText) end;
  begin GenText := '    cmp eax, edx'; GenLine(GenText) end;
  begin GenText := '    jb near .len_less'; GenLine(GenText) end;
  begin GenText := '    ja near .len_greater'; GenLine(GenText) end;
  begin GenText := '    mov eax, 0'; GenLine(GenText) end;
  begin GenText := '    ret'; GenLine(GenText) end;
  begin GenText := '.len_less:'; GenLine(GenText) end;
  begin GenText := '    mov eax, -1'; GenLine(GenText) end;
  begin GenText := '    ret'; GenLine(GenText) end;
  begin GenText := '.len_greater:'; GenLine(GenText) end;
  begin GenText := '    mov eax, 1'; GenLine(GenText) end;
  begin GenText := '    ret'; GenLine(GenText) end;
  begin GenText := '__str_write:'; GenLine(GenText) end;
  begin GenText := '    movzx ecx, byte [esi]'; GenLine(GenText) end;
  begin GenText := '    inc esi'; GenLine(GenText) end;
  begin GenText := '.w_loop:'; GenLine(GenText) end;
  begin GenText := '    test ecx, ecx'; GenLine(GenText) end;
  begin GenText := '    jz near .w_done'; GenLine(GenText) end;
  begin GenText := '    mov al, [esi]'; GenLine(GenText) end;
  begin GenText := '    call serial_putc'; GenLine(GenText) end;
  begin GenText := '    inc esi'; GenLine(GenText) end;
  begin GenText := '    dec ecx'; GenLine(GenText) end;
  begin GenText := '    jmp near .w_loop'; GenLine(GenText) end;
  begin GenText := '.w_done:'; GenLine(GenText) end;
  begin GenText := '    ret'; GenLine(GenText) end
end;

procedure GenEmitCall(CallNameOffset, CallNameLength, Builtin: integer);
var
  Offsets: array[1..HeaderParamMax] of integer;
  I, Duplicates, OriginalBytes, CleanupBytes, RoutineIndex: integer;
begin
  OriginalBytes := 0;
  I := ArgCount;
  while I >= 1 do
  begin
    Offsets[I] := OriginalBytes;
    OriginalBytes := OriginalBytes + 4 + ArgTemp[I];
    I := I - 1
  end;

  if Builtin = 13 then
  begin
    GenFlags[4] := true; GenFlags[3] := true;
    GenText := '    mov ebx, [esp]'; GenLine(GenText);
    GenText := '    add esp, '; GenPut(GenText); GenInt(OriginalBytes); GenChar(chr(10));
    GenText := '    sub esp, 256'; GenLine(GenText);
    GenText := '    mov edi, esp'; GenLine(GenText);
    GenText := '    push edi'; GenLine(GenText);
    GenText := '    push ebx'; GenLine(GenText);
    GenText := '    call rtl_paramstr'; GenLine(GenText);
    GenText := '    add esp, 8'; GenLine(GenText);
    GenText := '    mov eax, esp'; GenLine(GenText);
    GenData[5] := 256
  end
  else
  begin
  Duplicates := 0;
  if ((Builtin = 2) or (Builtin = 3)) and (ArgCount = 1) then
  begin
    begin GenText := '    push dword 1'; GenLine(GenText) end;
    Duplicates := 1
  end;
  I := ArgCount;
  while I >= 1 do
  begin
    begin GenText := '    mov eax, [esp+'; GenPut(GenText) end;
    GenInt(Offsets[I] + Duplicates * 4);
    begin GenText := ']'; GenPut(GenText) end;
    GenChar(Chr(10));
    begin GenText := '    push eax'; GenLine(GenText) end;
    Duplicates := Duplicates + 1;
    I := I - 1
  end;

  if Builtin <> 0 then
  begin
    GenFlags[4] := true;
    if Builtin = 1 then begin GenText := '    call rtl_file_assign'; GenLine(GenText) end
    else if Builtin = 2 then begin GenText := '    call rtl_file_reset'; GenLine(GenText) end
    else if Builtin = 3 then begin GenText := '    call rtl_file_rewrite'; GenLine(GenText) end
    else if Builtin = 4 then begin GenText := '    call rtl_file_blockread'; GenLine(GenText) end
    else if Builtin = 5 then begin GenText := '    call rtl_file_blockwrite'; GenLine(GenText) end
    else if Builtin = 6 then begin GenText := '    call rtl_file_close'; GenLine(GenText) end
    else if Builtin = 7 then begin GenText := '    call rtl_file_erase'; GenLine(GenText) end
    else if Builtin = 8 then begin GenText := '    call rtl_file_rename'; GenLine(GenText) end
    else if Builtin = 9 then begin GenText := '    call rtl_ioresult'; GenLine(GenText) end
    else if Builtin = 10 then begin GenText := '    call rtl_readkey'; GenLine(GenText) end
    else if Builtin = 11 then begin GenText := '    call rtl_file_attributes'; GenLine(GenText) end
    else if Builtin = 12 then begin GenText := '    call rtl_paramcount'; GenLine(GenText) end
    else if Builtin = 13 then begin GenText := '    call rtl_paramstr'; GenLine(GenText) end
    else if Builtin = 14 then begin GenText := '    call rtl_screen_page'; GenLine(GenText) end
    else if Builtin = 15 then begin GenText := '    call rtl_execute'; GenLine(GenText) end
    else if Builtin = 16 then begin GenText := '    call rtl_halt'; GenLine(GenText) end
    else if Builtin = 17 then begin GenText := '    call rtl_dos_memory'; GenLine(GenText) end
    else GenFail(TokLine, TokCol, 4)
  end
  else
  begin
    RoutineIndex := FindRoutine(CallNameOffset, CallNameLength);
    if RoutineIndex = 0 then
      GenFail(TokLine, TokCol, 4)
    else
    begin
      begin GenText := '    call pf_'; GenPut(GenText) end;
      PoolGet(SymNameOffset[RoutineIndex],
              PoolLengthAt(SymNameOffset[RoutineIndex]), DumpText);
      GenPut(DumpText);
      GenChar(Chr(10))
    end
  end;
  CleanupBytes := OriginalBytes + Duplicates * 4;
  if CleanupBytes > 0 then
  begin
    begin GenText := '    add esp, '; GenPut(GenText) end;
    GenInt(CleanupBytes);
    GenChar(Chr(10))
  end;
  { TP's $I+: every I/O call except Assign is checked where it stands. }
  if ((Builtin >= 2) and (Builtin <= 8)) or (Builtin = 14) then
    if IoChecks then
      begin GenText := '    call rtl_io_check'; GenLine(GenText) end
  end
end;

procedure ParseExpression(); forward;
procedure ParseStatement(); forward;
procedure ParseBlock(); forward;

procedure ParseArrayBound();
var
  Negative: boolean;
  BoundLine, BoundCol, BoundOffset, BoundLength, Found: integer;
begin
  TraceEnter(PrArrayBound);
  Negative := false;
  ParsedBound := 0;
  BoundLine := TokLine;
  BoundCol := TokCol;
  BoundOffset := TokOffset;
  BoundLength := TokLength;
  if not ParseFailed then
  begin
    if TokKind = TkMinus then
    begin
      Negative := true;
      NextToken()
    end;
    if TokKind = TkInteger then
    begin
      ParsedBound := TokValue;
      if Negative then ParsedBound := -ParsedBound;
      NextToken()
    end
    else if not Negative then
    begin
      if TokKind = TkIdent then
      begin
        if TypeMode then
        begin
          Found := FindGlobalData(TokOffset, TokLength);
          if Found = 0 then
            FailType(TeBoundForm, TokLine, TokCol, TokOffset, TokLength, 0, 0)
          else if SymKind[Found] <> SkConst then
            FailType(TeBoundForm, TokLine, TokCol, TokOffset, TokLength, 0, 0)
          else if SymType[Found] <> TyInteger then
            FailType(TeBoundForm, TokLine, TokCol, TokOffset, TokLength, 0, 0)
          else
            ParsedBound := SymRef[Found]
        end;
        if not ParseFailed then NextToken()
      end
      else
      begin
        if TypeMode then
          FailType(TeBoundForm, BoundLine, BoundCol, BoundOffset, BoundLength, 0, 0)
        else
          FailParse(ExpArrayBound)
      end
    end
    else
    begin
      if TypeMode then
        FailType(TeBoundForm, BoundLine, BoundCol, BoundOffset, BoundLength, 0, 0)
      else
        FailParse(ExpArrayBound)
    end
  end;
  if not ParseFailed then TraceExit(PrArrayBound)
end;

procedure ParseTypeName();
var
  TypeLine, TypeCol, TypeOffset, TypeLength, RecordIndex: integer;
begin
  TraceEnter(PrTypeName);
  ParsedType := TyUnknown;
  ParsedRef := 0;
  ParsedStrCap := 0;
  TypeLine := TokLine;
  TypeCol := TokCol;
  TypeOffset := TokOffset;
  TypeLength := TokLength;
  if not ParseFailed then
  begin
    if TokKind = TkIntegerType then begin ParsedType := TyInteger; NextToken() end
    else if TokKind = TkBoolean then begin ParsedType := TyBoolean; NextToken() end
    else if TokKind = TkChar then begin ParsedType := TyChar; NextToken() end
    else if TokKind = TkFile then begin ParsedType := TyFile; NextToken() end
    else if TokKind = TkIdent then
    begin
      if TypeMode then
      begin
        RecordIndex := FindRecordType(TokOffset, TokLength);
        if RecordIndex = 0 then
          FailType(TeUnknownType, TokLine, TokCol, TokOffset, TokLength, 0, 0)
        else
        begin
          ParsedType := TyRecord;
          ParsedRef := RecordIndex
        end
      end;
      if not ParseFailed then NextToken()
    end
    else if TokKind = TkStringType then
    begin
      ParsedType := TyString;
      ParsedStrCap := 255;
      NextToken();
      if TokKind = TkLBracket then
      begin
        NextToken();
        ParseArrayBound();
        if not ParseFailed then
        begin
          ParsedStrCap := ParsedBound;
          if TypeMode then
            if (ParsedStrCap < 1) or (ParsedStrCap > 255) then
              FailType(TeRule, TypeLine, TypeCol, TypeOffset,
                       TypeLength, 0, 0)
        end;
        if not ParseFailed then ExpectToken(TkRBracket, TkRBracket)
      end
    end
    else
      FailParse(ExpTypeName)
  end;
  if not ParseFailed then TraceExit(PrTypeName)
end;

procedure ParseResultType();
begin
  TraceEnter(PrResultType);
  ParsedType := TyUnknown;
  ParsedRef := 0;
  ParsedStrCap := 0;
  if not ParseFailed then
  begin
    if TokKind = TkIntegerType then begin ParsedType := TyInteger; NextToken() end
    else if TokKind = TkBoolean then begin ParsedType := TyBoolean; NextToken() end
    else if TokKind = TkChar then begin ParsedType := TyChar; NextToken() end
    else FailParse(ExpResultType)
  end;
  if not ParseFailed then TraceExit(PrResultType)
end;

procedure ParseCall();
var
  Done: boolean;
  StartLine, StartCol, CallNameOffset, CallNameLength: integer;
  CallNameLine, CallNameCol, OuterArgCount, OuterFileBuiltin, I: integer;
  Builtin, RoutineIndex, ParamIndex, ThisTemp: integer;
  SavedArgType, SavedArgLvalue, SavedArgForm, SavedArgSym: array[1..HeaderParamMax] of integer;
  SavedArgLine, SavedArgCol: array[1..HeaderParamMax] of integer;
  SavedArgTemp: array[1..HeaderParamMax] of integer;
  ThisWantsValue, SavedWantAddress: boolean;
begin
  TraceEnter(PrCall);
  CallNameOffset := PendingIdentOffset;
  CallNameLength := PendingIdentLength;
  CallNameLine := PendingIdentLine;
  CallNameCol := PendingIdentCol;
  ThisWantsValue := CallWantsValue;
  OuterArgCount := ArgCount;
  OuterFileBuiltin := ParsingFileBuiltin;
  I := 1;
  while I <= OuterArgCount do
  begin
    SavedArgType[I] := ArgType[I];
    SavedArgLvalue[I] := ArgLvalue[I];
    SavedArgForm[I] := ArgForm[I];
    SavedArgSym[I] := ArgSym[I];
    SavedArgLine[I] := ArgLine[I];
    SavedArgCol[I] := ArgCol[I];
    SavedArgTemp[I] := ArgTemp[I];
    I := I + 1
  end;
  ArgCount := 0;
  ParsingFileBuiltin := 0;
  if TypeMode then
    ParsingFileBuiltin := FileBuiltinId(CallNameOffset, CallNameLength);
  Builtin := ParsingFileBuiltin;
  RoutineIndex := 0;
  if Builtin = 0 then
    RoutineIndex := FindRoutine(CallNameOffset, CallNameLength);
  if not ParseFailed then ExpectToken(TkLParen, TkLParen);
  if not ParseFailed then
  begin
    if TokKind <> TkRParen then
    begin
      Done := false;
      while not Done do
      begin
        StartLine := TokLine;
        StartCol := TokCol;
        SavedWantAddress := GenFlags[5];
        GenFlags[5] := false;
        if GenFlags[1] then
        begin
          I := ArgCount + 1;
          if Builtin <> 0 then
          begin
            if (I = 1) and ((Builtin <= 8) or (Builtin = 11)) then GenFlags[5] := true
            else if (Builtin = 14) and (I = 2) then GenFlags[5] := true
            else if ((Builtin = 4) or (Builtin = 5)) and
                    ((I = 2) or (I = 4)) then
              GenFlags[5] := true
          end
          else if RoutineIndex > 0 then
          begin
            ParamIndex := RoutineParamAt(RoutineIndex, I);
            if ParamIndex > 0 then
              if SymKind[ParamIndex] = SkVarParam then
                GenFlags[5] := true
          end
        end;
        ParseExpression();
        GenFlags[5] := SavedWantAddress;
        ThisTemp := GenData[5];
        if not ParseFailed then
          if TypeMode then
          begin
            if ArgCount >= HeaderParamMax then
              FailType(TeRule, StartLine, StartCol, 0, 0, 0, 0)
            else
            begin
              ArgCount := ArgCount + 1;
              ArgType[ArgCount] := LastExprType;
              ArgLvalue[ArgCount] := 0;
              if LastExprLvalue then ArgLvalue[ArgCount] := 1;
              ArgForm[ArgCount] := LastExprForm;
              ArgSym[ArgCount] := LastExprSym;
              ArgLine[ArgCount] := StartLine;
              ArgCol[ArgCount] := StartCol
            end
          end;
        if not ParseFailed then
          if GenFlags[1] then
          begin
            if ((Builtin = 1) or (Builtin = 8)) and (ArgCount = 2) then
              if LastExprType = TyChar then GenCoerceChar(ThisTemp);
            ArgTemp[ArgCount] := ThisTemp;
            begin GenText := '    push eax'; GenLine(GenText) end
          end;
        if ParseFailed then
          Done := true
        else if TokKind = TkComma then
          NextToken()
        else
          Done := true
      end
    end
  end;
  if not ParseFailed then ExpectToken(TkRParen, TkRParen);
  if not ParseFailed then
    if TypeMode then
    begin
      PendingIdentOffset := CallNameOffset;
      PendingIdentLength := CallNameLength;
      PendingIdentLine := CallNameLine;
      PendingIdentCol := CallNameCol;
      CallWantsValue := ThisWantsValue;
      ValidateParsedCall();
      if not ParseFailed then
        if GenFlags[1] then
          GenEmitCall(CallNameOffset, CallNameLength, Builtin)
    end;
  ArgCount := OuterArgCount;
  I := 1;
  while I <= OuterArgCount do
  begin
    ArgType[I] := SavedArgType[I];
    ArgLvalue[I] := SavedArgLvalue[I];
    ArgForm[I] := SavedArgForm[I];
    ArgSym[I] := SavedArgSym[I];
    ArgLine[I] := SavedArgLine[I];
    ArgCol[I] := SavedArgCol[I];
    ArgTemp[I] := SavedArgTemp[I];
    I := I + 1
  end;
  ParsingFileBuiltin := OuterFileBuiltin;
  if not ParseFailed then TraceExit(PrCall)
end;

procedure ParseIndexSuffix();
begin
  TraceEnter(PrIndexSuffix);
  if not ParseFailed then ExpectToken(TkLBracket, TkLBracket);
  if not ParseFailed then ParseExpression();
  if not ParseFailed then ExpectToken(TkRBracket, TkRBracket);
  if not ParseFailed then TraceExit(PrIndexSuffix)
end;

procedure ParseFieldSuffix();
begin
  TraceEnter(PrFieldSuffix);
  if not ParseFailed then ExpectToken(TkDot, TkDot);
  if not ParseFailed then
  begin
    if TokKind = TkIdent then
    begin
      ParsedFieldOffset := TokOffset;
      ParsedFieldLength := TokLength;
      ParsedFieldLine := TokLine;
      ParsedFieldCol := TokCol;
      NextToken()
    end
    else FailParse(ExpFieldName)
  end;
  if not ParseFailed then TraceExit(PrFieldSuffix)
end;

procedure ParseFactor();
var
  OpLine, OpCol, Found, BaseType, BaseSym, BaseRecord, BaseStrCap: integer;
  IndexType, FieldIndex, LiteralValue, OperandTemp: integer;
  BaseLvalue, BaseWasString, HadIndex, SavedWantAddress: boolean;
  LiteralText: string;
begin
  TraceEnter(PrFactor);
  if TypeMode then ResetLastExpression();
  if not ParseFailed then
  begin
    if TokKind = TkMinus then
    begin
      OpLine := TokLine;
      OpCol := TokCol;
      NextToken();
      ParseFactor();
      OperandTemp := GenData[5];
      if not ParseFailed then
        if TypeMode then
        begin
          if LastExprType <> TyInteger then
            FailType(TeRule, OpLine, OpCol, 0, 0, 0, 0)
          else
            MarkComputed(TyInteger)
        end;
      if not ParseFailed then
        if GenFlags[1] then
        begin
          begin GenText := '    neg eax'; GenLine(GenText) end;
          GenData[5] := OperandTemp
        end
    end
    else if TokKind = TkNot then
    begin
      OpLine := TokLine;
      OpCol := TokCol;
      NextToken();
      ParseFactor();
      OperandTemp := GenData[5];
      if not ParseFailed then
        if TypeMode then
        begin
          if LastExprType <> TyBoolean then
            FailType(TeRule, OpLine, OpCol, 0, 0, 0, 0)
          else
            MarkComputed(TyBoolean)
        end;
      if not ParseFailed then
        if GenFlags[1] then
        begin
          begin GenText := '    xor eax, 1'; GenLine(GenText) end;
          GenData[5] := OperandTemp
        end
    end
    else if TokKind = TkTrue then
    begin
      if TypeMode then MarkComputed(TyBoolean);
      if GenFlags[1] then begin GenText := '    mov eax, 1'; GenLine(GenText) end;
      NextToken()
    end
    else if TokKind = TkFalse then
    begin
      if TypeMode then MarkComputed(TyBoolean);
      if GenFlags[1] then begin GenText := '    mov eax, 0'; GenLine(GenText) end;
      NextToken()
    end
    else if TokKind = TkInteger then
    begin
      LiteralValue := TokValue;
      if TypeMode then MarkComputed(TyInteger);
      if GenFlags[1] then
      begin
        begin GenText := '    mov eax, '; GenPut(GenText) end;
        GenInt(LiteralValue);
        GenChar(Chr(10))
      end;
      NextToken()
    end
    else if TokKind = TkString then
    begin
      LiteralText := TokText;
      if TypeMode then
      begin
        if TokLength = 1 then MarkComputed(TyChar)
        else MarkComputed(TyString)
      end;
      if GenFlags[1] then
      begin
        if TokLength = 1 then
        begin
          begin GenText := '    mov eax, '; GenPut(GenText) end;
          GenInt(Ord(TokText[1]));
          GenChar(Chr(10))
        end
        else
        begin
          GenInlineString(LiteralText, true);
          GenFlags[3] := true
        end
      end;
      NextToken()
    end
    else if TokKind = TkOrd then
    begin
      OpLine := TokLine;
      OpCol := TokCol;
      NextToken();
      if not ParseFailed then ExpectToken(TkLParen, TkLParen);
      if not ParseFailed then ParseExpression();
      OperandTemp := GenData[5];
      if not ParseFailed then ExpectToken(TkRParen, TkRParen);
      if not ParseFailed then
        if TypeMode then
        begin
          if (LastExprType <> TyInteger) and (LastExprType <> TyBoolean) and
             (LastExprType <> TyChar) then
            FailType(TeRule, OpLine, OpCol, 0, 0, 0, 0)
          else
            MarkComputed(TyInteger)
        end;
      if not ParseFailed then
        if GenFlags[1] then GenData[5] := OperandTemp
    end
    else if TokKind = TkChr then
    begin
      OpLine := TokLine;
      OpCol := TokCol;
      NextToken();
      if not ParseFailed then ExpectToken(TkLParen, TkLParen);
      if not ParseFailed then ParseExpression();
      OperandTemp := GenData[5];
      if not ParseFailed then ExpectToken(TkRParen, TkRParen);
      if not ParseFailed then
        if TypeMode then
        begin
          if LastExprType <> TyInteger then
            FailType(TeRule, OpLine, OpCol, 0, 0, 0, 0)
          else
            MarkComputed(TyChar)
        end;
      if not ParseFailed then
        if GenFlags[1] then
        begin
          begin GenText := '    and eax, 0xFF'; GenLine(GenText) end;
          GenData[5] := OperandTemp
        end
    end
    else if TokKind = TkLength then
    begin
      OpLine := TokLine;
      OpCol := TokCol;
      NextToken();
      if not ParseFailed then ExpectToken(TkLParen, TkLParen);
      if not ParseFailed then ParseExpression();
      OperandTemp := GenData[5];
      if not ParseFailed then ExpectToken(TkRParen, TkRParen);
      if not ParseFailed then
        if TypeMode then
        begin
          if LastExprType <> TyString then
            FailType(TeRule, OpLine, OpCol, 0, 0, 0, 0)
          else
            MarkComputed(TyInteger)
        end;
      if not ParseFailed then
        if GenFlags[1] then
        begin
          begin GenText := '    movzx eax, byte [eax]'; GenLine(GenText) end;
          if OperandTemp > 0 then
          begin
            begin GenText := '    add esp, '; GenPut(GenText) end;
            GenInt(OperandTemp);
            GenChar(Chr(10))
          end;
          GenData[5] := 0
        end
    end
    else if TokKind = TkIdent then
    begin
      PendingIdentOffset := TokOffset;
      PendingIdentLength := TokLength;
      PendingIdentLine := TokLine;
      PendingIdentCol := TokCol;
      NextToken();
      if TokKind = TkLParen then
      begin
        CallWantsValue := true;
        ParseCall()
      end
      else
      begin
        Found := 0;
        if TypeMode then
        begin
          Found := ResolveName(PendingIdentOffset, PendingIdentLength);
          if Found = 0 then
            FailType(TeRule, PendingIdentLine, PendingIdentCol,
                     PendingIdentOffset, PendingIdentLength, 0, 0)
          else
          begin
            LastExprType := SymType[Found];
            LastExprSym := Found;
            LastExprRecord := SymRef[Found];
            LastExprStrCap := 0;
            if SymType[Found] = TyString then
            begin
              LastExprStrCap := SymHi[Found];
              if SymKind[Found] = SkConst then LastExprStrCap := 255
            end;
            LastExprForm := 1;
            LastExprLvalue := SymKind[Found] <> SkConst
          end
        end;
        if not ParseFailed then
          if GenFlags[1] then
          begin
            if SymKind[Found] = SkConst then
            begin
              if SymType[Found] = TyString then
              begin
                PoolGet(SymRef[Found], SymSize[Found], LiteralText);
                GenInlineString(LiteralText, true);
                GenFlags[3] := true
              end
              else
              begin
                begin GenText := '    mov eax, '; GenPut(GenText) end;
                GenInt(SymRef[Found]);
                GenChar(Chr(10))
              end
            end
            else
              GenDesignatorAddress(Found)
          end;
        BaseWasString := false;
        HadIndex := false;
        if not ParseFailed then
          if TokKind = TkLBracket then
          begin
            BaseType := LastExprType;
            BaseSym := LastExprSym;
            BaseRecord := LastExprRecord;
            BaseStrCap := LastExprStrCap;
            BaseLvalue := LastExprLvalue;
            BaseWasString := BaseType = TyString;
            HadIndex := true;
            if GenFlags[1] then begin GenText := '    push eax'; GenLine(GenText) end;
            SavedWantAddress := GenFlags[5];
            GenFlags[5] := false;
            ParseIndexSuffix();
            GenFlags[5] := SavedWantAddress;
            if not ParseFailed then
              if GenFlags[1] then GenApplyIndex(BaseSym, BaseType);
            if not ParseFailed then
              if TypeMode then
              begin
                IndexType := LastExprType;
                if IndexType <> TyInteger then
                  FailType(TeRule, PendingIdentLine, PendingIdentCol,
                           PendingIdentOffset, PendingIdentLength, 0, 0)
                else if BaseType = TyString then
                begin
                  LastExprType := TyChar;
                  LastExprRecord := 0;
                  LastExprStrCap := BaseStrCap;
                  LastExprSym := BaseSym;
                  LastExprLvalue := BaseLvalue;
                  LastExprForm := 2
                end
                else if (SymKind[BaseSym] = SkArray) or
                        (SymKind[BaseSym] = SkLocalArray) then
                begin
                  LastExprType := BaseType;
                  LastExprRecord := BaseRecord;
                  LastExprStrCap := 0;
                  LastExprSym := BaseSym;
                  LastExprLvalue := BaseLvalue;
                  LastExprForm := 2
                end
                else
                  FailType(TeRule, PendingIdentLine, PendingIdentCol,
                           PendingIdentOffset, PendingIdentLength, 0, 0)
              end
          end;
        if not ParseFailed then
          if TokKind = TkDot then
          begin
            BaseType := LastExprType;
            BaseRecord := LastExprRecord;
            BaseSym := LastExprSym;
            BaseLvalue := LastExprLvalue;
            ParseFieldSuffix();
            if not ParseFailed then
              if GenFlags[1] then
              begin
                FieldIndex := FindRecordField(BaseRecord, ParsedFieldOffset,
                                              ParsedFieldLength);
                GenApplyField(BaseSym, FieldIndex, HadIndex)
              end;
            if not ParseFailed then
              if TypeMode then
              begin
                if BaseType <> TyRecord then
                  FailType(TeRule, ParsedFieldLine, ParsedFieldCol,
                           ParsedFieldOffset, ParsedFieldLength, 0, 0)
                else
                begin
                  FieldIndex := FindRecordField(BaseRecord, ParsedFieldOffset,
                                                ParsedFieldLength);
                  if FieldIndex = 0 then
                    FailType(TeRule, ParsedFieldLine, ParsedFieldCol,
                             ParsedFieldOffset, ParsedFieldLength, 0, 0)
                  else
                  begin
                    LastExprType := SymType[FieldIndex];
                    LastExprRecord := 0;
                    LastExprStrCap := 0;
                    LastExprSym := BaseSym;
                    LastExprLvalue := BaseLvalue;
                    LastExprForm := 3
                  end
                end
              end
          end;
        if not ParseFailed then
          if TypeMode then
          begin
            if (SymKind[Found] = SkArray) or (SymKind[Found] = SkLocalArray) then
            begin
              if LastExprForm = 1 then
                FailType(TeRule, PendingIdentLine, PendingIdentCol,
                         PendingIdentOffset, PendingIdentLength, 0, 0)
            end;
            if not ParseFailed then
              if SymType[Found] = TyFile then
                if LastExprForm = 1 then
                  if not ((ParsingFileBuiltin <> 0) and (ArgCount = 0)) then
                    FailType(TeRule, PendingIdentLine, PendingIdentCol,
                             PendingIdentOffset, PendingIdentLength, 0, 0)
          end
        ;
        if not ParseFailed then
          if GenFlags[1] then
            if SymKind[Found] <> SkConst then
              if not GenFlags[5] then
              begin
                if LastExprType = TyChar then
                  begin GenText := '    movzx eax, byte [eax]'; GenLine(GenText) end
                else if (LastExprType = TyInteger) or
                        (LastExprType = TyBoolean) then
                  begin GenText := '    mov eax, [eax]'; GenLine(GenText) end
              end
      end
    end
    else if TokKind = TkLParen then
    begin
      NextToken();
      ParseExpression();
      if not ParseFailed then ExpectToken(TkRParen, TkRParen)
    end
    else
      FailParse(ExpFactor)
  end;
  if not ParseFailed then TraceExit(PrFactor)
end;

function IsRelationalOperator(): boolean;
begin
  IsRelationalOperator := false;
  if TokKind = TkEq then IsRelationalOperator := true
  else if TokKind = TkNe then IsRelationalOperator := true
  else if TokKind = TkLt then IsRelationalOperator := true
  else if TokKind = TkLe then IsRelationalOperator := true
  else if TokKind = TkGt then IsRelationalOperator := true
  else if TokKind = TkGe then IsRelationalOperator := true
end;

procedure ParseTerm();
var
  Done: boolean;
  LeftType, OperatorKind, OperatorLine, OperatorCol, LeftTemp: integer;
begin
  TraceEnter(PrTerm);
  ParseFactor();
  Done := ParseFailed;
  while not Done do
  begin
    if TokKind = TkStar then begin { TURBO_PARSE_MUT_PRECEDENCE }
      LeftType := LastExprType;
      OperatorKind := TokKind;
      OperatorLine := TokLine;
      OperatorCol := TokCol;
      LeftTemp := GenData[5];
      if GenFlags[1] then begin GenText := '    push eax'; GenLine(GenText) end;
      NextToken();
      ParseFactor();
      if not ParseFailed then
        if TypeMode then
        begin
          if (LeftType <> TyInteger) or (LastExprType <> TyInteger) then
            FailType(TeRule, OperatorLine, OperatorCol, 0, 0, 0, 0)
          else
            MarkComputed(TyInteger)
        end;
      if not ParseFailed then
        if GenFlags[1] then GenScalarBinary(OperatorKind)
    end
    else if TokKind = TkDiv then
    begin
      LeftType := LastExprType;
      OperatorKind := TokKind;
      OperatorLine := TokLine;
      OperatorCol := TokCol;
      LeftTemp := GenData[5];
      if GenFlags[1] then begin GenText := '    push eax'; GenLine(GenText) end;
      NextToken();
      ParseFactor();
      if not ParseFailed then
        if TypeMode then
        begin
          if (LeftType <> TyInteger) or (LastExprType <> TyInteger) then
            FailType(TeRule, OperatorLine, OperatorCol, 0, 0, 0, 0)
          else
            MarkComputed(TyInteger)
        end;
      if not ParseFailed then
        if GenFlags[1] then GenScalarBinary(OperatorKind)
    end
    else if TokKind = TkMod then
    begin
      LeftType := LastExprType;
      OperatorKind := TokKind;
      OperatorLine := TokLine;
      OperatorCol := TokCol;
      LeftTemp := GenData[5];
      if GenFlags[1] then begin GenText := '    push eax'; GenLine(GenText) end;
      NextToken();
      ParseFactor();
      if not ParseFailed then
        if TypeMode then
        begin
          if (LeftType <> TyInteger) or (LastExprType <> TyInteger) then
            FailType(TeRule, OperatorLine, OperatorCol, 0, 0, 0, 0)
          else
            MarkComputed(TyInteger)
        end;
      if not ParseFailed then
        if GenFlags[1] then GenScalarBinary(OperatorKind)
    end
    else if TokKind = TkAnd then
    begin
      LeftType := LastExprType;
      OperatorKind := TokKind;
      OperatorLine := TokLine;
      OperatorCol := TokCol;
      LeftTemp := GenData[5];
      if GenFlags[1] then begin GenText := '    push eax'; GenLine(GenText) end;
      NextToken();
      ParseFactor();
      if not ParseFailed then
        if TypeMode then
        begin
          if (LeftType <> TyBoolean) or (LastExprType <> TyBoolean) then
            FailType(TeRule, OperatorLine, OperatorCol, 0, 0, 0, 0)
          else
            MarkComputed(TyBoolean)
        end;
      if not ParseFailed then
        if GenFlags[1] then GenScalarBinary(OperatorKind)
    end
    else
      Done := true;
    if ParseFailed then Done := true
  end;
  if not ParseFailed then TraceExit(PrTerm)
end;

procedure ParseSimpleExpression();
var
  Done: boolean;
  LeftType, RightType, OperatorKind, OperatorLine, OperatorCol: integer;
  LeftTemp, RightTemp: integer;
  LeftIsText, RightIsText: boolean;
begin
  TraceEnter(PrSimpleExpression);
  ParseTerm();
  Done := ParseFailed;
  while not Done do
  begin
    if TokKind = TkPlus then
    begin
      LeftType := LastExprType;
      OperatorKind := TokKind;
      OperatorLine := TokLine;
      OperatorCol := TokCol;
      LeftTemp := GenData[5];
      if GenFlags[1] then
      begin
        if LeftType = TyChar then GenCoerceChar(LeftTemp);
        begin GenText := '    push eax'; GenLine(GenText) end
      end;
      NextToken();
      ParseTerm();
      RightType := LastExprType;
      RightTemp := GenData[5];
      if not ParseFailed then
        if TypeMode then
        begin
          if (LeftType = TyString) or (LastExprType = TyString) then
          begin
            LeftIsText := (LeftType = TyString) or (LeftType = TyChar);
            RightIsText := (LastExprType = TyString) or
                           (LastExprType = TyChar);
            if LeftIsText then
            begin
              if RightIsText then MarkComputed(TyString)
              else FailType(TeRule, OperatorLine, OperatorCol, 0, 0, 0, 0)
            end
            else
              FailType(TeRule, OperatorLine, OperatorCol, 0, 0, 0, 0)
          end
          else if (LeftType <> TyInteger) or (LastExprType <> TyInteger) then
            FailType(TeRule, OperatorLine, OperatorCol, 0, 0, 0, 0)
          else
            MarkComputed(TyInteger)
        end;
      if not ParseFailed then
        if GenFlags[1] then
        begin
          if (LeftType = TyString) or (RightType = TyString) then
          begin
            if RightType = TyChar then GenCoerceChar(RightTemp);
            GenConcat(LeftTemp, RightTemp)
          end
          else
            GenScalarBinary(OperatorKind)
        end
    end
    else if TokKind = TkMinus then
    begin
      LeftType := LastExprType;
      OperatorKind := TokKind;
      OperatorLine := TokLine;
      OperatorCol := TokCol;
      LeftTemp := GenData[5];
      if GenFlags[1] then begin GenText := '    push eax'; GenLine(GenText) end;
      NextToken();
      ParseTerm();
      if not ParseFailed then
        if TypeMode then
        begin
          if (LeftType <> TyInteger) or (LastExprType <> TyInteger) then
            FailType(TeRule, OperatorLine, OperatorCol, 0, 0, 0, 0)
          else
            MarkComputed(TyInteger)
        end;
      if not ParseFailed then
        if GenFlags[1] then GenScalarBinary(OperatorKind)
    end
    else if TokKind = TkOr then
    begin
      LeftType := LastExprType;
      OperatorKind := TokKind;
      OperatorLine := TokLine;
      OperatorCol := TokCol;
      LeftTemp := GenData[5];
      if GenFlags[1] then begin GenText := '    push eax'; GenLine(GenText) end;
      NextToken();
      ParseTerm();
      if not ParseFailed then
        if TypeMode then
        begin
          if (LeftType <> TyBoolean) or (LastExprType <> TyBoolean) then
            FailType(TeRule, OperatorLine, OperatorCol, 0, 0, 0, 0)
          else
            MarkComputed(TyBoolean)
        end;
      if not ParseFailed then
        if GenFlags[1] then GenScalarBinary(OperatorKind)
    end
    else
      Done := true;
    if ParseFailed then Done := true
  end;
  if not ParseFailed then TraceExit(PrSimpleExpression)
end;

procedure ParseExpression();
var
  LeftType, RightType, OperatorKind, OperatorLine, OperatorCol: integer;
  LeftTemp, RightTemp: integer;
  LeftIsText, RightIsText: boolean;
begin
  TraceEnter(PrExpression);
  ParseSimpleExpression();
  if not ParseFailed then
  begin
    if IsRelationalOperator() then
    begin
      LeftType := LastExprType;
      LeftTemp := GenData[5];
      OperatorKind := TokKind;
      OperatorLine := TokLine;
      OperatorCol := TokCol;
      if GenFlags[1] then begin GenText := '    push eax'; GenLine(GenText) end;
      NextToken();
      ParseSimpleExpression();
      RightType := LastExprType;
      RightTemp := GenData[5];
      if not ParseFailed then
        if TypeMode then
        begin
          if (LeftType = TyRecord) or (LastExprType = TyRecord) then
            FailType(TeRule, OperatorLine, OperatorCol, 0, 0, 0, 0)
          else if (LeftType = TyString) or (LastExprType = TyString) then
          begin
            LeftIsText := (LeftType = TyString) or (LeftType = TyChar);
            RightIsText := (LastExprType = TyString) or
                           (LastExprType = TyChar);
            if LeftIsText then
            begin
              if RightIsText then MarkComputed(TyBoolean)
              else FailType(TeRule, OperatorLine, OperatorCol, 0, 0, 0, 0)
            end
            else
              FailType(TeRule, OperatorLine, OperatorCol, 0, 0, 0, 0)
          end
          else if LeftType <> LastExprType then
            FailType(TeRule, OperatorLine, OperatorCol, 0, 0, 0, 0)
          else
            MarkComputed(TyBoolean)
        end;
      if not ParseFailed then
        if GenFlags[1] then
        begin
          if (LeftType = TyString) or (RightType = TyString) then
          begin
            if (LeftType = TyChar) and (RightType = TyString) then
            begin
              begin GenText := '    sub esp, 4'; GenLine(GenText) end;
              begin GenText := '    mov ecx, [esp+'; GenPut(GenText) end;
              GenInt(4 + RightTemp);
              begin GenText := ']'; GenPut(GenText) end;
              GenChar(Chr(10));
              begin GenText := '    mov byte [esp], 1'; GenLine(GenText) end;
              begin GenText := '    mov byte [esp+1], cl'; GenLine(GenText) end;
              begin GenText := '    mov ecx, esp'; GenLine(GenText) end;
              begin GenText := '    mov [esp+'; GenPut(GenText) end;
              GenInt(4 + RightTemp);
              begin GenText := '], ecx'; GenPut(GenText) end;
              GenChar(Chr(10));
              RightTemp := RightTemp + 4;
              GenFlags[3] := true
            end;
            if (RightType = TyChar) and (LeftType = TyString) then
              GenCoerceChar(RightTemp);
            GenStringCompare(OperatorKind, LeftTemp, RightTemp)
          end
          else
            GenScalarBinary(OperatorKind)
        end;
      if not ParseFailed then
        if IsRelationalOperator() then
          FailParse(ExpNoChainedRelation)
    end
  end;
  if not ParseFailed then TraceExit(PrExpression)
end;

procedure ParseAssignment();
var
  TargetSym, TargetType, TargetRecord, TargetForm, ValueType, ValueRecord: integer;
  ValueTemp, FieldIndex, I, OffsetValue: integer;
  TargetLvalue, WholeString, TargetHadIndex: boolean;
  SavedWantAddress: boolean;
begin
  TraceEnter(PrAssignment);
  TargetSym := 0;
  TargetType := TyUnknown;
  TargetRecord := 0;
  TargetForm := 1;
  TargetLvalue := false;
  WholeString := false;
  TargetHadIndex := false;
  if TypeMode then
  begin
    TargetSym := ResolveName(PendingIdentOffset, PendingIdentLength);
    if TargetSym = 0 then
      FailType(TeRule, PendingIdentLine, PendingIdentCol,
               PendingIdentOffset, PendingIdentLength, 0, 0)
    else
    begin
      TargetType := SymType[TargetSym];
      TargetRecord := SymRef[TargetSym];
      TargetLvalue := SymKind[TargetSym] <> SkConst;
      if TargetType = TyFile then
        FailType(TeFileAssign, PendingIdentLine, PendingIdentCol,
                 PendingIdentOffset, PendingIdentLength, 0, 0)
      else if not TargetLvalue then
        FailType(TeRule, PendingIdentLine, PendingIdentCol,
                 PendingIdentOffset, PendingIdentLength, 0, 0)
    end
  end;
  if not ParseFailed then
    if GenFlags[1] then GenAssignmentAddress(TargetSym);
  if not ParseFailed then
    if TokKind = TkLBracket then
    begin
      TargetHadIndex := true;
      if GenFlags[1] then begin GenText := '    push eax'; GenLine(GenText) end;
      SavedWantAddress := GenFlags[5];
      GenFlags[5] := false;
      ParseIndexSuffix();
      GenFlags[5] := SavedWantAddress;
      if not ParseFailed then
        if GenFlags[1] then GenApplyIndex(TargetSym, TargetType);
      if not ParseFailed then
        if TypeMode then
        begin
          if LastExprType <> TyInteger then
            FailType(TeRule, PendingIdentLine, PendingIdentCol,
                     PendingIdentOffset, PendingIdentLength, 0, 0)
          else if TargetType = TyString then
          begin
            TargetType := TyChar;
            TargetRecord := 0;
            TargetForm := 2
          end
          else if (SymKind[TargetSym] = SkArray) or
                  (SymKind[TargetSym] = SkLocalArray) then
            TargetForm := 2
          else
            FailType(TeRule, PendingIdentLine, PendingIdentCol,
                     PendingIdentOffset, PendingIdentLength, 0, 0)
        end
    end;
  if not ParseFailed then
    if TokKind = TkDot then
    begin
      ParseFieldSuffix();
      if not ParseFailed then
        if GenFlags[1] then
        begin
          FieldIndex := FindRecordField(TargetRecord, ParsedFieldOffset,
                                        ParsedFieldLength);
          GenApplyField(TargetSym, FieldIndex, TargetHadIndex)
        end;
      if not ParseFailed then
        if TypeMode then
        begin
          if TargetType <> TyRecord then
            FailType(TeRule, ParsedFieldLine, ParsedFieldCol,
                     ParsedFieldOffset, ParsedFieldLength, 0, 0)
          else
          begin
            ValueRecord := FindRecordField(TargetRecord, ParsedFieldOffset,
                                           ParsedFieldLength);
            if ValueRecord = 0 then
              FailType(TeRule, ParsedFieldLine, ParsedFieldCol,
                       ParsedFieldOffset, ParsedFieldLength, 0, 0)
            else
            begin
              TargetType := SymType[ValueRecord];
              TargetRecord := 0;
              TargetForm := 3
            end
          end
        end
    end;
  if not ParseFailed then
    if TypeMode then
      if ((SymKind[TargetSym] = SkArray) or
          (SymKind[TargetSym] = SkLocalArray)) and
         (TargetForm = 1) then
        FailType(TeRule, PendingIdentLine, PendingIdentCol,
                 PendingIdentOffset, PendingIdentLength, 0, 0);
  if not ParseFailed then
    if GenFlags[1] then
    begin
      begin GenText := '    push eax'; GenLine(GenText) end
    end;
  if not ParseFailed then ExpectToken(TkAssign, TkAssign);
  WholeString := false;
  if not ParseFailed then
    if TypeMode then
      if SymType[TargetSym] = TyString then
        if TargetForm = 1 then WholeString := true;
  SavedWantAddress := GenFlags[5];
  if GenFlags[1] then
  begin
    GenFlags[5] := false;
    if TargetType = TyRecord then GenFlags[5] := true
  end;
  if not ParseFailed then ParseExpression();
  GenFlags[5] := SavedWantAddress;
  ValueTemp := GenData[5];
  if not ParseFailed then
    if TypeMode then
    begin
      ValueType := LastExprType;
      ValueRecord := LastExprRecord;
      if WholeString then
      begin
        if (ValueType <> TyString) and (ValueType <> TyChar) then
          FailType(TeRule, PendingIdentLine, PendingIdentCol,
                   PendingIdentOffset, PendingIdentLength, 0, 0)
      end
      else if TargetType = TyRecord then
      begin
        if ValueType <> TyRecord then
          FailType(TeRule, PendingIdentLine, PendingIdentCol,
                   PendingIdentOffset, PendingIdentLength, 0, 0)
        else if ValueRecord <> TargetRecord then
          FailType(TeRule, PendingIdentLine, PendingIdentCol,
                   PendingIdentOffset, PendingIdentLength, 0, 0)
      end
      else
      if ValueType <> TargetType then begin { TURBO_TYPE_MUT_ASSIGN }
        FailType(TeAssignMismatch, PendingIdentLine, PendingIdentCol,
                 PendingIdentOffset, PendingIdentLength, TargetType, ValueType)
      end
    end;
  if not ParseFailed then
    if GenFlags[1] then
    begin
      if WholeString then
      begin
        if LastExprType = TyChar then
        begin
          begin GenText := '    pop edx'; GenLine(GenText) end;
          begin GenText := '    mov byte [edx], 1'; GenLine(GenText) end;
          begin GenText := '    mov [edx+1], al'; GenLine(GenText) end
        end
        else
        begin
          begin GenText := '    mov esi, eax'; GenLine(GenText) end;
          begin GenText := '    mov edi, [esp+'; GenPut(GenText) end;
          GenInt(ValueTemp);
          begin GenText := ']'; GenPut(GenText) end;
          GenChar(Chr(10));
          begin GenText := '    mov ecx, '; GenPut(GenText) end;
          GenInt(SymHi[TargetSym]);
          GenChar(Chr(10));
          begin GenText := '    call __str_assign'; GenLine(GenText) end;
          begin GenText := '    add esp, '; GenPut(GenText) end;
          GenInt(ValueTemp + 4);
          GenChar(Chr(10));
          GenFlags[3] := true
        end
      end
      else if TargetType = TyRecord then
      begin
        begin GenText := '    mov edx, [esp]'; GenLine(GenText) end;
        I := 0;
        while I < SymSize[TargetRecord] div 4 do
        begin
          OffsetValue := I * 4;
          begin GenText := '    mov ecx, [eax'; GenPut(GenText) end;
          if OffsetValue > 0 then
          begin
            GenChar('+');
            GenInt(OffsetValue)
          end;
          begin GenText := ']'; GenPut(GenText) end;
          GenChar(Chr(10));
          begin GenText := '    mov [edx'; GenPut(GenText) end;
          if OffsetValue > 0 then
          begin
            GenChar('+');
            GenInt(OffsetValue)
          end;
          begin GenText := '], ecx'; GenPut(GenText) end;
          GenChar(Chr(10));
          I := I + 1
        end;
        begin GenText := '    add esp, 4'; GenLine(GenText) end
      end
      else
      begin
        begin GenText := '    pop edx'; GenLine(GenText) end;
        if TargetType = TyChar then
          begin GenText := '    mov [edx], al'; GenLine(GenText) end
        else
          begin GenText := '    mov [edx], eax'; GenLine(GenText) end
      end
    end;
  if not ParseFailed then TraceExit(PrAssignment)
end;

procedure ParseIdentStatement();
begin
  TraceEnter(PrIdentStatement);
  if not ParseFailed then
  begin
    PendingIdentOffset := TokOffset;
    PendingIdentLength := TokLength;
    PendingIdentLine := TokLine;
    PendingIdentCol := TokCol;
    ExpectToken(TkIdent, TkIdent)
  end;
  if not ParseFailed then
  begin
    if TokKind = TkLParen then
    begin
      CallWantsValue := false;
      ParseCall()
    end
    else ParseAssignment()
  end;
  if not ParseFailed then TraceExit(PrIdentStatement)
end;

procedure ParseWriteArg();
var
  LiteralText: string;
  ValueType, ValueTemp, LabelNumber: integer;
begin
  TraceEnter(PrWriteArg);
  if not ParseFailed then
  begin
    if TokKind = TkString then
    begin
      LiteralText := TokText;
      if GenFlags[1] then
      begin
        GenInlineString(LiteralText, false);
        begin GenText := '    call serial_puts'; GenLine(GenText) end
      end;
      NextToken()
    end
    else
    begin
      ParseExpression();
      ValueType := LastExprType;
      ValueTemp := GenData[5];
      if not ParseFailed then
        if TypeMode then
          if (LastExprType <> TyInteger) and
             (LastExprType <> TyBoolean) and
             (LastExprType <> TyChar) and
             (LastExprType <> TyString) then
            FailType(TeRule, TokLine, TokCol, 0, 0, 0, 0)
      ;
      if not ParseFailed then
        if GenFlags[1] then
        begin
          if ValueType = TyBoolean then
          begin
            LabelNumber := GenNewLabel();
            begin GenText := '    test eax, eax'; GenLine(GenText) end;
            begin GenText := '    jz near '; GenPut(GenText) end;
            begin GenText := '.Lbfalse_'; GenPut(GenText); GenInt(LabelNumber) end;
            GenChar(Chr(10));
            begin GenText := '    mov eax, str_bool_true'; GenLine(GenText) end;
            begin GenText := '    jmp near '; GenPut(GenText) end;
            begin GenText := '.Lbdone_'; GenPut(GenText); GenInt(LabelNumber) end;
            GenChar(Chr(10));
            begin GenText := '.Lbfalse_'; GenPut(GenText); GenInt(LabelNumber) end;
            begin GenText := ':'; GenPut(GenText) end;
            GenChar(Chr(10));
            begin GenText := '    mov eax, str_bool_false'; GenLine(GenText) end;
            begin GenText := '.Lbdone_'; GenPut(GenText); GenInt(LabelNumber) end;
            begin GenText := ':'; GenPut(GenText) end;
            GenChar(Chr(10));
            begin GenText := '    call serial_puts'; GenLine(GenText) end
          end
          else if ValueType = TyChar then
            begin GenText := '    call serial_putc'; GenLine(GenText) end
          else if ValueType = TyString then
          begin
            begin GenText := '    mov esi, eax'; GenLine(GenText) end;
            begin GenText := '    call __str_write'; GenLine(GenText) end;
            if ValueTemp > 0 then
            begin
              begin GenText := '    add esp, '; GenPut(GenText) end;
              GenInt(ValueTemp);
              GenChar(Chr(10))
            end;
            GenFlags[3] := true
          end
          else
            begin GenText := '    call serial_put_int'; GenLine(GenText) end
        end
    end
  end;
  if not ParseFailed then TraceExit(PrWriteArg)
end;

procedure ParseWrite();
var
  Done: boolean;
  IsNewline: boolean;
begin
  TraceEnter(PrWrite);
  IsNewline := TokKind = TkWriteln;
  if not ParseFailed then NextToken();
  if not ParseFailed then
  begin
    if TokKind = TkLParen then
    begin
      NextToken();
      if TokKind <> TkRParen then
      begin
        Done := false;
        while not Done do
        begin
          ParseWriteArg();
          if ParseFailed then
            Done := true
          else if TokKind = TkComma then
            NextToken()
          else
            Done := true
        end
      end;
      if not ParseFailed then ExpectToken(TkRParen, TkRParen)
    end
  end;
  if not ParseFailed then
    if GenFlags[1] then
      if IsNewline then
      begin
        begin GenText := '    mov al, 10'; GenLine(GenText) end;
        begin GenText := '    call serial_putc'; GenLine(GenText) end
      end;
  if not ParseFailed then TraceExit(PrWrite)
end;

procedure ParseIf();
var
  ConditionType, ConditionLine, ConditionCol, LabelNumber: integer;
begin
  TraceEnter(PrIf);
  LabelNumber := 0;
  if GenFlags[1] then LabelNumber := GenNewLabel();
  ParseIfDepth := ParseIfDepth + 1;
  if not ParseFailed then ExpectToken(TkIf, TkIf);
  ConditionLine := TokLine;
  ConditionCol := TokCol;
  if not ParseFailed then ParseExpression();
  ConditionType := LastExprType;
  if not ParseFailed then
    if TypeMode then
      if ConditionType <> TyBoolean then
        FailType(TeRule, ConditionLine, ConditionCol, 0, 0, 0, 0);
  if not ParseFailed then
    if GenFlags[1] then
    begin
      begin GenText := '    test eax, eax'; GenLine(GenText) end;
      begin GenText := '    jz near '; GenPut(GenText) end;
      begin GenText := '.Lelse_'; GenPut(GenText); GenInt(LabelNumber) end;
      GenChar(Chr(10))
    end;
  if not ParseFailed then ExpectToken(TkThen, TkThen);
  if not ParseFailed then ParseStatement();
  if not ParseFailed then
    if GenFlags[1] then
    begin
      begin GenText := '    jmp near '; GenPut(GenText) end;
      begin GenText := '.Lendif_'; GenPut(GenText); GenInt(LabelNumber) end;
      GenChar(Chr(10));
      begin GenText := '.Lelse_'; GenPut(GenText); GenInt(LabelNumber) end;
      begin GenText := ':'; GenPut(GenText) end;
      GenChar(Chr(10))
    end;
  if not ParseFailed then
  begin
    if TokKind = TkElse then begin { TURBO_PARSE_MUT_ELSE }
      TraceEnter(PrElseClause);
      NextToken();
      ParseStatement();
      if not ParseFailed then TraceExit(PrElseClause)
    end
  end;
  if not ParseFailed then
    if GenFlags[1] then
    begin
      begin GenText := '.Lendif_'; GenPut(GenText); GenInt(LabelNumber) end;
      begin GenText := ':'; GenPut(GenText) end;
      GenChar(Chr(10))
    end;
  ParseIfDepth := ParseIfDepth - 1;
  if not ParseFailed then TraceExit(PrIf)
end;

procedure ParseWhile();
var
  ConditionType, ConditionLine, ConditionCol, LabelNumber: integer;
begin
  TraceEnter(PrWhile);
  LabelNumber := 0;
  if GenFlags[1] then
  begin
    LabelNumber := GenNewLabel(); { TURBO_GEN_MUT_LABELS }
    begin GenText := '.Lwhile_top_'; GenPut(GenText); GenInt(LabelNumber) end;
    begin GenText := ':'; GenPut(GenText) end;
    GenChar(Chr(10))
  end;
  if not ParseFailed then ExpectToken(TkWhile, TkWhile);
  ConditionLine := TokLine;
  ConditionCol := TokCol;
  if not ParseFailed then ParseExpression();
  ConditionType := LastExprType;
  if not ParseFailed then
    if TypeMode then
      if ConditionType <> TyBoolean then
        FailType(TeRule, ConditionLine, ConditionCol, 0, 0, 0, 0);
  if not ParseFailed then
    if GenFlags[1] then
    begin
      begin GenText := '    test eax, eax'; GenLine(GenText) end;
      begin GenText := '    jz near '; GenPut(GenText) end;
      begin GenText := '.Lwhile_end_'; GenPut(GenText); GenInt(LabelNumber) end;
      GenChar(Chr(10))
    end;
  if not ParseFailed then ExpectToken(TkDo, TkDo);
  if not ParseFailed then ParseStatement();
  if not ParseFailed then
    if GenFlags[1] then
    begin
      begin GenText := '    jmp near '; GenPut(GenText) end;
      begin GenText := '.Lwhile_top_'; GenPut(GenText); GenInt(LabelNumber) end;
      GenChar(Chr(10));
      begin GenText := '.Lwhile_end_'; GenPut(GenText); GenInt(LabelNumber) end;
      begin GenText := ':'; GenPut(GenText) end;
      GenChar(Chr(10))
    end;
  if not ParseFailed then TraceExit(PrWhile)
end;

procedure ParseFor();
var
  ControlOffset, ControlLength, ControlLine, ControlCol, ControlSym: integer;
  Direction, LabelNumber: integer;
begin
  TraceEnter(PrFor);
  ControlSym := 0;
  if not ParseFailed then ExpectToken(TkFor, TkFor);
  if not ParseFailed then
  begin
    ControlOffset := TokOffset;
    ControlLength := TokLength;
    ControlLine := TokLine;
    ControlCol := TokCol;
    if TokKind = TkIdent then
    begin
      if TypeMode then
      begin
        ControlSym := ResolveName(ControlOffset, ControlLength);
        if ControlSym = 0 then
          FailType(TeRule, ControlLine, ControlCol, ControlOffset,
                   ControlLength, 0, 0)
        else if SymType[ControlSym] <> TyInteger then
          FailType(TeRule, ControlLine, ControlCol, ControlOffset,
                   ControlLength, 0, 0)
        else if SymKind[ControlSym] = SkConst then
          FailType(TeRule, ControlLine, ControlCol, ControlOffset,
                   ControlLength, 0, 0)
        else if (SymKind[ControlSym] = SkArray) or
                (SymKind[ControlSym] = SkLocalArray) then
          FailType(TeRule, ControlLine, ControlCol, ControlOffset,
                   ControlLength, 0, 0)
      end;
      if not ParseFailed then NextToken()
    end
    else FailParse(ExpVarName)
  end;
  if not ParseFailed then ExpectToken(TkAssign, TkAssign);
  if not ParseFailed then ParseExpression();
  if not ParseFailed then
    if TypeMode then
      if LastExprType <> TyInteger then
        FailType(TeRule, ControlLine, ControlCol, ControlOffset,
                 ControlLength, 0, 0);
  if not ParseFailed then
    if GenFlags[1] then
    begin
      { Keep the initial value separate until the final expression has read
        the old control variable. FPC -Mtp is the execution reference. }
      begin GenText := '    push eax'; GenLine(GenText) end
    end;
  Direction := 0;
  if not ParseFailed then
  begin
    if TokKind = TkTo then begin Direction := 1; NextToken() end
    else if TokKind = TkDownto then begin Direction := -1; NextToken() end
    else FailParse(ExpToOrDownto)
  end;
  if not ParseFailed then ParseExpression();
  if not ParseFailed then
    if TypeMode then
      if LastExprType <> TyInteger then
        FailType(TeRule, ControlLine, ControlCol, ControlOffset,
                 ControlLength, 0, 0);
  if not ParseFailed then
    if GenFlags[1] then
    begin
      begin GenText := '    pop ecx'; GenLine(GenText) end;
      begin GenText := '    push eax'; GenLine(GenText) end;
      GenDesignatorAddress(ControlSym);
      begin GenText := '    mov [eax], ecx'; GenLine(GenText) end;
      LabelNumber := GenNewLabel();
      begin GenText := '.Lfor_top_'; GenPut(GenText); GenInt(LabelNumber) end;
      begin GenText := ':'; GenPut(GenText) end;
      GenChar(Chr(10));
      GenDesignatorAddress(ControlSym);
      begin GenText := '    mov eax, [eax]'; GenLine(GenText) end;
      begin GenText := '    cmp eax, [esp]'; GenLine(GenText) end;
      if Direction = 1 then begin GenText := '    jg near '; GenPut(GenText) end
      else begin GenText := '    jl near '; GenPut(GenText) end;
      begin GenText := '.Lfor_end_'; GenPut(GenText); GenInt(LabelNumber) end;
      GenChar(Chr(10))
    end;
  if not ParseFailed then ExpectToken(TkDo, TkDo);
  if not ParseFailed then ParseStatement();
  if not ParseFailed then
    if GenFlags[1] then
    begin
      GenDesignatorAddress(ControlSym);
      begin GenText := '    mov ecx, [eax]'; GenLine(GenText) end;
      begin GenText := '    mov edx, [esp]'; GenLine(GenText) end;
      begin GenText := '    cmp ecx, edx'; GenLine(GenText) end;
      begin GenText := '    jz near '; GenPut(GenText) end;
      begin GenText := '.Lfor_end_'; GenPut(GenText); GenInt(LabelNumber) end;
      GenChar(Chr(10));
      if Direction = 1 then begin GenText := '    add dword [eax], 1'; GenLine(GenText) end
      else begin GenText := '    sub dword [eax], 1'; GenLine(GenText) end;
      begin GenText := '    jmp near '; GenPut(GenText) end;
      begin GenText := '.Lfor_top_'; GenPut(GenText); GenInt(LabelNumber) end;
      GenChar(Chr(10));
      begin GenText := '.Lfor_end_'; GenPut(GenText); GenInt(LabelNumber) end;
      begin GenText := ':'; GenPut(GenText) end;
      GenChar(Chr(10));
      begin GenText := '    add esp, 4'; GenLine(GenText) end
    end;
  if not ParseFailed then TraceExit(PrFor)
end;

procedure ParseRepeat();
var
  Done: boolean;
  ConditionLine, ConditionCol, LabelNumber: integer;
begin
  TraceEnter(PrRepeat);
  LabelNumber := 0;
  if GenFlags[1] then
  begin
    LabelNumber := GenNewLabel();
    begin GenText := '.Lrepeat_top_'; GenPut(GenText); GenInt(LabelNumber) end;
    begin GenText := ':'; GenPut(GenText) end;
    GenChar(Chr(10))
  end;
  if not ParseFailed then ExpectToken(TkRepeat, TkRepeat);
  Done := ParseFailed;
  if not Done then
    if TokKind = TkUntil then Done := true;
  while not Done do
  begin
    ParseStatement();
    if ParseFailed then
      Done := true
    else if TokKind = TkSemi then
      NextToken()
    else
      Done := true
  end;
  if not ParseFailed then ExpectToken(TkUntil, TkUntil);
  ConditionLine := TokLine;
  ConditionCol := TokCol;
  if not ParseFailed then ParseExpression();
  if not ParseFailed then
    if TypeMode then
      if LastExprType <> TyBoolean then
        FailType(TeRule, ConditionLine, ConditionCol, 0, 0, 0, 0);
  if not ParseFailed then
    if GenFlags[1] then
    begin
      begin GenText := '    test eax, eax'; GenLine(GenText) end;
      begin GenText := '    jz near '; GenPut(GenText) end;
      begin GenText := '.Lrepeat_top_'; GenPut(GenText); GenInt(LabelNumber) end;
      GenChar(Chr(10))
    end;
  if not ParseFailed then TraceExit(PrRepeat)
end;

procedure ParseStatement();
begin
  TraceEnter(PrStatement);
  if not ParseFailed then
  begin
    if TokKind = TkBegin then ParseBlock()
    else if TokKind = TkWrite then ParseWrite()
    else if TokKind = TkWriteln then ParseWrite()
    else if TokKind = TkIf then ParseIf()
    else if TokKind = TkWhile then ParseWhile()
    else if TokKind = TkFor then ParseFor()
    else if TokKind = TkRepeat then ParseRepeat()
    else if TokKind = TkIdent then ParseIdentStatement()
  end;
  if not ParseFailed then TraceExit(PrStatement)
end;

procedure ParseBlock();
var
  Done: boolean;
begin
  TraceEnter(PrBlock);
  if not ParseFailed then ExpectToken(TkBegin, TkBegin);
  Done := ParseFailed;
  if not Done then
    if TokKind = TkEnd then Done := true;
  while not Done do
  begin
    ParseStatement();
    if ParseFailed then
      Done := true
    else if TokKind = TkSemi then
      NextToken()
    else
      Done := true
  end;
  { The protocol grammar historically expects END after its statement list.
    TP5.5 MINT06 07 instead locates the next statement and asks for ';'.
    Preserve the old dump text, but retain the captured IDE diagnostic. }
  if (not ParseFailed) and (FirstKind = 0) then
    if (TokKind = TkIdent) or (TokKind = TkWrite) or (TokKind = TkWriteln) or
       (TokKind = TkIf) or (TokKind = TkWhile) or (TokKind = TkFor) or
       (TokKind = TkRepeat) or (TokKind = TkBegin) then
    begin
      FirstKind := 2; FirstLine := TokLine; FirstCol := TokCol; FirstExpected := TkSemi
    end;
  if not ParseFailed then ExpectToken(TkEnd, TkEnd);
  if not ParseFailed then TraceExit(PrBlock)
end;

procedure ParseConstLiteral();
begin
  TraceEnter(PrConstLiteral);
  ParsedType := TyUnknown;
  ParsedConstValue := 0;
  ParsedConstLen := 0;
  if not ParseFailed then
  begin
    if TokKind = TkMinus then
    begin
      NextToken();
      if TokKind = TkInteger then
      begin
        ParsedType := TyInteger;
        ParsedConstValue := -TokValue;
        NextToken()
      end
      else FailParse(ExpConstLiteral)
    end
    else if TokKind = TkInteger then
    begin
      ParsedType := TyInteger;
      ParsedConstValue := TokValue;
      NextToken()
    end
    else if TokKind = TkString then
    begin
      ParsedConstLen := TokLength;
      if TokLength = 1 then
      begin
        ParsedType := TyChar;
        ParsedConstValue := Ord(TokText[1])
      end
      else
      begin
        ParsedType := TyString;
        if GenFlags[1] then ParsedConstValue := PoolAdd(TokText)
      end;
      NextToken()
    end
    else if TokKind = TkTrue then
    begin
      ParsedType := TyBoolean;
      ParsedConstValue := 1;
      NextToken()
    end
    else if TokKind = TkFalse then
    begin
      ParsedType := TyBoolean;
      ParsedConstValue := 0;
      NextToken()
    end
    else FailParse(ExpConstLiteral)
  end;
  if not ParseFailed then TraceExit(PrConstLiteral)
end;

procedure ParseConstSection();
var
  Done: boolean;
  NameOffset, NameLength, NameLine, NameCol: integer;
begin
  TraceEnter(PrConstSection);
  if not ParseFailed then ExpectToken(TkConst, TkConst);
  Done := ParseFailed;
  while not Done do
  begin
    TraceEnter(PrConstDecl);
    NameOffset := TokOffset;
    NameLength := TokLength;
    NameLine := TokLine;
    NameCol := TokCol;
    if TokKind = TkIdent then NextToken()
    else FailParse(ExpConstLiteral);
    if not ParseFailed then ExpectToken(TkEq, TkEq);
    if not ParseFailed then ParseConstLiteral();
    if not ParseFailed then
      if TypeMode then
        AddConstSymbol(NameOffset, NameLength, NameLine, NameCol, ParsedType,
                       ParsedConstValue, ParsedConstLen);
    if not ParseFailed then ExpectToken(TkSemi, TkSemi);
    if not ParseFailed then TraceExit(PrConstDecl);
    if ParseFailed then Done := true
    else if TokKind <> TkIdent then Done := true
  end;
  if not ParseFailed then TraceExit(PrConstSection)
end;

procedure ParseRecordType();
var
  Done, NamesDone: boolean;
  FieldType, I: integer;
begin
  TraceEnter(PrRecordType);
  if not ParseFailed then ExpectToken(TkRecord, TkRecord);
  Done := ParseFailed;
  if not Done then
    if TokKind = TkEnd then Done := true;
  while not Done do
  begin
    TraceEnter(PrFieldGroup);
    PendingNameCount := 0;
    NamesDone := false;
    while not NamesDone do
    begin
      if TokKind = TkIdent then
      begin
        if TypeMode then
        begin
          if PendingNameCount >= PendingNameMax then
            FailType(TeRule, TokLine, TokCol, TokOffset, TokLength, 0, 0)
          else
          begin
            PendingNameCount := PendingNameCount + 1;
            PendingNameOffset[PendingNameCount] := TokOffset;
            PendingNameLine[PendingNameCount] := TokLine;
            PendingNameCol[PendingNameCount] := TokCol
          end
        end;
        if not ParseFailed then NextToken()
      end
      else FailParse(ExpFieldName);
      if ParseFailed then
        NamesDone := true
      else if TokKind = TkComma then
        NextToken()
      else
        NamesDone := true
    end;
    if not ParseFailed then ExpectToken(TkColon, TkColon);
    if not ParseFailed then
    begin
      FieldType := TyUnknown;
      if TokKind = TkIntegerType then begin FieldType := TyInteger; NextToken() end
      else if TokKind = TkBoolean then begin FieldType := TyBoolean; NextToken() end
      else if TokKind = TkChar then begin FieldType := TyChar; NextToken() end
      else FailParse(ExpTypeName)
    end;
    if not ParseFailed then
      if TypeMode then
      begin
        I := 1;
        while I <= PendingNameCount do
        begin
          AddRecordField(PendingNameOffset[I],
                         PoolLengthAt(PendingNameOffset[I]),
                         PendingNameLine[I], PendingNameCol[I], FieldType);
          I := I + 1
        end
      end;
    if not ParseFailed then ExpectToken(TkSemi, TkSemi);
    if not ParseFailed then TraceExit(PrFieldGroup);
    if ParseFailed then Done := true
    else if TokKind = TkEnd then Done := true
  end;
  if not ParseFailed then ExpectToken(TkEnd, TkEnd);
  if not ParseFailed then TraceExit(PrRecordType)
end;

procedure ParseTypeSection();
var
  Done: boolean;
  NameOffset, NameLength, NameLine, NameCol: integer;
begin
  TraceEnter(PrTypeSection);
  if not ParseFailed then ExpectToken(TkType, TkType);
  Done := ParseFailed;
  while not Done do
  begin
    TraceEnter(PrTypeDecl);
    NameOffset := TokOffset;
    NameLength := TokLength;
    NameLine := TokLine;
    NameCol := TokCol;
    if TokKind = TkIdent then
    begin
      if TypeMode then
        CurrentRecord := AddRecordType(NameOffset, NameLength, NameLine, NameCol);
      if not ParseFailed then NextToken()
    end
    else FailParse(ExpTypeName);
    if not ParseFailed then ExpectToken(TkEq, TkEq);
    if not ParseFailed then ParseRecordType();
    if not ParseFailed then ExpectToken(TkSemi, TkSemi);
    if not ParseFailed then TraceExit(PrTypeDecl);
    CurrentRecord := 0;
    if ParseFailed then Done := true
    else if TokKind <> TkIdent then Done := true
  end;
  if not ParseFailed then TraceExit(PrTypeSection)
end;

procedure ParseArrayType();
var
  LowBound, HighBound: integer;
begin
  TraceEnter(PrArrayType);
  ParsedIsArray := true;
  if not ParseFailed then ExpectToken(TkArray, TkArray);
  if not ParseFailed then ExpectToken(TkLBracket, TkLBracket);
  if not ParseFailed then ParseArrayBound();
  LowBound := ParsedBound;
  if not ParseFailed then ExpectToken(TkDotDot, TkDotDot);
  if not ParseFailed then ParseArrayBound();
  HighBound := ParsedBound;
  if not ParseFailed then ExpectToken(TkRBracket, TkRBracket);
  if not ParseFailed then
    if TypeMode then
      if LowBound > HighBound then
        FailType(TeRule, TokLine, TokCol, 0, 0, 0, 0);
  if not ParseFailed then ExpectToken(TkOf, TkOf);
  if not ParseFailed then ParseTypeName();
  ParsedLo := LowBound;
  ParsedHi := HighBound;
  ParsedIsArray := true;
  if not ParseFailed then
    if TypeMode then
      if (ParsedType = TyString) or (ParsedType = TyFile) then
        FailType(TeRule, TokLine, TokCol, 0, 0, 0, 0);
  if not ParseFailed then TraceExit(PrArrayType)
end;

procedure ParseVarDecl();
var
  Done: boolean;
  I: integer;
begin
  TraceEnter(PrVarDecl);
  PendingNameCount := 0;
  ParsedIsArray := false;
  Done := false;
  while not Done do
  begin
    if TokKind = TkIdent then
    begin
      if TypeMode then
      begin
        if PendingNameCount >= PendingNameMax then
          FailType(TeRule, TokLine, TokCol, TokOffset, TokLength, 0, 0)
        else
        begin
          PendingNameCount := PendingNameCount + 1;
          PendingNameOffset[PendingNameCount] := TokOffset;
          PendingNameLine[PendingNameCount] := TokLine;
          PendingNameCol[PendingNameCount] := TokCol
        end
      end;
      if not ParseFailed then NextToken()
    end
    else FailParse(ExpVarName);
    if ParseFailed then
      Done := true
    else if TokKind = TkComma then
      NextToken()
    else
      Done := true
  end;
  if not ParseFailed then ExpectToken(TkColon, TkColon);
  if not ParseFailed then
  begin
    if TokKind = TkArray then ParseArrayType()
    else
    begin
      ParsedIsArray := false;
      ParseTypeName()
    end
  end;
  if not ParseFailed then
    if TypeMode then
    begin
      I := 1;
      while I <= PendingNameCount do
      begin
        AddDataSymbol(PendingNameOffset[I],
                      PoolLengthAt(PendingNameOffset[I]),
                      PendingNameLine[I], PendingNameCol[I], ParsedType,
                      ParsedRef, ParsedStrCap, ParsedLo, ParsedHi,
                      ParsedIsArray);
        I := I + 1
      end
    end;
  if not ParseFailed then TraceExit(PrVarDecl)
end;

procedure ParseVarSection();
var
  Done: boolean;
begin
  TraceEnter(PrVarSection);
  if not ParseFailed then ExpectToken(TkVar, TkVar);
  Done := ParseFailed;
  while not Done do
  begin
    ParseVarDecl();
    if not ParseFailed then ExpectToken(TkSemi, TkSemi);
    if ParseFailed then Done := true
    else if TokKind <> TkIdent then Done := true
  end;
  if not ParseFailed then TraceExit(PrVarSection)
end;

procedure ParseParamGroup();
var
  Done, IsVarParam, DuplicateName: boolean;
  I, J, ParamKind: integer;
begin
  TraceEnter(PrParamGroup);
  IsVarParam := false;
  if not ParseFailed then
    if TokKind = TkVar then
    begin
      IsVarParam := true;
      NextToken()
    end;
  PendingNameCount := 0;
  Done := false;
  while not Done do
  begin
    if TokKind = TkIdent then
    begin
      if TypeMode then
      begin
        if PendingNameCount >= PendingNameMax then
          FailType(TeRule, TokLine, TokCol, TokOffset, TokLength, 0, 0)
        else
        begin
          PendingNameCount := PendingNameCount + 1;
          PendingNameOffset[PendingNameCount] := TokOffset;
          PendingNameLine[PendingNameCount] := TokLine;
          PendingNameCol[PendingNameCount] := TokCol
        end
      end;
      if not ParseFailed then NextToken()
    end
    else FailParse(ExpParamName);
    if ParseFailed then
      Done := true
    else if TokKind = TkComma then
      NextToken()
    else
      Done := true
  end;
  if not ParseFailed then ExpectToken(TkColon, TkColon);
  if not ParseFailed then ParseTypeName();
  if not ParseFailed then
    if TypeMode then
    begin
      if ParsedType = TyRecord then
        if not IsVarParam then
          FailType(TeRule, TokLine, TokCol, 0, 0, 0, 0);
      if not ParseFailed then
        if ParsedType = TyString then
        begin
          if not IsVarParam then
            FailType(TeRule, TokLine, TokCol, 0, 0, 0, 0)
          else if ParsedStrCap <> 255 then
            FailType(TeRule, TokLine, TokCol, 0, 0, 0, 0)
        end;
      if not ParseFailed then
        if ParsedType = TyFile then
          FailType(TeRule, TokLine, TokCol, 0, 0, 0, 0);
      I := 1;
      while I <= PendingNameCount do
      begin
        if not ParseFailed then
        begin
          DuplicateName := false;
          J := 1;
          while J <= HeaderCount do
          begin
            if NamesEqual(HeaderNameOffset[J],
                          PoolLengthAt(HeaderNameOffset[J]),
                          PendingNameOffset[I],
                          PoolLengthAt(PendingNameOffset[I])) then
              DuplicateName := true;
            J := J + 1
          end;
          if DuplicateName then
            FailType(TeDuplicate, PendingNameLine[I], PendingNameCol[I],
                     PendingNameOffset[I],
                     PoolLengthAt(PendingNameOffset[I]), 0, 0)
          else if NameIsFileBuiltin(PendingNameOffset[I],
                                    PoolLengthAt(PendingNameOffset[I])) then
            FailType(TeRule, PendingNameLine[I], PendingNameCol[I],
                     PendingNameOffset[I],
                     PoolLengthAt(PendingNameOffset[I]), 0, 0)
          else if HeaderCount >= HeaderParamMax then
            FailType(TeRule, PendingNameLine[I], PendingNameCol[I],
                     PendingNameOffset[I],
                     PoolLengthAt(PendingNameOffset[I]), 0, 0)
          else
          begin
            HeaderCount := HeaderCount + 1;
            HeaderNameOffset[HeaderCount] := PendingNameOffset[I];
            HeaderType[HeaderCount] := ParsedType;
            ParamKind := SkValueParam;
            if IsVarParam then ParamKind := SkVarParam;
            HeaderKind[HeaderCount] := ParamKind;
            HeaderRef[HeaderCount] := ParsedRef;
            HeaderLine[HeaderCount] := PendingNameLine[I];
            HeaderCol[HeaderCount] := PendingNameCol[I]
          end
        end;
        I := I + 1
      end
    end;
  if not ParseFailed then TraceExit(PrParamGroup)
end;

procedure ParseParamList();
var
  Done: boolean;
begin
  TraceEnter(PrParamList);
  if not ParseFailed then ExpectToken(TkLParen, TkLParen);
  Done := ParseFailed;
  if not Done then
    if TokKind = TkRParen then Done := true;
  while not Done do
  begin
    ParseParamGroup();
    if ParseFailed then
      Done := true
    else if TokKind = TkSemi then
      NextToken()
    else
      Done := true
  end;
  if not ParseFailed then ExpectToken(TkRParen, TkRParen);
  if not ParseFailed then TraceExit(PrParamList)
end;

procedure ParseRoutine(IsFunction: boolean);
var
  Production, NameOffset, NameLength, NameLine, NameCol, RoutineIndex: integer;
  I: integer;
  IsForward: boolean;
begin
  Production := PrRoutineProcedure;
  if IsFunction then Production := PrRoutineFunction;
  TraceEnter(Production);
  HeaderCount := 0;
  ParsedType := TyNone;
  if not ParseFailed then NextToken();
  if not ParseFailed then
  begin
    NameOffset := TokOffset;
    NameLength := TokLength;
    NameLine := TokLine;
    NameCol := TokCol;
    if TokKind = TkIdent then NextToken()
    else FailParse(ExpRoutineName)
  end;
  if not ParseFailed then
    if TokKind = TkLParen then ParseParamList();
  if not ParseFailed then
  begin
    if IsFunction then
    begin
      ExpectToken(TkColon, TkColon);
      if not ParseFailed then ParseResultType()
    end
  end;
  if not IsFunction then ParsedType := TyNone;
  if not ParseFailed then ExpectToken(TkSemi, TkSemi);
  IsForward := false;
  if not ParseFailed then
    if TokKind = TkForward then IsForward := true;
  RoutineIndex := 0;
  if not ParseFailed then
    if TypeMode then
      RoutineIndex := RegisterRoutine(NameOffset, NameLength, NameLine,
                                      NameCol, IsFunction, IsForward);
  if not ParseFailed then
  begin
    if IsForward then
    begin
      NextToken();
      ExpectToken(TkSemi, TkSemi)
    end
    else
    begin
      if TypeMode then
      begin
        ActiveRoutine := RoutineIndex;
        LocalSlots := 0;
        if IsFunction then LocalSlots := 1
      end;
      if TokKind = TkVar then ParseVarSection();
      if not ParseFailed then
        if GenFlags[1] then
        begin
          GenSetSection(3);
          begin GenText := 'pf_'; GenPut(GenText) end;
          PoolGet(SymNameOffset[RoutineIndex],
                  PoolLengthAt(SymNameOffset[RoutineIndex]), DumpText);
          GenPut(DumpText);
          begin GenText := ':'; GenPut(GenText) end;
          GenChar(Chr(10));
          begin GenText := '    push ebp'; GenLine(GenText) end;
          begin GenText := '    mov ebp, esp'; GenLine(GenText) end;
          if LocalSlots > 0 then
          begin
            begin GenText := '    sub esp, '; GenPut(GenText) end;
            GenInt(LocalSlots * 4);
            GenChar(Chr(10))
          end;
          I := 1;
          while I <= SymCount do
          begin
            if SymScope[I] = RoutineIndex then
              if SymKind[I] = SkLocal then
              begin
                if SymType[I] = TyString then
                begin
                  begin GenText := '    mov byte '; GenPut(GenText) end;
                  GenEbp(I);
                  begin GenText := ', 0'; GenPut(GenText) end;
                  GenChar(Chr(10))
                end
                else if SymType[I] = TyFile then
                begin
                  begin GenText := '    mov dword '; GenPut(GenText) end;
                  GenEbp(I);
                  begin GenText := ', 0'; GenPut(GenText) end;
                  GenChar(Chr(10));
                  begin GenText := '    mov byte [ebp'; GenPut(GenText) end;
                  if GenFrameOffset(I) + 4 >= 0 then GenChar('+');
                  GenInt(GenFrameOffset(I) + 4);
                  begin GenText := '], 0'; GenPut(GenText) end;
                  GenChar(Chr(10))
                end
              end;
            I := I + 1
          end
        end;
      if not ParseFailed then ParseBlock();
      if not ParseFailed then ExpectToken(TkSemi, TkSemi);
      if not ParseFailed then
        if GenFlags[1] then
        begin
          if IsFunction then
          begin
            I := FindLocal(NameOffset, NameLength);
            if SymType[I] = TyChar then
              begin GenText := '    movzx eax, byte '; GenPut(GenText) end
            else
              begin GenText := '    mov eax, '; GenPut(GenText) end;
            GenEbp(I);
            GenChar(Chr(10))
          end;
          begin GenText := '    leave'; GenLine(GenText) end;
          begin GenText := '    ret'; GenLine(GenText) end;
          begin GenText := ''; GenLine(GenText) end
        end;
      if TypeMode then
      begin
        if RoutineIndex > 0 then SymSize[RoutineIndex] := LocalSlots * 4;
        ActiveRoutine := 0;
        LocalSlots := 0
      end
    end
  end;
  if not ParseFailed then TraceExit(Production)
end;

procedure ParseProgram();
var
  Done: boolean;
begin
  TraceEnter(PrProgram);
  if not ParseFailed then ExpectToken(TkProgram, TkProgram);
  if not ParseFailed then
  begin
    if TokKind = TkIdent then NextToken()
    else FailParse(ExpProgramName)
  end;
  if not ParseFailed then ExpectToken(TkSemi, TkSemi);
  Done := ParseFailed;
  while not Done do
  begin
    if TokKind = TkConst then ParseConstSection()
    else if TokKind = TkVar then ParseVarSection()
    else if TokKind = TkType then ParseTypeSection()
    else if TokKind = TkProcedure then ParseRoutine(false)
    else if TokKind = TkFunction then ParseRoutine(true)
    else Done := true;
    if ParseFailed then Done := true
  end;
  if not ParseFailed then
    if GenFlags[1] then
    begin
      GenSetSection(3);
      begin GenText := 'global pas_main'; GenLine(GenText) end;
      begin GenText := 'pas_main:'; GenLine(GenText) end;
      begin GenText := '    push ebp'; GenLine(GenText) end;
      begin GenText := '    mov ebp, esp'; GenLine(GenText) end
    end;
  if not ParseFailed then ParseBlock();
  if not ParseFailed then
    if GenFlags[1] then
    begin
      begin GenText := '    leave'; GenLine(GenText) end;
      begin GenText := '    ret'; GenLine(GenText) end;
      begin GenText := ''; GenLine(GenText) end
    end;
  if not ParseFailed then ExpectToken(TkDot, TkDot);
  if not ParseFailed then
    if TokKind <> TkEof then FailParse(TkEof);
  if not ParseFailed then
    if TypeMode then CheckUnresolvedForwards();
  if not ParseFailed then TraceExit(PrProgram)
end;

procedure RunParser();
begin
  ReadInput();
  ScanPos := 1;
  ScanLine := 1;
  IoChecks := true;
  ScanCol := 1;
  PoolUsed := 0;
  PoolCount := 0;
  ParseFailed := false;
  ParseIfDepth := 0;
  TypeMode := false;
  EmitParseTrace := true;
  NextToken();
  if not HadError then ParseProgram();
  if not ParseFailed then
    if not HadError then Writeln('TURBO-PARSE-OK')
end;

procedure RunTypeChecker();
begin
  ReadInput();
  ScanPos := 1;
  ScanLine := 1;
  IoChecks := true;
  ScanCol := 1;
  PoolUsed := 0;
  PoolCount := 0;
  ParseFailed := false;
  ParseIfDepth := 0;
  TypeFailed := false;
  TypeMode := true;
  EmitParseTrace := false;
  SymCount := 0;
  GlobalBytes := 0;
  ActiveRoutine := 0;
  LocalSlots := 0;
  CurrentRecord := 0;
  HeaderCount := 0;
  PendingNameCount := 0;
  ArgCount := 0;
  ParsingFileBuiltin := 0;
  ResetLastExpression();
  NextToken();
  if not HadError then ParseProgram();
  if not TypeFailed then
    if not ParseFailed then
    begin
      if DumpMode then
      begin DumpSymbols(); Writeln('TURBO-TYPE-OK') end
    end;
  TypeMode := false
end;

procedure NativeHalf(Value: integer);
begin
  NativeByte(Value);
  NativeByte(Value div 256)
end;

procedure NativeHeader();
var I, Size: integer;
begin
  Size := Native[12] + Native[3] + 31;
  NativeHalf(23117);
  NativeHalf(Size mod 512);
  NativeHalf((Size + 511) div 512);
  NativeHalf(Native[11]);
  NativeHalf(Native[12] div 16);
  NativeHalf(0);
  NativeHalf(65535);
  I := 1;
  while I <= 5 do begin NativeHalf(0); I := I + 1 end;
  NativeHalf(56);
  NativeHalf(0);
  NativeHalf(22601);
  NativeHalf(0);
  NativeWord(1);
  NativeWord(24);
  NativeWord(Native[5] - Native[3] - 31);
  NativeWord(0);
  NativeWord(65536);
  NativeWord(4096);
  I := 1;
  while I <= Native[11] do
  begin
    NativeWord(NativeRelocs[I]);
    I := I + 1
  end;
  while Native[2] < Native[12] do NativeByte(0)
end;


procedure RunCodeGenerator();
begin
  ReadInput();
  ScanPos := 1;
  ScanLine := 1;
  IoChecks := true;
  ScanCol := 1;
  PoolUsed := 0;
  PoolCount := 0;
  ParseFailed := false;
  ParseIfDepth := 0;
  TypeFailed := false;
  TypeMode := true;
  GenFlags[1] := true;
  EmitParseTrace := false;
  SymCount := 0;
  GlobalBytes := 0;
  ActiveRoutine := 0;
  LocalSlots := 0;
  CurrentRecord := 0;
  HeaderCount := 0;
  PendingNameCount := 0;
  ArgCount := 0;
  ParsingFileBuiltin := 0;
  GenFlags[5] := false;
  ResetLastExpression();
  OutputChunk := '';
  GenData[2] := 0;
  GenData[3] := 0;
  GenData[4] := 0;
  GenFlags[2] := false;
  GenFlags[3] := false;
  GenFlags[4] := false;
  NativeLineText := '';
  Native[8] := 0;
  if Native[1] = 0 then
  begin
    {$I-}
    Assign(OutputFile, OutputAsmName);
    Rewrite(OutputFile, 1);
    CompilerIoError := IOResult();
    {$I+}
    CompilerIO(CompilerIoError)
  end
  else
  begin
    if Native[1] = 2 then
    begin
      {$I-}
      if Native[10] = 0 then Assign(OutputFile, OutputComName)
      else Assign(OutputFile, OutputExeName);
      Rewrite(OutputFile, 1);
      CompilerIoError := IOResult();
      {$I+}
      CompilerIO(CompilerIoError)
    end;
    Native[2] := 0;
    if Native[1] = 2 then
      if Native[10] = 1 then NativeHeader();
    Native[2] := 0;
    if Native[10] = 0 then NativeRuntime() else NativeExtendedRuntime()
  end;
  begin GenText := '; Generated by Turbo Initech (B9.4).'; GenLine(GenText) end;
  begin GenText := '; Ref: ADR-0007 DEC-04/DEC-05; deterministic stack-machine x86.'; GenLine(GenText) end;
  begin GenText := 'bits 32'; GenLine(GenText) end;
  begin GenText := ''; GenLine(GenText) end;
  begin GenText := 'extern serial_putc'; GenLine(GenText) end;
  begin GenText := 'extern serial_puts'; GenLine(GenText) end;
  begin GenText := 'extern serial_put_int'; GenLine(GenText) end;
  begin GenText := ''; GenLine(GenText) end;
  GenSetSection(1);
  begin GenText := 'str_bool_true: db 84,82,85,69,0'; GenLine(GenText) end;
  begin GenText := 'str_bool_false: db 70,65,76,83,69,0'; GenLine(GenText) end;
  begin GenText := ''; GenLine(GenText) end;
  GenSetSection(2);
  NextToken();
  if not HadError then ParseProgram();
  if not GenFlags[2] then
    if not TypeFailed then
      if not ParseFailed then
      begin
        if GenFlags[4] then
        begin
          begin GenText := 'extern rtl_file_assign'; GenLine(GenText) end;
          begin GenText := 'extern rtl_file_reset'; GenLine(GenText) end;
          begin GenText := 'extern rtl_file_rewrite'; GenLine(GenText) end;
          begin GenText := 'extern rtl_file_blockread'; GenLine(GenText) end;
          begin GenText := 'extern rtl_file_blockwrite'; GenLine(GenText) end;
          begin GenText := 'extern rtl_file_close'; GenLine(GenText) end;
          begin GenText := 'extern rtl_file_erase'; GenLine(GenText) end;
          begin GenText := 'extern rtl_file_rename'; GenLine(GenText) end;
          begin GenText := 'extern rtl_ioresult'; GenLine(GenText) end;
          begin GenText := 'extern rtl_io_check'; GenLine(GenText) end;
          begin GenText := 'extern rtl_readkey'; GenLine(GenText) end;
          begin GenText := 'extern rtl_file_attributes'; GenLine(GenText) end;
          begin GenText := 'extern rtl_paramcount'; GenLine(GenText) end;
          begin GenText := 'extern rtl_paramstr'; GenLine(GenText) end;
          begin GenText := 'extern rtl_screen_page'; GenLine(GenText) end;
          begin GenText := 'extern rtl_execute'; GenLine(GenText) end;
          begin GenText := 'extern rtl_halt'; GenLine(GenText) end;
          begin GenText := 'extern rtl_dos_memory'; GenLine(GenText) end
        end;
        if GenFlags[3] then GenStringIntrinsics();
        GenFlush()
      end;
  if not GenFlags[2] then
    if not TypeFailed then
      if not ParseFailed then
        if (Native[1] = 0) and (not QuietCompile) then
          Writeln('TURBO-GEN-OK bytes=', GenData[2],
                  ' labels=', GenData[3]);
  if Native[1] = 0 then
  begin
    {$I-} Close(OutputFile); CompilerIoError := IOResult(); {$I+}
    CompilerIO(CompilerIoError)
  end;
  GenFlags[1] := false;
  TypeMode := false
end;

procedure NativeEmit();
begin
  Native[1] := 2;
  RunCodeGenerator();
  if not GenFlags[2] then
  begin
    while Native[2] < Native[3] do NativeByte(0);
    GenText := '46494C45494F2D4552524F522041583D00000000'; NativeHex(GenText);
    GenText := '545255450046414C534500'; NativeHex(GenText);
    GenFlush();
    {$I-} Close(OutputFile); CompilerIoError := IOResult(); {$I+}
    CompilerIO(CompilerIoError);
    if (not GenFlags[2]) and (not QuietCompile) then
      Writeln('TURBO-NATIVE-OK form=', Native[10], ' bytes=', GenData[2],
              ' end=', Native[9] + Native[5])
  end
end;

procedure RunNative();
var I: integer;
begin
  I := 1;
  while I <= 8192 do begin NativeLabels[I] := 0; I := I + 1 end;
  I := 1;
  while I <= SymbolMax do begin NativeRoutines[I] := 0; I := I + 1 end;
  Native[1] := 1;
  Native[9] := 0;
  Native[10] := 0;
  Native[11] := 0;
  RunCodeGenerator();
  Native[3] := ((Native[2] + 3) div 4) * 4; { NATIVE_SECTION }
  Native[4] := Native[3] + 32;
  Native[5] := Native[4] + GlobalBytes;
  Native[12] := ((56 + Native[11]*4 + 15) div 16) * 16;
  if not GenFlags[2] then
  begin
    if 262400 + Native[5] <= 454656 then
    begin
      Native[9] := 262400;
      NativeEmit()
    end;
    if (Native[3] + 31 > 262144) or
       (Native[5] > 2027504) then GenFail(TokLine, TokCol, 414);
    if not GenFlags[2] then
    begin
      Native[10] := 1;
      Native[9] := 0;
      NativeEmit()
    end
  end;
  Native[1] := 0
end;

procedure DumpCompile();
begin
  HadError := false;
  Writeln('TURBO-LEX-BEGIN');
  ReadInput();
  RunLexer();
  Writeln('TURBO-LEX-END');
  Writeln('TURBO-PARSE-BEGIN');
  if HadError then
    Writeln('TURBO-PARSE-SKIP lexer-error')
  else
    RunParser();
  Writeln('TURBO-PARSE-END');
  SyntaxClean := not ParseFailed;
  Writeln('TURBO-TYPE-BEGIN');
  if HadError then
    Writeln('TURBO-TYPE-SKIP lexer-error')
  else if not SyntaxClean then
    Writeln('TURBO-TYPE-SKIP parser-error')
  else
    RunTypeChecker();
  Writeln('TURBO-TYPE-END');
  Writeln('TURBO-GEN-BEGIN');
  if HadError then
    Writeln('TURBO-GEN-SKIP lexer-error')
  else if not SyntaxClean then
    Writeln('TURBO-GEN-SKIP parser-error')
  else if TypeFailed then
    Writeln('TURBO-GEN-SKIP type-error')
  else
  begin
    RunCodeGenerator();
    if not GenFlags[2] then RunNative()
  end;
  Writeln('TURBO-GEN-END')
end;

{ No protocol passes in normal compilation. The semantic parse locates
  syntax errors too; repeated generator passes retain the first record. }
procedure CompileSource();
begin
  FirstKind := 0; FirstLine := 1; FirstCol := 1;
  FirstExpected := 0; FirstDetail := 0;
  HadError := false; ParseFailed := false; TypeFailed := false;
  GenFlags[1] := false; GenFlags[2] := false; Native[1] := 0;
  if DumpMode then DumpCompile()
  else
  begin
    RunTypeChecker();
    if (not HadError) and (not ParseFailed) and (not TypeFailed) then
    begin
      RunCodeGenerator();
      if not GenFlags[2] then RunNative()
    end
  end
end;

procedure CompilerDefaults();
begin
  InputName := 'TURBOIN.PAS'; OutputAsmName := 'TURBOOUT.S';
  OutputComName := 'TURBOOUT.COM'; OutputExeName := 'TURBOOUT.EXE';
  QuietCompile := false; DumpMode := false
end;


{ BEGIN IDE screen }
{$I-}
{ The IDE checks its own file errors with IOResult (TP 5.5 Reference Guide):
  the switch above is off for everything that follows. }
{ screen.pas -- the IDE's 80x25 screen: characters, attributes, cursor.

  Everything the IDE shows is drawn into these cells first; the product
  copies them to the console and the tier-0 driver dumps them for the
  comparison with the TP 5.5 captures (turbo/README.md "IDE").
  Attributes are VGA (background*16 + foreground), as in the captures.
  Source discipline as turbo.pas: complete boolean evaluation, so every
  bounds-sensitive access sits under nested ifs. }
const
  ScrRows = 25;
  ScrCols = 80;
  AtMenu = 112;        { 0x70 black on grey: menu bar, pulldowns, boxes }
  AtMenuKey = 116;     { 0x74 red on grey: hotkey letters }
  AtSelect = 15;       { 0x0F white on black: selected title or item }
  AtInfo = 23;         { 0x17 grey on blue: info line }
  AtText = 30;         { 0x1E yellow on blue: source text }
  AtError = 78;        { 0x4E yellow on red: error line }
  AtVerify = 127;      { 0x7F bright white on grey: Verify frame }
  AtErrFrame = 71;     { 0x47 grey on red: Error box frame }
  AtErrText = 79;      { 0x4F bright white on red: Error box text }
  AtBarText = 31;      { 0x1F bright white on blue: compile box result }
  AtBlink = 159;       { 0x9F blinking bright white on blue: Press any key }

var
  ScrCh: array[0..1999] of char;
  ScrAt: array[0..1999] of integer;
  ScrCurRow, ScrCurCol: integer;
  ScrCurOn: boolean;

procedure ScrPut(Row, Col: integer; C: char; A: integer);
begin
  if (Row >= 0) and (Row < ScrRows) then
    if (Col >= 0) and (Col < ScrCols) then
    begin
      ScrCh[Row * ScrCols + Col] := C;
      ScrAt[Row * ScrCols + Col] := A
    end
end;

procedure ScrFill(Row, Col0, Col1: integer; C: char; A: integer);
var I: integer;
begin
  I := Col0;
  while I <= Col1 do
  begin
    ScrPut(Row, I, C, A);
    I := I + 1
  end
end;

procedure ScrText(Row, Col: integer; var S: string; A: integer);
var I: integer;
begin
  I := 1;
  while I <= Length(S) do
  begin
    ScrPut(Row, Col + I - 1, S[I], A);
    I := I + 1
  end
end;

procedure ScrAttr(Row, Col: integer; A: integer);
begin
  if (Row >= 0) and (Row < ScrRows) then
    if (Col >= 0) and (Col < ScrCols) then
      ScrAt[Row * ScrCols + Col] := A
end;

procedure ScrCursor(Row, Col: integer; On: boolean);
begin
  ScrCurRow := Row;
  ScrCurCol := Col;
  ScrCurOn := On
end;

{ Decimal digits of a non-negative number, left-aligned. }
procedure NumText(N: integer; var S: string);
var D: string;
begin
  S := '';
  if N = 0 then S := '0';
  while N > 0 do
  begin
    D := chr(ord('0') + N mod 10);
    S := D + S;
    N := N div 10
  end
end;

{ A single-line frame (pulldowns and name boxes) or a double-line frame
  (Verify, Error and compile boxes), CP437 as captured. }
procedure ScrBox(Row0, Col0, Row1, Col1: integer; Double: boolean; A: integer);
var R: integer;
begin
  if Double then
  begin
    ScrPut(Row0, Col0, chr(201), A); ScrFill(Row0, Col0 + 1, Col1 - 1, chr(205), A);
    ScrPut(Row0, Col1, chr(187), A);
    ScrPut(Row1, Col0, chr(200), A); ScrFill(Row1, Col0 + 1, Col1 - 1, chr(205), A);
    ScrPut(Row1, Col1, chr(188), A)
  end
  else
  begin
    ScrPut(Row0, Col0, chr(218), A); ScrFill(Row0, Col0 + 1, Col1 - 1, chr(196), A);
    ScrPut(Row0, Col1, chr(191), A);
    ScrPut(Row1, Col0, chr(192), A); ScrFill(Row1, Col0 + 1, Col1 - 1, chr(196), A);
    ScrPut(Row1, Col1, chr(217), A)
  end;
  R := Row0 + 1;
  while R < Row1 do
  begin
    if Double then
    begin ScrPut(R, Col0, chr(186), A); ScrPut(R, Col1, chr(186), A) end
    else
    begin ScrPut(R, Col0, chr(179), A); ScrPut(R, Col1, chr(179), A) end;
    R := R + 1
  end
end;

{ A box title centred in the top edge with one space each side. }
procedure ScrTitle(Row, Col0, Col1: integer; var S: string; A: integer);
var C: integer;
    T: string;
begin
  C := Col0 + (Col1 - Col0 + 1 - Length(S) - 2) div 2;
  T := ' ' + S + ' ';
  ScrText(Row, C, T, A)
end;
{ END IDE screen }

{ BEGIN IDE buffer }
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
  Close(WorkIn); IoNote();
  Close(WorkOut); IoNote()
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
{ END IDE buffer }

{ BEGIN IDE editor }
{ editor.pas -- the Turbo Initech IDE: chrome, menus, prompts and editing.

  Layout, attributes and behaviour follow the TP 5.5 captures named in
  turbo/README.md "IDE" (turbopascal-decomp MINT06, cited by shot number).
  Keys arrive as DOS reports them through AH=07h: a character code, or
  256 + scan code for an extended key. }
const
  KeyEnter = 13; KeyEsc = 27; KeyBack = 8; KeyTab = 9; KeyCtrlY = 25;
  KeyF2 = 316; KeyF3 = 317; KeyF9 = 323; KeyF10 = 324;
  KeyAltF9 = 368; KeyCtrlF9 = 358; KeyAltF5 = 364;
  KeyAltX = 301; KeyAltE = 274; KeyAltF = 289; KeyAltR = 275; KeyAltC = 302;
  KeyHome = 327; KeyUp = 328; KeyPgUp = 329; KeyLeft = 331; KeyRight = 333;
  KeyEnd = 335; KeyDown = 336; KeyPgDn = 337; KeyIns = 338; KeyDel = 339;
  KeyCtrlEnd = 373; KeyCtrlPgDn = 374; KeyCtrlHome = 375; KeyCtrlPgUp = 388;

  ModeEditor = 0; ModeBar = 1; ModeMenu = 2; ModeName = 3; ModeVerify = 4;
  ModeQuit = 5; ModeError = 6; ModeOverwrite = 7;
  ModeCompile = 8; ModeUser = 9;
  CmdCompile = 1; CmdMake = 2; CmdRun = 3; CmdUser = 4;
  MenuFile = 1; MenuEdit = 2; MenuRun = 3; MenuCompile = 4;
  NameRename = 1; NameLoad = 2; NameWrite = 3;
  VerifyNew = 1; VerifyLoad = 2; VerifyQuit = 3;
  ViewRows = 22;

var
  EdLine, EdCol, EdTop, EdLeft: integer;
  EdInsert, EdModified, EdOnDisk: boolean;
  FileName, FilePath, FileDrive, FileDir: string;
  UiMode, MenuSel, LastMenu, NameKind, NameRow, NameCol, VerifyKind: integer;
  ItemSel: array[1..4] of integer;
  NameText, PendingName: string;
  NameFresh: boolean;
  ErrCode, ErrReturn: integer;       { Error box (MINT06 51, 54) }
  ErrLineShown: boolean;             { compiler error on the info row (07) }
  ErrLineText: string;
  RequestedCommand, CompileLines, CompileMemory: integer;
  CompileDone: boolean;
  PrivateBase: string;
  Tx: string;

{ ---- line geometry: tabs at 8-column stops (MINT06 36, 34) ---- }

function DispLen(var S: string): integer;
var I, C: integer;
begin
  C := 0; I := 1;
  while I <= Length(S) do
  begin
    if S[I] = chr(9) then C := (C div 8 + 1) * 8 else C := C + 1;
    I := I + 1
  end;
  DispLen := C
end;

{ Byte index (1-based) of the first byte whose display position is at or
  after column Col; Length+1 past the end. }
function ColByte(var S: string; Col: integer): integer;
var I, C, Found: integer;
begin
  C := 1; I := 1; Found := 0;
  while I <= Length(S) do
  begin
    if Found = 0 then
      if C >= Col then Found := I;
    if S[I] = chr(9) then C := ((C - 1) div 8 + 1) * 8 + 1 else C := C + 1;
    I := I + 1
  end;
  if Found = 0 then Found := Length(S) + 1;
  ColByte := Found
end;

function ByteCol(var S: string; B: integer): integer;
var I, C: integer;
begin
  C := 1; I := 1;
  while I < B do
  begin
    if I <= Length(S) then
    begin
      if S[I] = chr(9) then C := ((C - 1) div 8 + 1) * 8 + 1 else C := C + 1
    end;
    I := I + 1
  end;
  ByteCol := C
end;

 { ---- view keeping ---- }

procedure ScrollTo;
begin
  if EdLine < EdTop then EdTop := EdLine;
  if EdLine > EdTop + ViewRows - 1 then EdTop := EdLine - ViewRows + 1;
  if EdCol < EdLeft then EdLeft := EdCol;
  if EdCol - EdLeft > 78 then EdLeft := EdCol - 78
end;

{ A jump far away puts the line on view row 11 (MINT06 34) without
  scrolling past the last line (33). }
procedure JumpTo;
begin
  EdTop := EdLine - 11;
  if EdTop > DocLines - ViewRows + 2 then EdTop := DocLines - ViewRows + 2;
  if EdTop < 1 then EdTop := 1;
  ScrollTo()
end;

{ ---- drawing ---- }

procedure DrawText;
var R, L, C, I: integer;
    S: string;
begin
  PageCover(EdTop, ViewRows);
  if BufErr <> 0 then
  begin
    { A failed page replacement leaves the old page live. Stay on it;
      never use the requested line numbers to index another page. }
    EdTop := PgLine;
    if not PageHas(EdLine) then EdLine := PgLine;
    ErrCode := BufErr; ErrReturn := ModeEditor; UiMode := ModeError
  end;
  R := 0;
  while R < ViewRows do
  begin
    ScrFill(R + 2, 0, 79, ' ', AtText);
    L := EdTop + R;
    if L <= DocLines then
    begin
      LineText(L, S);
      { Walk source bytes directly: an expanded line can exceed the
        ShortString limit. Only the visible cells need storage. }
      C := 0; I := 1;
      while I <= Length(S) do
      begin
        if S[I] = chr(9) then C := (C div 8 + 1) * 8
        else
        begin
          if (C + 1 >= EdLeft) and (C + 1 < EdLeft + 80) then
            ScrPut(R + 2, C + 1 - EdLeft, S[I], AtText);
          C := C + 1
        end;
        I := I + 1
      end
    end;
    R := R + 1
  end
end;

procedure DrawInfo;
var N: string;
begin
  if ErrLineShown then
  begin
    { MINT06 07: the error replaces the info row, 0x4E across it }
    ScrFill(1, 0, 79, ' ', AtError);
    ScrText(1, 0, ErrLineText, AtError)
  end
  else
  begin
  ScrFill(1, 0, 79, ' ', AtInfo);
  Tx := 'Line'; ScrText(1, 6, Tx, AtInfo);
  NumText(EdLine, N); ScrText(1, 11, N, AtInfo);
  Tx := 'Col'; ScrText(1, 17, Tx, AtInfo);
  NumText(EdCol, N); ScrText(1, 21, N, AtInfo);
  if EdInsert then begin Tx := 'Insert'; ScrText(1, 25, Tx, AtInfo) end;
  if EdModified then ScrPut(1, 57, '*', AtInfo);
  Tx := FileDrive + ':' + FileName;
  ScrText(1, 59, Tx, AtInfo)
  end
end;

procedure DrawTitle(Which, Col: integer);
begin
  if Which = 1 then Tx := 'File'
  else if Which = 2 then Tx := 'Edit'
  else if Which = 3 then Tx := 'Run'
  else Tx := 'Compile';
  if ((UiMode = ModeBar) or (UiMode = ModeMenu)) and (MenuSel = Which) then
    ScrText(0, Col, Tx, AtSelect)
  else if (UiMode = ModeVerify) and (MenuSel = Which) and (VerifyKind <> VerifyQuit) then
    ScrText(0, Col, Tx, AtSelect)
  else if (UiMode = ModeName) and (MenuSel = Which) and (NameKind <> NameRename) then
    ScrText(0, Col, Tx, AtSelect)
  else if (UiMode = ModeOverwrite) and (MenuSel = Which) then
    ScrText(0, Col, Tx, AtSelect)
  else if (UiMode = ModeError) and (MenuSel = Which) and (ErrReturn = ModeMenu) then
    ScrText(0, Col, Tx, AtSelect)
  else
  begin
    ScrText(0, Col, Tx, AtMenu);
    ScrAttr(0, Col, AtMenuKey)
  end;
end;

procedure DrawBar;
begin
  ScrFill(0, 0, 79, ' ', AtMenu);
  DrawTitle(1, 4); DrawTitle(2, 12); DrawTitle(3, 20); DrawTitle(4, 27)
end;

procedure DrawStatus;
begin
  ScrFill(24, 0, 79, ' ', AtMenu);
  if (UiMode = ModeVerify) or (UiMode = ModeError) or (UiMode = ModeOverwrite) then
  begin
    Tx := 'Esc'; ScrText(24, 1, Tx, AtMenuKey);
    Tx := '-Abort'; ScrText(24, 4, Tx, AtMenu)
  end
  else
  begin
    Tx := 'F9'; ScrText(24, 1, Tx, AtMenuKey);
    Tx := '-Make'; ScrText(24, 3, Tx, AtMenu);
    Tx := 'F10'; ScrText(24, 10, Tx, AtMenuKey);
    Tx := '-Menu'; ScrText(24, 13, Tx, AtMenu)
  end
end;

function ItemCount(Menu: integer): integer;
begin
  if Menu = MenuFile then ItemCount := 5 else ItemCount := 2
end;

procedure ItemText(Menu, Item: integer; var S: string);
begin
  S := '';
  if Menu = MenuFile then
  begin
    if Item = 1 then S := ' Load      F3 '
    else if Item = 2 then S := ' New          '
    else if Item = 3 then S := ' Save      F2 '
    else if Item = 4 then S := ' Write to     '
    else S := ' Quit   Alt-X '
  end
  else if Menu = MenuRun then
  begin
    if Item = 1 then S := ' Run           Ctrl-F9 ' else S := ' User screen    Alt-F5 '
  end
  else
  begin
    if Item = 1 then S := ' Compile  Alt-F9                 '
    else S := ' Make         F9                 '
  end
end;

{ Pulldown frames as captured: File (3,1)-(18,..) MINT06 19, Run (19,1)
  20, Compile (26,1) 21; absent items are removed, the rest keep order. }
function MenuLeft(Menu: integer): integer;
begin
  if Menu = MenuFile then MenuLeft := 3
  else if Menu = MenuRun then MenuLeft := 19
  else MenuLeft := 26
end;

procedure DrawMenu;
var I, L: integer;
    S: string;
begin
  L := MenuLeft(MenuSel);
  ItemText(MenuSel, 1, S);
  ScrBox(1, L, ItemCount(MenuSel) + 2, L + Length(S) + 1, false, AtMenu);
  I := 1;
  while I <= ItemCount(MenuSel) do
  begin
    ItemText(MenuSel, I, S);
    if I = ItemSel[MenuSel] then
      ScrText(I + 1, L + 1, S, AtSelect)
    else
    begin
      ScrText(I + 1, L + 1, S, AtMenu);
      ScrAttr(I + 1, L + 2, AtMenuKey)
    end;
    I := I + 1
  end
end;

procedure DrawName;
var Len: integer;
begin
  ScrBox(NameRow, NameCol, NameRow + 2, NameCol + 43, false, AtMenu);
  ScrFill(NameRow + 1, NameCol + 1, NameCol + 42, ' ', AtMenu);
  if NameKind = NameRename then Tx := 'Rename NONAME'
  else if NameKind = NameLoad then Tx := 'Load File Name'
  else Tx := 'New Name';
  ScrTitle(NameRow, NameCol, NameCol + 43, Tx, AtMenu);
  ScrText(NameRow + 1, NameCol + 2, NameText, AtSelect);
  Len := Length(NameText);
  ScrCursor(NameRow + 1, NameCol + 2 + Len, true)
end;

procedure DrawVerify;
var W: integer;
begin
  Tx := ' ' + FileName + ' not saved.  Save? (Y/N) ';
  W := Length(Tx) + 2;
  ScrBox(5, 10, 7, 10 + W - 1, true, AtVerify);
  ScrText(6, 11, Tx, AtMenu);
  Tx := 'Verify';
  ScrTitle(5, 10, 10 + W - 1, Tx, AtVerify)
end;

procedure LeafName(var S, L: string); forward;

{ Overwrite prompt (MINT06 49): a Verify box where the name box was. }
procedure DrawOverwrite;
var L: string;
    W: integer;
begin
  LeafName(PendingName, L);
  Tx := ' Overwrite ' + L + '? (Y/N) ';
  W := Length(Tx) + 2;
  ScrBox(NameRow, NameCol, NameRow + 2, NameCol + W - 1, true, AtVerify);
  ScrText(NameRow + 1, NameCol + 1, Tx, AtMenu);
  Tx := 'Verify';
  ScrTitle(NameRow, NameCol, NameCol + W - 1, Tx, AtVerify)
end;

{ Error box (MINT06 51, 54): DOS 3 and 5 as captured; others not captured. }
procedure DrawError;
var W: integer;
    N: string;
begin
  { creating a file: 2 or 3 both mean the path cannot be reached }
  if (ErrCode = 2) or (ErrCode = 3) then Tx := 'Invalid drive or directory.'
  else if ErrCode = 5 then Tx := 'File is read only.'
  else
  begin
    NumText(ErrCode, N);
    Tx := 'File error ' + N + '.'
  end;
  Tx := ' ' + Tx + '  Press ';
  W := Length(Tx) + 5 + 2;
  ScrBox(10, 20, 12, 20 + W - 1, true, AtErrFrame);
  ScrText(11, 21, Tx, AtErrText);
  N := 'ESC';
  ScrText(11, 21 + Length(Tx), N, AtError);
  N := '. ';
  ScrText(11, 24 + Length(Tx), N, AtErrText);
  N := 'Error';
  ScrTitle(10, 20, 20 + W - 1, N, AtErrText)
end;

{ MINT06 13c: the disk compile box. Values describe this compilation and
  DOS's actual largest free conventional block, rather than TP's machine. }
procedure DrawCompile;
var N: string;
    R: integer;
begin
  R := 8;
  while R <= 18 do begin ScrFill(R, 20, 58, ' ', AtMenu); R := R + 1 end;
  ScrBox(7, 19, 19, 59, true, AtMenu);
  Tx := 'Compiling to Disk'; ScrTitle(7, 19, 59, Tx, AtMenu);
  Tx := 'Main file: '; ScrText(9, 21, Tx, AtMenu);
  ScrText(9, 32, FileName, AtMenu);
  Tx := 'Compiling: EDITOR '; ScrText(10, 21, Tx, AtMenu);
  ScrPut(10, 39, chr(26), AtMenu); ScrText(10, 41, FileName, AtMenu);
  Tx := 'Total    File'; ScrText(12, 40, Tx, AtMenu);
  Tx := 'Lines compiled:'; ScrText(13, 24, Tx, AtMenu);
  NumText(CompileLines, N); ScrText(13, 40, N, AtMenu); ScrText(13, 49, N, AtMenu);
  Tx := 'Available memory:'; ScrText(15, 24, Tx, AtMenu);
  NumText(CompileMemory div 1024, N); N := N + 'K'; ScrText(15, 43, N, AtMenu);
  if CompileDone then
  begin
    ScrFill(18, 20, 58, ' ', AtBarText);
    Tx := 'Success         :'; ScrText(18, 22, Tx, AtBarText);
    ScrFill(18, 39, 58, ' ', AtBlink);
    Tx := 'Press any key'; ScrText(18, 43, Tx, AtBlink)
  end;
  ScrCursor(0, 0, false)
end;

procedure DrawAll;
begin
  DrawText();
  DrawInfo();
  DrawBar();
  DrawStatus();
  ScrCursor(EdLine - EdTop + 2, EdCol - EdLeft, UiMode = ModeEditor);
  if (UiMode = ModeMenu) or ((UiMode = ModeVerify) and (VerifyKind <> VerifyQuit)) then
    DrawMenu()
  else if (UiMode = ModeName) and (NameKind <> NameRename) then
    DrawMenu()
  else if UiMode = ModeOverwrite then
    DrawMenu()
  else if (UiMode = ModeError) and (ErrReturn = ModeMenu) then
    DrawMenu();
  if UiMode = ModeName then DrawName();
  if UiMode = ModeVerify then DrawVerify();
  if UiMode = ModeOverwrite then DrawOverwrite();
  if UiMode = ModeError then DrawError();
  if UiMode = ModeCompile then DrawCompile()
end;

{ ---- file commands ---- }

procedure Upper(var S: string);
var I: integer;
begin
  I := 1;
  while I <= Length(S) do
  begin
    if (S[I] >= 'a') and (S[I] <= 'z') then S[I] := chr(ord(S[I]) - 32);
    I := I + 1
  end
end;

procedure BakName(var S, B: string);
var I, Dot: integer;
    C: string;
begin
  Dot := 0; I := 1;
  while I <= Length(S) do
  begin
    if S[I] = '.' then Dot := I;
    if S[I] = chr(92) then Dot := 0;
    I := I + 1
  end;
  B := ''; I := 1;
  if Dot = 0 then Dot := Length(S) + 1;
  while I < Dot do
  begin
    C := S[I];
    B := B + C;
    I := I + 1
  end;
  B := B + '.BAK'
end;

procedure CursorHome;
begin
  EdLine := 1; EdCol := 1; EdTop := 1; EdLeft := 1
end;

procedure DoNew;
begin
  BufNew();
  FileName := 'NONAME.PAS';
  FilePath := '';
  EdModified := false; EdOnDisk := false;
  CursorHome();
  UiMode := ModeEditor
end;

procedure ShowFileError(Code, Back: integer);
begin
  ErrCode := Code;
  ErrReturn := Back;
  UiMode := ModeError
end;

{ A missing file opens as a new, empty buffer of that name (TP 5.5 F3;
  not in the captures). Other errors leave the buffer as it was. }
procedure DoLoad(var Name: string);
var L: string;
begin
  BufLoad(Name);
  LeafName(Name, L);
  if BufErr = 2 then
  begin
    BufNew();
    FileName := L;
    FilePath := Name;
    EdModified := false; EdOnDisk := false;
    CursorHome();
    UiMode := ModeEditor
  end
  else if BufErr <> 0 then ShowFileError(BufErr, ModeMenu)
  else
  begin
    FileName := L;
    FilePath := Name;
    EdModified := false; EdOnDisk := true;
    CursorHome();
    UiMode := ModeEditor
  end
end;

procedure ExtName(var S, Ext, B: string);
var I, Dot: integer;
    C: string;
begin
  Dot := 0; I := 1;
  while I <= Length(S) do
  begin
    if S[I] = '.' then Dot := I;
    if S[I] = chr(92) then Dot := 0;
    I := I + 1
  end;
  B := ''; I := 1;
  if Dot = 0 then Dot := Length(S) + 1;
  while I < Dot do
  begin
    C := S[I];
    B := B + C;
    I := I + 1
  end;
  B := B + Ext
end;

{ DOS names compare without case; generated siblings must never alias
  the source. .BAK sources use .BK1 for their previous version. }
function SameName(var A, B: string): boolean;
var X, Y: string;
begin
  X := A; Y := B; Upper(X); Upper(Y);
  SameName := X = Y
end;

function PrivateName(var Path: string): boolean;
var Leaf, P: string;
begin
  LeafName(Path, Leaf); Upper(Leaf);
  P := PrivateBase + '.PAS';
  PrivateName := SameName(Leaf, P) or SameName(Leaf, WorkName0) or SameName(Leaf, WorkName1)
end;

{ Write aside, close and check, then use only DOS renames to publish.
  MINT06 54 refuses a read-only source; 49 confirms overwrites. The
  operator's save ruling permits the residual double rename failure:
  leave both the backup and stage intact if rollback also fails. }
function SaveAs(var Path: string; Existing: boolean): integer;
var Temp, Bak, Ext: string;
    Code, I, Attr: integer;
    F: file;
    Moved, KeepStage, StageOwned: boolean;
begin
  Ext := '.$$$'; ExtName(Path, Ext, Temp);
  Ext := '.BAK'; ExtName(Path, Ext, Bak);
  if SameName(Path, Temp) then
  begin Ext := '.$$1'; ExtName(Path, Ext, Temp) end;
  if SameName(Path, Bak) then
  begin Ext := '.BK1'; ExtName(Path, Ext, Bak) end;
  Code := 0; Moved := false; KeepStage := false; StageOwned := false;
  { A pre-existing stage belongs to someone else. Never truncate or erase
    it, including when a prior rollback left recovery material behind. }
  Assign(F, Temp); Attr := FileAttributes(F);
  if Attr >= 0 then Code := 80
  else if Attr <> -2 then Code := -Attr;
  if PrivateName(Path) then Code := 5;
  if Existing and (Code = 0) then
  begin
    Assign(F, Path);
    Attr := FileAttributes(F);
    if Attr < 0 then Code := -Attr
    else if Attr mod 2 = 1 then Code := 5
  end;
  if Code = 0 then
  begin
    StageOwned := true;
    BufSave(Temp);
    Code := BufErr
  end;
  if (Code = 0) and Existing then
  begin
    Assign(F, Bak); Erase(F); I := IOResult();
    if (I <> 0) and (I <> 2) then Code := I;
    if Code = 0 then
    begin
      Assign(F, Path); Rename(F, Bak); Code := IOResult();
      Moved := Code = 0
    end
  end;
  if Code = 0 then
  begin
    Assign(F, Temp); Rename(F, Path); Code := IOResult();
    if (Code <> 0) and Moved then
    begin
      Assign(F, Bak); Rename(F, Path); I := IOResult();
      KeepStage := I <> 0
    end
  end;
  if (Code <> 0) and StageOwned and (not KeepStage) then
  begin
    Assign(F, Temp); Erase(F); I := IOResult()
  end;
  SaveAs := Code
end;

function FileExists(var Path: string): boolean;
var F: file;
    Code: integer;
begin
  BufErr := 0;
  Assign(F, Path);
  Reset(F, 1);
  Code := IOResult();
  if Code = 0 then
  begin
    Close(F);
    Code := IOResult();
    FileExists := Code = 0
  end
  else FileExists := false;
  { Only FILE NOT FOUND proves absence. Access/close errors must not turn
    an existing destination into an unconfirmed new-file save. }
  if (Code <> 0) and (Code <> 2) then BufErr := Code
end;

procedure DoSave;
var Code: integer;
begin
  Code := SaveAs(FilePath, EdOnDisk);
  if Code = 0 then
  begin
    EdOnDisk := true;
    EdModified := false;
    UiMode := ModeEditor
  end
  else ShowFileError(Code, ModeEditor)
end;

procedure OpenName(Kind: integer);
begin
  NameKind := Kind;
  NameFresh := true;
  if Kind = NameRename then
  begin
    NameRow := 3; NameCol := 7;
    NameText := FileDir + chr(92) + FileName
  end
  else
  begin
    NameCol := 5;
    NameRow := ItemSel[MenuFile] + 2;
    if Kind = NameLoad then NameText := '*.PAS' else NameText := ''
  end;
  UiMode := ModeName
end;

function Wild(var S: string): boolean;
var I: integer;
    W: boolean;
begin
  W := false; I := 1;
  while I <= Length(S) do
  begin
    if (S[I] = '*') or (S[I] = '?') then W := true;
    I := I + 1
  end;
  Wild := W
end;

{ Strip a typed drive and directory for the info line and file name. }
procedure LeafName(var S, L: string);
var I: integer;
    C: string;
begin
  L := ''; I := 1;
  while I <= Length(S) do
  begin
    if (S[I] = chr(92)) or (S[I] = ':') then L := ''
    else
    begin
      C := S[I];
      L := L + C
    end;
    I := I + 1
  end
end;

{ Save under a new name; on success the buffer takes that name. }
procedure WriteTo(var Path: string; Existing: boolean);
var L: string;
    Code: integer;
begin
  Code := SaveAs(Path, Existing);
  if Code = 0 then
  begin
    LeafName(Path, L);
    FileName := L;
    FilePath := Path;
    EdOnDisk := true;
    EdModified := false;
    { after the Rename box TP shows the file from its start (MINT06 06) }
    if NameKind = NameRename then CursorHome();
    UiMode := ModeEditor
  end
  else if NameKind = NameRename then ShowFileError(Code, ModeEditor)
  else ShowFileError(Code, ModeMenu)
end;

procedure NameAccept;
var L: string;
begin
  Upper(NameText);
  if Length(NameText) = 0 then UiMode := ModeEditor
  else if Wild(NameText) then NameFresh := false
  else if NameKind = NameLoad then
  begin
    if EdModified then
    begin
      PendingName := NameText;
      VerifyKind := VerifyLoad;
      UiMode := ModeVerify
    end
    else DoLoad(NameText)
  end
  else
  begin
    if FileExists(NameText) then
    begin
      PendingName := NameText;
      UiMode := ModeOverwrite
    end
    else if BufErr <> 0 then
    begin
      if NameKind = NameRename then ShowFileError(BufErr, ModeEditor)
      else ShowFileError(BufErr, ModeMenu)
    end
    else WriteTo(NameText, false)
  end
end;

procedure SaveCommand;
begin
  if FileName = 'NONAME.PAS' then OpenName(NameRename)
  else
    DoSave()
end;

procedure VerifyDone(Saved: boolean);
begin
  if Saved then DoSave();
  if UiMode <> ModeError then
  begin
    if VerifyKind = VerifyNew then DoNew()
    else if VerifyKind = VerifyLoad then DoLoad(PendingName)
    else UiMode := ModeQuit
  end
end;

procedure RunItem;
var I: integer;
begin
  I := ItemSel[MenuSel];
  LastMenu := MenuSel;
  if MenuSel = MenuFile then
  begin
    if I = 1 then OpenName(NameLoad)
    else if I = 2 then
    begin
      if EdModified then begin VerifyKind := VerifyNew; UiMode := ModeVerify end
      else DoNew()
    end
    else if I = 3 then SaveCommand()
    else if I = 4 then OpenName(NameWrite)
    else
    begin
      if EdModified then begin VerifyKind := VerifyQuit; UiMode := ModeVerify end
      else UiMode := ModeQuit
    end
  end
  else
  begin
    UiMode := ModeEditor;
    if MenuSel = MenuRun then
    begin
      if I = 1 then RequestedCommand := CmdRun else RequestedCommand := CmdUser
    end
    else if I = 1 then RequestedCommand := CmdCompile else RequestedCommand := CmdMake
  end
end;

{ ---- editing ---- }

procedure CurLineBytes(var S: string; var Start: integer);
var Len: integer;
begin
  PageCover(EdLine, 1);
  if not PageHas(EdLine) then EdLine := PgLine;
  LineSpan(EdLine, Start, Len);
  LineText(EdLine, S)
end;

procedure TypeChar(C: char);
var S: string;
    Start, B, Len: integer;
begin
  CurLineBytes(S, Start);
  Len := DispLen(S);
  if Len < LineMax then
  begin
    while DispLen(S) + 1 < EdCol do
    begin
      PageInsert(Start + Length(S), ' ', EdLine);
      CurLineBytes(S, Start)
    end;
    B := ColByte(S, EdCol);
    if (not EdInsert) and (B <= Length(S)) then PageDelete(Start + B - 1, EdLine);
    CurLineBytes(S, Start);
    PageInsert(Start + B - 1, C, EdLine);
    EdCol := EdCol + 1;
    EdModified := true
  end
end;

procedure NewLine;
var S: string;
    Start, B: integer;
begin
  CurLineBytes(S, Start);
  B := ColByte(S, EdCol);
  PageInsert(Start + B - 1, chr(10), EdLine);
  EdLine := EdLine + 1;
  EdCol := 1;
  EdModified := true
end;

procedure BackSpace;
var S, P: string;
    Start, B: integer;
begin
  CurLineBytes(S, Start);
  if EdCol > DispLen(S) + 1 then EdCol := EdCol - 1
  else if EdCol > 1 then
  begin
    B := ColByte(S, EdCol);
    EdCol := ByteCol(S, B - 1);
    PageDelete(Start + B - 2, EdLine);
    EdModified := true
  end
  else if EdLine > 1 then
  begin
    EdLine := EdLine - 1;
    CurLineBytes(P, Start);
    EdCol := DispLen(P) + 1;
    PageDelete(Start + Length(P), EdLine);
    EdModified := true
  end
end;

procedure DeleteChar;
var S: string;
    Start, B: integer;
begin
  CurLineBytes(S, Start);
  if EdCol <= DispLen(S) then
  begin
    B := ColByte(S, EdCol);
    PageDelete(Start + B - 1, EdLine);
    EdModified := true
  end
  else if EdLine < DocLines then
  begin
    while DispLen(S) + 1 < EdCol do
    begin
      PageInsert(Start + Length(S), ' ', EdLine);
      CurLineBytes(S, Start)
    end;
    PageDelete(Start + Length(S), EdLine);
    EdModified := true
  end
end;

procedure DeleteLine;
var S: string;
    Start, I: integer;
begin
  CurLineBytes(S, Start);
  I := Length(S);
  if EdLine < DocLines then I := I + 1;
  while I > 0 do
  begin
    PageDelete(Start, EdLine);
    I := I - 1
  end;
  EdCol := 1;
  EdModified := true
end;

procedure EditKey(K: integer);
var S: string;
    Start: integer;
begin
  if (K >= 32) and (K < 256) then TypeChar(chr(K))
  else if K = KeyEnter then NewLine()
  else if K = KeyBack then BackSpace()
  else if K = KeyDel then DeleteChar()
  else if K = KeyCtrlY then DeleteLine()
  else if K = KeyIns then EdInsert := not EdInsert
  else if K = KeyLeft then begin if EdCol > 1 then EdCol := EdCol - 1 end
  else if K = KeyRight then begin if EdCol < LineMax then EdCol := EdCol + 1 end
  else if K = KeyUp then begin if EdLine > 1 then EdLine := EdLine - 1 end
  else if K = KeyDown then begin if EdLine < DocLines then EdLine := EdLine + 1 end
  else if K = KeyHome then EdCol := 1
  else if K = KeyEnd then
  begin
    CurLineBytes(S, Start);
    EdCol := DispLen(S) + 1
  end
  else if K = KeyPgDn then
  begin
    { MINT06 27, 28: 21 lines, the cursor keeps its view row }
    if EdTop + ViewRows - 1 <= DocLines then
    begin
      EdTop := EdTop + ViewRows - 1;
      EdLine := EdLine + ViewRows - 1;
      if EdLine > DocLines then EdLine := DocLines
    end
    else EdLine := DocLines
  end
  else if K = KeyPgUp then
  begin
    EdTop := EdTop - ViewRows + 1;
    EdLine := EdLine - ViewRows + 1;
    if EdTop < 1 then EdTop := 1;
    if EdLine < 1 then EdLine := 1
  end
  else if K = KeyCtrlHome then EdLine := EdTop
  else if K = KeyCtrlEnd then
  begin
    EdLine := EdTop + ViewRows - 1;
    if EdLine > DocLines then EdLine := DocLines
  end
  else if K = KeyCtrlPgUp then begin EdLine := 1; EdTop := 1 end
  else if K = KeyCtrlPgDn then begin EdLine := DocLines; JumpTo() end;
  ScrollTo()
end;

{ ---- key dispatch ---- }

procedure OpenMenu(Menu: integer);
begin
  MenuSel := Menu;
  LastMenu := Menu;
  if Menu = MenuEdit then UiMode := ModeEditor else UiMode := ModeMenu
end;

function HotMenu(K: integer): integer;
var M: integer;
begin
  M := 0;
  if (K = ord('f')) or (K = ord('F')) or (K = KeyAltF) then M := MenuFile
  else if (K = ord('e')) or (K = ord('E')) or (K = KeyAltE) then M := MenuEdit
  else if (K = ord('r')) or (K = ord('R')) or (K = KeyAltR) then M := MenuRun
  else if (K = ord('c')) or (K = ord('C')) or (K = KeyAltC) then M := MenuCompile;
  HotMenu := M
end;

function HotItem(K: integer): integer;
var I, Found: integer;
    S: string;
begin
  Found := 0; I := 1;
  if (K >= ord('a')) and (K <= ord('z')) then K := K - 32;
  while I <= ItemCount(MenuSel) do
  begin
    ItemText(MenuSel, I, S);
    if ord(S[2]) = K then Found := I;
    I := I + 1
  end;
  HotItem := Found
end;

procedure GlobalKey(K: integer);
begin
  if (K = KeyAltF) or (K = KeyAltR) or (K = KeyAltC) then OpenMenu(HotMenu(K))
  else if K = KeyAltE then UiMode := ModeEditor
  else if K = KeyF10 then begin MenuSel := LastMenu; UiMode := ModeBar end
  else if K = KeyF2 then SaveCommand()
  else if K = KeyF3 then
  begin
    MenuSel := MenuFile; ItemSel[MenuFile] := 1; LastMenu := MenuFile;
    OpenName(NameLoad)
  end
  else if K = KeyF9 then RequestedCommand := CmdMake
  else if K = KeyAltF9 then RequestedCommand := CmdCompile
  else if K = KeyCtrlF9 then RequestedCommand := CmdRun
  else if K = KeyAltF5 then RequestedCommand := CmdUser
  else if K = KeyAltX then
  begin
    if EdModified then begin VerifyKind := VerifyQuit; UiMode := ModeVerify end
    else UiMode := ModeQuit
  end
end;

function IsGlobal(K: integer): boolean;
begin
  IsGlobal := (K = KeyAltF) or (K = KeyAltR) or (K = KeyAltC) or (K = KeyAltE) or
              (K = KeyF10) or (K = KeyF2) or (K = KeyF3) or (K = KeyAltX) or
              (K = KeyF9) or (K = KeyAltF9) or (K = KeyCtrlF9) or (K = KeyAltF5)
end;

{ Show a compiler error (MINT06 07, 34): the message replaces the info
  row and the cursor goes to the token; Col is the source byte column,
  shown through the tabs. A line off screen is placed on view row 11. }
procedure IdeError(Line, Col: integer; var Msg: string);
var S: string;
begin
  if Line > DocLines then Line := DocLines;
  if Line < 1 then Line := 1;
  EdLine := Line;
  PageCover(EdLine, 1);
  LineText(EdLine, S);
  EdCol := ByteCol(S, Col);
  if (EdLine < EdTop) or (EdLine > EdTop + ViewRows - 1) then JumpTo()
  else ScrollTo();
  ErrLineText := Msg;
  ErrLineShown := true;
  UiMode := ModeEditor
end;

procedure IdeKey(K: integer);
var M, I: integer;
    C: string;
begin
  if (UiMode = ModeCompile) or (UiMode = ModeUser) then UiMode := ModeEditor
  else if (UiMode = ModeEditor) and ErrLineShown then
  begin
    { the next key restores the info row (MINT06 08); Esc does no more }
    ErrLineShown := false;
    if K <> KeyEsc then
    begin
      if IsGlobal(K) then GlobalKey(K) else EditKey(K)
    end
  end
  else if UiMode = ModeEditor then
  begin
    if IsGlobal(K) then GlobalKey(K) else EditKey(K)
  end
  else if UiMode = ModeError then
  begin
    if K = KeyEsc then UiMode := ErrReturn
  end
  else if UiMode = ModeOverwrite then
  begin
    if (K = ord('y')) or (K = ord('Y')) then WriteTo(PendingName, true)
    else if (K = ord('n')) or (K = ord('N')) or (K = KeyEsc) then
    begin
      if NameKind = NameRename then UiMode := ModeEditor else UiMode := ModeMenu
    end
  end
  else if UiMode = ModeBar then
  begin
    M := HotMenu(K);
    if M > 0 then OpenMenu(M)
    else if K = KeyLeft then begin MenuSel := (MenuSel + 2) mod 4 + 1; LastMenu := MenuSel end
    else if K = KeyRight then begin MenuSel := MenuSel mod 4 + 1; LastMenu := MenuSel end
    else if (K = KeyEnter) or (K = KeyDown) then OpenMenu(MenuSel)
    else if (K = KeyEsc) or (K = KeyF10) then UiMode := ModeEditor
    else if IsGlobal(K) then GlobalKey(K)
  end
  else if UiMode = ModeMenu then
  begin
    I := HotItem(K);
    if I > 0 then begin ItemSel[MenuSel] := I; RunItem() end
    else if K = KeyDown then ItemSel[MenuSel] := ItemSel[MenuSel] mod ItemCount(MenuSel) + 1
    else if K = KeyUp then
      ItemSel[MenuSel] := (ItemSel[MenuSel] + ItemCount(MenuSel) - 2) mod ItemCount(MenuSel) + 1
    else if K = KeyEnter then RunItem()
    else if K = KeyEsc then UiMode := ModeBar
    else if K = KeyLeft then
    begin
      MenuSel := (MenuSel + 2) mod 4 + 1; LastMenu := MenuSel;
      if MenuSel = MenuEdit then UiMode := ModeBar
    end
    else if K = KeyRight then
    begin
      MenuSel := MenuSel mod 4 + 1; LastMenu := MenuSel;
      if MenuSel = MenuEdit then UiMode := ModeBar
    end
    else if IsGlobal(K) then GlobalKey(K)
  end
  else if UiMode = ModeName then
  begin
    if K = KeyEnter then NameAccept()
    else if K = KeyEsc then
    begin
      if NameKind = NameRename then UiMode := ModeEditor else UiMode := ModeMenu
    end
    else if K = KeyBack then
    begin
      NameFresh := false;
      if Length(NameText) > 0 then
      begin
        C := '';
        I := 1;
        while I < Length(NameText) do
        begin
          C := C + NameText[I];
          I := I + 1
        end;
        NameText := C
      end
    end
    else if (K > 32) and (K < 127) then
    begin
      if NameFresh then NameText := '';
      NameFresh := false;
      if Length(NameText) < 40 then
      begin
        C := chr(K);
        NameText := NameText + C
      end
    end
  end
  else if UiMode = ModeVerify then
  begin
    if (K = ord('y')) or (K = ord('Y')) then VerifyDone(true)
    else if (K = ord('n')) or (K = ord('N')) then VerifyDone(false)
    else if K = KeyEsc then
    begin
      { a cancelled Alt-X leaves the bar on File (MINT06 57) }
      if VerifyKind = VerifyQuit then
      begin
        MenuSel := MenuFile;
        LastMenu := MenuFile;
        UiMode := ModeBar
      end
      else UiMode := ModeMenu
    end
  end
end;

{ The state after start (MINT06 02_editor): an empty NONAME.PAS, the menu
  bar active on File, the cursor hidden. }
function FreeSibling(var Base, Ext: string): boolean;
var Path: string;
    F: file;
begin
  Path := Base + Ext; Assign(F, Path);
  FreeSibling := FileAttributes(F) = -2
end;

procedure IdeStart;
var I: integer;
    S, Ext: string;
    Free: boolean;
begin
  I := 0; Free := false;
  while (I < 1000) and (not Free) do
  begin
    NumText(I, S); while Length(S) < 3 do S := '0' + S;
    PrivateBase := 'TURBO' + S;
    Ext := '.$00'; Free := FreeSibling(PrivateBase, Ext);
    Ext := '.$01'; Free := Free and FreeSibling(PrivateBase, Ext);
    Ext := '.PAS'; Free := Free and FreeSibling(PrivateBase, Ext);
    I := I + 1
  end;
  if not Free then begin Writeln('Cannot create working files.'); Halt(1) end;
  WorkName0 := PrivateBase + '.$00'; WorkName1 := PrivateBase + '.$01';
  RequestedCommand := 0;
  EdInsert := true;
  ItemSel[1] := 1; ItemSel[2] := 1; ItemSel[3] := 1; ItemSel[4] := 1;
  DoNew();
  if BufErr <> 0 then begin Writeln('Cannot create working files.'); Halt(1) end;
  MenuSel := MenuFile;
  LastMenu := MenuFile;
  UiMode := ModeBar
end;
{ END IDE editor }

{ BEGIN IDE product }
{ Product integration: compiler + editor, one TURBO.COM. Compile commands
  use the actual compiler record, not a diagnostic-file parser. Geometry
  is TP5.5 MINT06 07/08/13c/34/35. All services use public DOS calls. }
var
  DisplayPage, UserPage: array[1..4002] of char;
  BuildRequest: integer;
  BuildPending, ProgramReady, KeepRecovery: boolean;
  BuildBase, ReadyName, BuildInput: string;

procedure ProductPaint;
var I, Code: integer;
begin
  if UiMode = ModeUser then ScreenPage(1, UserPage[1])
  else
  begin
    DrawAll(); I := 0;
    while I < 2000 do
    begin
      DisplayPage[I * 2 + 1] := ScrCh[I];
      DisplayPage[I * 2 + 2] := chr(ScrAt[I]); I := I + 1
    end;
    if ScrCurOn then
    begin
      DisplayPage[4001] := chr(ScrCurRow); DisplayPage[4002] := chr(ScrCurCol)
    end
    else begin DisplayPage[4001] := chr(25); DisplayPage[4002] := chr(0) end;
    ScreenPage(2, DisplayPage[1])
  end;
  Code := IOResult();
  if Code <> 0 then Halt(1)
end;

procedure RemoveOwned(var Path: string);
var F: file;
    Code: integer;
begin
  Assign(F, Path); Erase(F); Code := IOResult();
  if (Code <> 0) and (Code <> 2) then KeepRecovery := true
end;

procedure CleanBuild;
var Path: string;
begin
  if (Length(BuildBase) > 0) and (not KeepRecovery) then
  begin
    Path := BuildBase + '.S'; RemoveOwned(Path);
    Path := BuildBase + '.COM'; RemoveOwned(Path);
    Path := BuildBase + '.EXE'; RemoveOwned(Path);
    Path := BuildBase + '.CBK'; RemoveOwned(Path);
    Path := BuildBase + '.EBK'; RemoveOwned(Path)
  end
end;

{ Output siblings share the selected file's directory: DOS AH=56h is a
  same-directory rename. Choose names absent before the first create. }
function NewBuildNames(): boolean;
var Prefix, S, Ext, Path, Target: string;
    I, Last, J: integer;
    Free: boolean;
begin
  Prefix := ''; Last := 0; I := 1;
  while I <= Length(FilePath) do
  begin
    if (FilePath[I] = chr(92)) or (FilePath[I] = ':') then Last := I;
    I := I + 1
  end;
  I := 1;
  while I <= Last do begin S := FilePath[I]; Prefix := Prefix + S; I := I + 1 end;
  J := 0; Free := false;
  while (J < 1000) and (not Free) do
  begin
    NumText(J, S); while Length(S) < 3 do S := '0' + S;
    BuildBase := Prefix + 'COMPI' + S;
    Ext := '.S'; Free := FreeSibling(BuildBase, Ext);
    Ext := '.COM'; Free := Free and FreeSibling(BuildBase, Ext);
    Ext := '.EXE'; Free := Free and FreeSibling(BuildBase, Ext);
    Ext := '.CBK'; Free := Free and FreeSibling(BuildBase, Ext);
    Ext := '.EBK'; Free := Free and FreeSibling(BuildBase, Ext);
    Target := FilePath; if Length(Target) = 0 then Target := FileName;
    Ext := '.EXE'; ExtName(Target, Ext, Path);
    S := BuildBase + Ext;
    if SameName(Path, S) then Free := false;
    J := J + 1
  end;
  OutputAsmName := BuildBase + '.S'; OutputComName := BuildBase + '.COM';
  OutputExeName := BuildBase + '.EXE';
  NewBuildNames := Free
end;

function PublishProgram(var From, Into, Backup: string): integer;
var F: file;
    Attr, Code, Rollback: integer;
    Moved: boolean;
begin
  Code := 0; Moved := false;
  if SameName(Into, FilePath) then Code := 5;
  if Code = 0 then
  begin
    Assign(F, Into); Attr := FileAttributes(F);
    if Attr >= 0 then
    begin
      if (Attr mod 2 = 1) or (Attr div 16 mod 2 = 1) then Code := 5
      else
      begin
        Rename(F, Backup); Code := IOResult(); Moved := Code = 0
      end
    end
    else if Attr <> -2 then Code := -Attr
  end;
  if Code = 0 then
  begin
    Assign(F, From); Rename(F, Into); Code := IOResult();
    if (Code <> 0) and Moved then
    begin
      Assign(F, Backup); Rename(F, Into); Rollback := IOResult();
      if Rollback <> 0 then KeepRecovery := true
    end
  end;
  if (Code = 0) and Moved then RemoveOwned(Backup);
  PublishProgram := Code
end;

procedure CompilerMessage(var Msg: string);
var N: string;
begin
  if (FirstKind = 2) and (FirstExpected = TkSemi) then
    Msg := 'Error 85: ";" expected'
  else if (FirstKind = 1) and (FirstDetail = ErrSourceFull) then
    Msg := 'Source exceeds compiler capacity.'
  else
  begin
    NumText(FirstDetail, N); Msg := 'Compiler error ' + N + '.'
  end
end;

procedure FinishBuild;
var Target, Backup, Ext, Msg: string;
    Code, Attr: integer;
    F: file;
begin
  BuildPending := false; ProgramReady := false;
  CleanBuild();
  BuildInput := PrivateBase + '.PAS';
  BufSave(BuildInput); Code := BufErr;
  if Code = 0 then
  begin
    if not NewBuildNames() then Code := 5
  end;
  if Code = 0 then
  begin
    InputName := BuildInput; DumpMode := false; QuietCompile := true;
    CompileSource(); CompileLines := DocLines;
    if FirstKind = 5 then Code := FirstDetail
    else if FirstKind = 4 then Code := 101
    else if FirstKind <> 0 then
    begin
      CompilerMessage(Msg); IdeError(FirstLine, FirstCol, Msg)
    end
    else
    begin
      Target := FilePath; if Length(Target) = 0 then Target := FileName;
      Ext := '.EXE'; ExtName(Target, Ext, ReadyName);
      Backup := BuildBase + '.EBK';
      Code := PublishProgram(OutputExeName, ReadyName, Backup);
      if Code = 0 then
      begin
        Assign(F, OutputComName); Attr := FileAttributes(F);
        if Attr >= 0 then
        begin
          Ext := '.COM'; ExtName(Target, Ext, ReadyName);
          Backup := BuildBase + '.CBK';
          Code := PublishProgram(OutputComName, ReadyName, Backup);
          ProgramReady := Code = 0
        end
      end;
      if Code = 0 then begin CompileDone := true; UiMode := ModeCompile end
    end
  end;
  if Code <> 0 then ShowFileError(Code, ModeEditor)
end;

procedure ProductKey(K: integer);
begin
  IdeKey(K);
  if RequestedCommand <> 0 then
  begin
    if RequestedCommand = CmdUser then UiMode := ModeUser
    else
    begin
      BuildRequest := RequestedCommand; BuildPending := true; ProgramReady := false;
      CompileDone := false; CompileLines := 0; CompileMemory := DosMemory();
      UiMode := ModeCompile
    end;
    RequestedCommand := 0
  end
end;

procedure ProductCleanup;
var Path: string;
begin
  CleanBuild();
  Path := PrivateBase + '.PAS'; RemoveOwned(Path);
  RemoveOwned(WorkName0); RemoveOwned(WorkName1)
end;

procedure ProductMain;
var K: integer;
    C: char;
begin
  BuildBase := ''; KeepRecovery := false; BuildPending := false;
  ProgramReady := false;
  ScreenPage(0, UserPage[1]); K := IOResult(); if K <> 0 then Halt(1);
  FileDrive := 'A'; FileDir := '.';
  IdeStart();
  while UiMode <> ModeQuit do
  begin
    ProductPaint();
    if BuildPending then FinishBuild()
    else
    begin
      C := ReadKey(); K := ord(C);
      if K = 0 then begin C := ReadKey(); K := 256 + ord(C) end;
      ProductKey(K)
    end
  end;
  ProductCleanup(); ScreenPage(1, UserPage[1])
end;
{ END IDE product }
begin
  CompilerDefaults();
  if ParamCount() > 0 then
  begin
    InputName := ParamStr(1);
    if InputName = '/DUMP' then
    begin
      DumpMode := true; InputName := 'TURBOIN.PAS';
      if ParamCount() > 1 then InputName := ParamStr(2)
    end;
    CompileSource();
    if (FirstKind <> 0) and (not DumpMode) then Halt(1)
  end
  else ProductMain()
end.
