import argparse
import os
import subprocess

if __name__ == "__main__":
  parser = argparse.ArgumentParser()
  parser.add_argument("--dxc-exe")

  parser.add_argument("--shader")
  parser.add_argument("--entry")
  parser.add_argument("--profile")
  parser.add_argument("--output")
  parser.add_argument("--spirv", action="store_true")
  options = parser.parse_args()

  reflection_cmd = [options.dxc_exe]
  reflection_cmd += ["-E", options.entry]
  reflection_cmd += ["-T", options.profile]
  reflection_cmd += ["-Fo", options.output]
  if (options.spirv):
      reflection_cmd += ["-spirv"]
  reflection_cmd += [options.shader]

  ret = subprocess.call(reflection_cmd)
  if ret != 0:
    raise RuntimeError("dxc.exe has returned non-zero status: {0}".format(ret))
