This is a customized version Smoldyn (based on v2.37) to enable simulations of macromolecules such as CaMKII holoenzymes. Rule-based descriptions of reactions are allowed at the binding sites level. For the original version of Smoldyn, please see [here](http://www.smoldyn.org).<br>

Installation has only been conducted on Debian or Ubuntu so far. A simple procedure to install can be as follows:<br>
```
cd cmake
cmake ..
make
sudo make install
```

There are some OS dependent issues that need to take care of when installing. For example, varaibles in the `CMakeLists.txt` file such as `glib_CFLAGS` can be pre-set in `~\.bashrc` using `export gsl_CFLAGS=$(pkg-config --cflags glib-2.0)`.
