#!/usr/bin/env bash

TEST_DIR=$( dirname "${BASH_SOURCE[0]}" )

if [[ $(command -v python) ]]; then
    if [[ $(python --version) == "Python 3."* ]]; then
        python $TEST_DIR/wiztests.py $@
    else
        if [[ $(command -v python3) ]]; then
            python3 $TEST_DIR/wiztests.py $@
        else
            echo Incompatible Python interpreter. Please install a Python 3 interpreter that is version Python 3.6 or greater, and put it on your PATH.
            exit 1
        fi
    fi
elif [[ $(command -v python) ]]; then
    python3 $TEST_DIR/wiztests.py $@
else
    echo No python installation was found. Please install a Python 3 interpreter that is version Python 3.6 or greater, and put it on your PATH.
    exit 1
fi

