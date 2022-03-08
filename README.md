# Race64: high performance base64 encoder and decoder

Race64 is a multi-threaded command line program that encodes and decodes
Base64 as fast as possible. The interface is similar to `base64` in GNU
Coreutils, except that only the `-d` option is supported. To achieve its
speeds, Race64 produces and consumes a very rigid format — though it is
compatible with `base64` defaults.

```
# These will typically work fine
$ base64 <input | race64 -d >output
$ race64 <input | base64 -d >output
```

GNU Coreutils `base64` already has quite reasonable performance, so, in
the single threaded case, it's only about 1.5x faster at encoding and 2x
at decoding. However, on a machine with 8 logical cores, it's typically
around 3x faster for encoding and 6x faster when decoding. If you have
many CPU cores, you may need to tweak the compile-time option
`LINES_PER_BLOCK`.

It's written in pure C99, so virtually every system anywhere is
supported. However, its default compile-time parameters are tweaked for
desktops. Multi-threading is only available when compiled with OpenMP.
Currently it has by far the best performance when compiled with Clang
due to its superior support for loop unrolling.

## Command-line Options

```
usage: race64 [-dh] [-o OUTFILE] [INFILE]
  -d       Decode input (default: encode)
  -h       Print this help message
  -o FILE  Output to file instead of standard output
```

Despite its performance focus, Race64 rigorously checks its input for
validity when decoding, and it will report the earliest line number
where decoding fails.

## Benchmarking

The GNU Coreutils `yes` program is good at producing a high rate of
output with few CPU resources, and `pv` can efficiently (at least on
Linux with `splice(2)`) measure the data rate of a pipe. Use a random
string to better exercise the encoder:

    $ yes "$(od -An -x /dev/urandom | head -n1)" | pv -a | race64 >/dev/null
    $ yes "$(od -An -x /dev/urandom | head -n1)" | pv -a | base64 >/dev/null

To benchmark the decoder, give `yes` a valid line of input:

    $ yes $(race64 /dev/urandom | head -n1) | pv -a | race64 -d >/dev/null
    $ yes $(base64 /dev/urandom | head -n1) | pv -a | base64 -d >/dev/null
