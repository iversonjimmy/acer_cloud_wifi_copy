#!/usr/bin/python

import fileinput
import sys

if (len(sys.argv) != 2):
    print >> sys.stderr, (
        '\n'
        'Usage:\n'
        '  ' + sys.argv[0] + ' <source file>\n'
        '\n'
        'This script filters out lines in <source file> that are intended\n'
        '  for internal-use-only and writes the result to stdout.\n'
        'To mark a single line for removal by this script, start\n'
        '  that line with "//%" (not including the quotes).\n'
        'To mark a block of lines for removal, place them between a line starting\n'
        '  with "//%{INTERNAL" and another line starting with "//%}INTERNAL".\n'
        'If the line following "//%}INTERNAL" is empty or is only whitespace, it\n'
        '  will also be removed.  To suppress that behavior, use "//%}INTERNAL---"\n'
        '  instead.\n'
        'You can also mark a line with "//%#EXTERNAL " and this script will remove\n'
        '  that part of the line, leaving the rest of the line after that intact.\n'
        'Note that leading whitespace is always ignored when looking for the "//%".\n'
        'Trailing whitespace will also be removed from the result.\n'
    )
    sys.exit(255)

# Current number of INTERNAL blocks that the current line is within (nesting is allowed).
internal = 0

# If == 1, it indicates that the previous line closed an INTERNAL block and we want to omit the
# current line if it is empty or contains only whitespace.
skipEmptyLine = 0

def ERROR(msg):
    print >> sys.stderr, ("*** " + fileinput.filename() + ":" + str(fileinput.lineno()) + ': ' + msg)
    sys.exit(1)

for line in fileinput.input():
    currLine = line.strip()
    if currLine.startswith("//%"):
        if currLine.startswith("//%{"):
            if currLine != "//%{INTERNAL":
                ERROR('if it starts with "//%{" it must be "//%{INTERNAL"')
            internal += 1
        elif currLine.startswith("//%}"):
            if currLine == "//%}INTERNAL":
                skipEmptyLine = 2 # Set to 2 because we always decrement this at the end of the loop
            elif currLine == "//%}INTERNAL---":
                pass # The "---" indicates that we don't want to skip the next line, even if empty.
            else:
                ERROR('if it starts with "//%}" it must be "//%}INTERNAL" or "//%}INTERNAL---"')
            if internal < 1:
                ERROR('INTERNAL block closed here has no matching open')
            internal -= 1
        elif currLine.startswith("//%#"):
            if currLine.startswith("//%#EXTERNAL "):
                if internal > 0:
                    ERROR('EXTERNAL directive was found within an INTERNAL block')
                sys.stdout.write(line[len("//%#EXTERNAL "):])
            else:
                ERROR('if it starts with "//%#" a valid directive (EXTERNAL) must immediately follow')
        # else, simply ignore the line
    else:
        if internal == 0:
            if (skipEmptyLine > 0) and (len(currLine) == 0):
                pass # Special case: remove an empty line immediately following "//%}INTERNAL".
            else:
                # Write the line through to stdout
                sys.stdout.write(line)
        else:
            pass # we are within an INTERNAL block
    if skipEmptyLine > 0:
        skipEmptyLine -= 1

# At end, check that we matched all blocks:
if internal != 0:
    ERROR(internal + ' INTERNAL block(s) opened but not closed')

# Make sure there's an empty newline at the end:
print

sys.exit(0)
