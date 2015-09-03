Channel Emulator
=============

About
-----

Channel Emulator is Gstreamer 1.0 plugin for simulating network environment.

Channel Emulation enables simulating network environment like packets drop, duplication, delay, etc. in the pipelines created based on GStreamer multimedia framework. Channel Emulation can be combined with other GStreamer multimedia plugins for emulating full range of possible events occurred in network channel in case of GStreamer plugins evaluation.

Channel Emulator by default do not provide any changes to received packets. To enable any of them, it's required to change corresponding loss probabilities or other parameters by setting properties of element.

License
-------

This plugin is licensed under the LGPL v2.1.
Copyright 2015 Samsung R&D Institute Ukraine.

Features
--------
* Packets dropping
* Data corruption by bit flipping and partial package corruption
* Packets delaying
* Packets duplicating

Drop modes
---------
* Random drop
* Burst drop
* Combined (burst and random) losses
* Four state Markov chain drop model
* Gilbert-Elliot drop model
* User defined model

Usage
-----------

* Random drop:

  ```
  $ gst-launch-1.0 ... ! channelemulator drop-probability=0.1 ! ...
  ```

* Burst drop:

  ```
  $ gst-launch-1.0 ... ! channelemulator drop-mode=1 burst-probability=0.05 ! ...
  ```

* Combined (burst and random) losses
  ```
  $ gst-launch-1.0 ... ! channelemulator drop-mode=2 burst-probability=0.05 drop-probability=0.1 ! ...
  ```

* [Four state Markov chain](https://en.wikipedia.org/wiki/Markov_chain) drop model

  ```
  $ gst-launch-1.0... ! channelemulator drop-mode=3 model-param="model_param, p1=(double)0.05, p2 =(double)0.25, p3=(double)0.25, p4=(double)0.1, p5=(double)0.75 ! ...
  ```
  where <code>p1</code>, <code>p2</code>, <code>p3</code>, <code>p4</code>, <code>p5</code> corresponds to <code>p13</code>, <code>p31</code>, <code>p32</code>, <code>p14</code> and <code>p23</code> in Markov model respectively.

* [Gilbert-Elliott](https://en.wikipedia.org/wiki/Burst_error) drop model

    ```
   $ gst-launch-1.0 ... ! channelemulator drop-mode=3 model-param="model_param, p1=(double)0.05, p2 =(double)0.25, p3=(double)0.25, p4=(double)0.1 ! ...
    ```
    where <code>p1</code>, <code>p2</code>, <code>p3</code>, <code>p4</code> corresponds to
    <code>p</code>, <code>r</code>, <code>h</code>, <code>1-k</code> for Gilbert-Elliot drop model

* User defined model
    For enabling this feature, set drop-mode to <code>5</code> and set user-model to file location that consists of <code>0</code> and <code>1</code> in text form, where <code>1</code> corresponds to packet drop.

   Random and burst drop mode can be additionally limited by setting <code>max-drop-count</code> property. Burst drop mode have one additional parameter for burst length that describes how many package can be dropped by one round of burst drop.

* Data corruption
	* To enable bit flipping, set <code>bitflip-probability</code>, <code>bit-length</code> and <code>bit-flip-packet</code> properties. <code>-1</code> in bit length mean over all packet

		```
		$ gst-launch-1.0 ... ! channelemulator bitflip-probability=0.1 bit-length=-1 bit-flip-packet=10 ! ...
		```

    * To enable data corruption, set <code>data-corrupt-probability</code>, <code>corrupt-length</code>, <code>corrupt-packet-number</code> properties.
	  ```
	  $ gst-launch-1.0 ... ! channelemulator data-corrupt-probability=0.1 corrupt-length=10 corrupt-packet-number=100 ! ...
	  ```

All above properties can be limited with <code>loop-return-period</code> and corresponding properties like <code>max-drop-count</code>

* Packets delaying

	```
  	$ gst-launch-1.0 … ! channelemulator delay-probability=0.1 min-delay=3 max-delay=40 ! …
	```

* Packets duplicating enabling

	```
	$ gst-launch-1.0 … ! channelemulator duplicate-probability=0.1 ! …
	```


Build steps:
-----------

1. Build and install gstreamer core
2. <code>$ ./autogen.sh --prefix=/usr/local</code>
3. <code>$ make</code>
4. <code>$ sudo make install</code>

Element Properties
-----------
```
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
```

Dependencies
------------

You'll need a GStreamer 1.x installation.
