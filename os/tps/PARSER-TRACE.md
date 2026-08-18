# Turbo Initech parser trace format

`os/tps/tps.pas` reads the fixed B8 filename `TPSIN.PAS` once. The driver
first emits the unchanged lexer dump and then replays the retained source
through the single-lookahead recursive-descent parser. The two independent
surfaces are bracketed as follows:

```text
TPS-LEX-BEGIN
...
TPS-LEX-END
TPS-PARSE-BEGIN
...
TPS-PARSE-END
```

The parser keeps exactly one current token and calls `NextToken` on demand;
there is no token buffer. A successful trace has one line for every named
production entry and successful exit:

```text
ENTER program L1
ENTER block L4
EXIT block L7
EXIT program L8
TPS-PARSE-OK
```

`L` is the 1-based line of the current lookahead token at that event. On a
successful exit it therefore identifies the first token after the production.
At end of input it is the lexer's line after trailing whitespace has been
consumed. Production names are fixed lower-case ASCII identifiers. A failed
production has its `ENTER` event but no `EXIT` event.

The first syntax error emits exactly one deterministic line and stops:

```text
TPS-PARSE-ERROR pos=21 line=2 col=1 expected=SEMI found=KW_BEGIN
```

`pos`, `line`, and `col` are 1-based coordinates of the found lookahead.
`expected` is either a fixed token kind (`SEMI`, `KW_END`, and so on) or one
of the grammar categories `FACTOR`, `TYPE_NAME`, `RESULT_TYPE`,
`CONST_LITERAL`, `PARAM_NAME`, `FIELD_NAME`, `VAR_NAME`, `ROUTINE_NAME`,
`PROGRAM_NAME`, `ARRAY_BOUND`, `TO_OR_DOWNTO`, and `NO_CHAINED_RELATION`.
`found` is always the fixed lexer token-kind name. The error line is followed only by
`TPS-PARSE-END`. If the first lexer pass fails, the parser bracket contains
`TPS-PARSE-SKIP lexer-error`; replaying a known-bad byte stream would only
duplicate the already located lexer diagnostic.

## Seed grammar mirrored by B9.2

The grammar authority is `seed/parser.c`, not a reconstructed Pascal grammar.
B9.2 mirrors these source productions:

- expression precedence and non-chaining relations: `seed/parser.c:309-640`
  (`parse_factor`, `parse_term`, `parse_simple_expr`, and `parse_expr`);
- calls, assignments, optional array index and scalar record field suffixes,
  and `write`/`writeln`: `seed/parser.c:645-815`;
- nearest-unmatched-`if` else binding, `while`, `for` (`to` and `downto`),
  and `repeat`: `seed/parser.c:884-1110`;
- semicolon-separated `begin`/`end` blocks, including empty statements:
  `seed/parser.c:1112-1163`;
- literal constants; repeating/interleaved top-level `const`, `type`, and
  `var` sections; static arrays; named scalar-field records; ShortStrings;
  standalone `file` variables: `seed/parser.c:1181-1686`;
- top-level procedures/functions, optional parameter lists with value and
  `var` groups, `forward`, recursion, one optional local `var` section, and
  function results restricted to `integer`, `boolean`, or `char`:
  `seed/parser.c:1694-1958`;
- the program shell, repeated/interleaved declaration loop, final dot, and
  required EOF: `seed/parser.c:1963-2036`.

B9.2 deliberately constructs no AST and owns no symbol table. Consequently,
checks that require declaration knowledge (constant folding in array bounds,
record-type declare-before-use, duplicate names, and aggregate parameter/type
rules) remain B9.3 semantic work. The syntax consumed is still the seed's
surface; the parser does not grant meaning to an identifier merely by
accepting it in a type-name or bound position.

One verified divergence is retained and made explicit: `case` is a lexer
keyword, but the seed parser does not implement a `case` statement. The seed
dispatch at `seed/parser.c:1114-1134` omits it and `seed/parser.h:85-90`
records it as unimplemented. TPS therefore rejects `case` in B9.2; the
`parse_case_unsupported` negative golden prevents an invented grammar from
silently entering the self-host subset.
