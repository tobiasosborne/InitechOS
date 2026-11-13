-- charset.lua -- Appendix C of the FLAIR User's Guide: the ASCII codes 32
-- to 126 a FLAIR application stores, three columns of (decimal, hex,
-- character).  The characters of the backslash key (92, 124) are marked:
-- that key is reserved for InitechBase (os/milton/kmain.c, make code 0x2B)
-- and never reaches an application.  ASCII only (Rule 12).
local function cell(c)
  local ch
  if c == 32 then
    ch = "\\textit{space}"
  else
    ch = string.format("\\texttt{\\char\"%02X}", c)
  end
  if c == 92 or c == 124 then ch = ch .. "\\,*" end
  return string.format("%d & \\texttt{%02X} & %s", c, c, ch)
end
tex.sprint("\\begin{tabular}{@{}rll@{\\hspace{6mm}}rll@{\\hspace{6mm}}rll@{}}")
tex.sprint("\\textbf{Dec} & \\textbf{Hex} & \\textbf{Char} & "
  .. "\\textbf{Dec} & \\textbf{Hex} & \\textbf{Char} & "
  .. "\\textbf{Dec} & \\textbf{Hex} & \\textbf{Char}\\\\\\hline")
for r = 0, 31 do
  local row = {}
  for k = 0, 2 do
    local c = 32 + k * 32 + r
    if c <= 126 then row[#row + 1] = cell(c) else row[#row + 1] = "& &" end
  end
  tex.sprint(table.concat(row, " & ") .. "\\\\")
end
tex.sprint("\\end{tabular}")
