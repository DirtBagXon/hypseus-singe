# CmdLine

#### Derived from Daphne [CmdLine](http://www.daphne-emu.com/mediawiki/index.php/CmdLine)

### Command Line Format
The command line format is:

    hypseus <game type> vldp -framefile ... <options>

The game type and framefile argument are required.

Refer also to **bezel arguments** [here](Bezels.md)

## Game Types
| Game Type    | Description                   |
|--------------|-------------------------------|
| ace, ace_a, ace_a2 | Space Ace NTSC            |
| ace91, ace91_euro | Space Ace '91              |
| aceeuro       | Space Ace PAL               |
| astron, astronp | Astron Belt                 |
| badlands, badlandp | Badlands                  |
| bega, begar1   | Bega's Battle               |
| blazer        | Star Blazer                 |
| cliff, cliffalt, cliffalt2 | Cliff Hanger         |
| cobra, cobraab, cobraconv, cobram3 | Cobra Command |
| dle11         | Dragon's Lair Enhancement v1.1 |
| dle21         | Dragon's Lair Enhancement v2.1 |
| esh, eshalt, eshalt2 | Esh's Aurunmilla       |
| galaxy, galaxyp | Galaxy Ranger              |
| gpworld       | GP World                    |
| gtg           | Goal to Go                  |
| interstellar  | Interstellar                |
| lair, lair_f, lair_e, lair_d, lair_c, lair_b, lair_a, lair_x | Dragon's Lair NTSC |
| lair_n1       | Dragon's Lair NTSC (prototype) |
| laireuro, lair_d2 | Dragon's Lair PAL         |
| lair_ita      | Dragon's Lair PAL (Italian) |
| lair2, lair2_* | Dragon's Lair 2             |
| mach3         | M.A.C.H. 3                  |
| roadblaster   | Road Blaster                |
| sae           | Space Ace Enhancement       |
| sdq, sdq*     | Super Don Quix-ote          |
| singe         | Run a Singe LUA game        |
| tq, tq*       | Thayer's Quest              |
| uvt           | Us vs Them                  |

## Laserdisc Player Types
| Laserdisc Type | Description                  |
|---------------|------------------------------|
| vldp          | Virtual LaserDisc Player     |

## Game Options
| Option                           | Description                                           |
|----------------------------------|-------------------------------------------------------|
| -altaudio \<suffix string>       | Specifies a suffix to be added to the audio filenames that Hypseus would normally try to use. The purpose is so you can use alternate audio for games. |
| -alwaysontop                     | Enable SDL WINDOW_ALWAYS_ON_TOP for main game window. |
| -bank \<which bank> \<base2 value> | Used to modify dip switch settings. The 'which bank' argument specifies which dip switch bank to modify, where 0 is the first bank. The 'base2 value' argument is in 8-bit binary form that is composed of 1's and 0's, where the right-most bit corresponds to the first dip switch. For example, if you wanted to enable dip switch 0 in bank 0, but disable switches 1-7, then you'd do "-bank 0 00000001". |
| -bezel \<bezel.png>              | Specify a png bezel in 'bezels' sub-folder             |
| -blank_blue                      | VLDP blank using YUV#1DEB6B                            |
| -blank_searches                  | Forces the screen to go blank during searches.         |
| -blank_skips                     | Forces the screen to go blank during skips.            |
| -cheat                           | Enables cheating. Cheating is not available for all games. Each game only has one cheat. Most cheats give you unlimited lives. |
| -enable_leds                     | Enables keyboard LEDs for Space Ace. The original Space Ace arcade game had three LED's that corresponded to the skill settings of Cadet, Captain, and Ace. Hypseus can make the keyboard LED's mimic this behavior. This requires administrator privileges. |
| -fastboot                        | Makes games start faster. Only available on a few games like Dragon's Lair, Space Ace, Cliff Hanger, and Goal to Go. |
| -framefile \<location>           | Points to the framefile used by VLDP. This is **required** with the VLDP.  |
| -force_aspect_ratio              | Tells Hypseus to force the 4:3 aspect ratio regardless of the video size. |
| -fullscreen                      | Runs Hypseus in fullscreen mode (instead of windowed mode). |
| -fullscreen_window               | Runs Hypseus in a fullscreen window.                    |
| -gamepad                         | Enable SDL_GameController configuration. Use -haptic [0-4] to configure rumble. |
| -gamepad_focus <0> <3>           | Specify 2 SDL_GameController indexes, focus polling on the specified Controllers for 'Pad' definitions in hypinput.ini:  &nbsp;_[Default: 0 1]_|
| -grabmouse                       | Capture mouse in SDL window.                           |
| -homedir \<dirname>              | Sets the Home Directory that Hypseus will use.         |
| -idleexit \<seconds>             | Tells Hypseus to exit after a certain number of seconds if no input has been received. |
| -ignore_aspect_ratio             | Tells Hypseus to ignore the aspect ratio defined in the MPEG header. |
| -keymapfile \<config>            | Specify an alternate hypinput.ini file.                |
| -latency \<ms>                   | Adds a delay before all searches occur which causes scenes to last a little longer. Useful for Dragon's Lair F2 ROMs that cut some scenes off prematurely. |
| -linear_scale                    | Enable linear filtering when scaling.                  |
| -manymouse                       | Use the alternate ManyMouse input system. Provides *Absolute* coordinates on supported devices. |
| -min_seek_delay \<ms>            | The minimum amount of milliseconds to force a seek to take (artificial delay). 0 = disabled. |
| -nocrc                           | Disables CRC32 checking (not recommended).            |
| -nohwaccel                       | Disables hardware acceleration for SDL2. Only use this if hardware acceleration doesn't work. |
| -noissues                        | Don't display warnings about game driver problems.    |
| -nojoystick                      | Disables any joysticks that may be plugged in. Joysticks will normally be used if available. |
| -nolog                           | Disables writing to the log file (hypseus.log).    |
| -noserversend                    | A legacy argument. No usage statistics are collected or sent. |
| -nosound                         | Disables all sound.                                   |
| -nospeech                        | Disables speech for Thayer's Quest.                   |
| -novsync                         | Disable VSYNC presentation on Renderer.               |
| -opengl                          | Enforces SDL_WINDOW_OPENGL                            |
| -original_overlay                | Enable daphne style overlays (lair, ace, lair2)       |
| -pal_dl                          | Tells Hypseus that you are using a PAL Philips Dragon's Lair disc instead of an NTSC Dragon's Lair disc. *Only relevant when playing the USA version of Dragon's Lair.* |
| -pal_dl_sc                       | Tells Hypseus that you are using a PAL Software Corner Dragon's Lair disc instead of an NTSC Dragon's Lair disc. *Only relevant when playing the USA version of Dragon's Lair.* |
| -pal_sa                          | Tells Hypseus that you are using a PAL Philips Space Ace disc instead of an NTSC Space Ace disc. *Only relevant when playing the USA version of Space Ace.* |
| -pal_sa_sc                       | Tells Hypseus that you are using a PAL Software Corner Space Ace disc instead of an NTSC Space Ace disc. *Only relevant when playing the USA version of Space Ace.* |
| -prefer_samples                  | Same games can emulate sound or use samples of sounds. If both emulated and sampled sounds are available, this option will force sampled sounds to be used. Otherwise, emulated sounds will always be used. |
| -preset \<number>                | A simple way to pass arguments directly to the game driver. Tells the game driver to use a specific preset configuration. Different for each game. |
| -rotate \<degrees>               | Rotates the screen a certain number of degrees counter-clockwise. Valid values are from 0-359. *This is a somewhat limited feature.* |
| -sboverlaymono                   | Use white LED's in (lair, ace) scoreboard overlay     |
| -seek_frames_per_ms \<frames> | The # of frames that we can seek per millisecond (to simulate seek delay). Typical values for real laserdisc players are about 30.0 for 29.97fps discs and 20.0 for 23.976fps discs (dragon's lair and space ace). FLOATING POINT VALUES ARE ALLOWED HERE. Minimum value is 12.0 (5 seconds for 60,000 frames), maximum value is 600.0 (100 milliseconds for 60,000 frames). If you want a value higher than the max, you should just use 0 (as fast as possible). *This option may be replaced by something more accurate in the future.* |
| -scalefactor \<25-100>            | Scale video display area [25-100]%.                    |
| -scanlines                      | Simulate scanline effect. See also -scanline_alpha and -scanline_shunt                               |
| -scoreboard                      | Enables external Scoreboard.                           |
| -scoreport \<port>               | Sets which parallel port to use with the scoreboard. 0 correspond to LPT1. *As of v1.0.12, any value over 1 will indicate the address of the parallel port in hexadecimal. So instead of passing in 0, I could pass in 378 which would achieve the same result.* |
| -screen \<1-254>                 | Defines the screen _hypseus_ should use for display. |
| -script                          | Defines the location of the primary Singe LUA game script. Required for Singe games. `-zlua` is an alias. |
| -shiftx \<-100 to 100>           | Shift x-axis on video window [%]                                        |
| -shifty \<-100 to 100>           | Shift y-axis on video window [%]                                        |
| -sound_buffer \<number of samples> | Sets the number of samples in the sound buffer. Hypseus runs at 44,100 kHz which means 44,100 samples per second. The sound buffer size is typically 2048 samples. Lower values make the sound more responsive but choppier, while higher values make the sound smoother but more sluggish. |
| -spaceace91                      | Tells Hypseus that you are using a Space Ace '91 disc instead of a Space Ace '83 NTSC disc. *Only relevant when you are playing the USA version of Space Ace '83.* |
| -sram_continuous_update          | Saves the static RAM after every search so that if Hypseus is terminated improperly, high scores are preserved. |
| -startsilent                     | Tells Hypseus to start with no sound until input has been received. |
| -texturestream                   | Enable SDL_TEXTUREACCESS_STREAMING           |
| -texturetarget                   | Enable SDL_TEXTUREACCESS_TARGET (Default).   |
| -tiphat                          | Invert joystick SDL_HAT_UP and SDL_HAT_DOWN. |
| -usbscoreboard \<args>           | Enable USB serial support for scoreboard. Arguments: *(i)mplementation, (p)ort, (b)aud* |
| -usbserial_rts_on                | Enable RTS on USB serial port setup [Default: off] |
| -use_annunciator                 | Use this when using a real Space Ace scoreboard with the annunciator board attached. *Space Ace only.* |
| -useoverlaysb \<overlay number>   | Enables a graphical scoreboard for Dragon's Lair, Space Ace, or Thayer's Quest. The 'overlay number' is the style of scoreboard. Currently the two choices for the 'overlay number' are 0 and 1. |
| -vertical_screen                  | Reorient calculations in the logical fullscreen |
| -vertical_stretch \<value>        | Vertically stretches the screen outward from the center. The purpose of this is to remove the black bars on the top and bottom of Cliff Hanger and Goal to Go. To get rid of the black bars completely, use a value of 24. |
| -volume_nonvldp \<volume>         | Sets the volume of all audio besides the laserdisc audio when using VLDP (max value is 64, 0 means muted). |
| -volume_vldp \<volume>            | Sets the volume of the laserdisc audio when using VLDP (max value is 64, 0 means muted). |
| -x \<horizontal resolution>      | Specifies the width of the game window (in pixels).    |
| -y \<vertical resolution>         | Specifies the height of the game window (in pixels).   |

## Singe Game Options
| Option                           | Description                                                                                 |
|----------------------------------|---------------------------------------------------------------------------------------------|
| -8bit_overlay                    | Restore original 8bit Singe overlays.                                                       |
| -blend_sprites                   | Restore BLENDMODE outline on Singe sprites.                                                 |
| -bootsilent                      | Mute sound during initVLDP() - if possible.                                                 |
| -js_range \<1-20>                | Adjust Singe joystick sensitivity: *[def:5]*                                                |
| -nocrosshair                     | Request game does not display crosshairs.                                                   |
| -retropath                       | Singe LUA data path rewrites [.daphne]                                                      |
| -script                          | Defines the location of the Singe LUA script. **Required** for Singe games.                 |
| -sinden \<1-10> \<color>         | Enable a Sinden style border for Gun Games. Color: *(w)hite, (r)ed, (g)reen, (b)lue or (x*) |

## Shortcuts
| Shortcut                         | Description                                                              |
|----------------------------------|--------------------------------------------------------------------------|
| Alt-Enter                        | Toggle Fullscreen.                                                       |
| Alt-Backspace                    | Toggle Scanlines.                                                        |
| Alt-[KEYPAD]                     | Viewport Positioning toolbox. Requires [KeyPad]                          |
| [KEY_BUTTON3]                    | Toggle *overlay* scorebard in lair/ace.                                  |
| [KEY_COIN1]=\|[KEY_START1]       | *Joystick* hotkey combination for [KEY_QUIT]                             |
| [KEY_TILT]                       | Switch *scorepanel* display screen lair/ace/tq.                          |

Credits: [Daphne Wiki](http://www.daphne-emu.com/mediawiki/index.php/CmdLine)
