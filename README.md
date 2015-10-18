
# Dependencies
 - avrdude 6.0.1
  - It seems this works with the provided .avrduderc file for 441/841 support
 - libftdi1 0.20-1ubuntu1

 - binutils-avr 2.23.1-2.1
 - gcc-avr 1:4.8-21





Note: Developed with following tools on Linux Mint 17.1 (Ubuntu 14.04.2 LTS).





## avrdude attiny441 support
avrduderc with support for attiny841
 - http://www.ve7xen.com/blog/2014/03/07/programming-the-attiny841-with-avrdude/
 - http://www.avrfreaks.net/comment/1015011#comment-1015011





## REFERENCE: avrdude 6.1
```
wget http://download.savannah.gnu.org/releases/avrdude/avrdude-6.1.tar.gz
sudo apt-get install libftdi-dev byacc flex build-essential
./configure
make
sudo make install
```
