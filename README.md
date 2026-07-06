# Sound of Music

![Screenshot](screenshot.png)

Sound of Music is a multiband bit crusher plugin with three bands, allowing detailed audio control and manipulation. 

## Build from Source

1. **Clone the Repository:**
```bash
git clone https://github.com/enter-opy/sound-of-music.git
cd sound-of-music
git submodule update --init --recursive
```
2. **Build the Plugin:**
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## License
This project is licensed under the GNU General Public License. See the [LICENSE](https://github.com/enter-opy/sound-of-music/blob/main/LICENSE) for details.
