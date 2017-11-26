#
# cvnirc-qt doc.pro - qmake file to build Qt Help documentation file
# Copyright (C) 2017  Fabian Pietsch <fabian@canvon.de>
#

TEMPLATE = aux

# The following has been taken from [1]:
# [1] http://www.qtcentre.org/threads/22075-QMake-to-Build-Help-Files
#
# {

# The help
# Using a "custom compiler"
QHP_FILES += cvnirc-qt.qhp
 
qhp_qhc.input = QHP_FILES
qhp_qhc.output = ${QMAKE_FILE_BASE}.qch
qhp_qhc.commands = qhelpgenerator ${QMAKE_FILE_NAME}
qhp_qhc.CONFIG = no_link target_predeps
QMAKE_EXTRA_COMPILERS += qhp_qhc

# }

DISTFILES += \
    index.html \
    build.html \
    features.html
