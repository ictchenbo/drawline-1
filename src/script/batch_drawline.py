#!/usr/bin/python

import sys, os, re
from time import time, strftime, localtime

def check_file(f):
  if not os.path.exists(f):
    print('Can\'t find file ' + f)
    return False
  return True

def write_log(msg):
  sys.stderr.write(strftime("%a, %d %b %Y %X", localtime()) + ': ' + \
                   msg)

if __name__ == '__main__':

  if len(sys.argv) != 5:
    print('Usage: python3 ' + sys.argv[0] + ' ne_file articles template' + \
          ' out_dir 2>>log')
    print('articles can be dir(but sub_dir will be ignore) or a file')
    print('\nPython3 should be used in order to deal with utf-8 conveniently')
    exit(1)

  if not check_file(sys.argv[1]):
    exit(1)
  ne_file = open(sys.argv[1]).readlines()

  if not os.path.isdir(sys.argv[2]):
    articles = [sys.argv[2]]
  else:
    dir_name = sys.argv[2]
    if dir_name[-1] != '/':
      dir_name += '/'
    articles = [dir_name + x for x in os.listdir(dir_name) if not os.path.isdir(x)]
  if len(articles) == 0:
    exit(0)
  articles_dir = os.path.dirname(articles[0])
  if articles_dir != '':
    articles_dir += '/'
  articles = set(articles)

  if not check_file(sys.argv[3]):
    exit(1)
  template = open(sys.argv[3]).read()

  out_dir = sys.argv[4]
  if os.path.exists(out_dir):
    print('Warning: ' + out_dir + ' exists, files may be overwritten')
  else:
    os.mkdir(out_dir)

  tmp = 'tmp%d' % time()

  # cnt = 0
  concepts = []
  title = None
  for line in ne_file[::-1]: # reverse it for conveniently processing
    line = line.strip('\n')
    if line.startswith('title:') :
      title = line[6:]
      # cnt += 1
      # if cnt < 5: print(cnt, title, concepts)
      # drawline processing
      if (articles_dir + title) in articles:
        open(tmp, 'w').write(template + '\n' + '\n'.join(concepts))
        cmd = './drawline ' + tmp + ' \'' + articles_dir + title + \
              '\' 2>log>\'' + out_dir + '/' + title + '\''
              #'\' 2>/dev/null >\'' + out_dir + '/' + title + '\''
        #print(cmd)
        if os.system(cmd) != 0:
          write_log('Drawline: there is some error processing file ' + \
                    title + 'Skip it.\n')
      else:
        write_log('Can\'t find file ' + title + ' Skip it.\n')

      concepts = []
    elif line.startswith('CONCEPT:'):
      concepts.append(line)

  os.system('rm ' + tmp)

