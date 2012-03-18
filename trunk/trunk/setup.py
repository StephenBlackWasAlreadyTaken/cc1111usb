import sys
import os
from distutils.core import setup, Extension

packages = ['chipconlib', 'vstruct', 'vstruct.defs']
mods = []
pkgdata = {}
scripts = ['ccusb',
        ]

setup  (name        = 'ccusb',
        version     = '1.0',
        description = "chipcon usb platform",
        author = 'atlas of d00m',
        author_email = 'atlas@r4780y.com',
        #include_dirs = [],
        packages  = packages,
        package_data = pkgdata,
        ext_modules = mods,
        scripts = scripts
       )


