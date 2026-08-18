# Turbo Initech lexer dump format

`os/tps/tps.pas` always opens the fixed input name `TPSIN.PAS`. This follows
the B8 InitechDOS runtime: there is no command-line filename surface in the
seed subset. The successful stream is bracketed by these exact lines:

```text
TPS-LEX-BEGIN
...
TPS-LEX-END
```

Since B9.2 the driver then resets the lexer over the retained source and emits
the separately bracketed parser trace specified by `PARSER-TRACE.md`. The
lexer payload and its golden files are unchanged. Lexer gates extract the
single anchored `TPS-LEX-BEGIN`/`TPS-LEX-END` region before diffing; this is a
neutral-to-stronger harness re-key because it additionally rejects missing or
duplicate markers while preserving the byte-exact B9.1 token oracle.

There is one line per token between the markers:

- `KW name` for a reserved word, with its canonical lower-case spelling.
- `IDENT name` for an identifier, read back from the global character pool;
  the name is canonical lower-case ASCII.
- `INTEGER value` for a decimal integer literal.
- `STRING length byte...` for a decoded string literal. Each payload byte is
  printed as an unsigned decimal ordinal, so quotes, spaces, and control bytes
  cannot make the one-token-per-line stream ambiguous. A doubled source quote
  contributes one byte `39`.
- The fixed kind names `SEMI`, `DOT`, `DOTDOT`, `COMMA`, `COLON`, `ASSIGN`,
  `LPAREN`, `RPAREN`, `LBRACKET`, `RBRACKET`, `PLUS`, `MINUS`, `STAR`, `EQ`,
  `NE`, `LT`, `LE`, `GT`, and `GE` cover punctuation and operators.
- `EOF` is the final token.

Comments and whitespace emit no token. A lexical or input-capacity failure
emits exactly one line beginning `TPS-LEX-ERROR pos=N line=N col=N`, stops
lexing, and is followed only by the closing marker. Positions are 1-based byte,
line, and column coordinates. The source-buffer capacity error uses line and
column zero because no source byte beyond the capacity was retained.

The Free Pascal and InitechDOS gates remove carriage returns before diffing.
The seed serial `Writeln` currently emits LF, as does the Linux-hosted Free
Pascal binary; normalization lives in the gate so the Pascal artifact has one
output path on both targets.
