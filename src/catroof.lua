#!/usr/bin/env lua
-- catroof - audio, midi and surface control device manager
-- SPDX-FileCopyrightText: Copyright © 2023 Nedko Arnaudov
-- SPDX-License-Identifier: GPL-3

local lgi = require("lgi")
local GLib = lgi.GLib
local Gtk = lgi.Gtk
local Gio = lgi.Gio
local dir = Gio.File.new_for_commandline_arg(arg[0]):get_parent()
local os = require("os")
local assert = lgi.assert

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
