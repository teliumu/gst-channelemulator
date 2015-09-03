Channel Emulator ReadMe
=============

About
-----

Channel Emulator is Gstreamer 1.0 plugin for simulating network environment like packets drop, duplication, delay, etc.

Channel Emulator by default do not provide any changes to received packets. To enable any of them, it's required to change corresponding loss probabilities or other parameters by setting properties of element.

License
-------

This plugin is licensed under the LGPL v2.1.
Copyright 2015 Samsung R&D Institute Ukraine.

Features
--------

* Packets dropping
  Current plugin have 6 drop modes:
  1. Random drop
  2. Burst drop
  3. Combined (burst and random) losses
  4. Four state Markov chain drop model
  5. Gilbert-Elliot drop model
  6. User defined model

* Data corruption
  Plugin has possibility to corrupt data by enabling bit flipping and package corruption.

* Packets delaying

* Packets duplicating

Usage
-----------

* Random drop enabling
  For enabling this feature, set drop-probability to desired probability:

  $ gst-launch-1.0 … ! channelemulator drop-probability=0.1 ! ...

* Burst drop enabling
  For enabling this feature, set burst-probability to desired probability and drop-mode to 1

  $ gst-launch-1.0 … ! channelemulator drop-mode=1 burst-probability=0.05 ! ...

* Combined (burst and random) losses enabling
  For enabling this feature, set burst-probability, drop-probability to desired probability and drop-mode to 2

  $ gst-launch-1.0 … ! channelemulator drop-mode=2 burst-probability=0.05 drop-probability=0.1 ! ...

* Four state Markov chain drop model enabling
  For enabling this feature, set drop-mode to 3 and model-param to file with parameters location

  $ gst-launch-1.0 … ! channelemulator drop-mode=3 model-param="model_param, p1=(double)0.05, p2 =(double)0.25, p3=(double)0.25, p4=(double)0.1, p5=(double)0.75 ! … 

  where
  p1 - p13 in Markov model
  p2 - p31 in Markov model
  p3 - p32 in Markov model
  p4 - p14 in Markov model
  p5 - p23 in Markov model

* Gilbert-Elliot drop model enabling
  For enabling this feature, set drop-mode to 4 and model-param to file with parameters location

  $ gst-launch-1.0 … ! channelemulator drop-mode=3 model-param="model_param, p1=(double)0.05, p2 =(double)0.25, p3=(double)0.25, p4=(double)0.1 ! … 

  where
  p1 - p for Gilbert-Elliot drop model
  p2 - r for Gilbert-Elliot drop model
  p3 - h for Gilbert-Elliot drop model
  p4 - 1-k for Gilbert-Elliot drop model

* User defined model
  For enabling this feature, set drop-mode to 5 and set user-model to file location that consists of '0' and '1' in text form, where '1' – drop packet
  Random and burst drop mode can be additionally limited by setting max-drop-count property.
  Burst drop mode have one additional parameter for burst length that describes how many package can be dropped by one round of burst drop.

* Data corruption enabling
  For enabling bit flipping feature, set bitflip-probability, bit-length, bit-flip-packet properties.
  '-1' in bit length mean over all packet

  $ gst-launch-1.0 … ! channelemulator bitflip-probability=0.1 bit-length=-1 bit-flip-packet=10 ! …

  For enabling data corruption feature, set data-corrupt-probability, corrupt-length, corrupt-packet-number properties.

  $ gst-launch-1.0 … ! channelemulator data-corrupt-probability=0.1 corrupt-length=10 corrupt-packet-number=100 ! …

  All above properties can be limited with loop-return-period and corresponding properties like max-drop-count

* Packets delaying enabling
  For enabling this feature, set delay-probability to desired probability and  min-delay, max-delay in milliseconds.

  $ gst-launch-1.0 … ! channelemulator delay-probability=0.1 ! …

* Packets duplicating enabling
  For enabling this feature, set duplicate-probability to desired probability.

  $ gst-launch-1.0 … ! channelemulator duplicate-probability=0.1 ! …


Build steps:
-----------

* 1. Build and install gstreamer core
* 2. ./autogen.sh --prefix=/usr/local
* 3. make
* 4. sudo make install

Element Properties
-----------

  name  - The name of the object
          flags: readable, writable
          String.
          Default: "channelemulator0"

  parent - The parent of the object
           flags: readable, writable
           Object of type "GstObject"

  rand-seed  - Seed for random number generator
               flags: readable, writable
               Unsigned Integer64.
               Range: 0 – 4294967295
               Default: 0

  min-delay - The minimum delay in ms to apply to buffers
              flags: readable, writable
              Integer.
              Range: 0 – 2147483647
              Default: 2000

  max-delay - The maximum delay in ms to apply to buffers
              flags: readable, writable
              Integer.
              Range: 0 – 2147483647
              Default: 4000

  delay-probability - The Probability a buffer is delayed
                      flags: readable, writable
                      Float.
                      Range: 0 – 1
                      Default: 0

  drop-mode  - Drop mode to use
               flags: readable, writable
               Enum "GstChannelEmulatorDropMode"
               (0): random   - Random drop mode
               (1): burst    - Burst drop mode
               (2): combined - Combined random and burst drop mode
               (3): mcmodel  - 4-state Markov chain model driven drop mode
               (4): gemodel  - Gilbert-Elliot model driven drop mode
               (4): userfile - User model for packet droping
               Default: 0, "random"

  drop-probability - The Probability a buffer is dropped
                     flags: readable, writable
                     Float.
                     Range: 0 – 1
                     Default: 0

  burst-probability - The Probability a buffer will be dropped by burst
                      flags: readable, writable
                      Float.
                      Range: 0 – 1
                      Default: 0

  burst-length - Maximum length of burst packet series
                 flags: readable, writable
                 Integer.
                 Range: 0 – 2147483647
                 Default: 5

  max-drop-count - Maximum count of packet that was dropped over within work period
                   flags: readable, writable
                   Integer.
                   Range: 0 – 2147483647
                   Default: 1000

  loop-return-period - The period in which fixed number of packets will be dropped
                       flags: readable, writable
                       Integer.
                       Range: 0 – 2147483647
                       Default: 1000

  user-model - Location of file with user specified model of packet drop
               flags: readable, writable
               String.
               Default: null

  model-param - Location of file with parameter for model of packet drop
                flags: readable, writable
                String.
                Default: null

  bitflip-probability - The Probability a that bytes in buffer was inverted
                        flags: readable, writable
                        Float.
                        Range: 0 – 1
                        Default: 0

  bit-length - Period in bytes where bits will be inverted
               flags: readable, writable
               Integer.
               Range: -1 – 2147483647
               Default: -1

  bit-flip-packet - Number of packets that has inverted bits
                    flags: readable, writable
                    Integer.
                    Range: 0 – 2147483647
                    Default: 1000

  data-corrupted-probability - The Probability that end of packet was changed to 0
                             flags: readable, writable
                             Float.
                             Range: 0 – 1
                             Default: 0

  corrupt-length - Number of bytes which would be set to 0 from the end of packet
                   flags: readable, writable
                   Integer.
                   Range: 0 – 2147483647
                   Default: 5

  corrupt-packet-number - Number of packets. End of which would be set to 0
                          flags: readable, writable
                          Integer.
                          Range: 0 – 2147483647
                          Default: 1000

  duplicate-probability - The Probability a buffer is duplicated
                          flags: readable, writable
                          Float.
                          Range: 0 – 1
                          Default: 0

Dependencies
------------

You'll need a GStreamer 1.x installation.
