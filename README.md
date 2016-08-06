# Build and Load Procedure

Install prebuilt toolchain on linux to build and then use avrstudio on windows
to program the tiny441.


## Prebuilt Toolchain

The attiny441 is not supported by binutils-avr 2.23.1-2.1 or gcc-avr 1:4.8-21
both from Ubuntu 14.04 apt sources.  As a result the atmel avr toolchain must
be used.

Download Atmel AVR 8-bit Toolchain 3.5.0 - Linux 64-bit:
http://www.atmel.com/tools/ATMELAVRTOOLCHAINFORLINUX.aspx

Unzip and update the AVRP parameter in the makefile accordingly


## Loading

Because avrdude doesn't support the tiny441, we are unfortunately forced to use
avrstudio.

## Fuses
 - efuse = 11111111 0xff
 - hfuse = 00000101 0x05
 - lfuse = 00000010 0x02


# Manually Built Toolchain
I did not get this to work, instead used prebuilt toolchain above.  Notes for
reference.

 - http://www.atmel.com/webdoc/AVRLibcReferenceManual/install_tools_1install_avr_binutils.html
 - http://distribute.atmel.no/tools/opensource/Atmel-AVR-GNU-Toolchain/3.5.0/

```
sudo apt-get install libgmp-dev libmpc-dev libmpfr-dev
```
Create install directory for custom toolchain:
```
mkdir ~/projects/avr/local
```

### binutils
```
bunzip2 -c binutils-2.25.1.tar.bz2  | tar xf -
cd binutils-2.25.1/
mkdir obj-avr
cd obj-avr
../configure --prefix=<PREFIX>/projects/avr/local --target=avr --disable-nls
make
make install
```

Update PATH
```
echo 'export PATH=$PATH:<PREFIX>/projects/avr/local/bin' >> ~/.bashrc
```

### gcc
```
bunzip2 -c gcc-5.2.0.tar.bz2 | tar xf -
cd gcc-5.2.0/
mkdir obj-avr
cd obj-avr
../configure --prefix=<PREFIX>/projects/avr/local/ --target=avr --enable-languages=c,c++  --disable-nls --disable-libssp --with-dwarf2
make
make install
```

### avrlibc
```
bunzip2 -c avr-libc-1.8.1.tar.bz2 | tar xf -
cd avr-libc-1.8.1/
./configure --prefix=<PREFIX>/projects/avr/local/ --build=`./config.guess` --host=avr
make
make install
```


### avrdude attiny441 support
This does not work as of May 2016, notes for reference however.

avrduderc with support for attiny841
 - http://www.ve7xen.com/blog/2014/03/07/programming-the-attiny841-with-avrdude/
 - http://www.avrfreaks.net/comment/1015011#comment-1015011

## References
 - http://www.embedds.com/programming-16-bit-timer-on-atmega328/
 - http://www.eevblog.com/forum/beginners/how-to-measure-frequency-using-avr/
 - http://electronics.stackexchange.com/questions/2811/measuring-0-1mhz-0-25hz-resolution-squarewave-using-an-mcu
