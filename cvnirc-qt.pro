TEMPLATE = subdirs

SUBDIRS += \
    cvnirc-core \
    cvnirc-gui \
    cvnirc-cli

cvnirc-gui.depends = cvnirc-core
cvnirc-cli.depends = cvnirc-core
