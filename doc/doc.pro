#
# cvnirc-qt doc.pro - qmake file to build Qt Help documentation file
# Copyright (C) 2017  Fabian Pietsch <fabian@canvon.de>
#

TEMPLATE = aux


# Convert parts of documentation from MarkDown to HTML format
MD_SOURCES += ../README.md ../TODO.md

md_html.input = MD_SOURCES
md_html.variable_out = QHP_INCLUDES
md_html.output = generated/${QMAKE_FILE_BASE}.html
md_html.commands = markdown ${QMAKE_FILE_NAME} >${QMAKE_FILE_OUT}
md_html.CONFIG = no_link target_predeps
QMAKE_EXTRA_COMPILERS += md_html


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
QHP_SOURCES += cvnirc-qt.qhp
QHP_INCLUDES_DIST += \
    style.css \
    index.html \
    build.html \
    features.html
for(src, QHP_INCLUDES_DIST) {
    QHP_INCLUDES_DIST_SRCDIR += $$relative_path($$PWD/$$src, $$OUT_PWD)
    QHP_INCLUDES += generated/$$src
}
#QHP_INCLUDES += $$MD_GENERATED

# Prepare "generated" subdirectory
prepare_generated.target = generated/stamp
prepare_generated.depends = $$QHP_INCLUDES_DIST_SRCDIR
prepare_generated.commands = mkdir -p generated && cp -t generated $$prepare_generated.depends && touch generated/stamp
QMAKE_EXTRA_TARGETS += prepare_generated

qhp_qch.input = QHP_SOURCES
qhp_qch.variable_out = QHCP_INCLUDES  # QHP_GENERATED
qhp_qch.depends = generated/stamp $$QHP_INCLUDES
qhp_qch.output = ${QMAKE_FILE_BASE}.qch
qhp_qch.commands = cp -t generated ${QMAKE_FILE_NAME} && qhelpgenerator generated/${QMAKE_FILE_BASE}.qhp -o ${QMAKE_FILE_OUT}
qhp_qch.CONFIG = no_link target_predeps
QMAKE_EXTRA_COMPILERS += qhp_qch

# The help collection file
QHCP_SOURCES += cvnirc-qt-collection.qhcp
#QHCP_INCLUDES += $$QHP_GENERATED

qhcp_qhc.input = QHCP_SOURCES
#qhcp_qhc.variable_out = QHCP_GENERATED
#qhcp_qhc.variable_out = INSTALLS
qhcp_qhc.depends = $$QHCP_INCLUDES
qhcp_qhc.output = ${QMAKE_FILE_BASE}.qhc
qhcp_qhc.commands = cp ${QMAKE_FILE_NAME} ${QMAKE_FILE_BASE}.qhcp ; qcollectiongenerator ${QMAKE_FILE_BASE}.qhcp -o ${QMAKE_FILE_OUT}
qhcp_qhc.CONFIG = no_link target_predeps
QMAKE_EXTRA_COMPILERS += qhcp_qhc

DISTFILES += $$QHP_INCLUDES_DIST
