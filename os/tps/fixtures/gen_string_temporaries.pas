{$MODE DELPHI}{$B+}{$H-}
{ B9.5 focused execution fixture for the B7 deep-string-temporary hole
  reported by B9.4. The right-nested expression requires four simultaneous
  ShortString temporaries; the same shape is consumed once by assignment and
  once directly by Write.

  Golden provenance: HAND-COMPUTED from ShortString concatenation semantics,
  not FPC-minted. A + (B + (C + (D + E))) is ABCDE, length 5, with no
  truncation in String[32]. The Pascal literal 'It''s' decodes its doubled
  quote to the four characters It's.
  Expected exact stdout:
    DEEP=ABCDE DIRECT=ABCDE LEN=5 QUOTE=It's
}
program StringTemporaries;
var
  A, B, C, D, E: string[8];
  ResultText: string[32];
  QuoteText: string[8];
begin
  A := 'A';
  B := 'B';
  C := 'C';
  D := 'D';
  E := 'E';
  QuoteText := 'It''s';
  ResultText := A + (B + (C + (D + E)));
  write('DEEP='); write(ResultText);
  write(' DIRECT='); write(A + (B + (C + (D + E))));
  write(' LEN='); write(length(ResultText));
  write(' QUOTE='); writeln(QuoteText)
end.
