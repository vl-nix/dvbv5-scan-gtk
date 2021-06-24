### Dvbv5-Gtk - Gtk+3 interface to [DVBv5 tool](https://www.linuxtv.org/wiki/index.php/DVBv5_Tools)

* Scan, Zap
* Drag and Drop: Scan, Zap

#### Dependencies

* gcc, meson
* libdvbv5 ( & dev )
* libgtk 3.0 ( & dev )

#### Quality

* Good - Magenta; Ok - Aqua; Poor - Orange

#### Record & Preview

* cat '/dev/dvb/adapter0/dvr0' > rec.ts
* ffplay '/dev/dvb/adapter0/dvr0'

#### Build

1. Clone: git clone https://github.com/vl-nix/dvbv5-gtk.git ( git@github.com:vl-nix/dvbv5-gtk.git )

2. Configure: meson build --prefix /usr --strip

3. Build: ninja -C build

4. Install: sudo ninja -C build install

5. Uninstall: sudo ninja -C build uninstall

