import argparse
import os
import subprocess

if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("--exe")

  parser.add_argument("--input")
  parser.add_argument("--output")
  parser.add_argument("--format")
  options = parser.parse_args()

  cmd = [options.exe]
  cmd += ["-fd", options.format]
  cmd += ["-EncodeWith", "GPU"]
  cmd += [options.input]
  cmd += [options.output]

  ret = subprocess.call(cmd)
  if ret != 0:
    raise RuntimeError("compressonator has returned non-zero status: {0}".format(ret))
