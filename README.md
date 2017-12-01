cvnirc-qt
=========

**canvon** _IRC_ client, "based-on-_Qt_-framework" incarnation.

Yes, this is in fact a new IRC (Internet Relay Chat) client, in 2017.
But it's rather a C++/Qt learning/evaluation project for me.


How to build
------------

Install dependencies:

    # apt-get install qtbase5-dev qt5-default
    # apt-get install qttools5-dev-tools markdown  # to build subdir doc
    # apt-get install libreadline6-dev  # on Debian 8
    # apt-get install libreadline-dev   # on Debian 9

Build:

    cvnirc-qt$ mkdir ../build-cvnirc-qt
    cvnirc-qt$ cd    ../build-cvnirc-qt
    build-cvnirc-qt$ qmake ../cvnirc-qt/cvnirc-qt.pro
    build-cvnirc-qt$ make

This should build:

* cvnirc-core (internal library),
* cvnirc-gui (the main program, graphical user interface),
* cvnirc-cli (command-line interface; chat in the terminal), and
* doc (documentation).


How to run
----------

Run the main program via

    build-cvnirc-qt$ ./cvnirc-gui/cvnirc-qt-gui

or, if you're using, e.g., Debian 8 or similarly older:

    build-cvnirc-qt$ LD_LIBRARY_PATH=:cvnirc-core ./cvnirc-gui/cvnirc-qt-gui

To view the documentation (which is mostly empty as of writing,
2017-11-26,) you can use:

    build-cvnirc-qt$ assistant -collectionFile doc/cvnirc-qt-collection.qhc

That should display the built cvnirc-qt documentation Qt Help file.
