#!/bin/sh
# argument $1 - path to project's directory

# Version info

if command -v git >/dev/null 2; then
  echo "Git detected."
else
  echo "Git not found. Aborting."
  exit 1;
fi

git --git-dir $1/.git log --pretty=format:"#define GIT_REVISION \"%H\" %n#define GIT_REVISION_ABBR \"%h\" %n#define GIT_COMMIT_ADATE \"%ai\" %n#define GIT_COMMIT_AT %at" -1 > $1/src/revision.h

# Formatting

ASTYLE=astyle

if command -v $ASTYLE >/dev/null 2; then
  echo "AStyle detected."
else
  echo "AStyle not found. Try included astyle.exe."

  ASTYLE=$1/astyle.exe

  if command -v $ASTYLE >/dev/null 2; then
    echo "AStyle detected at $ASTYLE. Try using it."
  else
    echo "AStyle not found. Aborting."
    exit 1;
  fi
fi

$ASTYLE --options=$1/astyle.conf "$1/src/*.cpp"
$ASTYLE --options=$1/astyle.conf "$1/src/*.h"
