WCPatcher - archives browsing accelerator for Total Commander
=============================================================

[![Build status](https://ci.appveyor.com/api/projects/status/6f8902snk5rkppl2/branch/master?svg=true)](https://ci.appveyor.com/project/remittor/wcpatcher/branch/master)
[![Codacy Badge](https://api.codacy.com/project/badge/Grade/0ec0b982ebfc439789126d44cc46b65c)](https://www.codacy.com/manual/remittor/wcpatcher?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=remittor/wcpatcher&amp;utm_campaign=Badge_Grade)
[![License: MIT](https://img.shields.io/badge/License-MIT-informational.svg)](https://github.com/remittor/wcpatcher/blob/master/License.txt)
[![Platforms](https://img.shields.io/badge/platform-windows-9cf)](https://en.m.wikipedia.org/wiki/Windows_XP)
[![Github All Releases](https://img.shields.io/github/downloads/remittor/wcpatcher/total.svg)](https://www.github.com/remittor/wcpatcher/releases/latest)
[![GitHub release (latest by date including pre-releases)](https://img.shields.io/github/v/release/remittor/wcpatcher?include_prereleases)](https://www.github.com/remittor/wcpatcher/releases)

This WCP-plugin intercepts 4 functions in TotalCmd image in order to change the algorithm for working with the file structure of opened archives.
It is the use of a tree-like file structure that allows the speed of working with archives that contain many files and directories.

**WCP-plugin speeds up browsing archives at least 1000 times !!!**

Results of performance tests
----------------------------

Testing was conducted on an 11GB TAR-file, which contained 453973 files and 52930 directories.

Test results of the original algorithm and the algorithm from WCP-plugin:

|Orig algo (ms)|WCP-plugin (ms)|  items | comment/directory        |
| ------------:| -------------:| ------:| ------------------------ |  
|     46527.07 |       3351.26 | 506900 | file collection building |
|      4615.79 |          0.12 |      1 | [root dir]               |
|      4952.83 |          1.00 |     51 | [AP\kernel\firmware]     |
|      4830.28 |          3.45 |    130 | [AP\kernel\kernel]       |
|      5653.89 |          5.08 |    188 | [AP\external]            |

Other performance tests can be found [here](https://www.ghisler.ch/board/viewtopic.php?f=6&t=56640).
