# Jumper Virtual Lab Peripheral Modeling Framework
This repo contains modeled peripherals using the jumper modeling framework

For more information, visit [the docs](https://docs.jumper.io).

### Prerequisites
- GCC and Make: `apt install build-essential`
- [Jumper Virtual Lab](https://docs.jumper.io)

## Usage
### Follow the following steps in order to build the peripheral model:

  ```bash
  git clone https://github.com/Jumperr-labs/modeling-framework.git
  cd modeling-framework/<PERIPHERAL_NAME>
  ```
  
#### Build using Makefile
  ```bash
  make -C build/
  ```
  
#### Build using CMakeLists.txt
  ```bash
  python build/build.py
  ```
 
- If the peripheral model was build successfully, the result will be ready under "build/_build/PERIPHERAL_NAME.so".
  
### Runing firmware :
- Copy "build/_build/PERIPHERAL_NAME.so" file to you working diretory, same one as the "board.json" file.
- See "board.json" example file under "example/" folder.
- Add the relveant component from the "board.json" example file in to your "board.json" file. Make sure to change the board.json pin numbers to fit your configuration.

## License
Licensed under the Apache License, Version 2.0. See the LICENSE file for more information
