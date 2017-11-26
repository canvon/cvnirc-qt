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
QHP_DEPENDS_SOURCE += \
    index.html \
    build.html \
    features.html
for(src, QHP_DEPENDS_SOURCE) {
    QHP_DEPENDS += $$PWD/$$src
}

qhp_qhc.input = QHP_FILES
qhp_qhc.depends = $$QHP_DEPENDS
qhp_qhc.output = ${QMAKE_FILE_BASE}.qch
qhp_qhc.commands = qhelpgenerator ${QMAKE_FILE_NAME} -o ${QMAKE_FILE_OUT}
qhp_qhc.CONFIG = no_link target_predeps
QMAKE_EXTRA_COMPILERS += qhp_qhc

# The help collection file
QHCP_FILES += cvnirc-qt-collection.qhcp
QHCP_DEPENDS += \
    cvnirc-qt.qch

qhcp_qhc.input = QHCP_FILES
qhcp_qhc.depends = $$QHCP_DEPENDS
qhcp_qhc.output = ${QMAKE_FILE_BASE}.qch
qhcp_qhc.commands = cp ${QMAKE_FILE_NAME} ${QMAKE_FILE_BASE}.qhcp ; qcollectiongenerator ${QMAKE_FILE_BASE}.qhcp -o ${QMAKE_FILE_OUT}
qhcp_qhc.CONFIG = no_link target_predeps
QMAKE_EXTRA_COMPILERS += qhcp_qhc

DISTFILES += $$QHP_DEPENDS_SOURCE
