#!/usr/bin/env lua
-- catroof - audio, midi and surface control device manager
-- SPDX-FileCopyrightText: Copyright © 2023 Nedko Arnaudov
-- SPDX-License-Identifier: GPL-3

local function optparser_run(handle_option_fn)
  while true do
    if not arg[1] then break end
    if arg[1]:match("^-") then
      handle_option_fn(arg[1])
      table.remove(arg, 1)
    else
      break
    end
  end
end

local progname = string.gsub(arg[0], "(.*/)(.*)", "%2")

opts = {}

-- defaults
opts.tui = false
opts.gui = false
opts.cui = false

optparser_run(
  function (opt)
    --print("option", opt)
    if opt == "-t" or opt == '-n' then
      opts.tui = true
    elseif opt == "-g" then
      opts.gui = true
    elseif opt == "-c" then
      opts.cui = true
    end
  end)

local os = require("os")
local lgi = pcall(function () require("lgi") end)
local newt = pcall(function () require('newt') end)

--print(("lgi %q"):format(lgi))
--print(("newt %q"):format(newt))

if not opts.tui and not opts.gui and not opts.cui then
  if progname == "ncatroof" and newt then
    opts.tui = true
  elseif progname == "gcatroof" and lgi then
    opts.gui = true
  else
    -- enable gtk+ gui if available
    if lgi then opts.gui = true end

    -- enable newt tui if available
    if newt then opts.tui = true end
  end
end

--print(("tui %q"):format(opts.tui))
--print(("gui %q"):format(opts.gui))

if opts.gui then
  local lgi = require("lgi")
  local assert = lgi.assert
  local GLib = lgi.GLib
  local Gtk = lgi.require('Gtk', '3.0')
  local Gio = lgi.Gio
  local dir = Gio.File.new_for_commandline_arg(arg[0]):get_parent()

  local CatRoofApp = lgi.package("CatRoof")
  local class = CatRoofApp:class("AppWindow", Gtk.ApplicationWindow)

  function CatRoofApp.AppWindow:_class_init(klass)
    local f = assert(io.open(dir:get_child('catroof.ui'):get_path(), "r"))
    local template = f:read("*all")
    f:close()

    local bytes = GLib.Bytes.new(template)
    klass:set_template(bytes)
  end

  function CatRoofApp.AppWindow:_init(...)
    self:init_template()
  end

  function CatRoofApp.AppWindow:run()
    self:show_all()
    Gtk.main()
    return 0
  end

  os.exit(CatRoofApp.AppWindow():run())
elseif opts.tui then
  local newt = require('newt')

  newt.Init()
  newt.Cls()

  newt.PopWindow()
  newt.OpenWindow(5, 5, 20, 4, "Window 1")
  newt.Refresh()
  newt.WaitForKey()

  newt.Finished()
else
  print("List of devices:")
end
