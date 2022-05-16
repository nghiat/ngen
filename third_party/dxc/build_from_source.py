##----------------------------------------------------------------------------##
## This file is distributed under the MIT License.                            ##
## See LICENSE.txt for details.                                               ##
## Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             ##
##----------------------------------------------------------------------------##
import distutils.dir_util
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
    abspath = os.path.abspath(__file__)
    script_dir = os.path.dirname(abspath)
    dxc_dir = os.path.join(script_dir, "DirectXShaderCompiler")
    cwd = os.getcwd()

    # Building
    vcvars = ""
    vs2019_vcvars = "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Community\\VC\\Auxiliary\\Build\\vcvars64.bat"
    if os.path.exists(vs2019_vcvars):
        vcvars = vs2019_vcvars
    if sys.platform == "win32":
        assert vcvars
    os.chdir(dxc_dir)
    build_dir = "build"
    os.makedirs(build_dir, exist_ok=True)
    build_cmd = "cmake -S . -B {0} -G Ninja -DCMAKE_BUILD_TYPE=Release -DDXC_BUILD_ARCH=x64 -DENABLE_SPIRV_CODEGEN=ON -DSPIRV_BUILD_TESTS=OFF -DHLSL_INCLUDE_TESTS=OFF -C cmake/caches/PredefinedParams.cmake && ninja -C {0} dxc".format(build_dir)
    if sys.platform == "win32":
        run_command('"{}" && {}'.format(vcvars, build_cmd))
    else:
        run_command(build_cmd)

    # Copying
    os.chdir(script_dir)
    output_bin_dir = "DirectXShaderCompiler/{}/bin/".format(build_dir)
    if sys.platform == "win32":
        target_bin_dir = "bin/win64/"
        os.makedirs(target_bin_dir, exist_ok=True)
        shutil.copy2(output_bin_dir + "dxc.exe", target_bin_dir)
        shutil.copy2(output_bin_dir + "dxcompiler.dll", target_bin_dir)
    elif sys.platform == "linux":
        target_bin_dir = "bin/linux64/" + variant[0]
        # os.makedirs(target_bin_dir, exist_ok=True)
        # shutil.copy2(relative_path + "bin/dxc", "bin/linux64/" + variant[0])
        # shutil.copy2(relative_path + "bin/dxcompiler.dll", "bin/linux64/" + variant[0])
    else:
        print("hmm...")

    # Done
    os.chdir(cwd)
