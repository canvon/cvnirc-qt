TODO ideas for cvnirc-qt by canvon
==================================

 * Interpret protocol command names case-insensitively? That is,
   treat PRIVMSG and privmsg the same...

   (2017-11-24)

 * Have some output in the Main tab again (on a really high level); like
   creation/deletion of IRCProtoClient instances, contexts or the like.
   Perhaps also connection states without all the raw or partially-parsed
   lines in-between.

   (2017-11-20/-21)

 * Carry unique connection tag, so that the protocol buffer contents
   with two time the same/similar server connected will be meaningful.

   (2017-11-20/-21)

 * Have close tabs support. (Remove contexts? Have I refcounting for them?
   Skip Main tab (index 0).)

   (2017-11-23)

 * Further test/improve the multi-server support.

   (2017-11-20/-21)

 * Have/support shorter timestamps, but this will likely require
   some kind of timer that outputs "day changed" messages now and then. ...

   (2017-11-23)

 * Automatic reconnects on disconnect.

   (2017-11-20/-21)
 
 * Automatic alternate nick generation when our choice gets rejected
   at connect time.

   (2017-11-20/-21)

 * Collect MOTD lines and pass the full MOTD as a multi-line text object,
   perhaps to be displayed in its own window tab.

   (2017-11-20/-21)

 * Have client-side interpreted user commands. (Like, /window close,
   perhaps? Or a /quit that exits the application... (?))

   (2017-11-20/-21)

 * Have colored nick names?

   (2017-11-24)

 * Save & restore settings, e.g., host/port/user/nick.

   (2017-11-20/-21)

 * SSL/TLS encryption. Both raw and on-demand?

   (2017-11-20/-21)

 * Implement logging?

   (2017-11-20/-21)

 * Implement scriptability, perhaps via dbus-connected script servers.

   (2017-11-20/-21)

 * Build and run on Mac? on Windows?

   (2017-11-23)

 * Have Continuous Integration builds for ~each commit?

   (2017-11-23)


cvnirc-gui
----------

 * Interpret IRC colors.

   (2017-11-27)

 * Use the status bar?

   (2017-11-24)


cvnirc-cli
----------

 * When changing context is supported, automatically switch to new context
   when it gets created. This isn't implemented at the moment just because
   of fear of some query "hijacking" the prompt, with no possibility
   to go back...

   (2017-11-24)

 * Add custom completer to GNU readline UI.

   (2017-11-21/-22)

 * Interpret IRC colors.

   (2017-11-27)
