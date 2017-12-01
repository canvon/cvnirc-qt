TEMPLATE = subdirs

SUBDIRS += \
    cvnirc-core \
    cvnirc-gui \
    cvnirc-cli \
    doc

cvnirc-gui.depends = cvnirc-core
cvnirc-cli.depends = cvnirc-core

VERSION = 0.5.10
