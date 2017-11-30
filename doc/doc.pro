#
# cvnirc-qt doc.pro - qmake file to build Qt Help documentation file
# Copyright (C) 2017  Fabian Pietsch <fabian@canvon.de>
#

TEMPLATE = aux


# Convert parts of documentation from MarkDown to HTML format
MD_SOURCES += ../README.md ../TODO.md

md_html.input = MD_SOURCES
#md_html.variable_out = QHP_INCLUDES  # This seems to not work with .depends, as that does not make a variable magic.
md_html.output = generated/${QMAKE_FILE_BASE}.html
md_html.commands = markdown ${QMAKE_FILE_NAME} >${QMAKE_FILE_OUT}
md_html.CONFIG = no_link  # target_predeps
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
    QHP_INCLUDES_DIST_GENDIR += generated/$$src
    QHP_INCLUDES += generated/$$src
}
QHP_INCLUDES += \
    generated/README.html \
    generated/TODO.html
# ^ Manual QHP_INCLUDES should not be necessary with the new .depend_command
#   (They led to duplicated dependency listings in the generated Makefile.)
# ^ Yes, they may well be necessary! The output of a qmake compiler's
#   .depend_command seems to be filtered to ignore files that don't exist,
#   apparently without regard for whether they might be buildable by us.
#   So specify everything explicitly, too, and try to live with the duplicates?

# Prepare "generated" subdirectory
prepare_generated.target = generated/stamp
prepare_generated.depends = $$QHP_INCLUDES_DIST_SRCDIR
prepare_generated.commands = mkdir -p generated && cp -t generated $$prepare_generated.depends && touch generated/stamp
QMAKE_EXTRA_TARGETS += prepare_generated
QMAKE_CLEAN += $$prepare_generated.target $$QHP_INCLUDES_DIST_GENDIR

qhp_qch.input = QHP_SOURCES
#qhp_qch.variable_out = QHCP_INCLUDES  # See above.
#qhp_qch.variable_out = TARGET  # This gives make warnings.
qhp_qch.variable_out = OBJECTS
qhp_qch.depends = generated/stamp $$QHP_INCLUDES
qhp_qch.depend_command = sed -n -f ${QMAKE_FILE_IN_PATH}/qhelpxml2files.sed ${QMAKE_FILE_NAME} | sed -e s,^,generated/,
qhp_qch.output = ${QMAKE_FILE_BASE}.qch
qhp_qch.commands = cp -t generated ${QMAKE_FILE_NAME} && qhelpgenerator generated/${QMAKE_FILE_BASE}.qhp -o ${QMAKE_FILE_OUT}
qhp_qch.clean = ${QMAKE_FILE_OUT} generated/${QMAKE_FILE_BASE}.qhp
qhp_qch.CONFIG = no_link  # target_predeps
QMAKE_EXTRA_COMPILERS += qhp_qch

# The help collection file
QHCP_SOURCES += cvnirc-qt-collection.qhcp
QHCP_INCLUDES += \
    cvnirc-qt.qch

qhcp_qhc.input = QHCP_SOURCES
#qhcp_qhc.variable_out = INSTALLS
#qhcp_qhc.variable_out = TARGET  # This gives make warnings.
qhcp_qhc.variable_out = OBJECTS
qhcp_qhc.depends = $$QHCP_INCLUDES
qhcp_qhc.depend_command = sed -n -f ${QMAKE_FILE_IN_PATH}/qhelpxml2files.sed ${QMAKE_FILE_NAME}
qhcp_qhc.output = ${QMAKE_FILE_BASE}.qhc
qhcp_qhc.commands = cp ${QMAKE_FILE_NAME} ${QMAKE_FILE_BASE}.build.qhcp && qcollectiongenerator ${QMAKE_FILE_BASE}.build.qhcp -o ${QMAKE_FILE_OUT}
qhcp_qhc.clean = ${QMAKE_FILE_OUT} ${QMAKE_FILE_BASE}.build.qhcp
qhcp_qhc.CONFIG = no_link  # target_predeps
QMAKE_EXTRA_COMPILERS += qhcp_qhc

DISTFILES += qhelpxml2files.sed
DISTFILES += $$QHP_INCLUDES_DIST
