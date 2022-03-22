import argparse
import os
import subprocess

if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("--reflection-parser-exe",
                      help="Relative path to the exe.")

  parser.add_argument("--reflection-in-dir",
                      help="Base directory with source headers.")
  parser.add_argument("--cc-out-dir",
                      help="Output directory for cpp files.")
  parser.add_argument("headers", nargs="+",
                      help="Input source reflection headers.")
  options = parser.parse_args()

  reflection_dir = os.path.relpath(options.reflection_in_dir)
  reflection_cmd = [os.path.realpath(options.reflection_parser_exe)]

  headers = options.headers
  cc_out_dir = options.cc_out_dir

  reflection_cmd += ["--reflection-path", reflection_dir]
  reflection_cmd += ["--cc-out-dir", cc_out_dir]

  reflection_cmd += [os.path.join(reflection_dir, name) for name in headers]

  ret = subprocess.call(reflection_cmd)
  if ret != 0:
    raise RuntimeError("reflection_parser has returned non-zero status: {0}".format(ret))
