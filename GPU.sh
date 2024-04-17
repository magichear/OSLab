#!/bin/bash
echo "Starting auto commit and push to git"
git add .
git commit -m "Normal update1"
eval "$(ssh-agent -s)"
git push
echo "-----------------"
echo "-Task completed--"
echo "-----------------"
read -p "Press enter to continue"

