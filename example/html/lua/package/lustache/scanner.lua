local string_find, string_match, string_sub =
      string.find, string.match, string.sub

local scanner = {}

-- Returns `true` if the tail is empty (end of string).
function scanner:eos()
  return self.tail == ""
end

-- Tries to match the given regular expression at the current position.
-- Returns the matched text if it can match, `null` otherwise.
function scanner:scan(pattern)
  local match = string_match(self.tail, pattern)

  if match and string_find(self.tail, pattern) == 1 then
    self.tail = string_sub(self.tail, #match + 1)
    self.pos = self.pos + #match

    return match
  end

end

-- Skips all text until the given regular expression can be matched. Returns
-- the skipped string, which is the entire tail of this scanner if no match
-- can be made.
function scanner:scan_until(pattern)

  local match
  local pos = string_find(self.tail, pattern)

  if pos == nil then
    match = self.tail
    self.pos = self.pos + #self.tail
    self.tail = ""
  elseif pos == 1 then
    match = nil
  else
    match = string_sub(self.tail, 1, pos - 1)
    self.tail = string_sub(self.tail, pos)
    self.pos = self.pos + #match
  end

  return match
end

function scanner:new(str)
  local out = {
    str  = str,
    tail = str,
    pos  = 1
  }
  return setmetatable(out, { __index = self } )
end

return scanner
