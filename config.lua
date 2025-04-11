-- Read the docs: https://www.lunarvim.org/docs/configuration
-- Example configs: https://github.com/LunarVim/starter.lvim
-- Video Tutorials: https://www.youtube.com/watch?v=sFA9kX-Ud_c&list=PLhoH5vyxr6QqGu0i7tt_XoVK9v-KvZ3m6
-- Forum: https://www.reddit.com/r/lunarvim/
-- Discord: https://discord.com/invite/Xb9B4Ny

-- Bail out if this isn't wanted.
-- if vim.g.skip_loading_mswin then
--     return
-- end

-- Add vim-lastplace plugin
table.insert(lvim.plugins, {
    "farmergreg/vim-lastplace",
})

-- Set the 'cpoptions' to its Vim default
-- local save_cpo = vim.opt.cpoptions:get()
-- vim.opt.cpoptions:append("v")  -- Set to Vim default options

-- Set 'selection', 'selectmode', 'mousemodel', and 'keymodel' for MS-Windows behavior
-- vim.cmd("behave mswin")
-- Set 'selection' to "exclusive"
vim.opt.selection = "exclusive"

-- Set 'selectmode' to include both mouse and key
vim.opt.selectmode = "mouse,key"

-- Set 'mousemodel' to "popup"
vim.opt.mousemodel = "popup"

-- Set 'keymodel' to include both startsel and stopsel
vim.opt.keymodel = "startsel,stopsel"

vim.opt.shiftwidth = 2
vim.opt.tabstop = 2
vim.opt.expandtab = true  -- Converts tabs to spaces
vim.opt.relativenumber = true

-- Enable line wrapping
vim.opt.wrap = true          -- Enable line wrapping
-- vim.opt.linebreak = true     -- Wrap lines at convenient points (like spaces)
vim.opt.breakindent = true   -- Indent wrapped lines
vim.opt.showbreak = "↳ "     -- Character to show at the beginning of wrapped lines

-- Backspace and cursor keys wrap to previous/next line
vim.opt.backspace = { "indent", "eol", "start" }
vim.opt.whichwrap:append("<,>,[,],h,l")

vim.opt.foldenable = true
-- vim.opt.foldmethod = "syntax"
vim.opt.foldmethod = "expr"
vim.opt.foldexpr = "nvim_treesitter#foldexpr()"

-- Backspace in Visual mode deletes selection
vim.api.nvim_set_keymap('v', '<BS>', 'd', { noremap = true })

local function is_wayland()
  return os.getenv("WAYLAND_DISPLAY") ~= nil
end

-- vim.g.clipboard = {
  -- name = 'myClipboard',
  -- copy = {
    -- ['+'] = is_wayland() and 'wl-copy',
    -- ['*'] = is_wayland() and 'wl-copy',
  -- },
  -- paste = {
    -- ['+'] = is_wayland() and 'wl-paste --no-newline',
    -- ['*'] = is_wayland() and 'wl-paste --no-newline --primary',
  -- },
  -- cache_enabled = true,
-- }

-- vim.opt.clipboard = "unnamedplus"

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

if vim.fn.has("clipboard") == 1 then
    -- CTRL-X and SHIFT-Del are Cut
    vim.api.nvim_set_keymap('v', '<C-X>', '"+x', { noremap = true })
    vim.api.nvim_set_keymap('v', '<S-Del>', '"+x', { noremap = true })

    -- CTRL-C and CTRL-Insert are Copy
    vim.api.nvim_set_keymap('v', '<C-C>', '"+y', { noremap = true })
    vim.api.nvim_set_keymap('v', '<C-Insert>', '"+y', { noremap = true })

    -- CTRL-V and SHIFT-Insert are Paste
    vim.api.nvim_set_keymap('n', '<C-V>', '"+gP', { noremap = true })
    vim.api.nvim_set_keymap('n', '<S-Insert>', '"+gP', { noremap = true })

    -- Command-line paste mappings
    vim.api.nvim_set_keymap('c', '<C-V>', '<C-R>+', { noremap = true })
    vim.api.nvim_set_keymap('c', '<S-Insert>', '<C-R>+', { noremap = true })
end

-- Pasting blockwise and linewise selections requires +virtualedit feature
vim.cmd([[
    inoremap <C-V> <C-G>u <C-R>=paste#paste_cmd['i']<CR>
    vnoremap <C-V> <C-R>=paste#paste_cmd['v']<CR>
]])

-- Use S-Insert for pasting in insert mode
vim.api.nvim_set_keymap('i', '<S-Insert>', '<C-V>', { noremap = true })
vim.api.nvim_set_keymap('v', '<S-Insert>', '<C-V>', { noremap = true })

-- Use CTRL-Q to perform paste action
vim.api.nvim_set_keymap('n', '<C-Q>', '<C-V>', { noremap = true })

-- Use CTRL-S for saving, also in Insert mode
vim.api.nvim_set_keymap('n', '<C-S>', ':update<CR>', { noremap = true })
vim.api.nvim_set_keymap('v', '<C-S>', ':update<CR>', { noremap = true })
vim.api.nvim_set_keymap('i', '<C-S>', '<Esc>:update<CR>gi', { noremap = true })

-- On Unix, disable autoselect for CTRL-V to work correctly
if not vim.fn.has("unix") then
    vim.opt.guioptions:remove("a")
end

-- CTRL-Z is Undo; not in command line though
vim.api.nvim_set_keymap('n', '<C-Z>', 'u', { noremap = true })
vim.api.nvim_set_keymap('i', '<C-Z>', '<C-O>u', { noremap = true })

-- CTRL-Y is Redo (not repeat); not in command line though
vim.api.nvim_set_keymap('n', '<C-Y>', '<C-R>', { noremap = true })
vim.api.nvim_set_keymap('i', '<C-Y>', '<C-O><C-R>', { noremap = true })

-- Alt-Space is System menu (only if GUI is available)
if vim.fn.has("gui") == 1 then
    vim.api.nvim_set_keymap('n', '<M-Space>', ':simalt ~<CR>', { noremap = true })
    vim.api.nvim_set_keymap('i', '<M-Space>', '<C-O>:simalt ~<CR>', { noremap = true })
    vim.api.nvim_set_keymap('c', '<M-Space>', '<C-C>:simalt ~<CR>', { noremap = true })
end

-- CTRL-A is Select all
vim.api.nvim_set_keymap('n', '<C-A>', 'ggVG<C-O>G', { noremap = true })
vim.api.nvim_set_keymap('i', '<C-A>', '<C-O>gg<C-O>gVG<C-O>G', { noremap = true })
vim.api.nvim_set_keymap('c', '<C-A>', '<C-C>gggVG<C-O>G', { noremap = true })
vim.api.nvim_set_keymap('o', '<C-A>', '<C-C>gggVG<C-O>G', { noremap = true })
vim.api.nvim_set_keymap('s', '<C-A>', '<C-C>gggVG<C-O>G', { noremap = true })
vim.api.nvim_set_keymap('x', '<C-A>', '<C-C>ggVG', { noremap = true })

-- CTRL-Tab is Next window
vim.api.nvim_set_keymap('n', '<C-Tab>', '<C-W>w', { noremap = true })
vim.api.nvim_set_keymap('i', '<C-Tab>', '<C-O><C-W>w', { noremap = true })
vim.api.nvim_set_keymap('c', '<C-Tab>', '<C-C><C-W>w', { noremap = true })
vim.api.nvim_set_keymap('o', '<C-Tab>', '<C-C><C-W>w', { noremap = true })

-- CTRL-F4 is Close window
vim.api.nvim_set_keymap('n', '<C-F4>', ':close<CR>', { noremap = true })  -- Changed to :close for proper window closing in nvim
vim.api.nvim_set_keymap('i', '<C-F4>', ':close<CR><Esc>', { noremap = true })  -- Exit insert mode before closing window
vim.api.nvim_set_keymap('c', '<C-F4>', ':close<CR><Esc>', { noremap = true })  -- Exit command mode before closing window
vim.api.nvim_set_keymap('o', '<C-F4>', ':close<CR><Esc>', { noremap = true })  -- Exit operator pending mode before closing window

-- Enable automatic viewsaving/restoring
vim.api.nvim_create_autocmd({"BufWinLeave"}, {
  pattern = "*",
  command = "silent! mkview"
})

vim.api.nvim_create_autocmd({"BufWinEnter"}, {
  pattern = "*",
  command = "silent! loadview"
})

local lspconfig = require("lspconfig")

lspconfig.autotools_ls.setup{}
require('mason-lspconfig').setup_handlers({
  autotools_ls = function() end
})

-- if vim.fn.has("gui") == 1 then
    -- CTRL-F is the search dialog
    -- vim.api.nvim_set_keymap('n', "<expr> <C-F>", "has(\"gui_running\") ? \":promptfind\<CR>\" : \"/\"", {})
    -- vim.api.nvim_set_keymap('i', "<expr> <C-F>", "has(\"gui_running\") ? \"\<C-\>\<C-O>:promptfind\<CR>\" : \"\<C-\>\<C-O>/\"", {})
    -- vim.api.nvim_set_keymap('c', "<expr> <C-F>", "has(\"gui_running\") ? \"\<C-\>\<C-C>:promptfind\<CR>\" : \"\<C-\>\<C-O>/\"", {})

    -- CTRL-H is the replace dialog,
    -- but in console, it might be backspace, so don't map it there
    -- vim.api.nvim_set_keymap('n'," <expr> <C-H>", "has(\"gui_running\") ? \":promptrepl\<CR>\" : \"\<C-H>\"", {})
    -- vim.api.nvim_set_keymap('i'," <expr> <C-H>", "has(\"gui_running\") ? \"\<C-\>\<C-O>:promptrepl\<CR>\" : \"\<C-H>\", {})
    -- vim.api.nvim_set_keymap('c'," <expr> <C-H>", "has(\"gui_running\") ? \"\<C-\>\<C-C>:promptrepl\<CR>\" : \"\<C-H>\", {})
-- end

-- Restore 'cpoptions'
-- vim.opt.cpoptions:set(save_cpo)

-- lvim.builtin.autopairs.active = false

