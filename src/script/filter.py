
import re
import os, sys

USAGE = \
'''python3 filter.py dir/files
filter relations that have less than 3 entity'''

if __name__ == '__main__':
  if len(sys.argv) != 2:
    print(USAGE)
    exit(1)

  files = []
  if os.path.isdir(sys.argv[1]):
    dir_name = sys.argv[1]
    files = [dir_name + '/' + x for x in os.listdir(dir_name) if not os.path.isdir(x)]
  else:
    files = [sys.argv[1]]

  for f in files:
    lines = [line.strip() for line in open(f)]
    with open(f, 'w') as out:
      last_lst = []
      for line in lines:
        if re.match('\d+\s+\d+', line) or line == '*****':
          if len(last_lst) >= 4+1:  # add 1 for '-----'
            out.write('\n'.join(last_lst) + '\n')
          last_lst = []
        last_lst.append(line)
      if len(last_lst) >= 4+1:
        out.write('\n'.join(last_lst))

