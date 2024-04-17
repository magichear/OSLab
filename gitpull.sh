#!/bin/sh
eval "$(ssh-agent -s)"
git reset --hard
git pull
