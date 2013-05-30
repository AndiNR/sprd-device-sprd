
#!/system/bin/sh


a = lookat 0xf5224008
$a |= 1<<0x14
lookat -s  $a  0xf5224008
