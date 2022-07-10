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
  parser.add_argument("--uniform-binding-offset")
  parser.add_argument("--texture-binding-offset")
  parser.add_argument("--sampler-binding-offset")
  options = parser.parse_args()

  reflection_cmd = [options.dxc_exe]
  reflection_cmd += ["-E", options.entry]
  reflection_cmd += ["-T", options.profile]
  reflection_cmd += ["-Fo", options.output]
  reflection_cmd += ["-Zi"]
  reflection_cmd += ["-O1"]
  if (options.spirv):
      reflection_cmd += ["-spirv"]
      if (options.uniform_binding_offset):
          reflection_cmd += [ "-fvk-b-shift", options.uniform_binding_offset, "all" ]
      if (options.texture_binding_offset):
          reflection_cmd += [ "-fvk-t-shift", options.texture_binding_offset, "all" ]
      if (options.sampler_binding_offset):
          reflection_cmd += [ "-fvk-s-shift", options.sampler_binding_offset, "all" ]
  reflection_cmd += [options.shader]

  ret = subprocess.call(reflection_cmd)
  if ret != 0:
    raise RuntimeError("dxc.exe has returned non-zero status: {0}".format(ret))
