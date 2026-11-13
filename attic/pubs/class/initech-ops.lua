-- initech-ops.lua -- transcript selection for the F-OPS operations manuals
-- (PUB-REF-0001 AN-15, NT-17, NT-21, NT-23; bead initech-ipu5.14).  Loaded by
-- initech-ops.sty.  Every printed line IS a line of a committed capture
-- (screens/<name>.txt, written by pubs/screens/capture.sh from the running
-- system and re-proved by `make test-pubs-screens`) or of a committed DRAFT
-- transcript (screens/<name>.draft, hand-written from a DECIDED change that
-- has not shipped yet; printed only under a DRAFT marker).  The selection is
-- by TEXT, not by line number, so a re-capture that moves the lines keeps
-- printing the same part of the session or fails loud.  ASCII only.
initech_ops = {}

local function readlines(path, want25)
  local f = io.open(path, "rb")
  if not f then tex.error("initech-ops: cannot open " .. path) return nil end
  local data = f:read("*a")
  f:close()
  local lines = {}
  for line in string.gmatch(data, "([^\n]*)\n") do
    line = string.gsub(line, "\r$", "")
    if string.len(line) > 80 or string.find(line, "[^\032-\126]") then
      tex.error("initech-ops: " .. path .. " line " .. (#lines + 1) .. " is not 80 columns of printable ASCII")
      return nil
    end
    lines[#lines + 1] = line
  end
  if want25 and #lines ~= 25 then
    tex.error("initech-ops: " .. path .. " has " .. #lines .. " lines; a screen capture has exactly 25")
    return nil
  end
  return lines
end

-- "TEXT" or "TEXT@n": the n-th line (from `start`) that begins with TEXT.
local function find(lines, spec, start, path)
  local text, n = string.match(spec, "^(.*)@(%d+)$")
  if text == nil then text, n = spec, "1" end
  n = tonumber(n)
  for i = start, #lines do
    if string.sub(lines[i], 1, string.len(text)) == text then
      n = n - 1
      if n == 0 then return i end
    end
  end
  tex.error("initech-ops: " .. path .. ": no line begins with '" .. spec .. "'")
  return nil
end

local function texline(line, a, b, mark)
  local out = {}
  for j = 1, string.len(line) do
    if j == a then out[#out + 1] = "{\\bfseries " end
    local c = string.byte(line, j)
    if c == 32 then out[#out + 1] = "\\ "
    elseif (c >= 48 and c <= 57) or (c >= 65 and c <= 90) or (c >= 97 and c <= 122) then
      out[#out + 1] = string.char(c)
    else out[#out + 1] = "\\char" .. c .. "\\relax " end
    if j == b then out[#out + 1] = "}" end
  end
  if mark then out[#out + 1] = "\\hfill\\OpsCallout{" .. mark .. "}" end
  return table.concat(out)
end

-- typed lines of a .keys file (capture.sh syntax: \n = Enter, \\, \0ooo)
local function keys(keyfile)
  local f = io.open(keyfile, "rb")
  if not f then return {} end
  local found = f:read("*a")
  f:close()
  found = string.gsub(found, "[\r\n]+$", "")
  -- \Wtext\W (capture.sh: wait for the guest) ends a typed line
  found = string.gsub(found, "\\W.-\\W", "\\n")
  local out, cur, i = {}, {}, 1
  while i <= string.len(found) do
    local c = string.sub(found, i, i)
    if c == "\\" then
      local d = string.sub(found, i + 1, i + 1)
      if d == "n" then out[#out + 1] = table.concat(cur); cur = {}; i = i + 2
      elseif d == "\\" then cur[#cur + 1] = "\\"; i = i + 2
      elseif d == "0" then
        local oct = string.match(found, "^0([0-7]?[0-7]?[0-7]?)", i + 1)
        cur[#cur + 1] = string.char(tonumber(oct ~= "" and oct or "0", 8))
        i = i + 2 + string.len(oct)
      else cur[#cur + 1] = c; i = i + 1 end
    else cur[#cur + 1] = c; i = i + 1 end
  end
  if #cur > 0 then out[#out + 1] = table.concat(cur) end
  return out
end

-- marks: "1=PREFIX;2=PREFIX@2" -> callout numeral beside that line
local function parsemarks(marks, lines, a, b, path)
  local m = {}
  for item in string.gmatch(marks or "", "[^;]+") do
    local num, spec = string.match(item, "^%s*(%d+)%s*=(.*)$")
    if num == nil then tex.error("initech-ops: bad mark '" .. item .. "'") return m end
    local i = find(lines, spec, a, path)
    if i == nil or i > b then tex.error("initech-ops: mark " .. num .. " outside the printed lines") return m end
    m[i] = num
  end
  return m
end

local function emit(lines, a, b, typed, marks)
  local width, rows, k = 0, {}, 1
  for i = a, b do
    local line = string.gsub(lines[i], " +$", "")
    if string.len(line) > width then width = string.len(line) end
    local s, e = 0, -1
    local pfx, rest = string.match(line, "^([A-Z]:\\[^>]*>)(.+)$")
    local j = k
    while j <= #typed do
      local t = typed[j]
      if string.find(t, "[^ ]") then
        if rest ~= nil and rest == t then s = string.len(pfx) + 1; e = string.len(line); break end
        if rest == nil and string.len(line) > string.len(t) and
           string.sub(line, -string.len(t)) == t and
           string.sub(line, -string.len(t) - 1, -string.len(t) - 1) == " " then
          s = string.len(line) - string.len(t) + 1; e = string.len(line); break
        end
      end
      j = j + 1
    end
    if s > 0 then k = j + 1 end
    rows[#rows + 1] = "\\hbox to \\linewidth{\\strut " .. texline(line, s, e, marks and marks[i]) .. "\\hss}"
  end
  if #rows == 0 then tex.error("initech-ops: nothing selected") return end
  local hasmarks = "0"
  if marks then for _ in pairs(marks) do hasmarks = "1" end end
  tex.sprint("\\OpsScreenBox{" .. width .. "}{" .. hasmarks .. "}{" .. table.concat(rows) .. "}")
end

-- A captured screen (25 lines).  fromtext/totext select the printed span
-- (default: line 1 to the last non-blank line, less the empty prompt the
-- capture settles on).
function initech_ops.shot(path, keyfile, fromtext, totext, marks)
  local lines = readlines(path, true)
  if not lines then return end
  local last = 25
  while last > 0 and not string.find(lines[last], "[^ ]") do last = last - 1 end
  if last > 0 and string.match(lines[last], "^[A-Z]:\\[^>]*>$") then last = last - 1 end
  local a, b = 1, last
  if fromtext ~= "" then a = find(lines, fromtext, 1, path) end
  if a == nil then return end
  if totext ~= "" then b = find(lines, totext, a, path) end
  if b == nil then return end
  if b < a then tex.error("initech-ops: " .. path .. ": empty span") return end
  emit(lines, a, b, keys(keyfile), parsemarks(marks, lines, a, b, path))
end

-- A DRAFT transcript: every line of screens/<name>.draft.  The typed
-- keystrokes for the bold face come from screens/<name>.keys.
function initech_ops.draft(path, keyfile, marks)
  local lines = readlines(path, false)
  if not lines then return end
  emit(lines, 1, #lines, keys(keyfile), parsemarks(marks, lines, 1, #lines, path))
end

-- Lines a..b of a report file written by the program (CR LF, form feeds,
-- Ctrl-Z padding), as the gate that replays the program compares it.
function initech_ops.report(path, a, b)
  local f = io.open(path, "rb")
  if not f then tex.error("initech-ops: cannot open " .. path) return end
  local data = f:read("*a")
  f:close()
  data = string.gsub(data, "\026+$", "")
  local lines = {}
  for line in string.gmatch(data, "([^\n]*)\n") do
    line = string.gsub(line, "\r$", "")
    line = string.gsub(line, "^\012", "")
    lines[#lines + 1] = line
  end
  if b > #lines or a < 1 or b < a then tex.error("initech-ops: " .. path .. " has " .. #lines .. " lines") return end
  emit(lines, a, b, {}, nil)
end
