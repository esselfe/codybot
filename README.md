# Codybot

20190824-20240707

## Overview

`codybot` is an IRC bot written with the C programming language, providing users
with fortune cookies, jokes, oneliner C/C++ compilation, shell access, text colorization,
ascii art, special characters and weather per city. It was inspired by candide on
Libera.chat, a great bot written in Perl (see https://github.com/pragma-/pbot/).

The bot can connect to `irc.libera.chat:6697` or any specified server and port;
it defaults to port 6697 with SSL.

## Compile and Run

In order to compile the source code into an executable, some dependencies are  
required to be installed. On RPM-based systems, install `glibc-devel binutils`  
`gcc make file-devel openssl-devel libcurl-devel json-c-devel`. On DEB-based  
systems install `libc6-dev binutils gcc make libmagic-dev libssl-dev`  
`libcurl4-openssl-dev libjson-c-dev`. On Arch install `make gcc`.  

To compile the program, just run `make` within the source directory, and run with  
`./codybot -n `_`YourBotNick`_

(Personally I run the bot in a docker container to limit general filesystem access
via the `!sh` command; see the "chroot" and "docker" sections below.)

Run `./codybot --help` to see all program options.

There is no installation mechanism, you should use the source directory or move
the files where you want. The files `data/ascii.txt`, `data/chars.txt`, `data/fortunes.txt`,
`data/jokes.txt` and `stats` must be in the program's current working directory
otherwise the program will not run. Other control files (detailed below) must
also be in the current working directory.

Before running for the first time, you should also run  

    scripts/initial-setup

so something like `!sh rm importantfiles` won't be able to do any damage;
see https://github.com/esselfe/codybot/issues/1

## Identifying

You should register and identify your bot nick with the IRC server's NickServ,
if it has one (not all IRC networks do). You can type `/msg NickServ help` in
your favorite connected IRC client (mIRC, irssi, weechat, hexchat, ...).
Otherwise you can identify to NickServ by typing `id `_`yourpasswordhere`_ or
`privmsg NickServ :identify `_`passhere`_ in the terminal running the codybot program.

Another console-only command you need to run is `join `_`#channel_name_here`_.
Note that bots are usually unwelcome on IRC networks, especially in popular channels,
and uninvited bots are not permitted, so always get permission if you don't own
the channel. (If you created a channel by joining an empty one, you now own it,
so you can give yourself permission to run codybot.)

## Using codybot

The bot responds to commands on its stdin (usually the terminal you run it from),
and in any channels it's joined. Commands in channels are only recognized if they
start with the command prefix, which defaults to exclamation point (`!`).
Do not include it when typing to its stdin. Some commands can only be used from stdin,
they are `exit`, `quit`, `curch`, `msg` and `id`.

In the following examples, "send _`!command`_" means send the command prefix and
_command_ as a message to a channel that the bot has joined. "type _command_" means
send _command_ to stdin of the running codybot process (usually by typing it into the terminal).
This differentiation may also be implied by the presence or absense of a leading prefix.

To see available commands, send `!help`.

To trigger a fortune cookie, send `!fortune`. The fortune cookie database file is `data/fortunes.txt`.
It's made of files in `/usr/share/games/fortunes` using the system-provided fortune package.
(see https://github.com/shlomif/fortune-mod)

To get a random joke send `!joke`. This database is hand written using https://www.funnyshortjokes.com
content and is far from containing all the site's jokes! There's 25 jokes as of 20200510.

To get astronomical informations such as sunrise, sunset, moonrise, moonset times, phase 
and illumination from https://www.weatherapi.com send `!astro `_`citynamehere`_;
which should return something like 
"_Montreal, Quebec, Canada: Sunrise 06:01 AM, sunset 06:30 PM, moonrise 08:01 AM, moonset 08:40, phase Waning Gibbous, illumination 91%_"
Note that you will need an API key to use this feature (It's free).
Just put the key in the api.key file.

To get weather report from https://www.weatherapi.com send `!weather `_`citynamehere`_;
this should return something like 
"_Montreal, Quebec, Canada: Partly cloudy, 2.6*C/36.7*F feels like -2.4*C/27.7F 15.0kmh/9.3mph, gust 20.4kmh/12.7mph, 0.0mm_"

To run a shell command from the chat onto the host of codybot, send `!sh `_`command and args`_
e.g. `!sh ls /home/codybot`. You can disable this by creating a file called `sh_disable`
(as always, in the program's current directory). You can also type `sh_enable` or `sh_disable`
in the console, or `!sh_enable` or `!sh_disable` in the channel (requires to be in the admins list).  

The other commands are:  
`!about` to retrieve the author and source code link.  
`!admins` to show current administrators of the bot who can run special commands like `!admins reload`.  
`!ascii` to show an ascii art image.  
`!cc printf("This is awesome!\n");` to compile C one-liners.  
`!chars` to show special UTF-8 characters.  
`!colorize SomeTextHere` to put random IRC color codes between each characters of the given text.  
`!date` to show current date and time with possible "utc-/+N" argument.  
`!dict TermHere` to retrieve the definition of a given term from dict.org.  
`!foldoc AnyComputerTerm` to retrieve the definition of a given term from the foldoc computer-related database.  
`!rainbow SomeText` (same but with ordered rainbow colors).  
`!rawmsg PRIVMSG ChanServ :OP #codybot esselfe` to send a raw message to the server (admins only).  
`!stats` to show how many fortunes have been given.  
`!time CityNameHere` to retrieve current time in a city.  
`!uptime` to get for how long the bot have been running.  
`!version` to show current bot's version.  

## Running in a chroot

If you want to use the chroot mechanism, you have to download the minimal chroot archive
and extract it into the source directory. The latest chroot is available at  

* https://hobby.esselfe.ca/codybot/chroot.tar.xz (Lunar minimal, 5.6MB download, extracts to 26MB)
* https://hobby.esselfe.ca/codybot/chroot-alpine.tar.xz (3.4MB to 73MB)  
* https://hobby.esselfe.ca/codybot/chroot-arch.tar.xz (8.3MB to 40MB)  
* https://hobby.esselfe.ca/codybot/chroot-aws.tar.xz (11MB to 66MB)  
* https://hobby.esselfe.ca/codybot/chroot-debian.tar.xz (120MB to 370MB)
* https://hobby.esselfe.ca/codybot/chroot-gentoo.tar.xz (244MB to 1.2GB)  

To run all shell commands in a locked chroot, create a file called `sh_lock` or
type `!sh_lock` or `!sh_unlock`. You have to run as root:  

    chroot chroot /bin/bash  
    su - dummy  
    run.sh  

You must ensure that the codybot user can write to
`/home/dummy/run.fifo` inside the chroot. The files starting with
`cmd.` also need to be owned by the `dummy` user.
Note that the `dummy` user must exist with the same UID in both the host
and in the chroot.

## Running in Docker

Update 210228 - new docker image available!

You can now run codybot more safely by using a small or full docker container.

Install docker, make sure the services are started (containerd+docker),
and run those commands to fetch and start the image:
(replace "small" with "full" if you want to)

    docker pull esselfe/lunar-codybot:small
    docker run -it esselfe/lunar-codybot:small bash

or

    docker pull esselfe/debian-codybot:small
    docker run -it esselfe/debian-codybot:small bash

Once in the container, run:

	exec su -
	tmux
	su - user
	cd codybot
	echo 'a8b2c3d4e5f6...your_www.weatherapi.com_key_here' > api.key
	./api-fetcher &
	scripts/logd.sh &
	<ctrl+b><c>
	su - codybot
	cd codybot
	./codybot -n NewNickHere

Personally I made a 1GB partition on my host just for the bot and
its users' usage, then mount it (on the host) to `/mnt/codybot-data`,
then start the container this way instead:

    docker run -it -v /mnt/codybot-data:/home/user/codybot/tmp esselfe/lunar-codybot bash

To make `/home/user/tmp` the only possible location to write, run _inside_ the container:

    rm -rf /tmp
    ln -sv /home/user/codybot/tmp /tmp

You can also limit the permitted storage size by adding "--tmpfs /home/user/codybot/tmp:rw,size=100M"  

## Running in Qemu  

Update 231222 - new qemu image available!  

You can now run codybot more safely by using a virtual machine.  
The command I use is:  
`qemu-system-x86_64 -vga std -display gtk -m 1024 -cpu host -smp 4 --enable-kvm -drive file=Lunar-codybot.qcow2,if=virtio -net user,hostfwd=tcp::2222-:22 -net nic`  
https://qemu.esselfe.ca/codybot/

## Running in VirtualBox

Update 240707 - new VirtualBox disk image available!

https://vbox.esselfe.ca/Lunar-codybot.vdi

----

### Links:  
- Code last changes: https://github.com/esselfe/codybot/commits
- Blinkenshell project:  
  - https://blinkenshell.org/wiki/Projects/codybot  
  - https://codybot.u.blinkenshell.org/  
- Main releases: https://github.com/esselfe/codybot/releases  
- Archives:
  - https://hobby.esselfe.ca/codybot/  

----

Enjoy!
