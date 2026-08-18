{$MODE DELPHI}{$B+}{$H-}
{
  tps.pas -- Turbo Initech single-file bootstrap, B9.4 x86 codegen slice.

  Ref: docs/plans/TPS-M7-subset-plan.md Sec 3 and Sec 5; ADR-0007
  DEC-02, DEC-04, DEC-05, and DEC-07; beads initech-6m52 and
  initech-lh83. Pascal lives here by CLAUDE.md Law 3.

  Input is the fixed B8 filename TPSIN.PAS. A two-ShortString rolling window
  supplies exact bytes to the monotonic lexer and is reopened for each driver
  pass; this lets TPS check its grown source without enlarging BSS. The B9.2
  SourceWords arena is retired; the build gate now proves the rolling window
  and the absence of that dead 80 KiB reservation. Overflow is loud.
  Identifiers are canonical lower-case and live in one global PoolChars array.

  Dump protocols are specified in LEXER-DUMP.md, PARSER-TRACE.md, and
  SYMBOLS-DUMP.md. The driver emits the unchanged lexer bracket, resets for
  the unchanged syntax-only recursive-descent trace, then resets once more
  for an AST-free semantic parse and deterministic symbol dump.

  PHASE DIVERGENCE (deliberate): seed/typecheck.c parses to an AST, collects
  declarations, then checks that AST (lines 10-23 and 2076-2127). TPS owns no
  AST by design. Its third driver pass mirrors the seed's semantic rules while
  checking expressions/statements as they are parsed. This is the plan's
  sanctioned single-pass architecture; it changes phase structure, not the
  type compatibility rules. A call to a later routine therefore needs an
  explicit prior forward in TPS, while the seed's collect_procs-before-bodies
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
    TPS-LEX-BEGIN
    one token per line
    TPS-LEX-END
  Keywords use KW plus their lower-case spelling. Identifiers use IDENT plus
  the pooled spelling. Integers use INTEGER plus decimal value. Strings use
  STRING, decoded length, then one decimal byte per payload character. Fixed
  punctuation kind names and EOF carry no payload. Errors use one located
  TPS-LEX-ERROR line, stop the driver, then close the output bracket.

  Parser trace:
    TPS-PARSE-BEGIN
    ENTER production Lline / EXIT production Lline
    TPS-PARSE-OK or one located TPS-PARSE-ERROR line
    TPS-PARSE-END

  Source discipline: boolean evaluation is complete. Every bounds-sensitive
  access is guarded by nested if statements or by a sentinel accessor, and
  flag-controlled loops test the flag alone.
}
program TurboInitech;

const
  SourceMax = 262144;
  PoolMax = 6144;
  NameMax = 704;
  ChunkMax = 255;
  EscapeQuoteCode = 39; { TPS_LEX_MUT_STRESC }

  { B9.4 fixed arenas. Nine integer columns x 1,024 symbols = 36,864 bytes;
    header/argument/name scratch remains fixed. Retiring B9.2's 81,920-byte
    whole-source arena pays for these tables. B9.4 raises the row ceiling for
    the generator's own helpers; the hard 0x6F000 image-arena ceiling remains
    authoritative (bead initech-6m52). }
  SymbolMax = 1024;
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
    through the B8 byte-file surface to the fixed name TPSOUT.S. Generation is
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

procedure ReadStreamBlock(var Text: string; var Got: integer);
begin
  Text := '';
  BlockRead(InputFile, Text[1], ChunkMax, Got);
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
  AtEnd := ChunkLen = 0
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
    HadError := true;
    Write('TPS-LEX-ERROR pos=', P, ' line=', L, ' col=', C);
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
    GenFlags[2] := true;
    ParseFailed := true;
    TypeFailed := true;
    Write('TPS-GEN-ERROR line=', L, ' col=', C, ' ');
    if Code = 1 then Writeln('output short write')
    else if Code = 2 then Writeln('invalid symbol storage')
    else if Code = 3 then Writeln('invalid expression storage')
    else if Code = 4 then Writeln('call stack invariant')
    else Writeln('internal invariant code=', Code)
  end
end;

procedure GenFlush();
var
  Need: integer;
begin
  Need := Length(OutputChunk);
  if Need > 0 then
  begin
    BlockWrite(OutputFile, OutputChunk[1], Need, OutputActual);
    if OutputActual <> Need then
      GenFail(TokLine, TokCol, 1)
    else
      GenData[2] := GenData[2] + OutputActual;
    OutputChunk := ''
  end
end;

procedure GenChar(C: char);
var
  N: integer;
begin
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
    TokText[I] := LowerAscii(TokText[I]); { TPS_LEX_MUT_CASEFOLD }
    I := I + 1
  end
end;

procedure SkipTrivia();
var
  C: char;
  Again, Done: boolean;
  StartPos, StartLine, StartCol: integer;
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
              C := TakeChar()
          end
        end;
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
  Assign(InputFile, 'TPSIN.PAS');
  Reset(InputFile, 1);
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
    ParseFailed := true;
    Write('TPS-PARSE-ERROR pos=', TokPos, ' line=', TokLine,
          ' col=', TokCol, ' expected=');
    WriteExpected(Expected);
    Write(' found=');
    WriteTokenName(TokKind);
    Writeln()
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
    TypeFailed := true;
    ParseFailed := true;
    Write('TPS-TYPE-ERROR line=', L, ' col=', C, ' ');
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
  if Existing <> 0 then begin { TPS_TYPE_MUT_DUPOK }
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
      else if TypeId = TyFile then OffsetValue := -(4 * (LocalSlots + Count));
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
  if Builtin <> 0 then
  begin
    if CallWantsValue then
      FailType(TeRule, PendingIdentLine, PendingIdentCol,
               PendingIdentOffset, PendingIdentLength, 0, 0)
    else
    begin
      if Builtin = 1 then
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
        if Builtin = 1 then
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
        else
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
    GenFrameOffset := SymOffset[Index] { TPS_GEN_MUT_OFFBYONE }
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
    if SymKind[Index] = SkLocalArray then
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
    if (SymKind[BaseIndex] = SkLocalArray) or
       ((SymScope[BaseIndex] > 0) and
        (SymKind[BaseIndex] <> SkVarParam) and
        (SymKind[BaseIndex] <> SkValueParam) and (not HadIndex)) then
      begin GenText := '    sub eax, '; GenPut(GenText) end
    else
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
  begin GenText := '    jmp '; GenPut(GenText) end;
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

procedure GenScalarBinary(OperatorKind: integer);
begin
  begin GenText := '    mov ecx, eax'; GenLine(GenText) end;
  begin GenText := '    pop eax'; GenLine(GenText) end;
  if OperatorKind = TkPlus then begin GenText := '    add eax, ecx'; GenLine(GenText) end
  else if OperatorKind = TkMinus then begin GenText := '    sub eax, ecx'; GenLine(GenText) end
  else if OperatorKind = TkStar then begin GenText := '    imul eax, ecx'; GenLine(GenText) end
  else if OperatorKind = TkDiv then
  begin
    begin GenText := '    cdq'; GenLine(GenText) end;
    begin GenText := '    idiv ecx'; GenLine(GenText) end
  end
  else if OperatorKind = TkMod then
  begin
    begin GenText := '    cdq'; GenLine(GenText) end;
    begin GenText := '    idiv ecx'; GenLine(GenText) end;
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
  begin GenText := '    jbe .len_ok'; GenLine(GenText) end;
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
  begin GenText := '    jbe .n_ok'; GenLine(GenText) end;
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
  begin GenText := '    jae .have_min'; GenLine(GenText) end;
  begin GenText := '    mov ecx, edx'; GenLine(GenText) end;
  begin GenText := '.have_min:'; GenLine(GenText) end;
  begin GenText := '    push eax'; GenLine(GenText) end;
  begin GenText := '    push edx'; GenLine(GenText) end;
  begin GenText := '    inc esi'; GenLine(GenText) end;
  begin GenText := '    inc edi'; GenLine(GenText) end;
  begin GenText := '.cmp_loop:'; GenLine(GenText) end;
  begin GenText := '    test ecx, ecx'; GenLine(GenText) end;
  begin GenText := '    jz .prefix_equal'; GenLine(GenText) end;
  begin GenText := '    mov al, [esi]'; GenLine(GenText) end;
  begin GenText := '    mov dl, [edi]'; GenLine(GenText) end;
  begin GenText := '    cmp al, dl'; GenLine(GenText) end;
  begin GenText := '    jb .a_less'; GenLine(GenText) end;
  begin GenText := '    ja .a_greater'; GenLine(GenText) end;
  begin GenText := '    inc esi'; GenLine(GenText) end;
  begin GenText := '    inc edi'; GenLine(GenText) end;
  begin GenText := '    dec ecx'; GenLine(GenText) end;
  begin GenText := '    jmp .cmp_loop'; GenLine(GenText) end;
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
  begin GenText := '    jb .len_less'; GenLine(GenText) end;
  begin GenText := '    ja .len_greater'; GenLine(GenText) end;
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
  begin GenText := '    jz .w_done'; GenLine(GenText) end;
  begin GenText := '    mov al, [esi]'; GenLine(GenText) end;
  begin GenText := '    call serial_putc'; GenLine(GenText) end;
  begin GenText := '    inc esi'; GenLine(GenText) end;
  begin GenText := '    dec ecx'; GenLine(GenText) end;
  begin GenText := '    jmp .w_loop'; GenLine(GenText) end;
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
            if I = 1 then GenFlags[5] := true
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
            if (Builtin = 1) and (ArgCount = 2) then
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
                begin
                  if BaseWasString and HadIndex then
                    begin GenText := '    movzx eax, byte [eax]'; GenLine(GenText) end
                  else
                    begin GenText := '    mov eax, [eax]'; GenLine(GenText) end
                end
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
    if TokKind = TkStar then begin { TPS_PARSE_MUT_PRECEDENCE }
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
  TargetLvalue, WholeString, TargetHadIndex, TargetDescending: boolean;
  SourceDescending, SavedWantAddress: boolean;
begin
  TraceEnter(PrAssignment);
  TargetSym := 0;
  TargetType := TyUnknown;
  TargetRecord := 0;
  TargetForm := 1;
  TargetLvalue := false;
  WholeString := false;
  TargetHadIndex := false;
  TargetDescending := false;
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
    if GenFlags[1] then GenDesignatorAddress(TargetSym);
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
      TargetDescending := SymKind[TargetSym] = SkLocalArray;
      if not TargetDescending then
        TargetDescending := (SymScope[TargetSym] > 0) and
                            (SymKind[TargetSym] <> SkVarParam) and
                            (SymKind[TargetSym] <> SkValueParam) and
                            (not TargetHadIndex);
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
      if ValueType <> TargetType then begin { TPS_TYPE_MUT_ASSIGN }
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
        SourceDescending := SymKind[LastExprSym] = SkLocalArray;
        if not SourceDescending then
          SourceDescending := (SymScope[LastExprSym] > 0) and
                              (SymKind[LastExprSym] <> SkVarParam) and
                              (SymKind[LastExprSym] <> SkValueParam) and
                              (LastExprForm = 1);
        begin GenText := '    mov edx, [esp]'; GenLine(GenText) end;
        I := 0;
        while I < SymSize[TargetRecord] div 4 do
        begin
          OffsetValue := I * 4;
          begin GenText := '    mov ecx, [eax'; GenPut(GenText) end;
          if OffsetValue > 0 then
          begin
            if SourceDescending then GenChar('-') else GenChar('+');
            GenInt(OffsetValue)
          end;
          begin GenText := ']'; GenPut(GenText) end;
          GenChar(Chr(10));
          begin GenText := '    mov [edx'; GenPut(GenText) end;
          if OffsetValue > 0 then
          begin
            if TargetDescending then GenChar('-') else GenChar('+');
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
        if (SymType[TargetSym] = TyString) and (TargetForm = 2) then
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
            begin GenText := '    jz '; GenPut(GenText) end;
            begin GenText := '.Lbfalse_'; GenPut(GenText); GenInt(LabelNumber) end;
            GenChar(Chr(10));
            begin GenText := '    mov eax, str_bool_true'; GenLine(GenText) end;
            begin GenText := '    jmp '; GenPut(GenText) end;
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
      begin GenText := '    jz '; GenPut(GenText) end;
      begin GenText := '.Lelse_'; GenPut(GenText); GenInt(LabelNumber) end;
      GenChar(Chr(10))
    end;
  if not ParseFailed then ExpectToken(TkThen, TkThen);
  if not ParseFailed then ParseStatement();
  if not ParseFailed then
    if GenFlags[1] then
    begin
      begin GenText := '    jmp '; GenPut(GenText) end;
      begin GenText := '.Lendif_'; GenPut(GenText); GenInt(LabelNumber) end;
      GenChar(Chr(10));
      begin GenText := '.Lelse_'; GenPut(GenText); GenInt(LabelNumber) end;
      begin GenText := ':'; GenPut(GenText) end;
      GenChar(Chr(10))
    end;
  if not ParseFailed then
  begin
    if TokKind = TkElse then begin { TPS_PARSE_MUT_ELSE }
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
    LabelNumber := GenNewLabel(); { TPS_GEN_MUT_LABELS }
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
      begin GenText := '    jz '; GenPut(GenText) end;
      begin GenText := '.Lwhile_end_'; GenPut(GenText); GenInt(LabelNumber) end;
      GenChar(Chr(10))
    end;
  if not ParseFailed then ExpectToken(TkDo, TkDo);
  if not ParseFailed then ParseStatement();
  if not ParseFailed then
    if GenFlags[1] then
    begin
      begin GenText := '    jmp '; GenPut(GenText) end;
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
      begin GenText := '    push eax'; GenLine(GenText) end;
      GenDesignatorAddress(ControlSym);
      begin GenText := '    pop ecx'; GenLine(GenText) end;
      begin GenText := '    mov [eax], ecx'; GenLine(GenText) end
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
      begin GenText := '    push eax'; GenLine(GenText) end;
      LabelNumber := GenNewLabel();
      begin GenText := '.Lfor_top_'; GenPut(GenText); GenInt(LabelNumber) end;
      begin GenText := ':'; GenPut(GenText) end;
      GenChar(Chr(10));
      GenDesignatorAddress(ControlSym);
      begin GenText := '    mov eax, [eax]'; GenLine(GenText) end;
      begin GenText := '    cmp eax, [esp]'; GenLine(GenText) end;
      if Direction = 1 then begin GenText := '    jg '; GenPut(GenText) end
      else begin GenText := '    jl '; GenPut(GenText) end;
      begin GenText := '.Lfor_end_'; GenPut(GenText); GenInt(LabelNumber) end;
      GenChar(Chr(10))
    end;
  if not ParseFailed then ExpectToken(TkDo, TkDo);
  if not ParseFailed then ParseStatement();
  if not ParseFailed then
    if GenFlags[1] then
    begin
      GenDesignatorAddress(ControlSym);
      if Direction = 1 then begin GenText := '    add dword [eax], 1'; GenLine(GenText) end
      else begin GenText := '    sub dword [eax], 1'; GenLine(GenText) end;
      begin GenText := '    jmp '; GenPut(GenText) end;
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
      begin GenText := '    jz '; GenPut(GenText) end;
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
            begin GenText := '    mov eax, '; GenPut(GenText) end;
            I := FindLocal(NameOffset, NameLength);
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
    if not HadError then Writeln('TPS-PARSE-OK')
end;

procedure RunTypeChecker();
begin
  ReadInput();
  ScanPos := 1;
  ScanLine := 1;
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
      DumpSymbols();
      Writeln('TPS-TYPE-OK')
    end;
  TypeMode := false
end;

procedure RunCodeGenerator();
begin
  ReadInput();
  ScanPos := 1;
  ScanLine := 1;
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
  Assign(OutputFile, 'TPSOUT.S');
  Rewrite(OutputFile, 1);
  begin GenText := '; Generated by Turbo Initech (TPS B9.4).'; GenLine(GenText) end;
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
          begin GenText := 'extern rtl_file_blockwrite'; GenLine(GenText) end
        end;
        if GenFlags[3] then GenStringIntrinsics();
        GenFlush()
      end;
  if not GenFlags[2] then
    if not TypeFailed then
      if not ParseFailed then
        Writeln('TPS-GEN-OK bytes=', GenData[2],
                ' labels=', GenData[3]);
  GenFlags[1] := false;
  TypeMode := false
end;

begin
  HadError := false;
  Writeln('TPS-LEX-BEGIN');
  ReadInput();
  RunLexer();
  Writeln('TPS-LEX-END');
  Writeln('TPS-PARSE-BEGIN');
  if HadError then
    Writeln('TPS-PARSE-SKIP lexer-error')
  else
    RunParser();
  Writeln('TPS-PARSE-END');
  SyntaxClean := not ParseFailed;
  Writeln('TPS-TYPE-BEGIN');
  if HadError then
    Writeln('TPS-TYPE-SKIP lexer-error')
  else if not SyntaxClean then
    Writeln('TPS-TYPE-SKIP parser-error')
  else
    RunTypeChecker();
  Writeln('TPS-TYPE-END');
  Writeln('TPS-GEN-BEGIN');
  if HadError then
    Writeln('TPS-GEN-SKIP lexer-error')
  else if not SyntaxClean then
    Writeln('TPS-GEN-SKIP parser-error')
  else if TypeFailed then
    Writeln('TPS-GEN-SKIP type-error')
  else
    RunCodeGenerator();
  Writeln('TPS-GEN-END')
end.
