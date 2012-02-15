#!/usr/bin/env ruby
# Possible flags are:
#   --release     this builds module for final distribution
#   --root DIR    install the binary into this directory. If this flag is not set - the script
#                 redeploys kext to local machine and restarts it

CWD = File.dirname(__FILE__)
KEXT_DIR = '/Library/Extensions/'
Dir.chdir(CWD)

release = ARGV.include?('--release')
root_dir = ARGV.index('--root') ? ARGV[ARGV.index('--root') + 1] : nil

abort("root directory #{root_dir} does not exist") if ARGV.index('--root') and not File.exists?(root_dir)

if File.exists?('../kext/build.rb') then
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

system('git clean -xdf') if release

unless File.exists?('Makefile') then
  flags = ''
  if release then
    archs = '-arch i386 -arch x86_64'
    flags = "CFLAGS='#{archs} -D_DARWIN_USE_64_BIT_INODE -mmacosx-version-min=10.5' LDFLAGS='#{archs}' --disable-dependency-tracking"
  else
    flags = "CFLAGS='-g -O0'"
  end

  system('autoreconf -f -i -Wall,no-obsolete') or abort
  system("./configure #{flags} --disable-static") or abort
end

system('make -s -j3') or abort

cmd = 'sudo make install'
system(cmd)

if root_dir
  # fuse is a special subproject. Other pieces (such as framework and sshfs) depend on it,
  # or be precise depend on fuse installed to /usr/local/lib. So in case if we build distribution package
  # we still install it to both places, to system and to root dir.
  system(cmd + ' DESTDIR=' + root_dir)
end


# Compile ino32 dynamic library and create a compatibility links for it
if release then
  system('git clean -xdf')

  archs = '-arch i386 -arch x86_64'
  flags = "CFLAGS='#{archs} -D_DARWIN_NO_64_BIT_INODE -mmacosx-version-min=10.5' LDFLAGS='#{archs}' --disable-dependency-tracking"
  system('autoreconf -f -i -Wall,no-obsolete') or abort
  system("./configure #{flags} --disable-static") or abort
  system('make -s -j3') or abort

  install_dir = "#{root_dir}/usr/local/lib"

  # An ugly way to mimic libtools
  system("sudo cp lib/.libs/libfuse4x.dylib #{install_dir}/libfuse4x_ino32.dylib && cd #{install_dir} && " +
         "sudo ln -sf libfuse4x_ino32.dylib libfuse.2.dylib && sudo ln -sf libfuse4x_ino32.dylib libfuse.dylib && sudo ln -sf libfuse4x_ino32.dylib libfuse.0.dylib && " +
         "sudo ln -sf libfuse4x.dylib libfuse_ino64.dylib && sudo ln -sf libfuse4x.dylib libfuse_ino64.2.dylib") or abort
end
