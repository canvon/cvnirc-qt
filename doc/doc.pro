#
# cvnirc-qt doc.pro - qmake file to build Qt Help documentation file
# Copyright (C) 2017  Fabian Pietsch <fabian@canvon.de>
#

TEMPLATE = aux

# The following has been taken from [1], then adjusted. Original quoted:
# [1] http://www.qtcentre.org/threads/22075-QMake-to-Build-Help-Files
#
# # The help
# # Using a "custom compiler"
# QHP_FILES += help1.qhp help2.qhp
#
# qhp_qhc.input = QHP_FILES
# qhp_qhc.output = ${QMAKE_FILE_BASE}.qch
# qhp_qhc.commands = qhelpgenerator ${QMAKE_FILE_NAME}
# qhp_qhc.CONFIG = no_link target_predeps
# QMAKE_EXTRA_COMPILERS += qhp_qhc
#
# # Or for a single file using a custom target
# myhelp.target = test3.qhc
# myhelp.depends = test3.qhp
# myhelp.commands = qhelpgenerator $$myhelp.depends
# QMAKE_EXTRA_TARGETS += myhelp
# PRE_TARGETDEPS += test3.qhc

# The help
# Using a "custom compiler"
QHP_FILES += cvnirc-qt.qhp
QHP_DEPENDS += \
    index.html \
    build.html \
    features.html

qhp_qhc.input = QHP_FILES
qhp_qhc.depends = $$QHP_DEPENDS
qhp_qhc.output = ${QMAKE_FILE_BASE}.qch
qhp_qhc.commands = qhelpgenerator ${QMAKE_FILE_NAME} -o ${QMAKE_FILE_OUT}
qhp_qhc.CONFIG = no_link target_predeps
QMAKE_EXTRA_COMPILERS += qhp_qhc

DISTFILES += $$QHP_DEPENDS
