# Peek (pk)

Peek the last [-n N] lines from a live or stale input.

ex.
```
$ cargo build | pk -n 3
    Compiling serde_json v1.0.128
    Compiling pk v0.1.0 (~/git/pk)
        Finished `dev` profile [unoptimized + debuginfo] target(s) in 12.40s
```

and just files too
```
pk -n 3 /var/log/syslog
```

### Quick install (mac + linux)

```sh
curl -fsSL https://raw.githubusercontent.com/aamtt/pk/main/main.c -o pk.c
cc -O2 -o pk pk.c
mkdir -p ~/.local/bin
cp pk ~/.local/bin
```

**(for mac)**: if ~/.local/bin isnt already in path add `export PATH="$HOME:/.local/bin:$PATH" to your .bashrc (or equiv)

### Build from source

```sh
cc -O2 -o pk main.c
```

boom done throw it in ur path

ya thanks
