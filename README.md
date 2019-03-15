# Concurrent Computing CW2: Operating System Kernel

### February 2019

## 1 Introduction

```
Any sufficiently large program eventually becomes an operating system.
```
- M. Might (http://matt.might.net/articles/what-cs-majors-should-know)

This assignment focuses on development of an operating system kernel. The practical nature of this task is
important from an educational perspective: itshouldoffer a) deeper understanding of various topics covered in
theory alone,andtransferable experience applicable when you either b) develop software whose effectiveness
and efficiency depend on detailed understanding how it interacts with hardware and/or a kernel, or c) develop
software for a platform that lacks a kernel yet still requires run-time support of some kind (thatyoumust
therefore offer). The constituent stages offer a variety of options, in attempt to cater for differing levels of
interest in the topic as a whole. In particular, note that they offer an initially low barrier to entry for what is
obviously a challenging task when considered as a whole.

## 2 Terms and conditions

- The assignment description may refer tomarksheet.txt. Download this ASCII text file from

```
http://tinyurl.com/ycgk8pce/csdsp/os/cw/CW2/marksheet.txt
```

then complete and include it in your submission: this is important, and failure to do so may result in a
loss of marks.

- The assignment design includes two heavily supported, closed initial stages which reflect a lower mark,
    and one mostly unsupported, open final stage which reflects a higher mark. This suggests the marking
    scale is non-linear: it is clearly easier to obtainXmarks in the initial stages than in the final stage. The
    terms open and closed should be read as meaning flexibility wrt. optionsforwork,notopen-endedness
    wrt. workload: each stage has clear success criteria that should limit the functionality you implement,
    meaning you can (and should) stop work once they have been satisfied.
    The stages are intentionally designed and ordered so as to be a) compatible (meaning that a solution for
    some stageican co-exist with that for another stagej), and b) cumulative (meaning that meeting the
    success criteria for some stageistrongly implies meeting it for another, previous stagei−1). The latter
    fact implies you do not (necessarily) need to maintain a “history” of solutions to demonstrate.
- Perhaps more so than other assignments, this one has a large design space of possible approaches for
    each stage. Part of the challenge, therefore, is to think about and understand which approach to take: this
    is not purely a programming exercise st. implementinganapproach (elsewhere perhaps even prescribed
    in the description) is enough. Such decisions should ideally be based on a reasoned argument formed
    via yourownbackground research (rather than reliance on the teaching material alone).
- You should submit your solution via the SAFE submission system at

```
http://tinyurl.com/y8jqeyrc
```

including, if/when relevant, any a) source code files, b) text or PDF files, (e.g., documentation) and c)
auxiliary files (e.g., example output)youfeel are of value.

- Your submission will be assessed in a 20 minute viva (meaning oral exam). By the stated submission
    deadline, select a viva slot online via

```
http://doodle.com/poll/xu8naniz4g7gnrtv
```

to suit your schedule. Note that:

- During the sign-up process it will ask for your name; ideally, you should use the format “full name
    (UoB user name)” (e.g., “Daniel Page (csdsp)”.
- The location of the viva is room MVB-3.42a,notthe CS lab. (i.e.,notMVB-2.11).
- Your submission will be marked using a platform equivalent to the CS lab. (MVB-2.11). As a
    result, itmustcompile, execute, and be thoroughly tested against the default operating system and
    development tool-chain versions available.


```
${ARCHIVE}
device
    disk.h
    disk.c
    disk.py
    *.h
    *.c
kernel
    int.h
    int.s
    lolevel.h
    lolevel.s
    hilevel.h
    hilevel.c
user
    libc.h
    libc.c
    P3.h
    P3.c
    P4.h
    P4.c
    P5.h
    P5.c
    console.h
    console.c
image.ld
Makefile
Makefile.console
Makefile.disk
```
```
Figure 1: A diagrammatic description of the material inquestion.tar.gz.
```
- The viva will be based on your submission via SAFE: you will need to know your candidate number^1
    so this can be downloaded.
- The discussion will focus on demonstration and explanation of your solution wrt. the stated success
    criteria. Keeping this in mind, it isessentialyou have a simple, clear way to execute and demonstrate
    your work. Ideally, you will be able to a) use one (or very few) command(s) to build a kernel image
    (e.g., using a script orMakefile), then b) demonstrate that a given success criteria has been met
    by discussing appropriate diagnostic output. Any significant editing and/or recompilation of the
    solution during the viva is strongly discouraged, as are multi-part solutions (e.g., use of separately
    compiled source code for each stage).
- Immediate personal feedback will be offered verbally, with a general, written marking report released
    at the same time as the marks.

## 3 Material

Download and unarchive the file

```
http://tinyurl.com/ycgk8pce/csdsp/os/cw/CW2/question.tar.gz
```
somewhere secure within your file system (e.g., in thePrivatesub-directory in your home directory). The
contentandstructure of this archive, as illustrated in Figure 1, should be familiar: it closely matches that used
by the lab. worksheets, and thus represents a skeleton starting point for your submission.

- Two extra (sub-)Makefileare provided: these relate to Appendix A and Appendix B, automating asso-
    ciated commands, and are introduced at the appropriate points below.
- The extra filesdisk.[ch]anddisk.pyalso relate to Appendix B, but are only relevant for one option
    within the final stage.
- In combination,image.ld,lolevel.[sh]andint.[sh]implement a interrupt handling skeleton for the
    reset, SVC and IRQ interrupts: this is analogous to worksheet #2, with each low-level interrupt handler
    function invoking an associated high-level, empty placeholder inhilevel.[ch].
- All the user programs provided, namelyconsole.[ch]plusP[345].[ch], relate specifically to success
    criteria in one or more of the stages: they shouldnotbe altered. If, say, you need to demonstrate or debug
    functionality in your kernel, then a better approach would be to includeadditionaluser programs of your
    own design; you can of course base them on those provided, e.g., embellishingconsole.[ch]to support
    additional commands.

## 4 Description

### 4.1 Context

The overarching goal of this assignment is to develop an initially simple but then increasingly capable operating
system kernel. It should execute and thus manage resources on a specific ARM-based target platform, namely a
RealView Platform Baseboard for Cortex-A8 [1] emulated by QEMU^2. The capabilities of said platform suggest
a remit for the kernel, or, equivalently, a motivating context: if and when it makes sense to do so, imagine the
platform and kernel form an embedded, consumer electronics product (e.g., set-top box^3 or media center).

### 4.2 Stages

Stage 1. This stage involves the implementation of a baseline kernel, which acts as a starting point then
improved in subsequent stages. It is important to note that each sub-stage is supported directly by
the lab. worksheet(s) and lecture slides(s); it would be sensible to complete or recap the associated
background material before starting.

- (a) The kernel developed in lab. worksheet #3 was based on the concept of cooperative multi-
    tasking: driven by regular invocation of theyieldsystem call, it context switched between and
    so concurrently executed a fixed number of user processes (that stemmed from statically linked
    user programs). The task presented in lab. worksheet #4 was to enhance this starting point by
    supporting pre-emptive multi-tasking instead: complete this task.
    Success criteria. Initialise the kernel so the user programsP3andP4are automatically executed
    (noting that neither program invokesyield), and thus demonstrate their concurrent execution.
- (b) The kernel developed in lab. worksheet #3 used a special-purpose scheduling algorithm: it could
    do so as the result of assuming a fixed number of user processes exist. Improve on this by a)
    generalising the implementation so it deals with any number of processes (i.e.,any n, vs. only
    n=2), and b) capitalising on the concept of priorities somehow, using a different scheduling
    algorithm of your choice.
    Success criteria. Demonstrate the differing behaviour of your implementation vs. round-robin
    scheduling (as implemented in the same kernel), and explain when and why this represents an
    improvement.

Stage 2. This stage involves the design and implementation of various improvements to the baseline kernel,
which, in combination, allow it to support richer and so more useful forms of user program. Each
sub-stage is less directly supported, meaning more emphasis onyoudesigning then implementing
associated solutions.
As a first step, we shift the baseline kernel into a more realistic setting. Initialise the kernel soonlythe
consoleuser program (see Appendix A) is executed automatically: this will allow you to interact with
the kernel via a command-line shell^4 , and thus, with appropriate alterations, control each (sub-)stage.

- (a) The kernel developed in lab. worksheet #3 assumed a fixed number of user processes exist.
    Improve on this by supporting dynamic^5 creation and termination of processes viafork,exec,
    andexitsystem calls. Since you design the semantics of these system calls, any reasoned
    simplifications are allowed provided they support the desired functionality:forkcan be much
    simpler than in POSIX [2, Page 881], for example.
    Success criteria. Altering the provided user programs where appropriate, demonstrate the
    dynamic creation and termination of some user processes (i.e., correct behaviour of the underlying
    fork,execandexitsystem calls) via appropriate use of the console.

some maximum number of processes, allocate a fixed set of resources (e.g., PCBs) to match, then allocate resources for any active processes
from that set.

- (b) The kernel developed in the lab. worksheet(s) lacked any mechanism for Inter-Process Commu-
    nication (IPC). The first half of the unit introduced various ways to support the concept of IPC:
    implement one of them in the kernel.
    Success criteria. Develop a new user program which uses your IPC mechanism to solve
    the dining philosophers^6 problem: upon execution, it should first useforkto spawn 16 new
    “philosopher child processes” which then interact with each other via IPC. Demonstrate execution
    of this new program from the console, and explain how the solution a) ensures mutual exclusion,
    and b) prevents starvation.

Stage 3. This stage includes several diverse, more challenging options which you can select between. Keep
in mind that a) you should only attempt this stage having first completed each previous (sub-)stage,
and b) permarksheet.txt, you select and submitoneoption only:ifyou submit more, only the option
with the highest mark will be considered wrt. assessment.

(- a) As shown in worksheet #1, the PB-A8 represents a complete computer system. As such, an
    ambitious but realistic goal is to investigate various devicesnotutilised thus far. For example,
    either:
    i. Success criteria. Demonstrate use of the MMU to a) prevent access by one process into an
    address space allocated to the kernel or another process, and b) offer some degree of memory
    virtualisation (i.e., a uniform address space per process).
    ii. Success criteria. Demonstrate a) management of the LCD and PS/2 controllers within
    an appropriate device driver framework, and b) implementation of an improved UI vs.
    interaction via the QEMU terminal and hence command-line shell: this could be achieved
    either with a user- or kernel-space window manager, for example, however simple.
- (b) In contrast to investigating one of the various real, albeit emulated devices supported by the
    PB-A8, we could consider a compromise: for certain cases we could consider a simplified device
    instead, and therefore focus on higher-level use rather than low-level detail of the device itself.
    Appendix B outlines the source code provided in order to support such a case. The goal is to use
    a simplified disk, which offers block-based storage of data, to implement a file system:ideally
    this will a) implement a UNIX-like, inode-based data structure, allowing some form of directory
    hierarchy, and b) support a suite of system calls such asopen,close,readandwrite, with
    semantics of your own design, which, in turn, demand management of file descriptors.
    Success criteria. Demonstrate either a) two new user programs which model thecatandwc
    tools (i.e., the ability to write data into a new file, or append to an existing file, then count the lines
    etc. in it), and/or b) the kernel dynamically loading a user program from the disk then executing
    it (vs. using one of the statically compiled user programs, as assumed above).
- (c) Originally, emulation of the PB-A8 was motivated by a need to a) reduce the challenge of software
    development, and b) address the issue of scale when used in the context of the unit. That said,
    investigation of aphysicalalternative such the RaspberryPi^7 can be a rewarding exercise. So,
    provided you are willing to accept the associated and significant challenges, the goal is to port
    your existing kernel and have it execute on such a platform.
    Success criteria. Demonstrate the kernel executing on whatever physical target platform you
    select, and, ideally, utilising board-specific devices available.

## References

[1] RealView Platform Baseboard for Cortex-A8. Tech. rep. HBI-0178. ARM Ltd., 2011.url:http://infocenter.
arm.com/help/topic/com.arm.doc.dui0417d/index.html(see p. 4).

[2] Standard for Information Technology - Portable Operating System Interface (POSIX). Institute of Electrical and
Electronics Engineers (IEEE) 1003.1-2008. 2008.url:http://standards.ieee.org(see p. 4).
