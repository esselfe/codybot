#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <magic.h>

#include "codybot.h"

int CheckCTCPTime(void) {
	t0 = time(NULL);
	if (strcmp(raw.nick, ctcp_prev_nick) == 0 && ctcp_prev_time <= t0-5) {
		ctcp_prev_time = t0;
		return 0;
	}
	else {
		ctcp_prev_time = t0;
		sprintf(ctcp_prev_nick, "%s", raw.nick);
		return 1;
	}
}

// Thread for reading raw data from the server.
void *ThreadRXFunc(void *argp);
void ThreadRXStart(void) {
	pthread_t thr;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thr, &attr, ThreadRXFunc, NULL);
	pthread_detach(thr);
	pthread_attr_destroy(&attr);
}

void *ThreadRXFunc(void *argp) {
	char buffer_rx[4096], buf[4096];
	struct stat st;
	while (!endmainloop) {
		memset(buffer_rx, 0, 4096);
		SSL_read(pSSL, buffer_rx, 4095);
		if (strlen(buffer_rx) == 0) {
			endmainloop = 1;
			break;
		}
		buffer_rx[strlen(buffer_rx)-2] = '\0';
		// Respond to ping request from the server with a pong
		if (strncmp(buffer_rx, "PING :", 6) == 0) {
			sprintf(buffer_cmd, "%s\n", buffer_rx);
			buffer_cmd[1] = 'O';
			MsgRaw(buffer_cmd);
			memset(buffer_cmd, 0, 4096);
			continue;
		}
		else
			Log(IN, buffer_rx);


		if (!RawLineParse(&raw, buffer_rx))
			continue;
		RawGetTarget(&raw);

		// Respond to few CTCP requests
		if (strcmp(raw.text, "\001CLIENTINFO\x01") == 0) {
			if (CheckCTCPTime())
				continue;

			sprintf(buf, "NOTICE %s :\001CLIENTINFO CLIENTINFO PING TIME VERSION\x01\n",
				raw.nick);
			MsgRaw(buf);
			continue;
		}
		else if (strncmp(raw.text, "\x01PING ", 6) == 0) {
			if (CheckCTCPTime())
				continue;

			sprintf(buf, "NOTICE %s :%s\n", raw.nick, raw.text);
			MsgRaw(buf);
			continue;
		}
		else if (strcmp(raw.text, "\x01TIME\x01") == 0) {
			if (CheckCTCPTime())
				continue;
			
			t0 = time(NULL);
			sprintf(buf, "NOTICE %s :\x01TIME %s\x01\n", raw.nick, ctime(&t0));
			MsgRaw(buf);
			continue;
		}
		else if (strcmp(raw.text, "\x01VERSION\x01") == 0) {
			if (CheckCTCPTime())
				continue;
			
			sprintf(buf, "NOTICE %s :\x01VERSION codybot %s\x01\n", raw.nick,
				codybot_version_string);
			MsgRaw(buf);
			continue;
		}

		if (strcmp(raw.channel, nick) == 0) {
			sprintf(buf, "PRIVMSG %s :Cannot use private messages", raw.nick);
			MsgRaw(buf);
			continue;
		}

if (raw.text != NULL && raw.nick != NULL && strcmp(raw.command, "JOIN") != 0 &&
strcmp(raw.command, "NICK") != 0) {
		SlapCheck(&raw);
		if (raw.text[0]==trigger_char) {
// help
		if (strncmp(raw.text+1, "help", 4) == 0) {
			char c = trigger_char;
			sprintf(buf, "commands: %cabout %cadmins %cascii %castro %ccal %ccalc %ccc "
				"%cchars %ccolorize %cdate %cdict %cfoldoc %chelp %cfortune %cjoke "
				"%cping %crainbow %csh %cstats %ctime %cuptime %cversion %cweather",
				c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c,c);
			Msg(buf);
			continue;
		}
// admins
		else if (strcmp(raw.text+1, "admins reload") == 0) {
			if (IsAdmin(raw.nick, raw.host)) {
				DestroyAdminList();
				ParseAdminFile();
				char *str = EnumerateAdmins();
				sprintf(buf, "Admins: %s", str);
				free(str);
				Msg(buf);
			}
		}
		else if (strncmp(raw.text+1, "admins", 6) == 0) {
			char *str = EnumerateAdmins();
			sprintf(buf, "Admins: %s", str);
			free(str);
			Msg(buf);
		}
// ascii
		else if (strncmp(raw.text+1, "ascii", 5) == 0) {
			if (strcmp(raw.channel, "#codybot")==0)
				AsciiArt(&raw);
			else
				Msg("ascii: can only be run in #codybot (due to output > 4 lines)");
		}
// about
		else if (strncmp(raw.text+1, "about", 5) == 0) {
			if (strcmp(nick, "codybot")==0)
				Msg("codybot is an IRC bot written in C by esselfe, "
					"sources @ https://github.com/esselfe/codybot");
			else {
				sprintf(buf, "codybot(%s) is an IRC bot written in C by esselfe, "
					"sources @ https://github.com/esselfe/codybot\n", nick);
				Msg(buf);
			}
		}
// astro
		else if (strcmp(raw.text+1, "astro") == 0) {
			sprintf(buf, "astro: missing city argument, example: "
				"'%castro montreal'", trigger_char);
			Msg(buf);
		}
		else if (strncmp(raw.text+1, "astro ", 6) == 0) {
			if (weather_disabled || stat("weather_disabled", &st) == 0) {
				sprintf(buf, "%castro is currently disabled, try again later or "
					"ask an admin to enable it", trigger_char);
				Msg(buf);
			}
			else
				Astro(&raw);
		}
// cal
		else if (strcmp(raw.text+1, "cal") == 0)
			Cal();
// calc
		else if (strcmp(raw.text+1, "calc") == 0)
			Msg("calc  example: '!calc 10+20'");
		else if (strncmp(raw.text+1, "calc ", 5) == 0)
			Calc(&raw);
// cc
		else if (strcmp(raw.text+1, "cc") == 0) {
			sprintf(buf, "example: %ccc printf(\"this\\n\");",
				trigger_char);
			Msg(buf);
		}
		else if (strcmp(raw.text+1, "cc clang") == 0) {
			cc_compiler = CC_COMPILER_CLANG;
			Msg("Compiler is now clang");
			continue;
		}
		else if (strcmp(raw.text+1, "cc gcc") == 0) {
			cc_compiler = CC_COMPILER_GCC;
			Msg("Compiler is now gcc");
			continue;
		}
		else if (strcmp(raw.text+1, "cc tcc") == 0) {
			cc_compiler = CC_COMPILER_TCC;
			Msg("Compiler is now tcc");
			continue;
		}
		else if (strcmp(raw.text+1, "cc_compiler") == 0) {
			if (cc_compiler == CC_COMPILER_CLANG) {
				Msg("Compiler is clang");
				continue;
			}
			else if (cc_compiler == CC_COMPILER_GCC) {
				Msg("Compiler is gcc");
				continue;
			}
			else if (cc_compiler == CC_COMPILER_TCC) {
				Msg("Compiler is tcc");
				continue;
			}
		}
		else if (strncmp(raw.text+1, "cc ", 3) == 0) {
			if (cc_disabled || stat("cc_disable", &st) == 0) {
			if (strcmp(server_name, "irc.blinkenshell.org") == 0)
				sprintf(buf, "%s: cc is permanently disabled", raw.nick);
			else {
				sprintf(buf, "%s: cc is temporarily disabled, "
					"try again later or ask esselfe to enable it", raw.nick);
			}

				Msg(buf);
				continue;
			}
			CC(&raw);
		}
		else if (strcmp(raw.text+1, "cc_disable") == 0) {
			if (IsAdmin(raw.nick, raw.host)) {
				cc_disabled = 1;
				Msg("cc_disabled = 1");
			}
			else {
				sprintf(buf, "cc_disable can only be used by an admin\n");
				Msg(buf);
			}
		}
		else if (strcmp(raw.text+1, "cc_enable") == 0) {
			if (IsAdmin(raw.nick, raw.host)) {
				cc_disabled = 0;
				Msg("cc_disabled = 0");
			}
			else {
				sprintf(buf, "cc_enable can only be used by an admin\n");
				Msg(buf);
			}
		}
// chars
		else if (strncmp(raw.text+1, "chars", 5) == 0)
			Chars(&raw);
// colorize
		else if (strcmp(raw.text+1, "colorlist") == 0) {
			Msg("\003011 \003022 \003044 \003055 \003066 \003077 \003088"
				" \00309 \0031010 \0031111 \0031212 \0031313 \0031414 \0031515");
		}
		else if (strcmp(raw.text+1, "colorize") == 0) {
			sprintf(buf, "Usage: %ccolorize some text to process", trigger_char);
			Msg(buf);
		}
		else if (strncmp(raw.text+1, "colorize ", 9) == 0)
			Colorize(&raw);
// date
		else if (strlen(raw.text) > 6 &&
			strncmp(raw.text+1, "date utc", 8) == 0) {
			if (strncmp(raw.text+5, "utc-12", 6) == 0)
				Date(-12);
			else if (strncmp(raw.text+6, "utc-11", 6) == 0)
				Date(-11);
			else if (strncmp(raw.text+6, "utc-10", 6) == 0)
				Date(-10);
			else if (strncmp(raw.text+6, "utc-9", 5) == 0)
				Date(-9);
			else if (strncmp(raw.text+6, "utc-8", 5) == 0)
				Date(-8);
			else if (strncmp(raw.text+6, "utc-7", 5) == 0)
				Date(-7);
			else if (strncmp(raw.text+6, "utc-6", 5) == 0)
				Date(-6);
			else if (strncmp(raw.text+6, "utc-5", 5) == 0)
				Date(-5);
			else if (strncmp(raw.text+6, "utc-4", 5) == 0)
				Date(-4);
			else if (strncmp(raw.text+6, "utc-3", 5) == 0)
				Date(-3);
			else if (strncmp(raw.text+6, "utc-2", 5) == 0)
				Date(-2);
			else if (strncmp(raw.text+6, "utc-1", 5) == 0)
				Date(-1);
			else if (strncmp(raw.text+6, "utc+1", 5) == 0)
				Date(1);
			else if (strncmp(raw.text+6, "utc+2", 5) == 0)
				Date(2);
			else if (strncmp(raw.text+6, "utc+3", 5) == 0)
				Date(3);
			else if (strncmp(raw.text+6, "utc+4", 5) == 0)
				Date(4);
			else if (strncmp(raw.text+6, "utc+5", 5) == 0)
				Date(5);
			else if (strncmp(raw.text+6, "utc+6", 5) == 0)
				Date(6);
			else if (strncmp(raw.text+6, "utc+7", 5) == 0)
				Date(7);
			else if (strncmp(raw.text+6, "utc+8", 5) == 0)
				Date(8);
			else if (strncmp(raw.text+6, "utc+9", 5) == 0)
				Date(9);
			else if (strncmp(raw.text+6, "utc+10", 6) == 0)
				Date(10);
			else if (strncmp(raw.text+6, "utc+11", 6) == 0)
				Date(11);
			else if (strncmp(raw.text+6, "utc+12", 6) == 0)
				Date(12);
		}
		else if (strncmp(raw.text+1, "date", 4) == 0)
			Date(0);
// dict
		else if (strcmp(raw.text+1, "dict") == 0) {
			sprintf(buf, "Missing argument, e.g. '%cdict wordhere'", trigger_char);
			Msg(buf);
		}
		else if (strncmp(raw.text+1, "dict ", 5) == 0)
			Dict(&raw);
// foldoc
		else if (strcmp(raw.text+1, "foldoc") == 0) {
			sprintf(buf, "Missing term argument, e.g. '%cfoldoc linux'", trigger_char);
			Msg(buf);
		}
		else if (strncmp(raw.text+1, "foldoc ", 7) == 0)
			Foldoc(&raw);
// forecast
		else if (strcmp(raw.text+1, "forecast") == 0) {
			sprintf(buf, "forecast: missing city argument, example: "
				"'%cforecast montreal'", trigger_char);
			Msg(buf);
		}
		else if (strncmp(raw.text+1, "forecast ", 9) == 0) {
			if (weather_disabled || stat("weather_disabled", &st) == 0) {
				sprintf(buf, "%cforecast is currently disabled, try again later or "
					"ask an admin to enable it", trigger_char);
				Msg(buf);
			}
			else
				Forecast(&raw);
		}
// fortune
		else if (strncmp(raw.text+1, "fortune", 7) == 0)
			Fortune(&raw);
// debug
		else if (strcmp(raw.text+1, "debug on") == 0) {
			if (IsAdmin(raw.nick, raw.host)) {
				debug = 1;
				Msg("debug = 1");
			}
			else
				Msg("debug mode can only be changed by an admin");
		}
		else if (strcmp(raw.text+1, "debug off") == 0) {
			if (IsAdmin(raw.nick, raw.host)) {
				debug = 0;
				Msg("debug = 0");
			}
			else
				Msg("debug mode can only be changed by an admin");
		}
// die
		else if (strncmp(raw.text+1, "die", 4) == 0) {
			if (IsAdmin(raw.nick, raw.host))
				exit(0);
		}
// joke
		else if (strncmp(raw.text+1, "joke", 4) == 0)
			Joke(&raw);
// msgbig
		else if (strcmp(raw.text+1, "msgbig") == 0 &&
			IsAdmin(raw.nick, raw.host)) {
			memset(buf, 0, 4096);
			memset(buf, '#', 1024);
			Msg(buf);
		}
		else if (strcmp(raw.text+1, "ping") == 0)
			Msg("pong");
// rainbow
		else if (strcmp(raw.text+1, "rainbow") == 0) {
			sprintf(buf, "Usage: %crainbow some random text", trigger_char);
			Msg(buf);
		}
		else if (strncmp(raw.text+1, "rainbow ", 8) == 0)
			Rainbow(&raw);
// rawmsg
		else if (strncmp(raw.text+1, "rawmsg ", 7) == 0) {
			printf("!!Sending raw message to server!!\n::%s::\n", raw.text+8);
			RawMsg(&raw);
		}
// stats
		else if (strcmp(raw.text+1, "stats") == 0)
			Stats(&raw);
// time
		else if (strcmp(raw.text+1, "time") == 0) {
			sprintf(buf, "time: missing city argument, example: "
				"'%ctime montreal'", trigger_char);
			Msg(buf);
		}
		else if (strncmp(raw.text+1, "time ", 5) == 0)
			Time(&raw);
// timeout
		else if (strcmp(raw.text+1, "timeout") == 0) {
			sprintf(buf, "timeout = %d", cmd_timeout);
			Msg(buf);
		}
		else if (strncmp(raw.text+1, "timeout ", 8) == 0) {
			if (strcmp(raw.nick, "codybot")==0 || IsAdmin(raw.nick, raw.host)) {
				raw.text[0] = ' ';
				raw.text[1] = ' ';
				raw.text[2] = ' ';
				raw.text[3] = ' ';
				raw.text[4] = ' ';
				raw.text[5] = ' ';
				raw.text[6] = ' ';
				raw.text[7] = ' ';
				raw.text[8] = ' ';
				cmd_timeout = atoi(raw.text);
				if (cmd_timeout == 0)
					cmd_timeout = 5;
				sprintf(buf, "timeout = %d", cmd_timeout);
				Msg(buf);
			}
			else {
				sprintf(buf, "timeout can only be set by an admin");
				Msg(buf);
			}
		}
// trigger
		else if (strcmp(raw.text+1, "trigger") == 0) {
			sprintf(buf, "trigger = '%c'", trigger_char);
			Msg(buf);
		}
		else if (raw.text[0]==trigger_char&&raw.text[1]=='t'&&raw.text[2]=='r'&&
			raw.text[3]=='i'&&raw.text[4]=='g'&&raw.text[5]=='g'&&raw.text[6]=='e'&&
			raw.text[7]=='r'&&raw.text[8]==' '&&raw.text[9]!='\n') {
			if (IsAdmin(raw.nick, raw.host))
				trigger_char = raw.text[9];
			sprintf(buf, "trigger = '%c'", trigger_char);
			Msg(buf);
		}
// uptime
		else if (strcmp(raw.text+1, "uptime") == 0) {
			gettimeofday(&tv0, NULL);
			t0 = (time_t)tv0.tv_sec - tv_start.tv_sec;
			tm0 = gmtime(&t0);
			if (tm0->tm_mday > 1)
				sprintf(buf, "uptime: %02d day%s %02d:%02d:%02d", tm0->tm_mday-1,
					(tm0->tm_mday>2)?"s":"", tm0->tm_hour, tm0->tm_min, tm0->tm_sec);
			else
				sprintf(buf, "uptime: %02d:%02d:%02d", tm0->tm_hour, tm0->tm_min,
					tm0->tm_sec);
			Msg(buf);
		}
// version
		else if (strcmp(raw.text+1, "version") == 0) {
			sprintf(buf, "codybot %s", codybot_version_string);
			Msg(buf);
		}
// weather
		else if (strcmp(raw.text+1, "weather") == 0) {
			sprintf(buf, "weather: missing city argument, example: "
				"'%cweather montreal'", trigger_char);
			Msg(buf);
		}
		else if (strncmp(raw.text+1, "weather ", 8) == 0) {
			if (weather_disabled || stat("weather_disabled", &st) == 0) {
				sprintf(buf, "%cweather is currently disabled, try again later or "
					"ask an admin to enable it", trigger_char);
				Msg(buf);
			}
			else
				Weather(&raw);
		}
		else if (strcmp(raw.text+1, "weather_disable") == 0) {
			if (IsAdmin(raw.nick, raw.host)) {
				weather_disabled = 1;
				Msg("weather_disabled = 1");
			}
			else {
				sprintf(buf, "Only an admin can use %cweather_disable",
					trigger_char);
				Msg(buf);
			}
		}
		else if (strcmp(raw.text+1, "weather_enable") == 0) {
			if (IsAdmin(raw.nick, raw.host)) {
				weather_disabled = 0;
				Msg("weather_disabled = 0");
			}
			else {
				sprintf(buf, "Only an admin can use %cweather_enable",
					trigger_char);
				Msg(buf);
			}
		}
// sh
		else if (strcmp(raw.text+1, "sh") == 0) {
			sprintf(buf, "sh: missing argument, example: '%csh ls -ld /tmp'",
				trigger_char);
			Msg(buf);
		}
		else if (strcmp(raw.text+1, "sh_lock") == 0) {
			if (IsAdmin(raw.nick, raw.host)) {
				sh_locked = 1;
				Msg("sh_locked = 1");
			}
			else {
				sprintf(buf, "sh_lock can only be used by an admin\n");
				Msg(buf);
			}
		}
		else if (strcmp(raw.text+1, "sh_enable") == 0) {
			if (IsAdmin(raw.nick, raw.host)) {
				sh_disabled = 0;
				Msg("sh_disabled = 0");
			}
			else {
				sprintf(buf, "sh_enable can only be used by an admin\n");
				Msg(buf);
			}
		}
		else if (strcmp(raw.text+1, "sh_disable") == 0) {
			if (IsAdmin(raw.nick, raw.host)) {
				sh_disabled = 1;
				Msg("sh_disabled = 1");
			}
			else {
				sprintf(buf, "sh_disable can only be used by an admin\n");
				Msg(buf);
			}
		}
		else if (strcmp(raw.text+1, "sh_unlock") == 0) {
			if (IsAdmin(raw.nick, raw.host)) {
				sh_locked = 0;
				Msg("sh_locked = 0");
			}
			else {
				sprintf(buf, "sh_unlock can only be used by an admin\n");
				Msg(buf);
			}
		}
		else if (strncmp(raw.text+1, "sh ", 3) == 0) {
			struct stat st;
			if (sh_disabled || stat("sh_disable", &st) == 0) {
				if (strcmp(server_name, "irc.blinkenshell.org") == 0)
					sprintf(buf, "%s: sh is permanently disabled", raw.nick);
				else
					sprintf(buf, "%s: sh is temporarily disabled, "
						"try again later or ask an admin to enable it", raw.nick);

				Msg(buf);
				continue;
			}
			// Remmember, touch $srcdir/sh_lock to have !sh commands run in a chroot
			else if(sh_locked || (stat("sh_lock", &st) == 0)) {
				raw.text[0] = ' ';
				raw.text[1] = ' ';
				raw.text[2] = ' ';
				sprintf(buffer_cmd, "bash -c \"echo '%s' > chroot/home/dummy/run.fifo\"", raw.text);
				system(buffer_cmd);

				// Wait for chroot/home/dummy/run.sh to write in cmd.output
				printf("++tail chroot/home/dummy/run.status++\n");
				sprintf(buffer_cmd, "tail chroot/home/dummy/run.status");
				system(buffer_cmd);
				printf("++tail chroot/home/dummy/run.status++ done\n");

				FILE *fr = fopen("chroot/home/dummy/cmd.output", "r");
				if (fr == NULL) {
					sprintf(buf, "codybot::ThreadRXFunc() error: Cannot "
						"open chroot/home/dummy/cmd.output: %s", strerror(errno));
					Msg(buf);
					continue;
				}

				// Count lines total
				int c;
				unsigned int line_total = 0;
				while (1) {
					c = fgetc(fr);
					if (c == -1)
						break;
					else if (c == '\n')
						++line_total;
				}
				fseek(fr, 0, SEEK_SET);

				size_t size = 4096;
				char *output = (char *)malloc(size);
				memset(output, 0, size);
				fgets(output, 400, fr);
				fclose(fr);
				Msg(output);

				if (line_total >= 2) {
					system("bash -c 'cat chroot/home/dummy/cmd.output | nc termbin.com 9999 >cmd.url'");
					fr = fopen("cmd.url", "r");
					if (fr == NULL) {
						sprintf(buf, "codybot::ThreadRXFunc() error: Cannot open cmd.url: %s", strerror(errno));
						Msg(buf);
						continue;
					}
					fgets(output, 4096, fr);
					Msg(output);
				
					fclose(fr);
				}

				continue;
			}

			char *cp = raw.text + 4;
// Check for the kill command
unsigned int dontrun = 0;
while (1) {
	if (*cp == '\n' || *cp == '\0')
		break;
	else if (*cp == ' ') {
		++cp;
		continue;
	}
	else if (*(cp+1)==':' && *(cp+2)=='(' && *(cp+3)==')') {
		dontrun = 1;
		break;
	}
	else if (*cp=='k' && *(cp+1)=='i' && *(cp+2)=='l' && *(cp+3)=='l' && *(cp+4)==' ') {
		dontrun = 1;
		break;
	}

	++cp;
}

if (!dontrun) {
// Check if running the cat program with an executable
char *c = raw.text + 4;
// Ignore whitespaces
while (1) {
	if (*c == ' ') {
		++c;
		continue;
	}
	else
		break;
}
if (strncmp(c, "cat ", 4)==0) {
	c += 3;
	while (1) {
		if (*c == ' ') {
			++c;
			continue;
		}
		else
			break;
	}
	char *filename = malloc(1024);
	memset(filename, 0, 1024);
	unsigned int cnt = 0;
	while (1) {
		if (*c == ' ' || *c == '\n' || *c == '\0') {
			magic_t mgc = magic_open (MAGIC_SYMLINK | MAGIC_PRESERVE_ATIME | MAGIC_MIME_TYPE);
			if (mgc == NULL) {
				Msg ("codybot error: magic_open() failed\n");
				break;
			}
			magic_load (mgc, NULL);
			char *magic_str;
			magic_str = (char *)magic_file (mgc, filename);
			if (strcmp(magic_str, "application/x-executable")==0)
				dontrun = 1;
			break;
		}
		else {
			filename[cnt] = *c;
			++c;
			++cnt;
		}
	}
} // if (strncmp(c, "cat ", 4)==0)
} // if (!dontrun)

			if (dontrun) {
				Msg("Will not run this!");
				continue;
			}

			raw.text[0] = ' ';
			raw.text[1] = ' ';
			raw.text[2] = ' ';
			
			// Run the received !sh shell command
			if (debug)
				printf("codybot::ThreadRXFunc() starting thread for ::%s::\n",
					raw.text);
			ShRunStart(raw.text);
		} // else if (strncmp(raw.text+1, "sh ", 3) == 0)
		} // if (raw.text[0] == trigger_char)

		usleep(10000);
} // if (raw.text != NULL && raw.nick != NULL && strcmp(raw.command, "JOIN") != 0 &&
  // strcmp(raw.command, "NICK")!=0)
	} // while (!endmainloop)
	return NULL;
} // void *ThreadRXFunc(void *argp)

