# tso-enabler
a simple linux kernel module for enable M1 TSO on asahilinux 
this is just a bit polish of [this example](https://gist.github.com/zhuowei/c712df9ce13d8eabf4c49968d6c6cb2b)

# usage
* insmod tsoenabler.ko cpu_mask=0x0f -> enable TSO for core 0-3
