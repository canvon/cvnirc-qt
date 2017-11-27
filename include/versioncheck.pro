#
# cvnirc-qt versioncheck.pro - qmake include file for checking library versions
# Copyright (C) 2017  Fabian Pietsch <fabian@canvon.de>
#

if(lessThan(QT_MAJOR_VERSION,5)|equals(QT_MAJOR_VERSION,5):lessThan(QT_MINOR_VERSION,5)) {
    # DEFINES -= CVN_HAVE_Q_ENUM
    warning("The Qt version is too old; it does not support Q_ENUM (new in Qt 5.5). Enum values will not be stringified to keys!")
    message(Qt version: $$[QT_VERSION])
}
else: DEFINES += CVN_HAVE_Q_ENUM

if(lessThan(QT_MAJOR_VERSION,5)|equals(QT_MAJOR_VERSION,5):lessThan(QT_MINOR_VERSION,6)) {
    # DEFINES -= CVN_HAVE_QPROCESS_ERROROCCURRED
    warning("The Qt version is too old; it does not support QProcess signal errorOccurred() (new in Qt 5.6).")
    message(Qt version: $$[QT_VERSION])
}
else: DEFINES += CVN_HAVE_QPROCESS_ERROROCCURRED
