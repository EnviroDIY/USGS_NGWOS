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

echo "\e[32mUpdating the core index\e[0m"
arduino-cli --config-file arduino_cli.yaml core update-index

echo "::group::Installing EnviroDIY Cores"
echo "\e[32mInstalling the EnviroDIY AVR Core\e[0m"
arduino-cli --config-file arduino_cli.yaml core install EnviroDIY:avr
echo "::endgroup::"

echo "::group::Adafruit SAMD"
echo "\e[32mInstalling the Adafruit SAMD Core\e[0m"
arduino-cli --config-file arduino_cli.yaml core install adafruit:samd
echo "::endgroup::"

echo "\e[32mUpdating the core index\e[0m"
arduino-cli --config-file arduino_cli.yaml core update-index

echo "\e[32mUpgrading all cores\e[0m"
arduino-cli --config-file arduino_cli.yaml core upgrade

echo "\e[32mCurrently installed cores:\e[0m"
arduino-cli --config-file arduino_cli.yaml core list
