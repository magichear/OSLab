@echo off
echo Starting auto commit and push to git
git add .
git commit -m "Normal update"
git push
echo Task completed
pause
