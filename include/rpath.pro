#
# cvnirc-qt rpath.pro - qmake include file for letting the built binaries find the internal library
# Copyright (C) 2017  Fabian Pietsch <fabian@canvon.de>
#

# If supported on this platform, set an rpath relative to
# the binary location. This makes it unnecessary to install
# the internal library to the system or to use tricks like
# environment LD_LIBRARY_PATH=:../cvnirc-core for running.
isEmpty(QMAKE_REL_RPATH_BASE): warning("qmake support for RPATH with $ORIGIN missing. You'll have to use LD_LIBRARY_PATH=:../cvnirc-core or the like to let the cvnirc-cli find the cvnirc-core shared library!")
else: QMAKE_RPATHDIR += ../cvnirc-core .
