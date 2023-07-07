#! /usr/bin/env python
# encoding: utf-8
#
# wscript for building and installing catroof

from __future__ import print_function

import os
import subprocess
import shutil
import re
import time

from waflib import Logs, Options, TaskGen, Context, Utils
from waftoolchainflags import WafToolchainFlags

APPNAME='catroof'
VERSION='0-dev'

# these variables are mandatory ('/' are converted automatically)
srcdir = '.'
blddir = 'build'

def git_ver(self):
    bld = self.generator.bld
    header = self.outputs[0].abspath()
    if os.access('./version.h', os.R_OK):
        header = os.path.join(os.getcwd(), out, "version.h")
        shutil.copy('./version.h', header)
        data = open(header).read()
        m = re.match(r'^#define GIT_VERSION "([^"]*)"$', data)
        if m != None:
            self.ver = m.group(1)
            Logs.pprint('BLUE', "tarball from git revision " + self.ver)
        else:
            self.ver = "tarball"
        return

    if bld.srcnode.find_node('.git'):
        self.ver = bld.cmd_and_log("LANG= git rev-parse HEAD", quiet=Context.BOTH).splitlines()[0]
        if bld.cmd_and_log("LANG= git diff-index --name-only HEAD", quiet=Context.BOTH).splitlines():
            self.ver += "-dirty"

        Logs.pprint('BLUE', "git revision " + self.ver)
    else:
        self.ver = "unknown"

    fi = open(header, 'w')
    if self.ver != "unknown":
        fi.write('#define GIT_VERSION "%s"\n' % self.ver)
    fi.close()

def display_msg(conf, msg="", status = None, color = None):
    if status:
        #Logs.pprint(msg, status, color)
        conf.msg(msg, status, color=color)
    else:
        Logs.pprint('NORMAL', msg)

def display_raw_text(conf, text, color = 'NORMAL'):
    Logs.pprint(color, text, sep = '')

def display_line(conf, text, color = 'NORMAL'):
    Logs.pprint(color, text, sep = os.linesep)

def options(opt):
    # options provided by the modules
    opt.load('compiler_c')
    opt.load('wafautooptions')

    opt.add_auto_option(
        'devmode',
        help='Enable devmode', # enable warnings and treat them as errors
        conf_dest='BUILD_DEVMODE',
        default=False,
    )

    opt.add_auto_option(
        'debug',
        help='Enable debug symbols',
        conf_dest='BUILD_DEBUG',
        default=False,
    )

    opt.add_option('--mandir', type='string', help="Manpage directory [Default: <prefix>/share/man]")

    opt.add_option('--enable-pkg-config-dbus-service-dir', action='store_true', default=False, help='force D-Bus service install dir to be one returned by pkg-config')

def configure(conf):
    conf.load('compiler_c')
    conf.load('wafautooptions')

    flags = WafToolchainFlags(conf)

    conf.check_cfg(
        package = 'alsa',
        mandatory = True,
        errmsg = "not installed, see http://www.alsa-project.org/",
        args = '--cflags --libs')

    conf.check_cfg(
        package = 'dbus-1',
        atleast_version = '1.0.0',
        mandatory = True,
        errmsg = "not installed, see http://dbus.freedesktop.org/",
        args = '--cflags --libs')

    dbus_dir = conf.check_cfg(
        package='dbus-1',
        args='--variable=session_bus_services_dir',
        msg="Retrieving D-Bus services dir")
    if not dbus_dir:
        return

    dbus_dir = dbus_dir.strip()
    conf.env['DBUS_SERVICES_DIR_REAL'] = dbus_dir

    if Options.options.enable_pkg_config_dbus_service_dir:
        conf.env['DBUS_SERVICES_DIR'] = dbus_dir
    else:
        conf.env['DBUS_SERVICES_DIR'] = os.path.join(
            os.path.normpath(conf.env['PREFIX']),
            'share',
            'dbus-1',
            'services')

    conf.check_cfg(
        package = 'cdbus-1',
        atleast_version = '1.0.0',
        mandatory = True,
        errmsg = "not installed, see https://github.com/LADI/cdbus",
        args = '--cflags --libs')

    if Options.options.mandir:
        conf.env['MANDIR'] = Options.options.mandir
    else:
        conf.env['MANDIR'] = conf.env['PREFIX'] + '/share/man'

    conf.define('CATROOF_VERSION', VERSION)
    conf.define('HAVE_GITVERSION_H', 1)
    conf.define('BUILD_TIMESTAMP', time.ctime())
    conf.write_config_header('config.h')

    flags.add_c('-std=gnu99')
    if conf.env['BUILD_DEVMODE']:
        flags.add_c(['-Wall', '-Wextra'])
        flags.add_c('-Wpedantic')
        flags.add_c('-Werror')
        flags.add_c(['-Wno-variadic-macros', '-Wno-gnu-zero-variadic-macro-arguments'])

        # https://wiki.gentoo.org/wiki/Modern_C_porting
        if conf.env['CC'] == 'clang':
            flags.add_c('-Wno-unknown-argumemt')
            flags.add_c('-Werror=implicit-function-declaration')
            flags.add_c('-Werror=incompatible-function-pointer-types')
            flags.add_c('-Werror=deprecated-non-prototype')
            flags.add_c('-Werror=strict-prototypes')
            if int(conf.env['CC_VERSION'][0]) < 16:
                flags.add_c('-Werror=implicit-int')
        else:
            flags.add_c('-Wno-unknown-warning-option')
            flags.add_c('-Werror=implicit-function-declaration')
            flags.add_c('-Werror=implicit-int')
            flags.add_c('-Werror=incompatible-pointer-types')
            flags.add_c('-Werror=strict-prototypes')
    if conf.env['BUILD_DEBUG']:
        flags.add_c(['-O0', '-g', '-fno-omit-frame-pointer'])
        flags.add_link('-g')

    flags.flush()

    gitrev = None
    if os.access('gitversion.h', os.R_OK):
        data = file('gitversion.h').read()
        m = re.match(r'^#define GIT_VERSION "([^"]*)"$', data)
        if m != None:
            gitrev = m.group(1)

    print()
    display_msg(conf, "==================")
    version_msg = "catroof-" + VERSION
    if gitrev:
        version_msg += " exported from " + gitrev
    else:
        version_msg += " git revision will checked and eventually updated during build"
    print(version_msg)
    print()

    display_msg(conf, "Install prefix", conf.env['PREFIX'], 'CYAN')
    display_msg(conf, "Compiler", conf.env['CC'][0], 'CYAN')
    conf.summarize_auto_options()
    if conf.env['DBUS_SERVICES_DIR'] != conf.env['DBUS_SERVICES_DIR_REAL']:
        display_msg(conf)
        display_line(conf,     "WARNING: D-Bus session services directory as reported by pkg-config is", 'RED')
        display_raw_text(conf, "WARNING:", 'RED')
        display_line(conf,      conf.env['DBUS_SERVICES_DIR_REAL'], 'CYAN')
        display_line(conf,     'WARNING: but service file will be installed in', 'RED')
        display_raw_text(conf, "WARNING:", 'RED')
        display_line(conf,      conf.env['DBUS_SERVICES_DIR'], 'CYAN')
        display_line(conf,     'WARNING: You may need to adjust your D-Bus configuration after installing ladish', 'RED')
        display_line(conf,     'WARNING: You can override dbus service install directory', 'RED')
        display_line(conf,     'WARNING: with --enable-pkg-config-dbus-service-dir option to this script', 'RED')
    flags.print()
    print()

def build(bld):
    bin_dir = bld.env['BINDIR']
    share_dir = bld.options.destdir + bld.env['PREFIX'] + '/share/' + APPNAME
    #print(bin_dir)
    #print(share_dir)

    bld(rule=git_ver,
        target='gitversion.h',
        update_outputs=True,
        always=True,
        ext_out=['.h'])

    # config.h, gitverson.h include path; public headers include path
    includes = [bld.path.get_bld(), "../include"]

    shlib = bld(features=['c', 'cshlib'])
    shlib.includes = includes
    shlib.target = 'catroof'
    shlib.env.cshlib_PATTERN = '%s.so'
    shlib.install_path = bld.env['LUA_INSTALL_CMOD']
    shlib.uselib = ['ALSA', 'LUA', 'CDBUS-1']
    shlib.source = [
        'src/alsa.c',
        'src/catdup.c',
        'src/log.c',
        ]

    prog = bld(features=['c', 'cprogram'])
    prog.source = [
        'src/alsa.c',
        'src/catdup.c',
        'src/log.c',
        'src/lscatroof.c',
        ]
    prog.includes = includes
    prog.target = 'lscatroof'
    prog.use = ['ALSA', 'CDBUS-1']
    prog.defines = ["HAVE_CONFIG_H"]

    bld.install_as(bin_dir + "/" + "catroof", 'src/catroof.lua', chmod=Utils.O755)
    bld.symlink_as(bin_dir + "/" + "ncatroof", 'catroof')
    bld.symlink_as(bin_dir + "/" + "gcatroof", 'catroof')
    bld.install_as(share_dir + "/" + "catroof.ui", 'src/catroof.ui')

    # prog = bld(features=['c', 'cprogram'])
    # prog.source = [
    #     'src/alsa.c',
    #     'src/log.c',
    #     'src/catdup.c',
    #     'src/catroofd.c',
    #     ]
    # prog.includes = includes
    # prog.target = 'catroofd'
    # prog.use = ['ALSA', 'CDBUS-1']
    # prog.defines = ["HAVE_CONFIG_H"]

    # install man pages
    man_pages = [
#TODO:        "catroof.1",
        ]

    for i in range(len(man_pages)):
        man_pages[i] = "man/" + man_pages[i]

    bld.install_files(os.path.join(bld.env['MANDIR'], 'man1'), man_pages)
