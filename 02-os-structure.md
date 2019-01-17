# Operating System Structure
_2019-01-17_

# Overview

- Architecture impact
- User operating interaction
  - User vs kernel
  - Syscall
- OS structure
  - Layers
  - Examples

# Lower-level arch affects the OS

- The OS supports sharing & protection
  - multiple apps can run concurrently, sharing resources
  - a buggy app shouldn't disrupt other apps
- Many approaches!
- The arch determines which approaches are viable (efficiency or even possibility)
  - include instruction set (sync, I/O)
  - also hardware components like MMU or DMA controllers

# Arch support for the OS
- Architectural support can simplify OS tasks
  - early OSs (DOS, macOS) lacked support for virtual mem, partially cos those PCs lacked hardware support
- Until recently, Intel-based PCs still lacked support for 64bit addressing
  - but has been available on other platforms like MIPS, IBM, etc
  - Change driven by AMD's 64bit arch

# Architectural features affecting OSs

These feature were built primarily to support OSs:
  - timer (clock) operation
  - synchronisation instructions
    - atomic test-and-set
    - locks
  - memory protection
  - I/O control operations
  - interrupts & exceptions
    - e.g. when the disk says "done with that write yo"
  - protected modes of execution (kernel vs user mode)
    - user mode can't do certain things like writing to disk etc, because it's dangerous
  - privileged instructions
  - syscalls (including software interrupts)
  - virtualization architectures

ASPLOS: a conference. Architecture Support for Programming Languages & Operating Systems

# Privileged instructions

- Some instructions are restrictred to the OS (known as _privileged_ instructions)
- Only the OS can;
  - directly access I/O devices (disks, network cards)
  - manipulate memory state management
    - page table pointers, TLB loads, etc.
  - manipulate special 'mode bits'
    - interrupt priority level
- **Restrictions provide safety and security** 

# OS protection

- So how does the processor know if a priv'd instruction should be executed?
  - the arch must support two modes (kernel or user mode)
    - x86 supports 4 protection modes
      - Ring 0: kernel (most privd)
      - Ring 1: Device drivers
      - Ring 2: Device drivers
      - Ring 3: Applications (least privd)
  - mode is set by status bit in a protected processor register
    - user programs execute in user mode
    - OS executes in kernel (priviliged) mode (OS == kernel)
- Privileged instructions CAN ONLY be executed in kernel mode
  - if privd instructions try to run in user mode, it results in an illegal instruction trap

# Crossing protection boundaries

- So how do user programs do priviliged stuff? How do you write to a disk if you can't do I/O instructions?
- User programs must call an OS procedure. i.e to ask the OS to do it for them
  - OS defines a set of syscalls
  - User-mode program executes syscall instruction
- A syscall is like a protected procedure call

# Syscall

- The syscall instruction atomically:
  - Saves the current PC
  - Sets the execution mode to priviliged
  - Sets the PC to a handler address
- Similar to a procedure call from now on
  - Caller puts args in a place callee expects (registers or stack)
    - One of the args is a syscall number, indicating which OS function to invoke
  - Callee (OS) saves caller's state (register, other control state) so it can use the CPU
  - OS function code runs
    - **OS must verify caller's arguments** (e.g. pointers)
  - OS returns using a special instruction
    - Automatically sets PC to return address **and sets execution mode to user
      - **(a normal return would not change the execution mode back to `user`)**

Syscalls are determined by IDs (well, not memory addresses) — this so you can't fuck with the program memory maliciously
(so you can't overwrite the syscall program code in memory or target it in some other way)

```
read(int fileDescription, void* buffer, int numBytes)--
|
-- Save user PC (PC = trap handler address)
-- Enter kernel mode
|
|
------------------ **user mode** ----------------------
|
trap handler ------------------------------------------
|
-- Save app state
-- Verify syscall number
-- Find sys_read() handler in vector table
|
sys_read() kernel routine -----------------------------
|
-- Verify args
-- Initiate read
-- Choose next process to run
-- Setup return values
-- Restore app state
|
ERET instruction --------------------------------------
|
-- PC = save PC
-- Enter user mode
|
Return to read() --------------------------------------
```

# Syscall issues

- A syscall is **not** a subroutine call, with the caller specifying the next PC.
  - the caller knows where the subroutines in memory, therefore they can be a target of attack
  - syscalls don't have this issue
- Has overhead:
  - The kernel saves state (to prevent overwriting of value)
  - The kernel verifies args (prevents buggy code crashing system)
  - Referring to kernel objects as arguments (data copied between user buffer and kernel buffer)

# Exception Handling and Protection

- _All_ entries to the OS occur via the mechanism just shown
  - acquiring priv mode and branching to the _trap_ handler are inseparable (it's atomic!)
- Terminology
  - **Interrupt**: _asynchronous_; caused by an external device
  - **Exception**: _synchronous_; unexpected problem with instruction
  - **Trap**: _synchronous_; intended transition to OS due to an instruction
- Privilged instructions and resources are the basis for most everything:
  - memory protection, protected I/O, limiting user resource consumption, ...

These three things cause the OS to _take control_!

# OS structure
