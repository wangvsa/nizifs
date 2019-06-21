# nizifs
A POSIX resistant file system

### Implement a file system in kernel with a loop device as the backend

![nizifs format](https://sysplay.github.io/books/LinuxDrivers/book/Images/Part18/figure_33_simula_file_system.png)


1. Write a formating tool (mkfs_nizifs.c) to create filesystem format on a regular file
    * `./mkfs_nizifs 1024`
    * This command initialzes 1024 empty blocks and writes the self-defined superblock.
    * The file is created under the current directory: ./.nizifs.img
2. `losetup -fp ./.nizifs.img` to setup the file as a loop device
    * -f Find the first unused loop device
3. Run losetup -a to check
4. Write and compile filesystem kernel code to get the .ko module file
5. Use insmod to load it and check with `cat /proc/filesystem` or `lsmod` or `dmesg | tail` command.
6. Mount our file system: `mount -t sfs /dev/loop0 /some_dir`
7. Check again with `mount` command
8. Clean up:
    * `umount`
    * `losetup -D` to delete all loop devices
