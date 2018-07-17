1. Format a flash drive as FAT32. If given the option, choose "full format" over "quick format".
Look below for a list of flash drives that are confirmed to work.
Most that I've tried work, but it's more likely to work with a relatively small one, like 16GB or less. 
Even if it is empty, do a full format.



3. place the avr32fwupgrade.uc3 file in the flash drive. It should be the only file on the drive, without any folders or anything else.



4. Put the flash drive in the MantaMate USB port.



5. Turn off the MantaMate power supply, and turn it back on while holding the DOWN button. 



6. The MantaMate will go blank (or show random garbage on the 7-segment display) for a few minutes (wait at least 5 minutes), and then wake up with the new firmware. 



7. Unplug the flash drive, and turn the power off and on again. 



//Changes
 in firmware 1.4:* changes the way MPE mode is entered. (now it is entered by holding P and pressing S = 0 indicates normal MIDI and 1 indicates MPE mode) * fixes switch debouncing so that P and S buttons are more reliable.
//
Changes in firmware 1.3:
* fixes a bug that prevents proper saving of USB-MIDI learned CC#s as user presets
* adds support for some MPE devices such as Roli Seaboard


//Changes in firmware 1.2:
* fixes a bug introduces by 1.1 that causes left option mode to have flashing lights interrupting functionality.
* cleans up how octave functionality works. 
//

Changes in firmware 1.1:
* fixes a bug that prevents proper saving of Manta Keyboard Mode presets
* fixes a bug that can cause strange octaves shifts on some notes
* adds a feature in Manta Sequencer Mode. The current pitch of the sequence is now displayed when you are in "play" mode.
* some minor fixes to ensure proper state when changing between user and factory presets

Flash drives that are confirmed to work:
SanDisk Cruzer Glide 16GB (https://www.amazon.com/SanDisk-Flash-Cruzer-Glide-SDCZ60-016G-B35/dp/B007YX9O9O)
Verbatim Store n' Go Micro USB Drive 4GB
Sandisk Cruzer Blade 4GB
Verbatim Store n' Go 2GB