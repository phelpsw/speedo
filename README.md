
## Dependencies
 - avrdude 6.0.1
  - It seems this works with the provided .avrduderc file for 441/841 support
 - libftdi1 0.20-1ubuntu1

## Prebuilt Toolchain

The attiny441 is not supported by binutils-avr 2.23.1-2.1 or gcc-avr 1:4.8-21
both from Ubuntu 14.04 apt sources.  As a result the atmel avr toolchain must
be used.

Download Atmel AVR 8-bit Toolchain 3.5.0 - Linux 64-bit:
http://www.atmel.com/tools/ATMELAVRTOOLCHAINFORLINUX.aspx

Unzip and update the AVRP parameter in the makefile accordingly


## Loading
```
make burn
```

## Fuses
 - efuse = 11111111 0xff
 - hfuse = 00000101 0x05
 - lfuse = 00000010 0x02

```
sudo avrdude -c jtag2isp -p t841 -F -U lfuse:w:0x02:m -U hfuse:w:0x05:m -U efuse:w:0xff:m
```

Notes: I misprogrammed hfuse to 0x65 which apparently can only be recovered in
high voltage programming mode.  See pg 219 in datasheet.  Whoops.

## Manually Built Toolchain
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
avrduderc with support for attiny841
 - http://www.ve7xen.com/blog/2014/03/07/programming-the-attiny841-with-avrdude/
 - http://www.avrfreaks.net/comment/1015011#comment-1015011

## References
 - http://www.embedds.com/programming-16-bit-timer-on-atmega328/
 - http://www.eevblog.com/forum/beginners/how-to-measure-frequency-using-avr/
 - http://electronics.stackexchange.com/questions/2811/measuring-0-1mhz-0-25hz-resolution-squarewave-using-an-mcu
