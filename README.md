imgviewer
=========
[![GitHub license](https://img.shields.io/badge/license-GPLv3-blue.svg?style=flat-square)](https://www.gnu.org/licenses/gpl-3.0.txt)
[![Coverity Scan Build Status](https://scan.coverity.com/projects/7077/badge.svg)](https://scan.coverity.com/projects/spillerrec-overmix)

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
- libpng

Linux specific libraries:

- x11extras
- libxcb

**Building**

1. qmake
2. make