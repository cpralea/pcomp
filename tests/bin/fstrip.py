import sys


COMMENT_START: str = ';'


def strip_comments(line: str) -> str:
    p = line.find(COMMENT_START)
    if p != -1:
        return line[:p]
    return line


def strip_right_whitespaces(line: str) -> str:
    return line.rstrip()


def print_nonempty_line(line: str):
    if line:
        print(line)


for line in sys.stdin:
    line = strip_comments(line)
    line = strip_right_whitespaces(line)
    print_nonempty_line(line)
