$ apt-get install qtbase5-dev qt5-default
$ apt-get install libreadline6-dev  # on Debian 8
$ apt-get install libreadline-dev   # on Debian 9

$ mkdir ../build-cvnirc-qt
$ cd    ../build-cvnirc-qt
$ qmake ../cvnirc-qt/cvnirc-qt.pro
$ make
