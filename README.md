# neon-arm-library-miyoo
 A library and header to link against for Neon instructions in Miyoo apps

## Disclaimer:
- I did not write this source. If you know the origin of this source please raise a PR for the correct references and attribution to be added to this repo & License updated
- I suspect `eggs` may have written some of this. 
- There are some equivalents here: https://github.com/M-HT/neon_scalers
  
## Building:

```
git clone https://github.com/shauninman/union-miyoomini-toolchain.git
cd union-miyoomini-toolchain
make shell
git clone https://github.com/XK9274/neon-arm-library-miyoo.git
export CROSS_COMPILE="arm-linux-gnueabihf-"
make
```
