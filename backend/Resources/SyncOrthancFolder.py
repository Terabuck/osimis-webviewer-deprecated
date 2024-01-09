#!/usr/bin/python3

#
# This maintenance script updates the content of the "Orthanc" folder
# to match the latest version of the Orthanc source code.
#

import multiprocessing
import os
import stat
import urllib.request

TARGET = os.path.dirname(__file__)
ORTHANC_FRAMEWORK_VERSION = '1.7.1'
PLUGIN_SDK_VERSION = '1.3.1'
REPOSITORY = 'https://orthanc.uclouvain.be/hg/orthanc/raw-file'

FILES = [
    ('OrthancFramework/Resources/CMake/DownloadOrthancFramework.cmake', 'CMake'),
    ('OrthancFramework/Resources/Toolchains/LinuxStandardBaseToolchain.cmake', 'CMake'),
    ('OrthancFramework/Resources/Toolchains/MinGW-W64-Toolchain32.cmake', 'CMake'),
    ('OrthancFramework/Resources/Toolchains/MinGW-W64-Toolchain64.cmake', 'CMake'),
]

SDK = [
    'orthanc/OrthancCPlugin.h',
]


def Download(x):
    branch = x[0]
    source = x[1]
    target = os.path.join(TARGET, x[2])
    print(target)

    try:
        os.makedirs(os.path.dirname(target))
    except:
        pass

    url = '%s/%s/%s' % (REPOSITORY, branch, source)

    with open(target, 'wb') as f:
        try:
            f.write(urllib.request.urlopen(url).read())
        except:
            print('ERROR %s' % url)
            raise


commands = []

for f in FILES:
    commands.append([ 'default',
                      f[0],
                      os.path.join(f[1], os.path.basename(f[0])) ])

for f in SDK:
    commands.append([
        'Orthanc-%s' % PLUGIN_SDK_VERSION, 
        'Plugins/Include/%s' % f,
        '../Orthanc/Sdk-%s/%s' % (PLUGIN_SDK_VERSION, f) 
    ])


pool = multiprocessing.Pool(10)  # simultaneous downloads
pool.map(Download, commands)
