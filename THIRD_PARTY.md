# Third-party components

The game code in this repository is NovaPowers' [DBZ Budokai 3 HD Collection](https://github.com/novapowers0/DBZ-Budokai-3-HD-Collection), under the MIT License ([LICENSE](LICENSE)). The PS5 app is built with the projects below, which keep their own licences.

| Component | How it's used | Licence |
|---|---|---|
| [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) runtime, with code from [Xenia](https://xenia.jp) | the runtime and its PS5 layer (the release's runtime source zip), linked into `eboot.bin` | BSD 3-Clause |
| [Mesa](https://mesa3d.org) RADV | the Vulkan driver, linked into `eboot.bin` | MIT (and the other licences in Mesa's `licenses/`) |
| [PS5 Payload SDK](https://github.com/ps5-payload-dev/sdk) | the toolchain, libc and system library stubs | GPLv3+, FreeBSD headers under BSD licences (its README) |
| [ps5-plume-triangle](https://github.com/Nazky/ps5-plume-triangle) | the app start-up code, link recipe and FSELF signer used by `ps5/tools/ps5_package.sh` | see its repository |
| [Dear ImGui](https://github.com/ocornut/imgui) | the hub and the in-game screens | MIT |
| [stb_image, stb_vorbis](https://github.com/nothings/stb) | the hub's pictures and OGG music | MIT or public domain |
| [dr_mp3, dr_flac](https://github.com/mackron/dr_libs) | the hub's MP3 and FLAC music | MIT-0 or public domain |
| Oswald, Inter (`ps5/fonts`) | the hub's text | SIL Open Font License 1.1 |
| M PLUS Rounded 1c (`ps5/tools/fonts`) | the letters on the drawn PlayStation buttons | SIL Open Font License 1.1 |

The runtime also carries the libraries the ReXGlue SDK includes (FFmpeg and libmspack under the LGPL 2.1, SDL 3, fmt, spdlog, toml++, xxHash and others, each under its own licence, as listed in the SDK).
