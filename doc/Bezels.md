# Bezels

Refer also to **CmdLine arguments** [here](CmdLine.md)

## Video Positional Arguments
| Option    | Description                   |
|--------------|-------------------------------|
| -scalefactor _\<25-100\>_   | Scale video image _[25-100]_%                           |
| -shiftx _\<-100 to 100\>_   | Position video window - horizontally                    |
| -shifty _\<-100 to 100\>_   | Position video window - vertically                      |
| -vertical_screen            | Reorient calculations above within a portrait screen    |


## Bezel Arguments
| Option    | Description                   |
|--------------|-------------------------------|
| -bezel _\<lair.png\>_              | Specify a _png_ bezel within the 'bezels' sub-folder  |
| -bezeldir _\<path\>_               | Specify an absolute path to an alternate folder       |
| -bezelflip                         | Reverse the priority bezels are rendered to screen    |
|                                    |                                                       |
| -scorebezel                        | Built-in bezel: ace/lair/tq scoreboard                |
| -scorebezel_alpha _\<1-2\>_        | Built-in bezel: alpha transparency levels             |
| -scorebezel_position _\<x\> \<y\>_ | Adjust position of software Scorepanel                |
| -scorebezel_scale _\<1-25\>_       | Software Scorepanel scale option                      |
|                                    |                                                       |
| -annunbezel                        | Built-in bezel: ace annunciator _[Conversion]_        |
| -annunlamps                        | Built-in bezel: staggered lamps only _[Conversion]_   |
| -dedannunbezel                     | Built-in bezel: ace annunciator _[Dedicated]_         |
| -tqkeys                            | Built-in bezel: Interactive tq keyboard displayed     |
|                                    |                                                       |
| -auxbezel_alpha _\<1-2\>_          | Built-in bezel: alpha transparency levels             |
| -auxbezel_position _\<x\> \<y\>_   | Built-in bezel: position options                      |
| -auxbezel_scale _\<1-25\>_         | Built-in bezel: scale option                          |

## Scorepanel Arguments
| Option    | Description                   |
|--------------|-------------------------------|
| -scorepanel                        | Enable windowed software scoreboard in lair/ace/tq    |
| -scorepanel_position _\<x\> \<y\>_ | Adjust position of software Scorepanel                |
| -scorepanel_scale _\<1-25\>_       | Software Scorepanel scale option                      |
| -scorescreen _\< x \>_             | Specify an initial Scorepanel display screen          |
|                                    |                                                       |
| _[KEY_TILT]_                       | Switch Scorepanel display screen lair/ace/tq          |


## Override built-in bezel graphic
| Game | Location                   |
|------|----------------------------|
| ace      | - bezels/annunoff.png      |
| ace      | - bezels/annunon.png       |
| ace      | - bezels/cadet.bmp         |
| ace      | - bezels/captain.bmp       |
| ace      | - bezels/spaceace.bmp      |
|          |                            |
| ace      | - bezels/onspaceace.bmp    |
| ace      | - bezels/oncaptain.bmp     |
| ace      | - bezels/oncadet.bmp       |
| ace      | - bezels/offspaceace.bmp   |
| ace      | - bezels/offcaptain.bmp    |
| ace      | - bezels/offcadet.bmp      |
|          |                            |
| tq       | - bezels/tqkeys.png        |
