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
local lgi_found
local lgi
lgi_found, lgi = pcall(function () return require("lgi") end)
local gtk_found
local Gtk
if lgi_found then
  gtk_found, Gtk = pcall(function () return lgi.require('Gtk', '3.0') end)
  if not gtk_found then Gtk = nil end
else
  lgi = nil
end
local newt_found
local newt
newt_found, newt = pcall(function () return require('newt') end)
if not newt_found then newt = nil end

-- print(("lgi %s"):format(tostring(lgi)))
-- print(("Gtk %s"):format(tostring(Gtk)))
-- print(("newt %s"):format(tostring(newt)))

if not opts.tui and not opts.gui and not opts.cui then
  if progname == "ncatroof" and newt then
    opts.tui = true
  elseif progname == "gcatroof" and lgi then
    opts.gui = true
  else
    -- enable gtk+ gui if available
    if lgi and Gtk then opts.gui = true end

    -- enable newt tui if available
    if newt then opts.tui = true end
  end
end

--print(("tui %q"):format(opts.tui))
--print(("gui %q"):format(opts.gui))

if opts.gui then
  Gtk = lgi.require('Gtk', '3.0')
  local GLib = lgi.GLib
  local Gio = lgi.Gio
  local dir = Gio.File.new_for_commandline_arg(arg[0]):get_parent()
  local assert = lgi.assert

  local CatRoofApp = lgi.package("CatRoof")
  local class = CatRoofApp:class("AppWindow", Gtk.ApplicationWindow)

  function CatRoofApp.AppWindow:_class_init(klass)
    local f = io.open(dir:get_child('catroof.ui'):get_path(), "r")
    if not f then
      f = io.open(dir:get_child('../share/catroof/catroof.ui'):get_path(), "r")
    end
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
