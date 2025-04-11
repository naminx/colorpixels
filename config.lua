--[[
  LunarVim Configuration
  ---------------------
  - Well-organized, easy to maintain
  - Win/mswin/Windows/GUI-like selection with Shift+Arrows enabled over SSH/TTY.
--]]

-- ===============================
-- 1. PLUGINS
-- ===============================
table.insert(lvim.plugins, {
  "farmergreg/vim-lastplace"
})

-- ===============================
-- 2. GENERAL VIM OPTIONS
-- ===============================
vim.opt.shiftwidth      = 2
vim.opt.tabstop         = 2
vim.opt.expandtab       = true  -- Converts tabs to spaces
vim.opt.relativenumber  = true
vim.opt.wrap            = true        -- Enable line wrapping
vim.opt.breakindent     = true        -- Indent wrapped lines
vim.opt.showbreak       = "↳ "        -- Character to show at the beginning of wrapped lines
vim.opt.foldenable      = true
vim.opt.foldmethod      = "expr"
vim.opt.foldexpr        = "nvim_treesitter#foldexpr()"
vim.opt.backspace       = { "indent", "eol", "start" }
vim.opt.whichwrap:append("<,>,[,],h,l")

-- ===============================
-- 3. SELECTION/KEYMODEL SETTINGS
-- ===============================
vim.opt.selection   = "exclusive"
vim.opt.selectmode  = "mouse,key"
vim.opt.mousemodel  = "popup"
vim.opt.keymodel    = "startsel,stopsel"

-- ===============================
-- 4. SHIFT+ARROW SELECTION ON TTY/SSH
--      (Like mswin/GVim/Wezterm but for remote shells!)
-- ===============================
if vim.fn.has("gui_running") == 0 then
  -- Normal mode: Shift+arrows begin selection just like GVim / Windows
  vim.api.nvim_set_keymap('n', '<S-Left>',  'v<Left>',  { noremap = true, silent = true })
  vim.api.nvim_set_keymap('n', '<S-Right>', 'v<Right>', { noremap = true, silent = true })
  vim.api.nvim_set_keymap('n', '<S-Up>',    'v<Up>',    { noremap = true, silent = true })
  vim.api.nvim_set_keymap('n', '<S-Down>',  'v<Down>',  { noremap = true, silent = true })
  -- Visual mode: continue extending selection
  vim.api.nvim_set_keymap('v', '<S-Left>',  '<Left>',   { noremap = true, silent = true })
  vim.api.nvim_set_keymap('v', '<S-Right>', '<Right>',  { noremap = true, silent = true })
  vim.api.nvim_set_keymap('v', '<S-Up>',    '<Up>',     { noremap = true, silent = true })
  vim.api.nvim_set_keymap('v', '<S-Down>',  '<Down>',   { noremap = true, silent = true })
end

-- ===============================
-- 5. CLIPBOARD INTEGRATION (Wayland, OSC52, etc)
-- ===============================
local function is_wayland()
  return os.getenv("WAYLAND_DISPLAY") ~= nil
end

local function detect_clipboard()
  if vim.fn.executable("wl-copy") == 1 and vim.fn.executable("wl-paste") == 1 then
    return {
      name = "WaylandClipboard",
      copy = { ["+"] = "wl-copy", ["*"] = "wl-copy", },
      paste = { ["+"] = "wl-paste --no-newline", ["*"] = "wl-paste --no-newline --primary", },
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
      }
    }
  else
    return nil -- No clipboard tool detected
  end
end

local clipboard_config = detect_clipboard()
if clipboard_config then
  vim.g.clipboard = clipboard_config
else
  print("No compatible clipboard tool detected!")
end

vim.opt.clipboard:append("unnamedplus")

-- ===============================
-- 6. KEYBINDINGS (Organized Logical Groups)
-- ===============================

-- --- Visual Backspace is delete selection (like mswin)
vim.api.nvim_set_keymap('v', '<BS>', 'd', { noremap = true })

-- --- Windows-style selection/copy/cut/paste bindings (when clipboard available)
if vim.fn.has("clipboard") == 1 then
  -- Cut
  vim.api.nvim_set_keymap('v', '<C-X>', '"+x', { noremap = true })
  vim.api.nvim_set_keymap('v', '<S-Del>', '"+x', { noremap = true })
  -- Copy
  vim.api.nvim_set_keymap('v', '<C-C>', '"+y', { noremap = true })
  vim.api.nvim_set_keymap('v', '<C-Insert>', '"+y', { noremap = true })
  -- Paste
  vim.api.nvim_set_keymap('n', '<C-V>', '"+gP', { noremap = true })
  vim.api.nvim_set_keymap('n', '<S-Insert>', '"+gP', { noremap = true })
  -- Paste in command-line
  vim.api.nvim_set_keymap('c', '<C-V>', '<C-R>+', { noremap = true })
  vim.api.nvim_set_keymap('c', '<S-Insert>', '<C-R>+', { noremap = true })
end

-- Paste mappings for virtualedit
vim.cmd([[
  inoremap <C-V> <C-G>u <C-R>=paste#paste_cmd['i']<CR>
  vnoremap <C-V> <C-R>=paste#paste_cmd['v']<CR>
]])
vim.api.nvim_set_keymap('i', '<S-Insert>', '<C-V>', { noremap = true })
vim.api.nvim_set_keymap('v', '<S-Insert>', '<C-V>', { noremap = true })
vim.api.nvim_set_keymap('n', '<C-Q>', '<C-V>', { noremap = true })

-- --- Common file commands
vim.api.nvim_set_keymap('n', '<C-S>', ':update<CR>', { noremap = true })
vim.api.nvim_set_keymap('v', '<C-S>', ':update<CR>', { noremap = true })
vim.api.nvim_set_keymap('i', '<C-S>', '<Esc>:update<CR>gi', { noremap = true })

-- --- Undo/Redo
vim.api.nvim_set_keymap('n', '<C-Z>', 'u', { noremap = true })
vim.api.nvim_set_keymap('i', '<C-Z>', '<C-O>u', { noremap = true })
vim.api.nvim_set_keymap('n', '<C-Y>', '<C-R>', { noremap = true })
vim.api.nvim_set_keymap('i', '<C-Y>', '<C-O><C-R>', { noremap = true })

-- --- Select all (GUI/mswin style)
vim.api.nvim_set_keymap('n', '<C-A>', 'ggVG<C-O>G', { noremap = true })
vim.api.nvim_set_keymap('i', '<C-A>', '<C-O>gg<C-O>gVG<C-O>G', { noremap = true })
vim.api.nvim_set_keymap('c', '<C-A>', '<C-C>gggVG<C-O>G', { noremap = true })
vim.api.nvim_set_keymap('o', '<C-A>', '<C-C>gggVG<C-O>G', { noremap = true })
vim.api.nvim_set_keymap('s', '<C-A>', '<C-C>gggVG<C-O>G', { noremap = true })
vim.api.nvim_set_keymap('x', '<C-A>', '<C-C>ggVG', { noremap = true })

-- --- Move between windows
vim.api.nvim_set_keymap('n', '<C-Tab>', '<C-W>w', { noremap = true })
vim.api.nvim_set_keymap('i', '<C-Tab>', '<C-O><C-W>w', { noremap = true })
vim.api.nvim_set_keymap('c', '<C-Tab>', '<C-C><C-W>w', { noremap = true })
vim.api.nvim_set_keymap('o', '<C-Tab>', '<C-C><C-W>w', { noremap = true })

-- --- Close window
vim.api.nvim_set_keymap('n', '<C-F4>', ':close<CR>', { noremap = true })
vim.api.nvim_set_keymap('i', '<C-F4>', ':close<CR><Esc>', { noremap = true })
vim.api.nvim_set_keymap('c', '<C-F4>', ':close<CR><Esc>', { noremap = true })
vim.api.nvim_set_keymap('o', '<C-F4>', ':close<CR><Esc>', { noremap = true })

-- --- GUI/mswin Alt+Space for menu (only in GUI)
if vim.fn.has("gui") == 1 then
  vim.api.nvim_set_keymap('n', '<M-Space>', ':simalt ~<CR>', { noremap = true })
  vim.api.nvim_set_keymap('i', '<M-Space>', '<C-O>:simalt ~<CR>', { noremap = true })
  vim.api.nvim_set_keymap('c', '<M-Space>', '<C-C>:simalt ~<CR>', { noremap = true })
end

-- --- On Unix, disable autoselect for CTRL-V to work correctly
if not vim.fn.has("unix") then
  vim.opt.guioptions:remove("a")
end

-- ===============================
-- 7. AUTOCOMMANDS
-- ===============================
vim.api.nvim_create_autocmd({ "BufWinLeave" }, {
  pattern = "*",
  command = "silent! mkview"
})
vim.api.nvim_create_autocmd({ "BufWinEnter" }, {
  pattern = "*",
  command = "silent! loadview"
})

-- ===============================
-- 8. LSP CONFIG
-- ===============================
local lspconfig = require("lspconfig")
lspconfig.autotools_ls.setup{}
require('mason-lspconfig').setup_handlers({
  autotools_ls = function() end
})

-- ===============================
-- 9. (Optional/Advanced/Misc)
-- ===============================
-- Uncomment for alternative cpoptions, linebreaks, etc.

-- vim.opt.linebreak = true     -- Wrap lines at convenient points (like spaces)
-- local save_cpo = vim.opt.cpoptions:get()
-- vim.opt.cpoptions:append("v")
-- vim.opt.cpoptions:set(save_cpo)
-- lvim.builtin.autopairs.active = false
