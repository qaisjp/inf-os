# 01 Introduction
_2019-01-14_

# Assessment

- Exam: 70%
- Coursework: 30%

3 task practical exercise (new coursework!!! :sad:)

# The Book

You can use the US edition.

Operating System Concepts, 9th International student edition, John Wiley, 2013.

You are expected to read/know this specific book. Most other books are suitable, though.

The slides aren't a replacement of the book.

# Defining an OS

A pretty big program: the Linux kernel has 20M LOC. An OS **manages** a computer's hardware.

It acts as an **intermediary** between the user of a computer and computer hardware.

An OS is a:

- **resource allocator**
  - manages all resources
  - decides between conflicting requests for efficient & fair resource use (who gets to write to this file if 2 programs want to?)
- **control program**
  - controls execution of programs to prevent errors and improper use of the computer
  
But there's actually **no universally accepted definition**.

"Everything a vendor ships" — a good approximation. but it varies.

"The one program running at all times on the computer" — nope this is the kernel. And for bare-metal embedded systems, there may not even be an OS.

Everything else is a system program (ships with the OS), or an application program.

## Some goals of operating systems

Lots of stuff

- Use computer hardware efficiently
- Make app software portable and versatile
- Provide isolation, security and protection among user programs
- Improve oerall system reliability

## Traditional picture

```
[Applications]
[     OS     ]
[  Hardware  ]
```

OS can be seen as some sort of library ("The OS is everything you don't need to write in order to run your application"), but not always.

- Yes: all ops on IO devices require syscalls (OS calls)
- No: you use the CPU/memory without syscalls. It intervenes without being explicitly called.

## OS & Hardware

- It **mediates** access to hardware resources (sharing and protection)
- **Abstracts** hardware into **logical resources** and well-defined **interfaces** to those resources (ease of use)

Abstraction makes things easier. But it costs (time).

## Why bother with an OS

- App benefits: programm simplicity (abstractions), portability (across machine config / architectures)
- User benefits: safety (sandboxing, resources multiplexed across programs), efficiency (cost and speed; concurrent execution)

Might have millions of threads — not parallelism (necessarily) — it's concurrent. Time-slicing could be in play.

# Historical look

## Hardware Complexity Increases

Moore's Law is DYING. It's not dead yet. We've basically peaked though. It's basically a fuckin' lie.

Also: your phone can land a person on the moon. And what do you use it for? lmao

_Software complexity also increases_

# Key functions

## Async I/O

The disk was super slow. So they added hardware so the disk could operate without tying up the CPU. Now it can start IO and come back to later to see if it's done.

- Upside: increases (expensive) CPU utilization
- Downsides: it's hard to get right, the benefits are job specific

## Multiprogramming

Pausing programs, continuing another. Programs don't know they are being paused.

## Timesharing

To support interactive use, create a **timesharing OS**:
- multiple terminals in one machine
- each user has illusion of entire machine to themselves
- optimize response time, perhaps at the cost of throughput

Timeslicing:
- divide CPU equally among users
- if the job is truly interactive (e.g editor), then can jump between programs and users faster than users can generate work
- permits users to interactively view, edit, debug running programs

## Multitasking

# Various types of OS
_Depends on platform & scenario_
