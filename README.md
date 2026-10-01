## Peek (pk)

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

ya thanks
