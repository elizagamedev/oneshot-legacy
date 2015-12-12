#!/bin/bash

cd "$(dirname "$0")"
unamestr=`uname`

if [[ "$unamestr" == 'Linux' ]]; then

case $1 in
	home)
		[ -z "$XDG_CONFIG_HOME" ] && XDG_CONFIG_HOME="$HOME/.config"
		mkdir -p "$XDG_CONFIG_HOME/Oneshot"
		winepath -w "$XDG_CONFIG_HOME/Oneshot" > return
		echo 0 > rc
		;;
	documents)
		type xdg-user-dir >/dev/null 2>&1
		if [[ $? -eq 0 ]]; then
			winepath -w "$(xdg-user-dir DOCUMENTS)" > return
		elif [[ -d "$HOME/Documents" ]]; then
			winepath -w "$HOME/Documents" > return
		else
			winepath -w "$HOME" > return
		fi
		echo 0 > rc
		;;
	desktop)
		type xdg-user-dir >/dev/null 2>&1
		if [[ $? -eq 0 ]]; then
			winepath -w "$(xdg-user-dir DESKTOP)" > return
		elif [[ -d "$HOME/Desktop" ]]; then
			winepath -w "$HOME/Desktop" > return
		else
			winepath -w "$HOME" > return
		fi
		echo 0 > rc
		;;	
	messagebox)
		type zenity >/dev/null 2>&1 || { echo -1 > rc; exit; }
		case $4 in
			0)
				type="--info --icon-name= --window-icon="
				;;
			1)
				type="--info"
				;;
			2)
				type="--warning"
				;;
			3)
				type="--error"
				;;
			4)
				type="--question"
				;;
		esac
		zenity $type --text="$2" --title="$3"
		rc=$?
		case $rc in
			0)
				rc=1
				;;
			1)
				rc=0
				;;
		esac
		echo $rc > rc
		;;
	wallpaper)
		session=0
		type gsettings >/dev/null 2>&1
		if [[ $? -eq 0 ]]; then
			# We have dconf!
			# Determine if we're running cinnamon or gnome/unity
			if [[ "$(pidof gnome-session)" ]]; then
				session=1
				dconf_background_path="org.gnome.desktop.background"
			elif [[ "$(pidof cinnamon-session)" ]]; then
				session=2
				dconf_background_path="org.cinnamon.desktop.background"
			elif [[ "$(pidof mate-session)" ]]; then
				session=3
				dconf_background_path="org.mate.desktop.background"
			fi
		fi
		case $2 in
			query)
				if [[ $session -eq 0 ]]; then
					echo 1 > rc
				else
					echo 2 > rc
				fi
				;;
			get)
				if [[ $session -eq 0 ]]; then
					if [[ `uname -m` == 'x86_64' ]]; then
						paper="paper64"
					else
						paper="paper32"
					fi
					chmod +x $paper
					./$paper get > return
					echo "$?" > rc
				else
					case $(gsettings get $dconf_background_path picture-options) in
						\'centered\')
							style=0
							;;
						\'wallpaper\')
							style=1
							;;
						\'stretched\')
							style=2
							;;
						\'scaled\')
							style=3
							;;
						\'zoom\')
							style=4
							;;
						\'spanned\')
							style=5
							;;
					esac
					case $(gsettings get $dconf_background_path color-shading-type) in
						\'solid\')
							shading=0
							;;
						\'vertical\')
							shading=1
							;;
						\'horizontal\')
							shading=2
							;;
					esac
					echo $(gsettings get $dconf_background_path picture-uri) | cut -c2- | rev | cut -c2- | rev > return
					echo $style >> return
					echo $shading >> return
					echo $(gsettings get $dconf_background_path primary-color) | cut -c3-8 >> return
					echo $(gsettings get $dconf_background_path secondary-color) | cut -c3-8 >> return
					echo 0 > rc
				fi
				;;
			set)
				if [[ $session -eq 0 ]]; then
					if [[ `uname -m` == 'x86_64' ]]; then
						paper="paper64"
					else
						paper="paper32"
					fi
					chmod +x $paper
					./$paper set "$3"
					echo "$?" > rc
				else
					case $4 in
						0)
							style="centered"
							;;
						1)
							style="wallpaper"
							;;
						2)
							style="stretched"
							;;
						3)
							style="scaled"
							;;
						4)
							style="zoom"
							;;
						5)
							style="spanned"
							;;
					esac
					case $5 in
						0)
							shading="solid"
							;;
						1)
							shading="vertical"
							;;
						2)
							shading="horizontal"
							;;
					esac
					gsettings set $dconf_background_path picture-uri "$3"
					gsettings set $dconf_background_path picture-options $style
					gsettings set $dconf_background_path color-shading-type $shading
					gsettings set $dconf_background_path primary-color "#$6"
					gsettings set $dconf_background_path secondary-color "#$7"
					echo 0 > rc
				fi
				;;
		esac
esac

elif [[ "$unamestr" == 'Darwin' ]]; then

case $1 in
	home)
		mkdir -p "$HOME/Library/Application Support/Oneshot"
		winepath -w "$HOME/Library/Application Support/Oneshot" > return
		echo 0 > rc
		;;
	documents)
		if [[ -d "$HOME/Documents" ]]; then
			winepath -w "$HOME/Documents" > return
		else
			winepath -w "$HOME" > return
		fi
		echo 0 > rc
		;;
	desktop)
		if [[ -d "$HOME/Desktop" ]]; then
			winepath -w "$HOME/Desktop" > return
		else
			winepath -w "$HOME" > return
		fi
		echo 0 > rc
		;;
	messagebox)
		if [[ $4 == 4 ]]; then
			buttons='{"No", "Yes"}'
			default='"Yes"'
		else
			buttons='{"OK"}'
			default='"OK"'
		fi
		button=$(osascript\
			-e 'tell application "System Events"'\
			-e "set result to display dialog \"$2\" with title \"$3\" buttons $buttons default button $default"\
			-e 'end tell'\
			-e 'get button returned of result'
			)
		case $button in
			OK)
				rc=1
				;;
			Yes)
				rc=1
				;;
			No)
				rc=0
				;;
		esac
		echo $rc > rc
		;;
	wallpaper)
		case $2 in
			query)
				echo 3 > rc
				;;
			get)
				filename=$(osascript -e 'tell application "System Events"'\
						     -e 'tell current desktop'\
						     -e 'set result to picture'\
						     -e 'end tell'\
						     -e 'end tell'\
						     -e 'get result')
				echo "$filename" > return
				echo 0 > rc
				;;
			set)
				osascript -e 'tell application "System Events"'\
					  -e 'tell current desktop'\
					  -e "set picture to \"$3\""\
					  -e 'end tell'\
					  -e 'end tell'
				echo 0 > rc
				;;
		esac
esac

fi

