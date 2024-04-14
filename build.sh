#!/bin/bash
set -e 

echo "Fetching..."
git remote update

echo "Rebasing..."
git checkout master
git rebase --rebase-merges upstream/master
git submodule update --init --recursive

if [ -d osx/build ]; then
	echo "Cleaning up..."
	rm -Rf osx/build
fi

echo "Building..."
xcodebuild -project osx/deadbeef.xcodeproj clean
xcodebuild -project osx/deadbeef.xcodeproj -target DeaDBeeF -configuration Release

echo "Copying to place..."
if [ -d /Applications/DeaDBeef.old.app ]; then
	sudo rm -Rf /Applications/DeaDBeeF.old.app
fi

if [ -d /Applications/DeaDBeef.app ]; then
	sudo mv /Applications/DeaDBeeF.app /Applications/DeaDBeeF.old.app
fi

git push --force-with-lease origin master

sudo rsync --delete -a osx/build/Release/DeaDBeeF.app/ /Applications/DeaDBeeF.app
echo "Done!"
