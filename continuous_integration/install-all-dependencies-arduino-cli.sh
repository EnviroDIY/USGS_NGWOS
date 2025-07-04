#!/bin/bash

# Set options,
set -e # Exit with nonzero exit code if anything fails
if [ "$RUNNER_DEBUG" = "1" ]; then
    echo "Enabling debugging!"
    set -v # Prints shell input lines as they are read.
    set -x # Print command traces before executing command.
fi

echo "\e[32mCurrent Arduino CLI version:\e[0m"
arduino-cli version

echo "\e[32mUnzipping the library\e[0m"
unzip -o AllDependencies.zip -d home/arduino/downloads/libs -x "*.git/*" "continuous_integration/*" "docs/*" "examples/*" "*.history/*"

if [ -d "home/arduino/user/libraries" ]; then
    echo "\e[32mEnsuring no old directories exist\e[0m"
    rm -rf home/arduino/user/libraries/*
fi

echo "\e[32mMoving the unzipped libraries to the libraries directory\e[0m"
mv home/arduino/downloads/libs/* home/arduino/user/libraries/

echo "\e[32mUpdating the library index\e[0m"
arduino-cli --config-file continuous_integration/arduino_cli.yaml lib update-index

echo "\e[32mListing libraries detected by the Arduino CLI\e[0m"
arduino-cli --config-file continuous_integration/arduino_cli.yaml lib list

echo "\e[32mListing the contents of the Arduino library directory\e[0m"
ls home/arduino/user/libraries
