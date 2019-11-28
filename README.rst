g510s
=====

Sources to make the Logitech G510S workable on Linux

Sources
-------

* libg15.patch: http://pastebin.com/5VQixu64
* g15daemon.patch: http://pastebin.com/bjmp8T56
* libg15-1.2.7: http://sourceforge.net/projects/g15tools/
* libg15render-1.2: http://sourceforge.net/projects/g15tools/
* g15daemon-1.9.5.3: http://sourceforge.net/projects/g15daemon/
* g15macro-1.0.3: http://sourceforge.net/projects/g15daemon/
* g15stats-1.9.7: http://sourceforge.net/projects/g15daemon/

References
----------

* https://wiki.archlinux.org/index.php/Logitech_Gaming_Keyboards#G510_on_g15daemon

Install
-----
Dependencies: `libusb-dev` `libgtop2-dev`

For each directory do a `./configure`, `make` and `sudo make install`. Order:
* libg15
* libg15render
* g15daemon
* the rest

Start with `sudo LD_LIBRARY_PATH=/lib:/usr/lib:/usr/local/lib g15daemon`. Alternatively you can allow non-root to the USB based on the keyboard's ID.
`g15stats &` can run without sudo afterwards.
