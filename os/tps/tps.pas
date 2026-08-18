{$MODE DELPHI}{$B+}{$H-}
{
  tps.pas -- Turbo Initech single-file bootstrap, B9.2 parser slice.

  Ref: docs/plans/TPS-M7-subset-plan.md Sec 3 and Sec 5; ADR-0007
  DEC-02, DEC-04, DEC-05, and DEC-07; beads initech-6m52 and
  initech-lh83. Pascal lives here by CLAUDE.md Law 3.

  Input is the fixed B8 filename TPSIN.PAS. BlockRead fills a ShortString
  staging block because the seed stores array-of-char elements four bytes
  apart. Bytes are packed three per integer in the fixed SourceWords array,
  keeping the whole static BSS inside the DOS runtime reserve. Overflow is
  loud. Identifiers are canonical lower-case and live in one global
  PoolChars array with integer offset and length tables. No array of string.

  Dump protocols are specified in LEXER-DUMP.md and PARSER-TRACE.md. The
  driver emits the unchanged lexer bracket, resets the one-token lexer, and
  then emits a recursive-descent production trace. No token buffer, AST, or
  symbol table exists in B9.2.

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
  SourceMax = 61440;
  PoolMax = 4096;
  NameMax = 512;
  ChunkMax = 255;
  EscapeQuoteCode = 39; { TPS_LEX_MUT_STRESC }

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
  ReadChunk: string;
  SourceWords: array[0..20479] of integer;
  SourceLen, ScanPos, ScanLine, ScanCol: integer;

  PoolChars: array[1..PoolMax] of char;
  PoolOffset, PoolLength: array[1..NameMax] of integer;
  PoolUsed, PoolCount: integer;

  TokKind, TokValue, TokOffset, TokLength: integer;
  TokPos, TokLine, TokCol: integer;
  TokText, FoldedText, DumpText: string;
  HadError, ParseFailed: boolean;
  ParseIfDepth: integer;

function SourcePower(P: integer): integer;
var
  Slot: integer;
begin
  Slot := (P - 1) mod 3;
  SourcePower := 1;
  if Slot = 1 then
    SourcePower := 256
  else if Slot = 2 then
    SourcePower := 65536
end;

function SourceGet(P: integer): char;
var
  WordIndex, Power: integer;
begin
  WordIndex := (P - 1) div 3;
  Power := SourcePower(P);
  SourceGet := Chr((SourceWords[WordIndex] div Power) mod 256)
end;

procedure SourcePut(P: integer; C: char);
var
  WordIndex, Slot, Power, NextPower: integer;
  OldValue, LowerPart, UpperPart: integer;
begin
  WordIndex := (P - 1) div 3;
  Slot := (P - 1) mod 3;
  Power := SourcePower(P);
  OldValue := SourceWords[WordIndex];
  LowerPart := OldValue mod Power;
  UpperPart := 0;
  if Slot < 2 then
  begin
    NextPower := Power * 256;
    UpperPart := (OldValue div NextPower) * NextPower
  end;
  SourceWords[WordIndex] := LowerPart + Ord(C) * Power + UpperPart
end;

function AtEnd(): boolean;
begin
  AtEnd := ScanPos > SourceLen
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
  if not HadError then
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
  Got, I: integer;
  Reading: boolean;
begin
  SourceLen := 0;
  Assign(InputFile, 'TPSIN.PAS');
  Reset(InputFile, 1);
  Reading := true;
  while Reading do
  begin
    ReadChunk := '';
    BlockRead(InputFile, ReadChunk[1], ChunkMax, Got);
    ReadChunk[0] := Chr(Got);
    if Got = 0 then
      Reading := false
    else
    begin
      if SourceLen + Got > SourceMax then
      begin
        FailLex(ErrSourceFull, SourceMax + 1, 0, 0, SourceMax);
        Reading := false
      end
      else
      begin
        I := 1;
        while I <= Got do
        begin
          SourcePut(SourceLen + I, ReadChunk[I]);
          I := I + 1
        end;
        SourceLen := SourceLen + Got
      end
    end
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
  Write('ENTER ');
  WriteProductionName(Code);
  Writeln(' L', TokLine)
end;

procedure TraceExit(Code: integer);
begin
  Write('EXIT ');
  WriteProductionName(Code);
  Writeln(' L', TokLine)
end;

procedure FailParse(Expected: integer);
begin
  if not ParseFailed then
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

procedure ParseExpression(); forward;
procedure ParseStatement(); forward;
procedure ParseBlock(); forward;

procedure ParseArrayBound();
var
  Negative: boolean;
begin
  TraceEnter(PrArrayBound);
  Negative := false;
  if not ParseFailed then
  begin
    if TokKind = TkMinus then
    begin
      Negative := true;
      NextToken()
    end;
    if TokKind = TkInteger then
      NextToken()
    else if not Negative then
    begin
      if TokKind = TkIdent then
        NextToken()
      else
        FailParse(ExpArrayBound)
    end
    else
      FailParse(ExpArrayBound)
  end;
  if not ParseFailed then TraceExit(PrArrayBound)
end;

procedure ParseTypeName();
begin
  TraceEnter(PrTypeName);
  if not ParseFailed then
  begin
    if TokKind = TkIntegerType then NextToken()
    else if TokKind = TkBoolean then NextToken()
    else if TokKind = TkChar then NextToken()
    else if TokKind = TkFile then NextToken()
    else if TokKind = TkIdent then NextToken()
    else if TokKind = TkStringType then
    begin
      NextToken();
      if TokKind = TkLBracket then
      begin
        NextToken();
        ParseArrayBound();
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
  if not ParseFailed then
  begin
    if TokKind = TkIntegerType then NextToken()
    else if TokKind = TkBoolean then NextToken()
    else if TokKind = TkChar then NextToken()
    else FailParse(ExpResultType)
  end;
  if not ParseFailed then TraceExit(PrResultType)
end;

procedure ParseCall();
var
  Done: boolean;
begin
  TraceEnter(PrCall);
  if not ParseFailed then ExpectToken(TkLParen, TkLParen);
  if not ParseFailed then
  begin
    if TokKind <> TkRParen then
    begin
      Done := false;
      while not Done do
      begin
        ParseExpression();
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
    if TokKind = TkIdent then NextToken()
    else FailParse(ExpFieldName)
  end;
  if not ParseFailed then TraceExit(PrFieldSuffix)
end;

procedure ParseFactor();
begin
  TraceEnter(PrFactor);
  if not ParseFailed then
  begin
    if TokKind = TkMinus then
    begin
      NextToken();
      ParseFactor()
    end
    else if TokKind = TkNot then
    begin
      NextToken();
      ParseFactor()
    end
    else if TokKind = TkTrue then NextToken()
    else if TokKind = TkFalse then NextToken()
    else if TokKind = TkInteger then NextToken()
    else if TokKind = TkString then NextToken()
    else if TokKind = TkOrd then
    begin
      NextToken();
      if not ParseFailed then ExpectToken(TkLParen, TkLParen);
      if not ParseFailed then ParseExpression();
      if not ParseFailed then ExpectToken(TkRParen, TkRParen)
    end
    else if TokKind = TkChr then
    begin
      NextToken();
      if not ParseFailed then ExpectToken(TkLParen, TkLParen);
      if not ParseFailed then ParseExpression();
      if not ParseFailed then ExpectToken(TkRParen, TkRParen)
    end
    else if TokKind = TkLength then
    begin
      NextToken();
      if not ParseFailed then ExpectToken(TkLParen, TkLParen);
      if not ParseFailed then ParseExpression();
      if not ParseFailed then ExpectToken(TkRParen, TkRParen)
    end
    else if TokKind = TkIdent then
    begin
      NextToken();
      if TokKind = TkLParen then ParseCall()
      else
      begin
        if TokKind = TkLBracket then ParseIndexSuffix();
        if not ParseFailed then
          if TokKind = TkDot then ParseFieldSuffix()
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
begin
  TraceEnter(PrTerm);
  ParseFactor();
  Done := ParseFailed;
  while not Done do
  begin
    if TokKind = TkStar then begin { TPS_PARSE_MUT_PRECEDENCE }
      NextToken();
      ParseFactor()
    end
    else if TokKind = TkDiv then
    begin
      NextToken();
      ParseFactor()
    end
    else if TokKind = TkMod then
    begin
      NextToken();
      ParseFactor()
    end
    else if TokKind = TkAnd then
    begin
      NextToken();
      ParseFactor()
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
begin
  TraceEnter(PrSimpleExpression);
  ParseTerm();
  Done := ParseFailed;
  while not Done do
  begin
    if TokKind = TkPlus then
    begin
      NextToken();
      ParseTerm()
    end
    else if TokKind = TkMinus then
    begin
      NextToken();
      ParseTerm()
    end
    else if TokKind = TkOr then
    begin
      NextToken();
      ParseTerm()
    end
    else
      Done := true;
    if ParseFailed then Done := true
  end;
  if not ParseFailed then TraceExit(PrSimpleExpression)
end;

procedure ParseExpression();
begin
  TraceEnter(PrExpression);
  ParseSimpleExpression();
  if not ParseFailed then
  begin
    if IsRelationalOperator() then
    begin
      NextToken();
      ParseSimpleExpression();
      if not ParseFailed then
        if IsRelationalOperator() then
          FailParse(ExpNoChainedRelation)
    end
  end;
  if not ParseFailed then TraceExit(PrExpression)
end;

procedure ParseAssignment();
begin
  TraceEnter(PrAssignment);
  if not ParseFailed then
    if TokKind = TkLBracket then ParseIndexSuffix();
  if not ParseFailed then
    if TokKind = TkDot then ParseFieldSuffix();
  if not ParseFailed then ExpectToken(TkAssign, TkAssign);
  if not ParseFailed then ParseExpression();
  if not ParseFailed then TraceExit(PrAssignment)
end;

procedure ParseIdentStatement();
begin
  TraceEnter(PrIdentStatement);
  if not ParseFailed then ExpectToken(TkIdent, TkIdent);
  if not ParseFailed then
  begin
    if TokKind = TkLParen then ParseCall()
    else ParseAssignment()
  end;
  if not ParseFailed then TraceExit(PrIdentStatement)
end;

procedure ParseWriteArg();
begin
  TraceEnter(PrWriteArg);
  if not ParseFailed then
  begin
    if TokKind = TkString then NextToken()
    else ParseExpression()
  end;
  if not ParseFailed then TraceExit(PrWriteArg)
end;

procedure ParseWrite();
var
  Done: boolean;
begin
  TraceEnter(PrWrite);
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
  if not ParseFailed then TraceExit(PrWrite)
end;

procedure ParseIf();
begin
  TraceEnter(PrIf);
  ParseIfDepth := ParseIfDepth + 1;
  if not ParseFailed then ExpectToken(TkIf, TkIf);
  if not ParseFailed then ParseExpression();
  if not ParseFailed then ExpectToken(TkThen, TkThen);
  if not ParseFailed then ParseStatement();
  if not ParseFailed then
  begin
    if TokKind = TkElse then begin { TPS_PARSE_MUT_ELSE }
      TraceEnter(PrElseClause);
      NextToken();
      ParseStatement();
      if not ParseFailed then TraceExit(PrElseClause)
    end
  end;
  ParseIfDepth := ParseIfDepth - 1;
  if not ParseFailed then TraceExit(PrIf)
end;

procedure ParseWhile();
begin
  TraceEnter(PrWhile);
  if not ParseFailed then ExpectToken(TkWhile, TkWhile);
  if not ParseFailed then ParseExpression();
  if not ParseFailed then ExpectToken(TkDo, TkDo);
  if not ParseFailed then ParseStatement();
  if not ParseFailed then TraceExit(PrWhile)
end;

procedure ParseFor();
begin
  TraceEnter(PrFor);
  if not ParseFailed then ExpectToken(TkFor, TkFor);
  if not ParseFailed then
  begin
    if TokKind = TkIdent then NextToken()
    else FailParse(ExpVarName)
  end;
  if not ParseFailed then ExpectToken(TkAssign, TkAssign);
  if not ParseFailed then ParseExpression();
  if not ParseFailed then
  begin
    if TokKind = TkTo then NextToken()
    else if TokKind = TkDownto then NextToken()
    else FailParse(ExpToOrDownto)
  end;
  if not ParseFailed then ParseExpression();
  if not ParseFailed then ExpectToken(TkDo, TkDo);
  if not ParseFailed then ParseStatement();
  if not ParseFailed then TraceExit(PrFor)
end;

procedure ParseRepeat();
var
  Done: boolean;
begin
  TraceEnter(PrRepeat);
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
  if not ParseFailed then ParseExpression();
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
  if not ParseFailed then
  begin
    if TokKind = TkMinus then
    begin
      NextToken();
      if TokKind = TkInteger then NextToken()
      else FailParse(ExpConstLiteral)
    end
    else if TokKind = TkInteger then NextToken()
    else if TokKind = TkString then NextToken()
    else if TokKind = TkTrue then NextToken()
    else if TokKind = TkFalse then NextToken()
    else FailParse(ExpConstLiteral)
  end;
  if not ParseFailed then TraceExit(PrConstLiteral)
end;

procedure ParseConstSection();
var
  Done: boolean;
begin
  TraceEnter(PrConstSection);
  if not ParseFailed then ExpectToken(TkConst, TkConst);
  Done := ParseFailed;
  while not Done do
  begin
    TraceEnter(PrConstDecl);
    if TokKind = TkIdent then NextToken()
    else FailParse(ExpConstLiteral);
    if not ParseFailed then ExpectToken(TkEq, TkEq);
    if not ParseFailed then ParseConstLiteral();
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
begin
  TraceEnter(PrRecordType);
  if not ParseFailed then ExpectToken(TkRecord, TkRecord);
  Done := ParseFailed;
  if not Done then
    if TokKind = TkEnd then Done := true;
  while not Done do
  begin
    TraceEnter(PrFieldGroup);
    NamesDone := false;
    while not NamesDone do
    begin
      if TokKind = TkIdent then NextToken()
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
      if TokKind = TkIntegerType then NextToken()
      else if TokKind = TkBoolean then NextToken()
      else if TokKind = TkChar then NextToken()
      else FailParse(ExpTypeName)
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
begin
  TraceEnter(PrTypeSection);
  if not ParseFailed then ExpectToken(TkType, TkType);
  Done := ParseFailed;
  while not Done do
  begin
    TraceEnter(PrTypeDecl);
    if TokKind = TkIdent then NextToken()
    else FailParse(ExpTypeName);
    if not ParseFailed then ExpectToken(TkEq, TkEq);
    if not ParseFailed then ParseRecordType();
    if not ParseFailed then ExpectToken(TkSemi, TkSemi);
    if not ParseFailed then TraceExit(PrTypeDecl);
    if ParseFailed then Done := true
    else if TokKind <> TkIdent then Done := true
  end;
  if not ParseFailed then TraceExit(PrTypeSection)
end;

procedure ParseArrayType();
begin
  TraceEnter(PrArrayType);
  if not ParseFailed then ExpectToken(TkArray, TkArray);
  if not ParseFailed then ExpectToken(TkLBracket, TkLBracket);
  if not ParseFailed then ParseArrayBound();
  if not ParseFailed then ExpectToken(TkDotDot, TkDotDot);
  if not ParseFailed then ParseArrayBound();
  if not ParseFailed then ExpectToken(TkRBracket, TkRBracket);
  if not ParseFailed then ExpectToken(TkOf, TkOf);
  if not ParseFailed then ParseTypeName();
  if not ParseFailed then TraceExit(PrArrayType)
end;

procedure ParseVarDecl();
var
  Done: boolean;
begin
  TraceEnter(PrVarDecl);
  Done := false;
  while not Done do
  begin
    if TokKind = TkIdent then NextToken()
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
    else ParseTypeName()
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
  Done: boolean;
begin
  TraceEnter(PrParamGroup);
  if not ParseFailed then
    if TokKind = TkVar then NextToken();
  Done := false;
  while not Done do
  begin
    if TokKind = TkIdent then NextToken()
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
  Production: integer;
begin
  Production := PrRoutineProcedure;
  if IsFunction then Production := PrRoutineFunction;
  TraceEnter(Production);
  if not ParseFailed then NextToken();
  if not ParseFailed then
  begin
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
  if not ParseFailed then ExpectToken(TkSemi, TkSemi);
  if not ParseFailed then
  begin
    if TokKind = TkForward then
    begin
      NextToken();
      ExpectToken(TkSemi, TkSemi)
    end
    else
    begin
      if TokKind = TkVar then ParseVarSection();
      if not ParseFailed then ParseBlock();
      if not ParseFailed then ExpectToken(TkSemi, TkSemi)
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
  if not ParseFailed then ParseBlock();
  if not ParseFailed then ExpectToken(TkDot, TkDot);
  if not ParseFailed then
    if TokKind <> TkEof then FailParse(TkEof);
  if not ParseFailed then TraceExit(PrProgram)
end;

procedure RunParser();
begin
  ScanPos := 1;
  ScanLine := 1;
  ScanCol := 1;
  PoolUsed := 0;
  PoolCount := 0;
  ParseFailed := false;
  ParseIfDepth := 0;
  NextToken();
  if not HadError then ParseProgram();
  if not ParseFailed then
    if not HadError then Writeln('TPS-PARSE-OK')
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
  Writeln('TPS-PARSE-END')
end.
