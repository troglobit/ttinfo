ttinfo
======

Display information about a process, group or tty.  By default the tty
and process of the current terminal is examined.


Example
-------

```
admin@viper-65-f7-60:/mnt # ./ttinfo -s 1
TeleType device    | TTY  : /dev/ttyS0
Process ID         | PID  : 3301
Process group ID   | PGID : 3301
Parent process ID  | PPID : 2348
Session ID         | SID  : 1
Foreground PGID    | TGID : 3301
Procs in same SID  | init(1 1 1 0:0)  logit(2056 1 1 0:0) 
Procs in same PGID | ttinfo(3301 3301 2336 /dev/ttyS0) 
```
