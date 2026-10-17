{
  CALCDA.PAS -- CALCULAT.EXE, the Calculator desk accessory, written in
  Turbo Initech Pascal (bead initech-tdnl.23, GUI remediation R5.3: the first
  shipped application authored in the North-Star language). The body that
  follows FLAIR.PAS and the generated CALCDATA.PAS:

      COPY /B FLAIR.PAS+CALCDATA.PAS+CALCDA.PAS TPSIN.PAS
      TPS

  IT IS THE C CALCULATOR, RE-AUTHORED. os/apps/calcda/calc_core.c (the
  engine), calc_view.c (the face) and calcda.c (the tenant) are the
  behavioural specification, routine for routine; the ground truth behind
  them is the real Mac OS 8.1 Calculator (calc_core.h cites every probe;
  spec/calcda/calcda_vectors.tsv). The two builds are INTERCHANGEABLE: for
  every event script, the ordered Toolbox requests this program makes are
  byte-identical to the C build's (test-pascal-calcda, the C, seed-built,
  Turbo Initech-built and Free Pascal-built programs run side by side), so
  the C build's oracles -- the vectors, the 20000-session reference, the
  click/key parity, the readout and face pixels against the captures --
  hold for this one, and the emu gates grade it against the same captures.

  THE ARITHMETIC. calc_core.c computes in x87 EXTENDED (the SANE format of
  the real Calculator): a 64-bit significand, a 15-bit exponent, round to
  nearest even, gradual underflow, infinities and the default NaN, and the
  readout rounds the EXACT binary value. Turbo Initech has no real type
  (ADR-0007 DEC-02 defers reals), so this program carries its own: an IEEE
  754 extended soft-float in integer Pascal -- registers of sign, biased
  exponent and 64 significand bits; exact add/subtract (a 135-bit window,
  the far operand's lost bits jammed into a sticky bit strictly below every
  rounding point), exact 128-bit multiply, a 67-bit restoring divide with a
  sticky remainder, and ONE rounding routine (RNE at the 64-bit or the
  denormal grid; overflow to infinity). No intermediate value exceeds
  2^31 - 1, so the three compilers agree by construction.

  MUTANT KNOBS (Rule 6; anchored lines a sed rewrites in a test copy, never
  in a shipped build): CALCDA_MUT_KEY_CLICK_DIVERGE (a click on '-'
  commands '+'), CALCDA_MUT_ROUND_TRUNC (the soft-float truncates instead of
  rounding), CALCDA_MUT_FIXED_E3 (the fixed range starts at e >= -3).

  Written order-independent under complete boolean evaluation (DEC-03):
  every bounds-sensitive loop runs on a flag. ASCII-clean (Rule 12).
}
const
  { calc_cmd_t (os/apps/calcda/calc_core.h) }
  CmdDot = 10;
  CmdAdd = 11;
  CmdSub = 12;
  CmdMul = 13;
  CmdDiv = 14;
  CmdEq = 15;
  CmdClear = 16;
  CmdNone = 17;
  DisplayW = 12;
  EntryMax = 13;
  { the PC keys (os/flair/event.c): main and keypad Enter, NumLock = Clear }
  VkeyEnter = 28;
  VkeyNumLock = 69;
  { calcda.c's bar }
  MenuFile = 129;
  MenuEdit = 130;
  ItemClose = 1;
  { IEEE 754 extended (x87): the exponent bias, the all-ones exponent, the
    least normal exponent and the weight of the least denormal step }
  ExpBias = 16383;
  ExpTop = 32767;
  ExpMinNormal = -16382;
  ExpMaxNormal = 16383;
  WeightMinStep = -16445;
  { the float registers }
  RegAcc = 0;
  RegX = 1;
  RegN = 2;
  RegP = 3;
  { the working array's regions (each 200 bits) }
  BaseA = 0;
  BaseB = 200;
  BaseR = 400;
  BaseQ = 600;

type
  KeyRec = record
    Cmd, L, T, W, H, Lbl, Ldx, Ldy: integer;
  end;

var
  { the face, decoded from CALCDATA.PAS }
  Keys: array[0..63] of KeyRec;
  GlyphCh, GlyphAdv, GlyphRows: array[0..63] of integer;
  GlyphBits: array[0..639] of integer;
  TablesOk: boolean;

  { the soft-float: register r is FSign[r], FExp[r] (biased) and
    FMan[r * 64 + k], k = 0 the integer bit .. 63 the least bit }
  FSign, FExp: array[0..3] of integer;
  FMan: array[0..255] of integer;
  W: array[0..799] of integer;
  Keep: array[0..63] of integer;

  { the readout's exact decimal expansion (calc_core.c Sec 1) }
  Big: array[0..2399] of integer;
  BigN: integer;
  Dig: array[0..11599] of integer;
  RK: array[0..47] of integer;
  Outc: array[0..31] of integer;
  OutLen: integer;

  { the calculator (calc_t) }
  Pending, Entering, Len: integer;
  EntryCh: array[0..15] of integer;
  Disp: string;

  { the tenant }
  Win, Rc: integer;
  Txt: string;

{ ======================================================================
  THE TABLES (CALCDATA.PAS: two hex digits a byte)
  ====================================================================== }
function HexVal(C: char): integer;
begin
  HexVal := 0;
  if (C >= '0') and (C <= '9') then HexVal := Ord(C) - Ord('0');
  if (C >= 'A') and (C <= 'F') then HexVal := Ord(C) - Ord('A') + 10
end;

function HexByte(var D: string; I: integer): integer;
begin
  HexByte := HexVal(D[2 * I + 1]) * 16 + HexVal(D[2 * I + 2])
end;

procedure LoadTables();
var
  Part, I, R, NKeys, NGlyphs, Count: integer;
  D: string;
begin
  NKeys := 0;
  NGlyphs := 0;
  TablesOk := (CalcKeyCount <= 64) and (CalcGlyphCount <= 64) and
              (CalcGlyphRowMax <= 10);
  Part := 1;
  while Part <= CalcKeyParts + CalcGlyphParts do
  begin
    CalcTableData(Part, D);
    if Part <= CalcKeyParts then
    begin
      Count := Length(D) div 16;
      I := 0;
      while I < Count do
      begin
        if NKeys < 64 then
        begin
          Keys[NKeys].Cmd := HexByte(D, 8 * I);
          Keys[NKeys].L := HexByte(D, 8 * I + 1);
          Keys[NKeys].T := HexByte(D, 8 * I + 2);
          Keys[NKeys].W := HexByte(D, 8 * I + 3);
          Keys[NKeys].H := HexByte(D, 8 * I + 4);
          Keys[NKeys].Lbl := HexByte(D, 8 * I + 5);
          Keys[NKeys].Ldx := HexByte(D, 8 * I + 6);
          Keys[NKeys].Ldy := HexByte(D, 8 * I + 7)
        end;
        NKeys := NKeys + 1;
        I := I + 1
      end
    end
    else
    begin
      Count := Length(D) div (2 * (3 + CalcGlyphRowMax));
      I := 0;
      while I < Count do
      begin
        if NGlyphs < 64 then
        begin
          GlyphCh[NGlyphs] := HexByte(D, (3 + CalcGlyphRowMax) * I);
          GlyphAdv[NGlyphs] := HexByte(D, (3 + CalcGlyphRowMax) * I + 1);
          GlyphRows[NGlyphs] := HexByte(D, (3 + CalcGlyphRowMax) * I + 2);
          R := 0;
          while R < CalcGlyphRowMax do
          begin
            GlyphBits[NGlyphs * 10 + R] := HexByte(D, (3 + CalcGlyphRowMax) * I + 3 + R);
            R := R + 1
          end
        end;
        NGlyphs := NGlyphs + 1;
        I := I + 1
      end
    end;
    Part := Part + 1
  end;
  if NKeys <> CalcKeyCount then TablesOk := false;
  if NGlyphs <> CalcGlyphCount then TablesOk := false
end;

{ ======================================================================
  THE SOFT-FLOAT (IEEE 754 extended, round to nearest even)
  ====================================================================== }
procedure FSetZero(R, Sign: integer);
var
  K: integer;
begin
  FSign[R] := Sign;
  FExp[R] := 0;
  for K := 0 to 63 do FMan[R * 64 + K] := 0
end;

procedure FSetInf(R, Sign: integer);
var
  K: integer;
begin
  FSign[R] := Sign;
  FExp[R] := ExpTop;
  for K := 0 to 63 do FMan[R * 64 + K] := 0;
  FMan[R * 64] := 1
end;

{ the x87 default NaN ("real indefinite"): sign 1, quiet, payload 0 }
procedure FSetNaN(R: integer);
begin
  FSetInf(R, 1);
  FMan[R * 64 + 1] := 1
end;

procedure FCopySigned(Dst, Src, Sign: integer);
var
  K: integer;
begin
  FSign[Dst] := Sign;
  FExp[Dst] := FExp[Src];
  for K := 0 to 63 do FMan[Dst * 64 + K] := FMan[Src * 64 + K]
end;

{ 0 zero, 1 finite non-zero, 2 infinity, 3 NaN }
function FClass(R: integer): integer;
var
  K, Any, Low: integer;
begin
  Any := 0;
  Low := 0;
  for K := 0 to 63 do
    if FMan[R * 64 + K] = 1 then
    begin
      Any := 1;
      if K > 0 then Low := 1
    end;
  if FExp[R] = ExpTop then
  begin
    if Low = 1 then FClass := 3 else FClass := 2
  end
  else if Any = 0 then FClass := 0
  else FClass := 1
end;

{ R's significand, normalized (bit 0 = 1), into W[Base..Base+63]; the
  result is the weight of that leading bit. A denormal's bits are shifted up
  and its weight lowered to match. }
function FUnpack(R, Base: integer): integer;
var
  K, P: integer;
  Found: boolean;
begin
  Found := false;
  P := 0;
  for K := 0 to 63 do
    if not Found then
      if FMan[R * 64 + K] = 1 then
      begin
        Found := true;
        P := K
      end;
  for K := 0 to 63 do
  begin
    W[Base + K] := 0;
    if K + P <= 63 then W[Base + K] := FMan[R * 64 + K + P]
  end;
  if FExp[R] = 0 then FUnpack := 1 - ExpBias - P
  else FUnpack := FExp[R] - ExpBias - P
end;

{ THE rounding: the exact value sum W[BaseR + i] * 2^(ER - i), i < Size
  (sticky already jammed in), rounded to nearest even onto the 64-bit
  significand -- or onto the denormal grid 2^WeightMinStep below the least
  normal exponent -- and packed into register Dst. Overflow is infinity. }
procedure FRound(Dst, Sign, ER, Size: integer);
var
  F, K, N, EP, WLsb, Guard, Sticky, Lsb, Carry, Kk: integer;
  Found, Up: boolean;
begin
  Found := false;
  F := 0;
  for K := 0 to Size - 1 do
    if not Found then
      if W[BaseR + K] = 1 then
      begin
        Found := true;
        F := K
      end;
  if not Found then FSetZero(Dst, Sign)
  else
  begin
    EP := ER - F;
    WLsb := EP - 63;
    if WLsb < WeightMinStep then WLsb := WeightMinStep;
    N := EP - WLsb + 1;
    for K := 0 to 63 do
    begin
      Keep[K] := 0;
      if K < N then
        if F + K < Size then Keep[K] := W[BaseR + F + K]
    end;
    Guard := 0;
    if N >= 0 then
      if F + N < Size then Guard := W[BaseR + F + N];
    Sticky := 0;
    K := F + N + 1;
    if K < F then K := F;
    while K < Size do
    begin
      if W[BaseR + K] = 1 then Sticky := 1;
      K := K + 1
    end;
    Lsb := 0;
    if N >= 1 then Lsb := Keep[N - 1];
    Up := (Guard = 1) and ((Sticky = 1) or (Lsb = 1));
    Up := Up { CALCDA_MUT_ROUND_TRUNC };
    if Up then
    begin
      if N <= 0 then
      begin
        N := 1;
        Keep[0] := 1;
        EP := WLsb
      end
      else
      begin
        Carry := 1;
        K := N - 1;
        while K >= 0 do
        begin
          if Carry = 1 then
          begin
            if Keep[K] = 1 then Keep[K] := 0
            else
            begin
              Keep[K] := 1;
              Carry := 0
            end
          end;
          K := K - 1
        end;
        if Carry = 1 then
        begin
          Keep[0] := 1;
          EP := EP + 1
        end
      end
    end;
    if N <= 0 then FSetZero(Dst, Sign)
    else if EP > ExpMaxNormal then FSetInf(Dst, Sign)
    else
    begin
      FSign[Dst] := Sign;
      if EP >= ExpMinNormal then
      begin
        FExp[Dst] := EP + ExpBias;
        for K := 0 to 63 do FMan[Dst * 64 + K] := Keep[K]
      end
      else
      begin
        FExp[Dst] := 0;
        Kk := ExpMinNormal - EP;
        for K := 0 to 63 do FMan[Dst * 64 + K] := 0;
        for K := 0 to 63 - Kk do FMan[Dst * 64 + Kk + K] := Keep[K]
      end
    end
  end
end;

{ compare W[X..X+Count-1] with W[Y..], most significant first: -1, 0, 1 }
function WCompare(X, Y, Count: integer): integer;
var
  K, Res: integer;
begin
  Res := 0;
  for K := 0 to Count - 1 do
    if Res = 0 then
    begin
      if W[X + K] > W[Y + K] then Res := 1;
      if W[X + K] < W[Y + K] then Res := -1
    end;
  WCompare := Res
end;

{ A + B (Subtract = 0) or A - B (Subtract = 1) into Dst }
procedure FAddSub(Dst, A, B, Subtract: integer);
var
  SA, SB, CA, CB, EA, EB, D, K, J, T, Lost, Carry, Borrow, Any: integer;
begin
  SA := FSign[A];
  SB := FSign[B];
  if Subtract = 1 then SB := 1 - SB;
  CA := FClass(A);
  CB := FClass(B);
  if (CA = 3) or (CB = 3) then FSetNaN(Dst)
  else if (CA = 2) and (CB = 2) then
  begin
    if SA = SB then FSetInf(Dst, SA) else FSetNaN(Dst)
  end
  else if CA = 2 then FSetInf(Dst, SA)
  else if CB = 2 then FSetInf(Dst, SB)
  else if (CA = 0) and (CB = 0) then
  begin
    if (SA = 1) and (SB = 1) then FSetZero(Dst, 1) else FSetZero(Dst, 0)
  end
  else if CA = 0 then FCopySigned(Dst, B, SB)
  else if CB = 0 then FCopySigned(Dst, A, SA)
  else
  begin
    EA := FUnpack(A, BaseA);
    EB := FUnpack(B, BaseB);
    { the larger magnitude in A }
    T := 0;
    if EB > EA then T := 1;
    if EB = EA then
      if WCompare(BaseB, BaseA, 64) > 0 then T := 1;
    if T = 1 then
    begin
      for K := 0 to 63 do
      begin
        J := W[BaseA + K];
        W[BaseA + K] := W[BaseB + K];
        W[BaseB + K] := J
      end;
      J := EA; EA := EB; EB := J;
      J := SA; SA := SB; SB := J
    end;
    D := EA - EB;
    { the window: index i has weight EA + 1 - i; A at 1..64, B at
      1 + D .. 64 + D exactly down to index 133, its lost bits jammed into
      index 134 -- strictly below every rounding point }
    for K := 0 to 134 do
    begin
      W[BaseR + K] := 0;
      W[BaseQ + K] := 0
    end;
    for K := 0 to 63 do W[BaseR + 1 + K] := W[BaseA + K];
    Lost := 0;
    for K := 0 to 63 do
      if W[BaseB + K] = 1 then
      begin
        if D > 140 then Lost := 1
        else
        begin
          J := 1 + D + K;
          if J <= 133 then W[BaseQ + J] := 1 else Lost := 1
        end
      end;
    if Lost = 1 then W[BaseQ + 134] := 1;
    if SA = SB then
    begin
      Carry := 0;
      for K := 134 downto 0 do
      begin
        T := W[BaseR + K] + W[BaseQ + K] + Carry;
        W[BaseR + K] := T mod 2;
        Carry := T div 2
      end
    end
    else
    begin
      Borrow := 0;
      for K := 134 downto 0 do
      begin
        T := W[BaseR + K] - W[BaseQ + K] - Borrow;
        Borrow := 0;
        if T < 0 then
        begin
          T := T + 2;
          Borrow := 1
        end;
        W[BaseR + K] := T
      end
    end;
    Any := 0;
    for K := 0 to 134 do
      if W[BaseR + K] = 1 then Any := 1;
    if Any = 0 then FSetZero(Dst, 0)
    else FRound(Dst, SA, EA + 1, 135)
  end
end;

procedure FMul(Dst, A, B: integer);
var
  S, CA, CB, EA, EB, J, K, P, T, Carry: integer;
begin
  S := (FSign[A] + FSign[B]) mod 2;
  CA := FClass(A);
  CB := FClass(B);
  if (CA = 3) or (CB = 3) then FSetNaN(Dst)
  else if ((CA = 2) and (CB = 0)) or ((CA = 0) and (CB = 2)) then FSetNaN(Dst)
  else if (CA = 2) or (CB = 2) then FSetInf(Dst, S)
  else if (CA = 0) or (CB = 0) then FSetZero(Dst, S)
  else
  begin
    EA := FUnpack(A, BaseA);
    EB := FUnpack(B, BaseB);
    for K := 0 to 127 do W[BaseR + K] := 0;
    { the exact 128-bit product: A's bits added at J + 1 for every 1 of B }
    for J := 63 downto 0 do
      if W[BaseB + J] = 1 then
      begin
        Carry := 0;
        for K := 63 downto 0 do
        begin
          P := K + J + 1;
          T := W[BaseR + P] + W[BaseA + K] + Carry;
          W[BaseR + P] := T mod 2;
          Carry := T div 2
        end;
        P := J;
        while Carry = 1 do
        begin
          T := W[BaseR + P] + 1;
          W[BaseR + P] := T mod 2;
          Carry := T div 2;
          P := P - 1
        end
      end;
    FRound(Dst, S, EA + EB + 1, 128)
  end
end;

procedure FDiv(Dst, A, B: integer);
var
  S, CA, CB, EA, EB, I, K, T, Borrow, Any: integer;
begin
  S := (FSign[A] + FSign[B]) mod 2;
  CA := FClass(A);
  CB := FClass(B);
  if (CA = 3) or (CB = 3) then FSetNaN(Dst)
  else if (CA = 2) and (CB = 2) then FSetNaN(Dst)
  else if (CA = 0) and (CB = 0) then FSetNaN(Dst)
  else if CA = 2 then FSetInf(Dst, S)
  else if CB = 2 then FSetZero(Dst, S)
  else if CB = 0 then FSetInf(Dst, S)
  else if CA = 0 then FSetZero(Dst, S)
  else
  begin
    EA := FUnpack(A, BaseA);
    EB := FUnpack(B, BaseB);
    { the remainder at BaseQ, the divisor at BaseB + 100, both 65 bits
      with a leading 0 }
    W[BaseQ] := 0;
    W[BaseB + 100] := 0;
    for K := 0 to 63 do
    begin
      W[BaseQ + 1 + K] := W[BaseA + K];
      W[BaseB + 101 + K] := W[BaseB + K]
    end;
    for I := 0 to 66 do
    begin
      if WCompare(BaseQ, BaseB + 100, 65) >= 0 then
      begin
        W[BaseR + I] := 1;
        Borrow := 0;
        for K := 64 downto 0 do
        begin
          T := W[BaseQ + K] - W[BaseB + 100 + K] - Borrow;
          Borrow := 0;
          if T < 0 then
          begin
            T := T + 2;
            Borrow := 1
          end;
          W[BaseQ + K] := T
        end
      end
      else W[BaseR + I] := 0;
      for K := 0 to 63 do W[BaseQ + K] := W[BaseQ + K + 1];
      W[BaseQ + 64] := 0
    end;
    Any := 0;
    for K := 0 to 64 do
      if W[BaseQ + K] = 1 then Any := 1;
    W[BaseR + 67] := Any;
    FRound(Dst, S, EA - EB, 68)
  end
end;

{ apply(a, op, b) of calc_core.c }
procedure FApply(Dst, A, Op, B: integer);
begin
  if Op = CmdAdd then FAddSub(Dst, A, B, 0)
  else if Op = CmdSub then FAddSub(Dst, A, B, 1)
  else if Op = CmdMul then FMul(Dst, A, B)
  else if Op = CmdDiv then FDiv(Dst, A, B)
  else FCopySigned(Dst, B, FSign[B])
end;

{ the exact non-negative integer W[BaseA..BaseA+63] (bit 0 = 2^63) into R }
procedure FFromBits(R: integer);
var
  K, F: integer;
  Found: boolean;
begin
  Found := false;
  F := 0;
  for K := 0 to 63 do
    if not Found then
      if W[BaseA + K] = 1 then
      begin
        Found := true;
        F := K
      end;
  if not Found then FSetZero(R, 0)
  else
  begin
    FSign[R] := 0;
    FExp[R] := 63 - F + ExpBias;
    for K := 0 to 63 do
    begin
      FMan[R * 64 + K] := 0;
      if K + F <= 63 then FMan[R * 64 + K] := W[BaseA + K + F]
    end
  end
end;

{ W[BaseA..+63] := W[BaseA..] * 10 + Digit (exact: below 2^64) }
procedure TimesTenPlus(Digit: integer);
var
  K, T, Carry: integer;
begin
  { x8 into BaseB, x2 into BaseQ, then BaseA := both + Digit }
  for K := 0 to 63 do
  begin
    W[BaseB + K] := 0;
    W[BaseQ + K] := 0;
    if K + 3 <= 63 then W[BaseB + K] := W[BaseA + K + 3];
    if K + 1 <= 63 then W[BaseQ + K] := W[BaseA + K + 1]
  end;
  Carry := Digit;
  for K := 63 downto 0 do
  begin
    T := W[BaseB + K] + W[BaseQ + K] + Carry;
    W[BaseA + K] := T mod 2;
    Carry := T div 2
  end
end;

{ entry_value(): the typed number n / 10^f, both exact, one rounding }
procedure EntryValue(R: integer);
var
  I, K, Frac: integer;
begin
  for K := 0 to 63 do W[BaseA + K] := 0;
  Frac := 0;
  I := 0;
  while I < Len do
  begin
    if EntryCh[I] = Ord('.') then Frac := 1
    else TimesTenPlus(EntryCh[I] - Ord('0'));
    I := I + 1
  end;
  FFromBits(RegN);
  { 10^f, f the digits after the point }
  for K := 0 to 63 do W[BaseA + K] := 0;
  W[BaseA + 63] := 1;
  I := 0;
  Frac := 0;
  while I < Len do
  begin
    if Frac = 1 then TimesTenPlus(0);
    if EntryCh[I] = Ord('.') then Frac := 1;
    I := I + 1
  end;
  FFromBits(RegP);
  FDiv(R, RegN, RegP)
end;

{ ======================================================================
  THE READOUT (calc_format of calc_core.c)
  ====================================================================== }
procedure BigMulSmall(M: integer);
var
  I, T, Carry: integer;
begin
  Carry := 0;
  for I := 0 to BigN - 1 do
  begin
    T := Big[I] * M + Carry;
    Big[I] := T mod 65536;
    Carry := T div 65536
  end;
  while Carry <> 0 do
  begin
    Big[BigN] := Carry mod 65536;
    BigN := BigN + 1;
    Carry := Carry div 65536
  end
end;

procedure BigTrim();
var
  Done: boolean;
begin
  Done := false;
  while not Done do
  begin
    if BigN = 0 then Done := true
    else if Big[BigN - 1] <> 0 then Done := true
    else BigN := BigN - 1
  end
end;

{ expand(): |R| (finite, non-zero) into Dig[0..Nd-1], most significant
  first; Q is the count right of the decimal point }
function Expand(R: integer; var Q: integer): integer;
var
  K, I, J, B, Limbs, Bits, E5, Rem, Cur, Nd, T: integer;
  Done: boolean;
begin
  if FExp[R] = 0 then K := 1 - ExpBias - 63
  else K := FExp[R] - ExpBias - 63;
  for J := 0 to 3 do
  begin
    Big[J] := 0;
    for B := 15 downto 0 do
      Big[J] := Big[J] * 2 + FMan[R * 64 + 63 - (16 * J + B)]
  end;
  BigN := 4;
  BigTrim();
  if K >= 0 then
  begin
    Limbs := K div 16;
    Bits := K mod 16;
    for I := BigN - 1 downto 0 do Big[I + Limbs] := Big[I];
    for I := 0 to Limbs - 1 do Big[I] := 0;
    BigN := BigN + Limbs;
    if Bits > 8 then
    begin
      BigMulSmall(256);
      Bits := Bits - 8
    end;
    if Bits > 0 then
    begin
      T := 1;
      for I := 1 to Bits do T := T * 2;
      BigMulSmall(T)
    end;
    Q := 0
  end
  else
  begin
    E5 := -K;
    while E5 >= 6 do
    begin
      BigMulSmall(15625);
      E5 := E5 - 6
    end;
    while E5 > 0 do
    begin
      BigMulSmall(5);
      E5 := E5 - 1
    end;
    Q := -K
  end;
  Nd := 0;
  while BigN > 0 do
  begin
    Rem := 0;
    for I := BigN - 1 downto 0 do
    begin
      Cur := Rem * 65536 + Big[I];
      Big[I] := Cur div 10000;
      Rem := Cur mod 10000
    end;
    BigTrim();
    for J := 0 to 3 do
    begin
      Dig[Nd] := Rem mod 10;
      Rem := Rem div 10;
      Nd := Nd + 1
    end
  end;
  Done := false;
  while not Done do
  begin
    if Nd <= 1 then Done := true
    else if Dig[Nd - 1] <> 0 then Done := true
    else Nd := Nd - 1
  end;
  I := 0;
  J := Nd - 1;
  while I < J do
  begin
    T := Dig[I];
    Dig[I] := Dig[J];
    Dig[J] := T;
    I := I + 1;
    J := J - 1
  end;
  Expand := Nd
end;

{ round_keep(): the expansion's first Keep digits, half to even, into RK }
function RoundKeep(Nd, Keep: integer; var Carry: integer): integer;
var
  I, D, Sticky, LastOdd: integer;
  Up, Done: boolean;
begin
  Carry := 0;
  if Keep >= Nd then
  begin
    for I := 0 to Nd - 1 do RK[I] := Dig[I];
    for I := Nd to Keep - 1 do RK[I] := 0;
    RoundKeep := Keep
  end
  else
  begin
    Up := false;
    if Keep >= 0 then
    begin
      D := Dig[Keep];
      Sticky := 0;
      for I := Keep + 1 to Nd - 1 do
        if Dig[I] <> 0 then Sticky := 1;
      LastOdd := 0;
      if Keep > 0 then LastOdd := Dig[Keep - 1] mod 2;
      Up := (D > 5) or ((D = 5) and ((Sticky = 1) or (LastOdd = 1)))
    end;
    if Keep <= 0 then
    begin
      RoundKeep := 0;
      if Up then
      begin
        RK[0] := 1;
        Carry := 1;
        RoundKeep := 1
      end
    end
    else
    begin
      for I := 0 to Keep - 1 do RK[I] := Dig[I];
      RoundKeep := Keep;
      if Up then
      begin
        I := Keep - 1;
        Done := false;
        while not Done do
        begin
          if I < 0 then Done := true
          else if RK[I] <> 9 then Done := true
          else
          begin
            RK[I] := 0;
            I := I - 1
          end
        end;
        if I >= 0 then RK[I] := RK[I] + 1
        else
        begin
          for I := Keep downto 1 do RK[I] := RK[I - 1];
          RK[0] := 1;
          Carry := 1;
          RoundKeep := Keep + 1
        end
      end
    end
  end
end;

procedure PutCh(C: char);
begin
  Outc[OutLen] := Ord(C);
  OutLen := OutLen + 1
end;

procedure PutDigit(D: integer);
begin
  Outc[OutLen] := Ord('0') + D;
  OutLen := OutLen + 1
end;

procedure PutWord(var S: string);
var
  I: integer;
begin
  for I := 1 to Length(S) do PutCh(S[I])
end;

procedure PutInt(V: integer);
var
  T: array[0..11] of integer;
  N: integer;
begin
  N := 0;
  repeat
    T[N] := V mod 10;
    V := V div 10;
    N := N + 1
  until V = 0;
  while N > 0 do
  begin
    N := N - 1;
    PutDigit(T[N])
  end
end;

function ExpLen(X: integer): integer;
var
  N: integer;
begin
  N := 1;
  if X < 0 then
  begin
    N := N + 1;
    X := -X
  end;
  repeat
    N := N + 1;
    X := X div 10
  until X = 0;
  ExpLen := N
end;

{ calc_format(): register R -> Disp }
procedure Format(R: integer);
var
  Neg, Cls, Nd, Q, E, S, ERec, Integral, Rc0, I, L, Carry, IntDigits, F,
  Keep0, Ip, Fs, Src, N, X, FixedLo: integer;
  Done, Finished: boolean;
begin
  OutLen := 0;
  Neg := FSign[R];
  Cls := FClass(R);
  Finished := false;
  if Cls = 3 then
  begin
    Txt := 'not a number';
    PutWord(Txt);
    Finished := true
  end;
  if Cls = 2 then
  begin
    if Neg = 1 then Txt := '-infinity' else Txt := 'infinity';
    PutWord(Txt);
    Finished := true
  end;
  if Cls = 0 then
  begin
    PutCh('0');
    Finished := true
  end;
  if not Finished then
  begin
    Nd := Expand(R, Q);
    E := Nd - 1 - Q;
    S := Neg;
    { THE DECISION RECORD: |v| to 12 significant digits }
    L := RoundKeep(Nd, DisplayW, Rc0);
    ERec := E + Rc0;
    Integral := 1;
    if ERec < 0 then Integral := 0;
    for I := ERec + 1 to DisplayW - 1 do
      if I >= 0 then
        if RK[I] <> 0 then Integral := 0;
    FixedLo := -2 { CALCDA_MUT_FIXED_E3 };
    if ERec >= FixedLo then
    begin
      if Integral = 1 then
      begin
        if S + ERec + 1 <= DisplayW then
        begin
          L := RoundKeep(Nd, E + 1, Carry);
          if Neg = 1 then PutCh('-');
          for I := 0 to L - 1 do PutDigit(RK[I]);
          Finished := true
        end
      end
      else
      begin
        IntDigits := 0;
        if ERec >= 0 then IntDigits := ERec + 1;
        F := DisplayW - S - IntDigits - 1;
        if F >= 1 then
        begin
          Keep0 := E + 1 + F;
          L := RoundKeep(Nd, Keep0, Carry);
          Ip := L - F;
          if Neg = 1 then PutCh('-');
          for I := 0 to Ip - 1 do PutDigit(RK[I]);
          Fs := OutLen;
          PutCh('.');
          for I := 0 to F - 1 do
          begin
            Src := Ip + I;
            if Src >= 0 then PutDigit(RK[Src]) else PutCh('0')
          end;
          Done := false;
          while not Done do
          begin
            if OutLen <= Fs + 1 then Done := true
            else if Outc[OutLen - 1] <> Ord('0') then Done := true
            else OutLen := OutLen - 1
          end;
          if OutLen = Fs + 1 then OutLen := OutLen - 1;
          if OutLen = S then PutCh('0');
          Finished := true
        end
      end
    end;
    if not Finished then
    begin
      { SCIENTIFIC: as many significant digits as fit }
      N := DisplayW - S - ExpLen(ERec) - 1;
      L := RoundKeep(Nd, N, Carry);
      X := E + Carry;
      if ExpLen(X) <> ExpLen(ERec) then
      begin
        N := DisplayW - S - ExpLen(X) - 1;
        L := RoundKeep(Nd, N, Carry);
        X := E + Carry
      end;
      Done := false;
      while not Done do
      begin
        if L <= 1 then Done := true
        else if RK[L - 1] <> 0 then Done := true
        else L := L - 1
      end;
      if Neg = 1 then PutCh('-');
      PutDigit(RK[0]);
      if L > 1 then
      begin
        PutCh('.');
        for I := 1 to L - 1 do PutDigit(RK[I])
      end;
      PutCh('E');
      if X < 0 then
      begin
        PutCh('-');
        X := -X
      end;
      PutInt(X)
    end
  end;
  Disp := '';
  for I := 0 to OutLen - 1 do Disp := Disp + Chr(Outc[I])
end;

{ ======================================================================
  THE STATE MACHINE (calc_init / calc_press of calc_core.c)
  ====================================================================== }
procedure CalcInit();
begin
  FSetZero(RegAcc, 0);
  Pending := CmdNone;
  Entering := 0;
  Len := 0;
  Disp := '0'
end;

procedure ShowEntry();
var
  I: integer;
begin
  Disp := '';
  for I := 0 to Len - 1 do Disp := Disp + Chr(EntryCh[I])
end;

procedure Operate(Op: integer);
begin
  if Entering = 1 then
  begin
    EntryValue(RegX);
    if Pending <> CmdNone then FApply(RegAcc, RegAcc, Pending, RegX)
    else FCopySigned(RegAcc, RegX, FSign[RegX]);
    Entering := 0
  end;
  { with no operand typed an operator replaces the pending one, '=' drops it }
  if Op = CmdEq then Pending := CmdNone else Pending := Op;
  Format(RegAcc)
end;

procedure TypeChar(Ch: integer);
var
  I: integer;
  Refused: boolean;
begin
  if Entering = 0 then
  begin
    Entering := 1;
    Len := 0
  end;
  Refused := false;
  if Ch = Ord('.') then
  begin
    for I := 0 to Len - 1 do
      if EntryCh[I] = Ord('.') then Refused := true;
    if not Refused then
      if Len = 0 then
      begin
        EntryCh[0] := Ord('0');
        Len := 1
      end
  end
  else if Len = 1 then
    if EntryCh[0] = Ord('0') then Len := 0;
  if Len >= EntryMax then Refused := true;
  if not Refused then
  begin
    EntryCh[Len] := Ch;
    Len := Len + 1;
    ShowEntry()
  end
end;

procedure CalcPress(K: integer);
begin
  if (K >= 0) and (K <= 9) then TypeChar(Ord('0') + K)
  else if K = CmdDot then TypeChar(Ord('.'))
  else if (K >= CmdAdd) and (K <= CmdEq) then Operate(K)
  else if K = CmdClear then CalcInit()
end;

{ calc_cmd_from_key(): message = (vkey << 8) | ascii }
function CmdFromKey(Message: integer): integer;
var
  M, A, Vk: integer;
begin
  M := Message;
  if M < 0 then M := M + 2147483647 + 1;
  A := M mod 256;
  Vk := (M div 256) mod 256;
  CmdFromKey := CmdNone;
  if (A >= Ord('0')) and (A <= Ord('9')) then CmdFromKey := A - Ord('0')
  else if A = Ord('.') then CmdFromKey := CmdDot
  else if A = Ord('+') then CmdFromKey := CmdAdd
  else if A = Ord('-') then CmdFromKey := CmdSub
  else if A = Ord('*') then CmdFromKey := CmdMul
  else if A = Ord('/') then CmdFromKey := CmdDiv
  else if A = Ord('=') then CmdFromKey := CmdEq
  else if (A = Ord('c')) or (A = Ord('C')) then CmdFromKey := CmdClear
  else if Vk = VkeyEnter then CmdFromKey := CmdEq
  else if Vk = VkeyNumLock then CmdFromKey := CmdClear
end;

{ ======================================================================
  THE FACE (calc_view.c), every pixel a FILLRECT
  ====================================================================== }
procedure Rect(L, T, R, B, Tok: integer);
begin
  Rc := FlairFillRect(Win, L, T, R, B, Tok)
end;

function GlyphOf(C: integer): integer;
var
  I, G: integer;
begin
  G := 0;
  for I := CalcGlyphCount - 1 downto 0 do
    if GlyphCh[I] = C then G := I;
  GlyphOf := G
end;

procedure DrawGlyph(G, X, Y, Tok: integer);
var
  R, C, S, Bits, Mask: integer;
  Run: boolean;
begin
  for R := 0 to GlyphRows[G] - 1 do
  begin
    Bits := GlyphBits[G * 10 + R];
    C := 0;
    Mask := 128;
    while C < 8 do
    begin
      if (Bits div Mask) mod 2 = 1 then
      begin
        S := C;
        Run := true;
        while Run do
        begin
          C := C + 1;
          Mask := Mask div 2;
          if C >= 8 then Run := false
          else if (Bits div Mask) mod 2 = 0 then Run := false
        end;
        Rect(X + S, Y + R, X + C, Y + R + 1, Tok)
      end
      else
      begin
        C := C + 1;
        Mask := Mask div 2
      end
    end
  end
end;

function TextLeft(): integer;
var
  I, Wd: integer;
begin
  Wd := 0;
  for I := 1 to Length(Disp) do Wd := Wd + GlyphAdv[GlyphOf(Ord(Disp[I]))];
  TextLeft := CalcDispTextRight - Wd
end;

procedure PaintReadout();
var
  I, X, G: integer;
begin
  Rect(CalcDispL + 1, CalcDispT + 1, CalcDispR, CalcDispB, FlairWhite);
  X := TextLeft();
  for I := 1 to Length(Disp) do
  begin
    G := GlyphOf(Ord(Disp[I]));
    DrawGlyph(G, X, CalcDispTextTop, FlairBlack);
    X := X + GlyphAdv[G]
  end
end;

procedure PaintKey(I: integer);
var
  L, T, R, B: integer;
begin
  L := Keys[I].L;
  T := Keys[I].T;
  R := Keys[I].L + Keys[I].W;
  B := Keys[I].T + Keys[I].H;
  Rect(R, T + CalcKeyShadow, R + CalcKeyShadow, B + CalcKeyShadow, FlairBlack);
  Rect(L + CalcKeyShadow, B, R, B + CalcKeyShadow, FlairBlack);
  Rect(L, T, R, B, FlairBlack);
  Rect(L + 1, T + 1, R - 1, B - 1, FlairWhite);
  DrawGlyph(GlyphOf(Keys[I].Lbl), L + Keys[I].Ldx, T + Keys[I].Ldy, FlairBlack)
end;

procedure PaintAll();
var
  I: integer;
begin
  Rect(0, 0, CalcContentW, CalcContentH, FlairGray);
  Rect(CalcDispL, CalcDispT, CalcDispR + 1, CalcDispB + 1, FlairBlack);
  PaintReadout();
  for I := 0 to CalcKeyCount - 1 do PaintKey(I)
end;

function KeyHit(X, Y: integer): integer;
var
  I, Hit: integer;
begin
  Hit := -1;
  for I := CalcKeyCount - 1 downto 0 do
    if (X >= Keys[I].L) and (X < Keys[I].L + Keys[I].W) and
       (Y >= Keys[I].T) and (Y < Keys[I].T + Keys[I].H) then Hit := I;
  KeyHit := Hit
end;

function ClickCmd(I: integer): integer;
var
  K: integer;
begin
  K := CmdNone;
  if (I >= 0) and (I < CalcKeyCount) then
  begin
    K := Keys[I].Cmd;
    K := K { CALCDA_MUT_KEY_CLICK_DIVERGE }
  end;
  ClickCmd := K
end;

procedure Command(K: integer);
begin
  if K <> CmdNone then
  begin
    CalcPress(K);
    PaintReadout()
  end
end;

{ ======================================================================
  THE TENANT (calcda.c)
  ====================================================================== }
procedure FlairEventHandler(var Ev: FlairEvent);
var
  Org, X, Y: integer;
begin
  if Ev.What = FlairUpdateEvt then PaintAll();
  if Ev.What = FlairMouseDown then
  begin
    Org := FlairContentOrigin(Win);
    if Org >= 0 then
    begin
      X := Ev.WhereH - Org mod 65536;
      Y := Ev.WhereV - Org div 65536;
      Command(ClickCmd(KeyHit(X, Y)))
    end
  end;
  if (Ev.What = FlairKeyDown) or (Ev.What = FlairAutoKey) then
    Command(CmdFromKey(Ev.Message));
  if Ev.What = FlairMenuEvt then
    if Ev.Message = MenuFile * 65536 + ItemClose then Rc := FlairExit(0)
end;

begin
  LoadTables();
  if TablesOk then
  begin
    Txt := 'CALCULAT';
    if FlairRegister(Txt) = FlairOK then
    begin
      Txt := 'File';
      Rc := FlairNewMenu(MenuFile, Txt);
      Txt := 'Close';
      Rc := FlairAppendItem(Txt, 'W', true);
      { the DA's Edit menu: present, every row dimmed (calcda.c) }
      Txt := 'Edit';
      Rc := FlairNewMenu(MenuEdit, Txt);
      Txt := 'Undo';
      Rc := FlairAppendItem(Txt, 'Z', false);
      Txt := '-';
      Rc := FlairAppendItem(Txt, Chr(0), false);
      Txt := 'Cut';
      Rc := FlairAppendItem(Txt, 'X', false);
      Txt := 'Copy';
      Rc := FlairAppendItem(Txt, Chr(0), false);
      Txt := 'Paste';
      Rc := FlairAppendItem(Txt, 'V', false);
      Txt := 'Clear';
      Rc := FlairAppendItem(Txt, Chr(0), false);
      Rc := FlairSetMenuBar(true);
      CalcInit();
      Win := FlairNewWindow(CalcWinL, CalcWinT, CalcWinL + CalcWinW,
                            CalcWinT + CalcWinH,
                            FlairGoAwayClose + FlairRDocProc);
      if Win > 0 then
      begin
        Txt := 'Calculator';
        Rc := FlairSetWTitle(Win, Txt)
      end
    end
  end
end.
