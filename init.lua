local function detect_clipboard()
  if vim.fn.executable("wl-copy") == 1 and vim.fn.executable("wl-paste") == 1 then
    return {
      name = "WaylandClipboard",
      copy = {
        ["+"] = "wl-copy",
        ["*"] = "wl-copy",
      },
      paste = {
        ["+"] = "wl-paste --no-newline",
        ["*"] = "wl-paste --no-newline --primary",
      },
    }
  elseif vim.fn.executable("osc-copy") == 1 and vim.fn.executable("osc-paste") == 1 then
    return {
      name = "OSC52Clipboard",
      copy = {
        ["+"] = function(lines, _) vim.fn.system("osc-copy", table.concat(lines, "\n")) end,
        ["*"] = function(lines, _) vim.fn.system("osc-copy", table.concat(lines, "\n")) end,
      },
      paste = {
        ["+"] = function() return {vim.fn.split(vim.fn.system("osc-paste"), "\n"), vim.fn.getregtype()} end,
        ["*"] = function() return {vim.fn.split(vim.fn.system("osc-paste"), "\n"), vim.fn.getregtype()} end,
      },
    }
  else
    return nil -- No clipboard tool detected
  end
end

-- Apply the detected clipboard configuration
local clipboard_config = detect_clipboard()
if clipboard_config then
  vim.g.clipboard = clipboard_config
else
  print("No compatible clipboard tool detected!")
end

-- Enable unnamedplus for system clipboard integration
vim.opt.clipboard:append("unnamedplus")
