Install dependencies:

  $ apt-get install qtbase5-dev qt5-default
  $ apt-get install libreadline6-dev  # on Debian 8
  $ apt-get install libreadline-dev   # on Debian 9

Build:

  $ mkdir ../build-cvnirc-qt
  $ cd    ../build-cvnirc-qt
  $ qmake ../cvnirc-qt/cvnirc-qt.pro
  $ make

Build and view the documentation manually:
Just open the doc/index.html file in your web browser.
Or:

  cvnirc-qt$ cd doc
  cvnirc-qt/doc$ qhelpgenerator cvnirc-qt.qhp
  [...]
  cvnirc-qt/doc$ assistant -collectionFile cvnirc-qt-collectionfile.raw -register cvnirc-qt.qch
  Documentation successfully registered.
  cvnirc-qt/doc$ assistant -collectionFile cvnirc-qt-collectionfile.raw

The last command should display the built cvnirc-qt documentation Qt Help file.
