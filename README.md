# Cjump

### Configure project
Assuming you have a llvm/clang installed into $HOME/toolchains/clang50
```
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_PREFIX_PATH=$HOME/toolchains/clang50/lib/cmake/llvm/ ..
```

### Usage:
```
                        |--- input source path / line nuber : column ---| |-------------------------------- include directories -------------------------------------|
 ./cjump  -def -source /opt/hfs16.5/toolkit/samples/SOP/SOP_Flatten.C:5:5 . /usr/include/c++/4.9/ /usr/include/x86_64-linux-gnu/c++/4.9/ /opt/hfs16.5/toolkit/include/

```

### Sublime
- Copy cjump.py to: ```$HOME/.config/sublime-text-3/Packages/User/cjump.py```
- Add a key binding in .sublime-keymap
```  { "keys": ["ctrl+h"], "command": "cjump" },```
