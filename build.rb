#!/usr/bin/env ruby
# Possible flags are:
#   --debug       this builds distribuition with debug flags enabled
#   --root DIR    install the binary into this directory. If this flag is not set - the script
#                 redeploys kext to local machine and restarts it

CWD = File.dirname(__FILE__)
KEXT_DIR = '/System/Library/Extensions/'
Dir.chdir(CWD)

debug = ARGV.include?('--debug')
root_dir = ARGV.index('--root') ? ARGV[ARGV.index('--root') + 1] : nil

abort("root directory #{root_dir} does not exist") if ARGV.index('--root') and not File.exists?(root_dir)

if File.exists?("../kext/build.rb") then
  # kext project exists - let's check that common files are the same
  common_files = [
      ['include/fuse_param.h', 'common/fuse_param.h'],
      ['include/fuse_mount.h', 'common/fuse_mount.h'],
      ['include/fuse_version.h', 'common/fuse_version.h'],
      ['include/fuse_kernel.h', 'fuse_kernel.h'],
    ]

  for pair in common_files do
    fuse_file,kext_file = *pair
    kext_file = '../kext/' + kext_file
    abort("File #{fuse_file} does not exist") unless File.exists?(fuse_file)
    abort("File #{kext_file} does not exist") unless File.exists?(kext_file)

    system("diff #{fuse_file} #{kext_file}") or abort("Files #{fuse_file} and #{kext_file} are different")
  end
end

unless File.exists?('Makefile') then
  system("autoreconf -f -i -Wall,no-obsolete") or abort
  system("./configure --disable-dependency-tracking --disable-static") or abort
end

system("make -s -j3") or abort

cmd = 'sudo make install'
system(cmd)

if root_dir
  # fuse is a special subproject. Other pieces (such as framework and sshfs) depend on it,
  # or be precise depend on fuse installed to /usr/local/lib. So in case if we build distribution package
  # we still install it to both places, to system and to root dir.
  system(cmd + ' DESTDIR=' + root_dir)
end
