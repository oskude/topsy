# topsy

`topsy` is a minimal "real-time" processor and memory usage monitor for [linux](https://www.kernel.org/)/[xorg](https://www.x.org/), suitable for (at least) [openbox dock area](http://openbox.org/wiki/Help:Configuration#Dock).

![screencap](screencap.gif?raw=true)

> NOTE: cause i'm a _lazy bastard_, `topsy` uses [`proc_topstat`](https://github.com/oskude/proc_topstat), but if you want to use upstream data sources instead, please implement it as fallback and send a patch.

## Display

- processor(s) used
- memory used | cached

## Config

key | default | info
----|---------|-----
update_interval | 250 | milliseconds
window_width | 64 | pixels
bar_height | 10 | pixels
bar_gap | 1 | pixels
color_back | 000000FF | hex rgba
color_cpu_used | 00FF0032 | hex rgba
color_cpu_fade | 00000032 | hex rgba
color_mem_back | 000000FF | hex rgba
color_mem_used | 1E90FFFF | hex rgba
color_mem_cached | 005A9CFF | hex rgba
