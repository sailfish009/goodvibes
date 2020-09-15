#!/bin/bash

# qdbus syntax  : qdbus org.gnome.SessionManager /org/gnome/SessionManager org.gnome.SessionManager.IsInhibited 4
# dbusctl syntax: busctl --user call org.gnome.SessionManager /org/gnome/SessionManager org.gnome.SessionManager IsInhibited u 4

set -eu

fail() { echo >&2 "$@"; exit 1; }
checkcmd()  { command -v "$1" >/dev/null 2>&1; }
assertcmd() { checkcmd "$1" || fail "'$1' is not installed"; }
checkdbusname() { qdbus "$1" >/dev/null 2>&1; }

assertcmd qdbus

if checkdbusname org.gnome.SessionManager; then
  qdbus \
    org.gnome.SessionManager \
    /org/gnome/SessionManager \
    org.gnome.SessionManager.IsInhibited 4

elif checkdbusname org.freedesktop.PowerManagement; then
  qdbus \
    org.freedesktop.PowerManagement \
    /org/freedesktop/PowerManagement/Inhibit \
    org.freedesktop.PowerManagement.Inhibit.HasInhibit

else
  fail "No D-Bus service found"

fi
