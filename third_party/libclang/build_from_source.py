##----------------------------------------------------------------------------##
## This file is distributed under the MIT License.                            ##
## See LICENSE.txt for details.                                               ##
## Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             ##
##----------------------------------------------------------------------------##
import distutils.dir_util
import argparse
import os
import shutil
import subprocess
import sys


def run_command(cmd):
    print("Cmd: " + cmd)
    p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, encoding="utf-8", bufsize=1)
    while True:
        output = p.stdout.readline()
        if p.poll() is not None:
            break
        if (output):
            print(output.strip())


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-d", action="store_true", help="Debug build", required=False)
    args = parser.parse_args()

    abspath = os.path.abspath(__file__)
    script_dir = os.path.dirname(abspath)
    llvm_dir = os.path.join(script_dir, "llvm-project")
    cwd = os.getcwd()

    # Building
    os.chdir(llvm_dir)
    vcvars = ""
    vs2019_vcvars = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat"
    if os.path.exists(vs2019_vcvars):
        vcvars = vs2019_vcvars
    if sys.platform == "win32":
        assert vcvars
    build_dir = "build"
    build_type = "-DCMAKE_BUILD_TYPE={}"
    if args.d:
        build_dir = "build_debug"
        build_type = build_type.format("Debug")
    else:
        build_type = build_type.format("Release")
    os.makedirs(build_dir, exist_ok=True)
    build_cmd = "cmake -S llvm -B {0} -G Ninja -DLLVM_ENABLE_PROJECTS='clang' {1} && ninja -C {0} libclang".format(build_dir, build_type)
    if sys.platform == "win32":
        run_command('"{}" && {}'.format(vcvars, build_cmd))
    else:
        run_command(build_cmd)

    # Copying
    os.chdir(script_dir)
    distutils.dir_util.copy_tree("llvm-project/clang/include/clang-c", "include/clang-c")
    relative_path = "llvm-project/{}/".format(build_dir)
    if sys.platform == "win32":
        os.makedirs("bin/win64", exist_ok=True)
        shutil.copy2(relative_path + "bin/libclang.dll", "bin/win64")
        os.makedirs("lib/win64", exist_ok=True)
        shutil.copy2(relative_path + "lib/libclang.lib", "lib/win64")
    elif sys.platform == "linux":
        os.makedirs("bin/linux64", exist_ok=True)
        shutil.copy2(relative_path + "bin/libclang.so".format(build_dir), "bin/linux64")
        os.makedirs("lib/linux64", exist_ok=True)
        shutil.copy2(relative_path + "lib/libclang.a".format(build_dir), "lib/linux64")
    else:
        print("hmm...")

    # Done
    os.chdir(cwd)
