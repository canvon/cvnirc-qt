TODO ideas for cvnirc-qt by canvon
==================================

 * Interpret protocol command names case-insensitively? That is,
   treat PRIVMSG and privmsg the same...

   (2017-11-24)

 * Have Outgoing data structure, and check all outgoing message parts
   for validity before sending.

   (2017-11-24)

 * Have a command help system.

   (2017-12-04)

    * At a first stage, just the command name + help strings
      from the cmdhelp_foo() method registered with the cmd_foo()
      command execution method.

      (2017-12-04)

    * Later, have full command-line parsing for (e.g. long) options, too.

      (2017-12-04)

    * In the GUI, let --help pop up an additional help tab, while -h
      just shows inline help. (Inspired by git.)

      (2017-12-04)

 * Learn about the server encoding via numeric messages and/or let the user
   override it. Then, reencode everything from/to the server.

   (2017-12-04)

    * Possibly reencode per target (e.g., nick, channel), too?

      (2017-12-04)

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

 * Have crude CTCP support, like /me and reply to CTCP VERSION?

   (2017-12-04)

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

 * Implement CTCP specs?

   (2017-12-04)

 * Build and run on Mac? on Windows?

   (2017-11-23)

 * Have Continuous Integration builds for ~each commit?

   (2017-11-23)


Source only
-----------

 * Have some kind of Originator data on Message-s that would be
   an interpreted prefix. Server, Client or arbitrary prefix.

   (2017-12-04)

 * Wrap into namespaces consistently (like already begun with the
   IRCProtoMessage -> cvnirc::core::IRCProto::Message reworking).

   (2017-12-04)

 * Have automatic command recognition (via QObject reflection?)
   so that it won't be possible anymore to forget adding the
   load-this-as-command code, and that it won't be necessary
   anymore to spell out the qualified(?) method name.

   (2017-12-04)


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

