{$MODE DELPHI}{$B+}{$H-}
{
  tps.pas -- Turbo Initech single-file bootstrap, B9.1 lexer slice.

  Ref: docs/plans/TPS-M7-subset-plan.md Sec 3 and Sec 5; ADR-0007
  DEC-02, DEC-04, DEC-05, and DEC-07; beads initech-6m52 and
  initech-lh83. Pascal lives here by CLAUDE.md Law 3.

  Input is the fixed B8 filename TPSIN.PAS. BlockRead fills a ShortString
  staging block because the seed stores array-of-char elements four bytes
  apart. Bytes are packed three per integer in the fixed SourceWords array,
  keeping the whole static BSS inside the DOS runtime reserve. Overflow is
  loud. Identifiers are canonical lower-case and live in one global
  PoolChars array with integer offset and length tables. No array of string.

  Dump protocol, also specified in LEXER-DUMP.md:
    TPS-LEX-BEGIN
    one token per line
    TPS-LEX-END
  Keywords use KW plus their lower-case spelling. Identifiers use IDENT plus
  the pooled spelling. Integers use INTEGER plus decimal value. Strings use
  STRING, decoded length, then one decimal byte per payload character. Fixed
  punctuation kind names and EOF carry no payload. Errors use one located
  TPS-LEX-ERROR line, stop the driver, then close the output bracket.

  Source discipline: boolean evaluation is complete. Every bounds-sensitive
  access is guarded by nested if statements or by a sentinel accessor, and
  flag-controlled loops test the flag alone.
}
program TurboInitech;

const
  SourceMax = 30720;
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

var
  InputFile: file;
  ReadChunk: string;
  SourceWords: array[0..10239] of integer;
  SourceLen, ScanPos, ScanLine, ScanCol: integer;

  PoolChars: array[1..PoolMax] of char;
  PoolOffset, PoolLength: array[1..NameMax] of integer;
  PoolUsed, PoolCount: integer;

  TokKind, TokValue, TokOffset, TokLength: integer;
  TokPos, TokLine, TokCol: integer;
  TokText, FoldedText, DumpText: string;
  HadError: boolean;

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

begin
  HadError := false;
  Writeln('TPS-LEX-BEGIN');
  ReadInput();
  RunLexer();
  Writeln('TPS-LEX-END')
end.
