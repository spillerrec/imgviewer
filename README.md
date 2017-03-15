imgviewer
=========
[![GitHub license](https://img.shields.io/badge/license-GPLv3-blue.svg?style=flat-square)](https://www.gnu.org/licenses/gpl-3.0.txt)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/7077/badge.svg)](https://scan.coverity.com/projects/spillerrec-imgviewer)

Customizable image viewer with minimalistic interface. Configuration and usage details can be found on the wiki.

### Features
- Color-managed
- Multi-monitor support
- Extensive mouse-actions, for example rocker-gestures
- Minimalistic user interface
- Animation support + APNG support
- Low-level configuration options
- Cross-platform

### Building

**Dependencies**

- Qt 5.2
- libexif
- lcms2
- zlib
- libpng
- libjpeg
- libgif

Linux specific libraries:

- x11extras (Qt5)
- libxcb

**Building**

1. ``cmake src``
2. ``make``
3. ``make install`` (Optional)
